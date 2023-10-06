#include "fork.h"

#include "interrupt.h"
#include "thread.h"
#include "memory.h"
#include "debug.h"
#include "process.h"
#include "string.h"
#include "file.h"
#include "stdio_kernel.h"
#include "pipe.h"

extern void intr_exit();

// 将父进程的 pcb 拷贝给子进程
static int32_t copy_pcb_vaddrbitmap_stack0(struct task_struct* child_thread, struct task_struct* parent_thread) {
  // 1.复制 pcb 所在的整个页，里面包含进程 pcb 信息及特级 0 极的栈，里面包含了返回地址
  memcpy(child_thread, parent_thread, PG_SIZE);
  // 2.下面再单独修改
  child_thread->pid = fork_pid();
  child_thread->elapsed_ticks = 0;
  child_thread->status = TASK_READY;
  //为新进程把时间片充满
  child_thread->ticks = child_thread->priority;
  child_thread->parent_pid = parent_thread->pid;
  child_thread->general_tag.prev = child_thread->general_tag.next = NULL;
  child_thread->all_list_tag.prev = child_thread->all_list_tag.next = NULL;
  block_desc_init(child_thread->u_block_desc);

  // 3.复制父进程的虚拟地址池的位图
  //申请一个内核页框来存储位图
  uint32_t bitmap_pg_cnt = DIV_ROUND_UP((0xc0000000 - USER_VADDR_START)/PG_SIZE/8, PG_SIZE);
  void* vaddr_btmp = get_kernel_pages(bitmap_pg_cnt);
  if (vaddr_btmp == NULL) return -1;
  //没明白为什么要复制父进程虚拟地址池的位图！！！！！
  //此时 child_thread->userprog_vaddr.vaddr_bitmap.bits还是指向父进程虚拟地址的位图地址
  //下面将 child_thread->userprog_vaddr.vaddr_bitmap.bits指向自己的位图 vaddr_btmp
  memcpy(vaddr_btmp, child_thread->ueserprog_vaddr.virtual_addr_bitmap.bits, bitmap_pg_cnt*PG_SIZE);
  //这里没有赋值就会导致新进程的虚拟地址位图和原进程的虚拟地址位图完全一样，如果原进程的虚拟地址位图释放
  //那么新进程的虚拟地址位图就找不到内核中对应的虚拟地址位图了！！！！！
  child_thread->ueserprog_vaddr.virtual_addr_bitmap.bits = vaddr_btmp;
  //调试用
  ASSERT(strlen(child_thread->name)<11);
  //pcb.name 的长度是 16，为避免下面 strcat 越界
  strcat(child_thread->name, "_fork");
  return 0;
}

//复制子进程的进程体（代码和数据）及用户栈
//bufpage是内核页，用他来做所有进程的数据共享缓冲区
static void copy_body_stack3(struct task_struct* child_thread, struct task_struct* parent_thread, void* buf_page) {
  uint8_t* vaddr_btmp = parent_thread->ueserprog_vaddr.virtual_addr_bitmap.bits;
  uint32_t btmp_bytes_len = parent_thread->ueserprog_vaddr.virtual_addr_bitmap.bimap_bytes_len;
  //虚拟地址开始使用的地址
  uint32_t vaddr_start = parent_thread->ueserprog_vaddr.virtual_addr_start;
  uint32_t idx_byte = 0;
  uint32_t idx_bit = 0;
  uint32_t prog_vaddr = 0;

  //在父进程的用户空间中查找已有数据的页
  while (idx_byte < btmp_bytes_len) {
    if (vaddr_btmp[idx_byte]) {
      idx_bit = 0;
      while (idx_bit < 8) {
        if ((BITMAP_MASK << idx_bit) & vaddr_btmp[idx_byte]) {
          prog_vaddr = (idx_byte*8+idx_bit)*PG_SIZE+vaddr_start;
          // 下面的操作是将父进程用户空间中的数据通过内核空间做中转
          //最终复制到子进程的用户空间
          //步骤1：将父进程在用户空间中的数据复制到内核缓冲区 buf_page
          //目的是下面切换到子进程的页表后，还能访问到父进程的数据
          memcpy(buf_page, (void*)prog_vaddr, PG_SIZE);

          //步骤2：将页表切换到子进程，目的是避免下面申请内存的函数将 pte 及 pde 安装在父进程的页表中
          page_dir_activate(child_thread);

          //步骤3：申请虚拟地址 prog_vaddr
          get_a_page_without_opvaddrbitmap(PF_USER, prog_vaddr);

          //步骤4：从内核缓冲区中将父进程数据复制到子进程的用户空间
          memcpy((void*)prog_vaddr, buf_page, PG_SIZE);

          //步骤5：恢复父进程页表
          page_dir_activate(parent_thread);
        }
        idx_bit++;
      }
    }
    idx_byte++;
  }
  // printk("ok");
}

//为子进程构建 thread_stack 和修改返回值
static int32_t build_child_stack(struct task_struct* child_thread) {
  //1.使子进程 pid 返回值为 0
  //获取子进程 0 级栈栈顶、
  struct intr_stack* intr_0_stack = (struct intr_stack*)((uint32_t)child_thread+ PG_SIZE-sizeof(struct intr_stack));
  //修改子进程的返回值为 0
  intr_0_stack->eax = 0;

  //2.为 switch_to 构建 struct thread_stack,将其构建在紧临 intr_stack 之下的空间
  uint32_t* ret_addr_in_thread_stack = (uint32_t*)intr_0_stack-1;
  //这三行不是必要的，只是为了梳理 thread_stack 中的关系
  uint32_t* esi_ptr_in_thread_stack = (uint32_t*)intr_0_stack - 2;
  uint32_t* edi_ptr_in_thread_stack = (uint32_t*)intr_0_stack - 3;
  uint32_t* ebx_ptr_in_thread_stack = (uint32_t*)intr_0_stack - 4;

  //ebp 在 thread_stack 中的地址便是当时的 esp（0 级栈的栈顶,
  //即 esp 为"(uint32_t*)intr_0_stack - 5
  uint32_t* ebp_ptr_in_thread_stack = (uint32_t*)intr_0_stack - 5;

  //switch_to 的返回地址更新为 intr_exit，直接从中断返回
  *ret_addr_in_thread_stack = (uint32_t)intr_exit;

  //下面这两行赋值只是为了使构建的 thread_stack 更加清晰，其实也不需要
  //因为在进入 intr_exit 后一系列的 pop会把寄存器中的数据覆盖
  *ebp_ptr_in_thread_stack = *ebx_ptr_in_thread_stack = *edi_ptr_in_thread_stack = *esi_ptr_in_thread_stack = 0;

  //把构建的 thread_stack 的栈顶作为 switch_to 恢复数据时的栈顶
  child_thread->self_kstack = ebp_ptr_in_thread_stack;
  return 0;
}

//更新 inode 打开数
static void update_inode_open_cnts(struct task_struct* thread) {
  //遍历除了3个标准文件描述符之外的所有文件描述符
  int32_t local_fd = 3, global_fd = 0;
  while (local_fd < MAX_FILES_OPEN_PER_PROC) {
    //获得全局文件表 file_table 的下标 global_fd
    global_fd = thread->fd_table[local_fd];
    ASSERT(global_fd < MAX_FILE_OPEN);
    if (global_fd != -1) {
      if (is_pipe(local_fd)) {
        //判断是否是管道
        file_table[global_fd].fd_pos++;
      } else {
        file_table[global_fd].fd_inode->i_open_cnts++;
      }
    }
    local_fd++;
  }
}

//拷贝父进程本身所占资源给子进程
static int32_t copy_process(struct task_struct* child_thread, struct task_struct* parent_thread) {
  //内核缓冲区,作为父进程用户空间的数据复制到子进程用户空间的中转
  void* buf_page = get_kernel_pages(1);
  if (buf_page == NULL) {
    return -1;
  }

  //1.复制父进程的 pcb、虚拟地址位图、内核栈到子进程
  if (copy_pcb_vaddrbitmap_stack0(child_thread, parent_thread) == -1) {
    return -1;
  }
  //2.为子进程创建页表，此页表仅包括内核空间
  child_thread->pgdir = create_page_dir();
  if (child_thread->pgdir == NULL) {
    return -1;
  }
  //3.复制父进程进程体及用户栈给子进程
  copy_body_stack3(child_thread, parent_thread, buf_page);
  //4.构建子进程 thread_stack 和修改返回值 pid
  build_child_stack(child_thread);
  //5.更新文件 inode 的打开数
  update_inode_open_cnts(child_thread);

  mfree_page(PF_KERNEL, buf_page, 1);
  return 0;
}

//fork 子进程，内核线程不可直接调用
pid_t sys_fork() {
  struct task_struct* parent_thread = running_thread();
  // 为子进程创建 pcb（task_struct 结构）
  struct task_struct* child_thread = get_kernel_pages(1);
  if (child_thread == NULL) {
    return -1;
  }
  ASSERT(INTR_OFF == intr_get_status() && parent_thread->pgdir != NULL);
  
  if (copy_process(child_thread, parent_thread) == -1) {
    return -1;
  }

  //添加到就绪线程队列和所有线程队列，子进程由调试器安排运行
  ASSERT(!elem_find(&thread_ready_list, &child_thread->general_tag));
  list_append(&thread_ready_list, &child_thread->general_tag);
  ASSERT(!elem_find(&thread_all_list, &child_thread->all_list_tag));
  list_append(&thread_all_list, &child_thread->all_list_tag);
  //父进程返回子进程的 pid
  return child_thread->pid;
}
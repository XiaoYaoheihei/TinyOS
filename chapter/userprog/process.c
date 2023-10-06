#include "process.h"
#include "../kernel/global.h"
#include "../kernel/memory.h"
#include "../kernel/debug.h"
#include "tss.h"

extern void intr_exit();

//构建用户进程初始上下文信息
void start_process(void* filename_) {
  void* function = filename_;
  struct task_struct* cur = running_thread();
  //跨过thread_stack,指向intr_stack
  cur->self_kstack += sizeof(struct thread_stack);  
  //可以不用定义成结构体指针	 
  struct intr_stack* proc_stack = (struct intr_stack*)cur->self_kstack;
  //8个通用寄存器进行初始化
  proc_stack->edi = proc_stack->esi = proc_stack->ebp = proc_stack->esp_dummy = 0;
  proc_stack->ebx = proc_stack->edx = proc_stack->ecx = proc_stack->eax = 0;
  proc_stack->gs = 0;		 // 不太允许用户态直接访问显存资源,用户态用不上,直接初始为0
  //用户使用的内存数据区
  proc_stack->ds = proc_stack->es = proc_stack->fs = SELECTOR_U_DATA;
  //待执行的 用户程序地址
  //start_process的参数
  proc_stack->eip = function;	 
  //用户使用的代码段
  proc_stack->cs = SELECTOR_U_CODE;
  proc_stack->eflags = (EFLAGS_IOPL_0 | EFLAGS_MBS | EFLAGS_IF_1);
  //为用户进程分配3特权级下的栈
  //指向从用户内存池中分配的地址
  //首先根据栈的下边界地址分配一页内存之后得到虚拟地址
  //加上PG_SIZE就是栈的上边界，就是栈顶位置，也就是0XC0000000
  proc_stack->esp = (void*)((uint32_t)get_a_page(PF_USER, USER_STACK3_VADDR) + PG_SIZE) ;
  //栈段使用普通的数据段
  proc_stack->ss = SELECTOR_U_DATA; 
  //将栈 esp 替换刚刚填充好的 proc_stack
  //通过中断出口程序的一系列pop指令和iretd指定，将proc_stack中的数据加载到CPU的寄存器
  //从而进入特权级3
  asm volatile ("movl %0, %%esp" : : "g" (proc_stack) : "memory");
  asm volatile ("jmp intr_exit");
}

//激活页表
void page_dir_activate(struct task_struct* p_thread) {
  // 执行此函数时，当前任务可能是线程。
  // 之所以对线程也要重新安装页表，原因是上一次被调度的可能是进程，
  // 否则不恢复页表的话，线程就会使用进程的页表了。
  
  //若为内核线程，需要重新填充页表为0x100000
  //所有内核线程使用的页表的物理地址
  uint32_t pagedir_phy_addr = 0x100000;
  if (p_thread->pgdir != NULL) {
    //用户态进程有自己的页目录表
    pagedir_phy_addr = addr_v2p((uint32_t)p_thread->pgdir);
  }
  // 更新页目录寄存器 cr3，使新页表生效
  asm volatile ("movl %0, %%cr3" : : "r" (pagedir_phy_addr) : "memory");
}

//激活线程或进程的页表
//更新 tss 中的 esp0 为进程的特权级 0 的栈
void process_activate(struct task_struct* p_thread) {
  ASSERT(p_thread != NULL);
  //激活该进程或者线程的页表
  page_dir_activate(p_thread);

  // 内核线程特权级本身就是 0，处理器进入中断时并不会
  // 从 tss 中获取 0 特权级栈地址，故不需要更新 esp0
  if (p_thread->pgdir) {
    //更新进程的esp0, 用于此进程被中断时保留上下文
    update_tss_esp(p_thread);
  }
}

//创建页目录表，将当前页表的表示内核空间的 pde 复制
//成功则返回页目录的虚拟地址，否则返回-1
uint32_t* create_page_dir() {
  //用户进程的页表不能让用户直接访问，在内核空间申请
  uint32_t* page_dir_vaddr = get_kernel_pages(1);
  if (page_dir_vaddr == NULL) {
    console_put_str("create_page_dir: get_kernel_page failed!");
    return NULL;
  }

  //把内核的页目录项复制到用户进程使用的页目录表中
  //这样就保证了每一个用户进程都可以访问到相同的内核了。相当于提供了一个入口
  //0x300*4 表示 768 个页目录项的偏移量,0x300是768
  memcpy((uint32_t*)((uint32_t)page_dir_vaddr+0x300*4),\
        (uint32_t*)(0xfffff000+0x300*4),\
        1024);
  //更新页目录地址
  //将用户页目录表的最后一个页目录项更新为用户进程自己的页目录表的物理地址
  uint32_t new_page_dir_phy_addr = addr_v2p((uint32_t)page_dir_vaddr);
  //内核访问页目录表的方法是通过虚拟地址0xfffff000，这会访问到当前页目录表的最后一个页目录项
  page_dir_vaddr[1023] = new_page_dir_phy_addr | PG_US_U | PG_RW_W | PG_P_1;
  return page_dir_vaddr;
}

//创建用户进程虚拟地址位图
void create_user_vaddr_bitmap(struct task_struct* user_prog) {
  //用户进程的起始地址
  user_prog->ueserprog_vaddr.virtual_addr_start = USER_VADDR_START;
  //记录位图需要的内存页框数
  uint32_t bitmap_pg_cnt = \
  DIV_ROUND_UP((0Xc0000000 - USER_VADDR_START)/PG_SIZE/8, PG_SIZE);
  //为位图分配内存，记录相应的位图指针
  user_prog->ueserprog_vaddr.virtual_addr_bitmap.bits = \
  get_kernel_pages(bitmap_pg_cnt);
  //记录相应的位图长度
  user_prog->ueserprog_vaddr.virtual_addr_bitmap.bimap_bytes_len = \
  (0xc0000000-USER_VADDR_START)/PG_SIZE/8;
  //进行位图初始化
  bitmap_init(&user_prog->ueserprog_vaddr.virtual_addr_bitmap);
}

//创建用户进程，参数分别是用户进程地址和用户名
void process_execute(void* filename, char* name) {
  //在内核内存池中申请pcb信息
  struct task_struct* thread = get_kernel_pages(1);
  init_thread(thread, name, default_prio);
  //创建用户进程虚拟地址位图
  create_user_vaddr_bitmap(thread);
  thread_create(thread, start_process, filename);
  //创建页目录表
  thread->pgdir = create_page_dir();
  //初始化用户进程内存块描述符
  block_desc_init(thread->u_block_desc);
  
  enum intr_status old_status = intr_disable();
  ASSERT(!elem_find(&thread_ready_list, &thread->general_tag));
  list_append(&thread_ready_list, &thread->general_tag);

  ASSERT(!elem_find(&thread_all_list, &thread->all_list_tag));
  list_append(&thread_all_list, &thread->all_list_tag);
  intr_set_status(old_status);
}
#include "exec.h"

#include "thread.h"    
#include "stdio_kernel.h"
#include "fs.h"
#include "string.h"
#include "global.h"
#include "memory.h"

extern void intr_exit();
typedef uint32_t Elf32_Word, Elf32_Addr, Elf32_Off;
typedef uint16_t Elf32_Half;

//32 位 elf 头
struct Elf32_Ehdr{
  unsigned char e_ident[16];
  Elf32_Half    e_type;
  Elf32_Half    e_machine;
  Elf32_Word    e_version;
  Elf32_Addr    e_entry;
  Elf32_Off     e_phoff;
  Elf32_Off     e_shoff;
  Elf32_Word    e_flags;
  Elf32_Half    e_ehsize;
  Elf32_Half    e_phentsize;
  Elf32_Half    e_phnum;
  Elf32_Half    e_shentsize;
  Elf32_Half    e_shnum;
  Elf32_Half    e_shstrndx;
};

//程序头表 Program header 就是段描述头
struct Elf32_Phdr {
  //下面的 enum segment_type
  Elf32_Word p_type;
  Elf32_Off  p_offset;
  Elf32_Addr p_vaddr;
  Elf32_Addr p_paddr;
  Elf32_Word p_filesz;
  Elf32_Word p_memsz;
  Elf32_Word p_flags;
  Elf32_Word p_align;
};

//段类型
enum segment_type {
  // 忽略
  PT_NULL,
  // 可加载程序段
  PT_LOAD,     
  // 动态加载信息       
  PT_DYNAMIC,        
  // 动态加载器名称 
  PT_INTERP,    
  // 一些辅助信息     
  PT_NOTE,  
  // 保留          
  PT_SHLIB, 
  // 程序头表          
  PT_PHDR             
};

//将文件描述符 fd 指向的文件中，偏移为 offset，
//大小为 filesz 的段加载到虚拟地址为 vaddr 的内存
static bool segment_load(int32_t fd, uint32_t offset, uint32_t filesz, uint32_t vaddr) {
  //虚拟地址 vaddr 所在的页框起始地址
  uint32_t vaddr_first_page = vaddr & 0xfffff000;
  //加载到内存后，文件在第一个页框中占用的字节大小
  uint32_t size_in_first_page = PG_SIZE - (vaddr&0x00000fff);
  //表示该段占用的总页框数
  uint32_t occupy_pages = 0;
   //若一个页框容不下该段
  if (filesz > size_in_first_page) {
    uint32_t left_size = filesz - size_in_first_page;
    //1 是指 vaddr_first_page
    occupy_pages = DIV_ROUND_UP(left_size, PG_SIZE)+1;
  } else {
    occupy_pages = 1;
  }

  //为进程分配内存
  uint32_t page_idx = 0;
  uint32_t vaddr_page = vaddr_first_page;
  while (page_idx < occupy_pages) {
    //获取新进程虚拟地址 vaddr_page 的 pde 和 pte
    uint32_t* pde = pde_ptr(vaddr_page);
    uint32_t* pte = pte_ptr(vaddr_page);

    //如果 pde 不存在，或者 pte 不存在就分配内存
    //pde 的判断要在 pte 之前，否则 pde 若不存在会导致判断 pte 时缺页异常
    if (!(*pde & 0x00000001) || !(*pte & 0x00000001)) {
      if (get_a_page(PF_USER, vaddr_page) == NULL) {
        return false;
      }
    }
    //如果原进程的页表已经分配了，利用现有的物理页直接覆盖进程体
    vaddr_page += PG_SIZE;
    page_idx++;
  }

  sys_lseek(fd, offset, SEEK_SET);
  //将该段读入虚拟地址vaddr处
  sys_read(fd, (void*)vaddr, filesz);
  return true;
  // uint32_t vaddr_first_page = vaddr & 0xfffff000;     //vaddr地址所在的页框
  // uint32_t size_in_first_page = PG_SIZE - (vaddr & 0x00000fff); //加载到内存后，文件在第一个页框中占用的字节大小
  // uint32_t occupy_pages = 0;
  // //若一个页框容不下该段
  // if (filesz > size_in_first_page) {
  //   uint32_t left_size = filesz - size_in_first_page;
  //   occupy_pages = DIV_ROUND_UP(left_size, PG_SIZE) + 1;  //1是指vaddr_first_page
  // } else {
  //   occupy_pages = 1;
  // }

  // //为进程分配内存
  // uint32_t page_idx = 0;
  // uint32_t vaddr_page = vaddr_first_page;
  // while (page_idx < occupy_pages) {
  //   uint32_t *pde = pde_ptr(vaddr_page);
  //   uint32_t *pte = pte_ptr(vaddr_page);
  //   //如果pde不存在，或者pte不存在就分配内存．pde的判断要在pte之前，否则pde若不存在会导致判断pte时缺页异常
  //   if (!(*pde & 0x00000001) || !(*pte & 0x00000001)) {
  //     if (get_a_page(PF_USER, vaddr_page) == NULL) {
  //       //exec是执行新进程，也就是用新进程的进程体替换当前老进程，但依然用的是老进程的那套页表，
  //       //这就涉及到老进程的页表是否满足新进程内存要求了。如果老进程已经分配了页框，我们不需要再重新分配页框，
  //       //只需要用新进程的进程体覆盖老进程就行了，只有新进程用到了在老进程中没有的地址时才需要分配新页框给新进程
  //       return false;
  //     }
  //   } //如果原进程的页表已经分配了，利用现有的物理页
  //     //直接覆盖进程体
  //     vaddr_page += PG_SIZE;
  //     page_idx++;
  // }
  // sys_lseek(fd, offset, SEEK_SET);
  // sys_read(fd, (void *)vaddr, filesz);
  // return true;
}

//从文件系统上加载用户程序 pathname，成功则返回程序的起始地址，否则返回−1
//可执行文件的绝对路径 pathname
static int32_t load(const char* pathname) {
  int32_t ret = -1;
  //elf头和程序头
  struct Elf32_Ehdr elf_header;
  struct Elf32_Phdr prog_header;
  memset(&elf_header, 0, sizeof(struct Elf32_Ehdr));

  int32_t fd = sys_open(pathname, O_RDONLY);
  if (fd == -1) {
    return -1;
  }

  if (sys_read(fd, &elf_header, sizeof(struct Elf32_Ehdr)) != sizeof(struct Elf32_Ehdr)) {
    ret = -1;
    goto done;
  }
  //校验 elf 头
  if (memcmp(elf_header.e_ident, "\177ELF\1\1\1", 7)\
      || elf_header.e_type != 2 \
      || elf_header.e_machine != 3\
      || elf_header.e_version != 1\
      || elf_header.e_phnum > 1024\
      || elf_header.e_phentsize != sizeof(struct Elf32_Phdr)) {
        ret = -1;
        goto done;
      }
  //程序头的起始地址和程序头条目大小
  Elf32_Off prog_header_offset = elf_header.e_phoff;
  Elf32_Half prog_header_size = elf_header.e_phentsize;

  struct task_struct* cur = running_thread();
  //在程序头表中遍历所有程序头，程序头也就是段头
  uint32_t prog_idx = 0;
  while (prog_idx < elf_header.e_phnum) {
    memset(&prog_header, 0, prog_header_size);
    //将文件的指针定位到程序头
    sys_lseek(fd, prog_header_offset, SEEK_SET);
    //只获取程序头
    if (sys_read(fd, &prog_header, prog_header_size) != prog_header_size) {
      ret = -1;
      goto done;
    }
    //如果是可加载段就调用 segment_load 加载到内存
    if (prog_header.p_type == PT_LOAD) {
      //调试信息
      // printk("filesize:%d\n", prog_header.p_filesz);
      if (!segment_load(fd, prog_header.p_offset, prog_header.p_filesz, prog_header.p_vaddr)) {
        ret = -1;
        goto done;
      }
      //执行完第4次的sys_free有问题，如果不处理的话就会导致下一次malloc出现PF错误
      //解决PF的方法，每一次执行完之后都将内存的free_list清空
      block_desc_init(cur->u_block_desc);
    }
    //更新下一个程序头的偏移
    prog_header_offset += elf_header.e_phentsize;
    prog_idx++;
  }
  //循环处理完所有的段之后将程序的入口值返回
  ret = elf_header.e_entry;

  done:
    sys_close(fd);
    return ret;
}

//用 path 指向的程序替换当前进程
int32_t sys_execv(const char* path, const char* argv[]) {
  uint32_t argc = 0;
  while (argv[argc]) {
    argc++;
  }
  int32_t entry_point = load(path);
  if (entry_point == -1) {
    //若加载失败，则返回−1
    return -1;
  }

  struct task_struct* cur = running_thread();
  //修改进程名
  memcpy(cur->name, path, TASK_NAME_LEN);
  cur->name[TASK_NAME_LEN-1] = 0;
  struct intr_stack* intr_0_stack = (struct intr_stack*)((uint32_t)cur + PG_SIZE - sizeof(struct intr_stack));
  //参数传递给用户进程
  intr_0_stack->ebx = (int32_t)argv;
  intr_0_stack->ecx = argc;
  intr_0_stack->eip = (void*)entry_point;
  //使新用户进程的栈地址为最高用户空间地址
  intr_0_stack->esp = (void*)0xc0000000;

  //exec 不同于 fork，为使新进程更快被执行，直接从中断返回
  asm volatile ("movl %0, %%esp; jmp intr_exit": : "g" (intr_0_stack):"memory");
  return 0;
}
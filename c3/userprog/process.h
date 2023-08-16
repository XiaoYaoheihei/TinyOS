#ifndef __USERPROG_PROCESS_H 
#define __USERPROG_PROCESS_H 
#include "../thread/thread.h"
#include "../lib/stdint.h"

#define default_prio 31
//用户栈顶的下边界
//申请内存的时候单位是1页，4096,并且返回的是内存空间的下边界
//0xc0000000-1是用户空间的最高地址,0xc0000000-0xffffffff是内核空间
#define USER_STACK3_VADDR  (0xc0000000 - 0x1000)
#define USER_VADDR_START 0x8048000

void process_execute(void* filename, char* name);
void start_process(void* filename_);
void process_activate(struct task_struct* p_thread);
void page_dir_activate(struct task_struct* p_thread);
uint32_t* create_page_dir(void);
void create_user_vaddr_bitmap(struct task_struct* user_prog);
#endif
#include "thread.h"
#include "../lib/stdint.h"
#include "../lib/string.h"
#include "../kernel/global.h"
#include "../kernel/memory.h"

#define PG_SIZE 4096

//由kernel_thread去执行function(func_argc)
static void kernel_thread(thread_func* function, void* func_argc) {
  function(func_argc);
}

//初始化线程栈
//将待执行的函数和参数放到线程栈中的相应位置
void thread_create(struct task_struct* pthread, thread_func function, void* func_argc) {
  //预留好了中断栈
  pthread->self_kstack -= sizeof(struct intr_stack);
  //再预留线程栈
  pthread->self_kstack -= sizeof(struct thread_stack);
  struct thread_stack* kthread_stack = (struct thread_stack*)pthread->self_kstack;
  kthread_stack->eip = kernel_thread;
  kthread_stack->function = function;
  kthread_stack->func_arg = func_argc;
  kthread_stack->ebp = kthread_stack->ebx = \
  kthread_stack->esi = kthread_stack->edi = 0;
}

//初始化线程对应的PCB基本信息
void init_thread(struct task_struct* pthread, char* name, int prio) {
  memset(pthread, 0, sizeof(*pthread));
  strcpy(pthread->name, name);
  pthread->status = TASK_RUNNING;
  pthread->priority = prio;
  //self_kstack是线程在自己的内核态下使用的栈顶地址
  //因为刚好PCB分配的是1页，所以指向的是这1页的顶端
  pthread->self_kstack = (uint32_t*)((uint32_t)pthread+PG_SIZE);
  //自定义的魔数
  pthread->stack_magic = 0x19870916;
}

//创建一个优先级为prio的线程
//线程名字为name,执行的函数是function(func_arg)
struct task_struct* thread_start(char* name,
                                 int prio,
                                 thread_func* function,
                                 void* func_arg) {
  //首先给PCB分配一个页空间,PCB位于内核空间中
  struct task_struct* thread = get_kernel_pages(1);
  //初始化PCB
  init_thread(thread, name, prio);
  thread_create(thread, function, func_arg);

  //开启线程的关键
  asm volatile ("movl %[input], %%esp; \
                pop %%ebp; pop %%ebx; pop %%edi; pop %%esi; \
                ret": : [input]"g"(thread->self_kstack) : "memory");
  return thread;
}
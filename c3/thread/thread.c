#include "thread.h"
#include "../lib/stdint.h"
#include "../lib/string.h"
#include "../kernel/global.h"
#include "../kernel/memory.h"
#include "../kernel/interrupt.h"
#include "../kernel/debug.h"

#define PG_SIZE 4096

//定义主线程的PCB
//进入内核之后一直运行的是main函数，其实他就是一个线程
struct task_struct* main_thread;
//就绪队列
struct list thread_ready_list;
//所有任务队列
struct list thread_all_list;
//保存队列中的线程节点
static struct list_elem* thread_tag;

extern void switch_to(struct task_struct* cur, struct task_struct* next);

//获取当前线程PCB指针
struct task_struct* running_thread() {
  uint32_t esp;
  asm ("mov %%esp, %0" : "=g"(esp));
  //取esp整数部分,也就是PCB的起始地址
  return (struct task_struct*)(esp & 0xfffff000);
}

//由kernel_thread去执行function(func_argc)
static void kernel_thread(thread_func* function, void* func_argc) {
  //执行function之前需要开中断，避免后面的时钟中断被屏蔽而无法调度线程
  intr_enable();
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
  if (pthread == main_thread) {
    pthread->status = TASK_RUNNING;
  } else {
    pthread->status = TASK_READY;
  }
  pthread->priority = prio;
  //self_kstack是线程在自己的内核态下使用的栈顶地址
  //因为刚好PCB分配的是1页，所以指向的是这1页的顶端
  pthread->self_kstack = (uint32_t*)((uint32_t)pthread+PG_SIZE);
  pthread->ticks = prio;
  //线程从未执行
  pthread->elapsed_ticks = 0;
  pthread->pgdir = NULL;
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

  //确保此线程之前不在就绪队列中
  ASSERT(!elem_find(&thread_ready_list, &thread->general_tag));
  //假如就绪队列
  list_append(&thread_ready_list, &thread->general_tag);

  //确保之前不在队列中
  ASSERT(!elem_find(&thread_all_list, &thread->all_list_tag));
  //加入全部线程队列
  list_append(&thread_all_list, &thread->all_list_tag);

  //开启线程的关键
  // asm volatile ("movl %[input], %%esp; \
  //               pop %%ebp; pop %%ebx; pop %%edi; pop %%esi; \
  //               ret": : [input]"g"(thread->self_kstack) : "memory");
  return thread;
}

//将kernel中的main函数完善为主线程
//在主线程的PCB中写入线程信息
static void make_main_thread() {
  main_thread = running_thread();
  init_thread(main_thread, "main", 31);

  //main函数是当前线程，当前线程现在肯定不在thread_ready_list中
  //将其只添加到thread_all_list中
  ASSERT(!elem_find(&thread_all_list, &main_thread->all_list_tag));
  list_append(&thread_all_list, &main_thread->all_list_tag);
} 
#ifndef _THREAD_THREAD_H
#define _THREAD_THREAD_H

#include "../lib/stdint.h"

// 自定义的通用函数类型
typedef void thread_func(void*);

// 进程或者线程的状态
enum task_status{
  TASK_RUNNING,
  TASK_READY,
  TASK_BLOCKED,
  TASK_WAITING,
  TASK_HANDING,
  TASK_DIED
};

// 中断栈
// 此结构用于中断发生时保护程序的上下文
// 进入中断之后，在中断入口程序所执行的上下文保护的一系列压栈操作都是压入此结构中
// 会按照此结构压入上下文寄存器
// 此栈在线程自己的内核栈中位置固定，所在页的最顶端
struct intr_stack{
  // kernel.asm 宏 VECTOR 中 push %1 压入的中断号
  uint32_t vector_number;
  uint32_t edi;
  uint32_t esi;
  uint32_t ebp;
  uint32_t esp_dummy;
  // 虽然 pushad 把 esp 也压入，但 esp 是不断变化的，所以会被 popad 忽略
  uint32_t ebx;
  uint32_t edx;
  uint32_t ecx;
  uint32_t eax;
  uint32_t gs;
  uint32_t fs;
  uint32_t es;
  uint32_t ds;
  // CPU从低特权级进入高特权级时才会压入
  // err_code 会被压入在 eip 之后
  uint32_t err_code;
  void (*eip) (void);
  uint32_t cs;
  void* esp;
  uint32_t ss;
};

// 线程栈
// 线程自己的栈，用于存储线程中待执行的函数
// 此结构在线程自己的内核栈中位置不固定
struct thread_stack{
  uint32_t ebp;
  uint32_t ebx;
  uint32_t edi;
  uint32_t esi;
  //线程首次执行函数的时候，eip就是待调用的函数
  //其他时候，当任务切换的时候，此eip保存的是任务切换后的新任务的返回地址
  void (*eip) (thread_func* func, void* func_arg);

  //以下仅供第一次被调度上CPU时使用
  void (*unused_retaddr);   //参数 unused_ret 只为占位置充数为返回地址
  thread_func* function;    //线程中执行的函数
  void* func_arg;           //执行函数需要使用的参数
};

//进程或者线程的程序控制块（PCB）
//PCB和0级栈是在同一个页内的
struct task_struct {
  //各个内核线程自己的内核栈
  uint32_t* self_kstack;
  enum task_status status;
  //优先级
  uint8_t priority;
  char name[16];
  //栈的边界标记，用于检测栈的溢出
  uint32_t stack_magic;
};
void thread_create(struct task_struct* pthread, thread_func function, void* func_argc);
void init_thread(struct task_struct* pthread, char* name, int prio);
struct task_struct* thread_start(char* name,int prio,thread_func* function, void* func_arg);

#endif
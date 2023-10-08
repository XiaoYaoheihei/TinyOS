#include "thread.h"
#include "../lib/stdint.h"
#include "../lib/string.h"
#include "../kernel/global.h"
#include "../kernel/memory.h"
#include "../kernel/interrupt.h"
#include "../kernel/debug.h"
#include "../lib/kernel/list.h"
#include "../userprog/process.h"
#include "sync.h"
#include "file.h"
#include "fs.h"
#include "stdio.h"

//定义主线程的PCB
//进入内核之后一直运行的是main函数，其实他就是一个线程
struct task_struct* main_thread;
//idle 线程
struct task_struct* idle_thread;
//就绪队列
struct list thread_ready_list;
//所有任务队列
struct list thread_all_list;
//分配pid锁
struct lock pid_lock;
//保存队列中的线程节点
static struct list_elem* thread_tag;

//pid 的位图，最大支持 1024 个 pid
uint8_t pid_bitmap_bits[128] = {0};

//pid池
struct pid_pool{
  //pid 位图
  struct bitmap pid_bitmap;
  //起始 pid
  uint32_t pid_start;
  //分配 pid 锁
  struct lock pid_lock;
}pid_pool;

extern void switch_to(struct task_struct* cur, struct task_struct* next);
extern void init(void);

//系统空闲时运行的线程
static void idle(void* arg UNUSED) {
  while (1) {
    thread_block(TASK_BLOCKED);
    //执行 hlt 时必须要保证目前处在开中断的情况下
    //hlt 指令的功能让处理器停止执行指令，也就是将处理器挂起,处理器停止运行
    //当有外部中断发生的时候，处理器恢复执行后续的指令
    asm volatile("sti; hlt": : :"memory");
  }
}

//获取当前线程PCB指针
//这里的处理方式很关键，因为要切换esp指针！！！！！！！
struct task_struct* running_thread() {
  uint32_t esp;
  asm ("mov %%esp, %0" : "=g"(esp));
  //各个线程所用的0级栈都在自己的PCB当中
  //所以取当前栈指针的高20位，也就是PCB的起始地址
  //这一部分原理：
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

//初始化 pid 池
static void pid_pool_init() {
  pid_pool.pid_start = 1;
  pid_pool.pid_bitmap.bits = pid_bitmap_bits;
  pid_pool.pid_bitmap.bimap_bytes_len = 128;
  bitmap_init(&pid_pool.pid_bitmap);
  lock_init(&pid_pool.pid_lock);
}

//分配pid
static pid_t allocate_pid() {
  lock_acquire(&pid_pool.pid_lock);
  int32_t bit_idx = bitmap_scan(&pid_pool.pid_bitmap, 1);
  bitmap_set(&pid_pool.pid_bitmap, bit_idx, 1);
  lock_release(&pid_pool.pid_lock);
  return (bit_idx + pid_pool.pid_start);
}

//释放 pid
void release_pid(pid_t pid) {
  lock_acquire(&pid_pool.pid_lock);
  int32_t bit_idx = pid-pid_pool.pid_start;
  bitmap_set(&pid_pool.pid_bitmap, bit_idx, 0);
  lock_release(&pid_pool.pid_lock);
}

//fork进程时为其分配pid,因为allocate_pid已经是静态的,别的文件无法调用
//不想改变函数定义了,故定义fork_pid函数来封装一下
pid_t fork_pid() {
  return allocate_pid();
}

//初始化线程对应的PCB基本信息
void init_thread(struct task_struct* pthread, char* name, int prio) {
  memset(pthread, 0, sizeof(*pthread));
  //分配线程pid
  pthread->pid = allocate_pid();
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
  //预留标准输入输出
  pthread->fd_table[0] = 0;
  pthread->fd_table[1] = 1;
  pthread->fd_table[2] = 2;
  //其余的全置为-1
  uint8_t fd_idx = 3;
  while (fd_idx < MAX_FILES_OPEN_PER_PROC) {
    pthread->fd_table[fd_idx] = -1;
    fd_idx++;
  }
  //以根目录作为默认工作路径
  pthread->cwd_inode_nr = 0;
  // -1表示没有父进程
  pthread->parent_pid = -1;
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
  //加入就绪队列
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

//实现任务调度
void schedule() {
  //确保当前是处于关中断的情况下
  ASSERT(intr_get_status() == INTR_OFF);
  //获取当前运行线程的PCB
  struct task_struct* cur = running_thread();
  if (cur->status == TASK_RUNNING) {
    //若此线程是CPU时间到了,将其添加到就绪队列尾部
    //说明此次调度不是由于阻塞发生的
    ASSERT(!elem_find(&thread_ready_list, &cur->general_tag));
    list_append(&thread_ready_list, &cur->general_tag);
    //重新设置ticks
    cur->ticks = cur->priority;
    cur->status = TASK_READY;
  } else {
    //若此线程需要某件事件发生之后才能继续上CPU运行，不需要加入队列
    // TASK_BLOCKED,TASK_WAITING,TASK_HANDING三个事件都不能将线程加入就绪队列中
  }

  //如果就绪队列中没有可运行的任务，就唤醒 idle
  if (list_empty(&thread_ready_list)) {
    thread_unblock(idle_thread);
  }

  ASSERT(!list_empty(&thread_ready_list));
  thread_tag = NULL;
  //将就绪队列中的第一个线程弹出，准备调度上CPU
  thread_tag = list_pop(&thread_ready_list);
  //此时的thread_tag仅仅只是一个tag，是pcb中的一个元素信息
  //要想获得线程的所有信息，必须将其转化成PCB才可以
  struct task_struct* next = elem2entry(struct task_struct, general_tag, thread_tag);
  //新的线程可以上CPU了
  next->status = TASK_RUNNING;
  //激活任务页表
  process_activate(next);
  switch_to(cur, next);
}

//主动让出 cpu，换其他线程运行
void thread_yield() {
  struct task_struct* cur = running_thread();
  enum intr_status old_status = intr_disable();
  ASSERT(!elem_find(&thread_ready_list, &cur->general_tag));
  //schedule之前的两个步骤必须保证是原子操作
  list_append(&thread_ready_list, &cur->general_tag);
  cur->status = TASK_READY;
  schedule();
  intr_set_status(old_status);
}


//以填充空格的方式对齐输出 buf
static void pad_print(char* buf, int32_t buf_len, void* ptr, char format) {
  memset(buf, 0, buf_len);
  uint8_t out_pad_0idx = 0;
  switch(format) {
    case 's'://处理字符串
      out_pad_0idx = sprintf(buf, "%s", ptr);
      break;
    case 'd'://处理 16 位整数
      out_pad_0idx = sprintf(buf, "%d", *((int16_t*)ptr));
    case 'x'://处理 32 位整数
      out_pad_0idx = sprintf(buf, "%x", *((uint32_t*)ptr));
  }
  //ptr长度不足buf_len，就以空格来填充
  while (out_pad_0idx < buf_len) {
    //以空格填充
    buf[out_pad_0idx] = ' ';
    out_pad_0idx++;
  }
  sys_write(stdout_no, buf, buf_len-1);
}

//用于在 list_traversal 函数中的回调函数，用于针对线程队列的处理
//打印相关信息
static bool elem2thread_info(struct list_elem* pelem, int arg UNUSED) {
  struct task_struct* pthread = elem2entry(struct task_struct, all_list_tag, pelem);
  char out_pad[16] = {0};
  //打印pid和ppid
  pad_print(out_pad, 16, &pthread->pid, pelem);
  if (pthread->pid == -1) {
    pad_print(out_pad, 16, "NULL", 's');
  } else {
    pad_print(out_pad, 16, &pthread->parent_pid, 'd');
  }
  //根据任务的status输出不同的任务状态
  switch (pthread->status) {
  case 0:
    pad_print(out_pad, 16, "RUNNING", 's');
    break;
  case 1:
    pad_print(out_pad, 16, "READY", 's');
    break;
  case 2:
    pad_print(out_pad, 16, "BLOCKED", 's');
    break;
  case 3:
    pad_print(out_pad, 16, "WAITING", 's');
    break;
  case 4:
    pad_print(out_pad, 16, "HANGING", 's');
    break;
  case 5:
    pad_print(out_pad, 16, "DIED", 's');
  }
  //输出总共执行的时钟数
  pad_print(out_pad, 16, &pthread->elapsed_ticks, 'x');

  memset(out_pad, 0, 16);
  ASSERT(strlen(pthread->name) < 17);
  memcpy(out_pad, pthread->name, strlen(pthread->name));
  strcat(out_pad, "\n");
  sys_write(stdout_no, out_pad, strlen(out_pad));
  //此处返回 false 是为了迎合主调函数 list_traversal
  //只有回调函数返回 false 时才会继续调用此函数
  return false;
}

//打印任务列表
void sys_ps() {
   char* ps_title = "PID            PPID           STAT           TICKS          COMMAND\n";
  sys_write(stdout_no, ps_title, strlen(ps_title));
  list_traversal(&thread_all_list, elem2thread_info, 0);
}

//释放进程PCB需要的功能
//回收退出任务thread_over的pcb和页表，并将其从调度队列中去除
void thread_exit(struct task_struct* thread_over, bool need_schedule) {
  //要保证 schedule 在关中断情况下调用
  intr_disable();
  thread_over->status = TASK_DIED;
  //如果 thread_over 不是当前正在运行的线程,就有可能还在就绪队列中，将其从中删除
  if (elem_find(&thread_ready_list, &thread_over->general_tag)) {
    list_remove(&thread_over->general_tag);
  }
  //如果是进程，回收进程的页表,进程的页目录表占用1个页框
  if (thread_over->pgdir) {
    mfree_page(PF_KERNEL, thread_over->pgdir, 1);
  }
  //从 all_thread_list 中去掉此任务
  list_remove(&thread_over->all_list_tag);

  //回收 pcb 所在的页，主线程的 pcb 不在堆中，跨过
  if (thread_over != main_thread) {
    mfree_page(PF_KERNEL, thread_over, 1);
  }

  //归还 pid
  release_pid(thread_over->pid);

  //如果需要下一轮调度则主动调用 schedule
  if (need_schedule) {
    schedule();
    PANIC("thread_exit: should not be here\n");
  }
}

//比对任务的 pid
//是函数 listr_traversal 的回调函数
static bool pid_check(struct list_elem* pelem, int32_t pid) {
  struct task_struct* pthread = elem2entry(struct task_struct, all_list_tag, pelem);
  if (pthread->pid == pid) {
    return true;
  }
  return false;
}

//根据 pid 找 pcb，若找到则返回该 pcb，否则返回 NULL
//原理是调用list_traversal遍历全部队列中的所有任务，通过回调函数过滤出特定pid的任务
struct task_struct* pid2thread(int32_t pid) {
  struct list_elem* pelem = list_traversal(&thread_all_list, pid_check, pid);
  if (pelem == NULL) {
    return NULL;
  }
  struct task_struct* thread = elem2entry(struct task_struct, all_list_tag, pelem);
  return thread;
}


//c初始化线程环境
void thread_init() {
  put_str("thread_init start\n");
  list_init(&thread_ready_list);
  list_init(&thread_all_list);
  pid_pool_init();

  //先创建第一个用户进程:init 
  //放在第一个初始化,这是第一个进程,init进程的pid为1
  process_execute(init, "init");
  //将当前main函数创建为线程
  make_main_thread();
  //创建idle线程
  idle_thread = thread_start("idle", 10, idle, NULL);
  put_str("thread_init done\n");
}

//当前线程自己阻塞，状态设置为stat
void thread_block(enum task_status stat) {
  ASSERT((stat == TASK_BLOCKED) || (stat == TASK_WAITING) || (stat == TASK_HANDING));
  enum intr_status old_status = intr_disable();
  struct task_struct* cur_thread = running_thread();
  //设置线程状态
  cur_thread->status = stat;
  //将当前线程换下处理器，将当前线程从就绪队列中撤下
  //并且下一次另一个线程进行schedule的时候不会调度此线程
  schedule();
  //在该线程在被换下处理器之后本次就没有机会执行此代码了
  //只有在该线程被唤醒之后才会执行此函数
  intr_set_status(old_status);
}

//阻塞线程pthread解除阻塞
void thread_unblock(struct task_struct* pthread) {
  enum intr_status old_status = intr_disable();
  ASSERT((pthread->status == TASK_BLOCKED) || (pthread->status == TASK_WAITING) || (pthread->status == TASK_HANDING));
  //这个判读是让自己放心
  if (pthread->status != TASK_READY) {
    ASSERT(!elem_find(&thread_ready_list, &pthread->general_tag));
    //确保当前待唤醒的线程不在就绪队列中，如果在就绪队列中就证明当时线程阻塞的过程中有问题
    if (elem_find(&thread_ready_list, &pthread->general_tag)) {
      PANIC("thread_unblock: blocked thread in ready_list\n");
    }
    //将阻塞的线程放到就绪队列的最前面，使其尽快得到调度
    list_push(&thread_ready_list, &pthread->general_tag);
    //设置状态，表明实现了线程的唤醒
    pthread->status = TASK_READY;
  }
  intr_set_status(old_status);
}
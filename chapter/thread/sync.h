#ifndef _THREAD_SYNC_H
#define _THREAD_SYNC_H

#include "../lib/kernel/list.h"
#include "../lib/stdint.h"
#include "../kernel/interrupt.h"
#include "../kernel/debug.h"
#include "thread.h"

//信号量结构
struct semaphore{
  //信号量值
  uint8_t value;
  //待唤醒的队列，也是所有在此信号量上阻塞的线程
  struct list waiters;
};

//锁结构
struct lock{
  //锁的持有者
  struct task_struct* holder;
  //信号量
  struct semaphore semaphorer;
  //锁的持有者重复申请锁的次数
  //需要理解一下此变量的作用
  uint32_t holder_repeat_nr;
};

void sema_init(struct semaphore* psema, uint8_t value);
void lock_init(struct lock* plock);
void sema_down(struct semaphore* psema);
void sema_up(struct semaphore* psema);
void lock_acquire(struct lock* plock);
void lock_release(struct lock* plock);

#endif
#include "sync.h"

//初始化信号量
void sema_init(struct semaphore* psema, uint8_t value) {
  //为信号量赋值
  psema->value = value;
  //初始化信号量的等待队列
  list_init(&psema->waiters);
}

//初始化锁结构
void lock_init(struct lock* plock) {
  plock->holder = NULL;
  plock->holder_repeat_nr = 0;
  //信号量初值是1
  sema_init(&plock->semaphorer, 1);
}

//信号量dowm操作，获取锁
void sema_down(struct semaphore* psema) {
  //关中断
  enum intr_status old_status = intr_disable();
  //此时信号量为0,表示锁被别人持有
  while (psema->value == 0) {
    struct list_elem* now = &running_thread()->general_tag;
    ASSERT(!elem_find(&psema->waiters, now));
    //确保当前线程不在信号量的waiter队列中
    if (elem_find(&psema->waiters, &running_thread()->general_tag)) {
      PANIC("sema_down: thread blocked has been in waiters_list\n");
    }
    //把当前线程添加到该锁的阻塞队列中
    list_append(&psema->waiters, &running_thread()->general_tag);
    //线程阻塞，等待唤醒
    //同时开启线程调度
    thread_block(TASK_BLOCKED);
  }
  //锁可以被当前线程持有
  psema->value--;
  ASSERT(psema->value == 0);
  //恢复之前的中断状态
  intr_set_status(old_status);
}

//信号量的up操作，释放锁
void sema_up(struct semaphore* psema) {
  enum intr_status old_status = intr_disable();
  ASSERT(psema->value == 0);
  if (!list_empty(&psema->waiters)) {
    //获得一个阻塞线程，并将其唤醒
    struct task_struct* thread_blocked = elem2entry(struct task_struct, general_tag, list_pop(&psema->waiters));
    thread_unblock(thread_blocked);
  }
  psema->value++;
  ASSERT(psema->value == 1);
  //恢复之前的中断状态
  intr_set_status(old_status);
}

//获取锁结构
void lock_acquire(struct lock* plock) {
  if (plock->holder != running_thread()) {
    //对信号量down操作
    sema_down(&plock->semaphorer);
    plock->holder = running_thread();
    ASSERT(plock->holder_repeat_nr == 0);
    plock->holder_repeat_nr = 1;
  } else {
    //线程可能会嵌套申请同一把锁，这种情况下申请锁就会形成死锁
    //如果有申请的话，repeat值++
    plock->holder_repeat_nr++;
  }
}

//释放锁结构
void lock_release(struct lock* plock) {
  ASSERT(plock->holder == running_thread());
  if (plock->holder_repeat_nr > 1) {
    //说明自己多次申请了该锁，此时还不能将锁释放
    plock->holder_repeat_nr--;
    return;
  }
  ASSERT(plock->holder_repeat_nr == 1);
  plock->holder = NULL;
  plock->holder_repeat_nr = 0;
  sema_up(&plock->semaphorer);
}
#include "ioqueue.h"

//初始化io队列
void ioqueue_init(struct ioqueue* ioq) {
  lock_init(&ioq->lock);
  ioq->producer = ioq->consumer = NULL;
  //首尾指针指向缓冲区的第0个位置
  ioq->head = ioq->tail = 0;
}

//返回下一个地址
static int32_t next_pos(int32_t pos) {
  return (pos + 1) % bufsize;
}

//判断队列是否满
bool ioq_full(struct ioqueue* ioq) {
  ASSERT(intr_get_status() == INTR_OFF);
  return next_pos(ioq->head) == ioq->tail;
}

//判断队列是否空
bool ioq_empty(struct ioqueue* ioq) {
  ASSERT(intr_get_status() == INTR_OFF);
  return ioq->head == ioq->tail;
}

// 使当前生产者或消费者在此缓冲区上等待
static void ioq_wait(struct task_struct** waiter) {
  ASSERT(*waiter == NULL && waiter != NULL);
  *waiter = running_thread();
  thread_block(TASK_BLOCKED);
}

//唤醒waiter
static void wakeup(struct task_struct** waiter) {
  ASSERT(*waiter != NULL);
  thread_unblock(*waiter);
  *waiter = NULL;
}

// 消费者从 ioq 队列中获取一个字符
char ioq_getchar(struct ioqueue* ioq) {
  ASSERT(intr_get_status() == INTR_OFF);
  //缓冲队列为空的话，记录消费者
  //当缓冲区里有生产者放的东西之后通知消费者，唤醒消费者
  while (ioq_empty(ioq)) {
    lock_acquire(&ioq->lock);
    //实参是线程指针成员的地址
    ioq_wait(&ioq->consumer);
    lock_release(&ioq->lock);
  }
  //从缓冲区取出数据
  char byte = ioq->buf[ioq->tail];
  //读下标向后移动
  ioq->tail = next_pos(ioq->tail);
  //唤醒生产者
  if (ioq->producer != NULL) {
    //实参是线程指针成员的地址
    wakeup(&ioq->producer);
  }
  return byte;
}

// 生产者往 ioq 队列中写入一个字符 byte
void ioq_putchar(struct ioqueue* ioq, char byte) {
  ASSERT(intr_get_status() == INTR_OFF);
  //若缓冲区已经满了，记录生产者
  //当缓冲区里的东西被消费者取走之后通知该生产者，也就是唤醒该生产者线程
  while (ioq_full(ioq)) {
    lock_acquire(&ioq->lock);
    ioq_wait(&ioq->producer);
    lock_release(&ioq->lock);
  }
  //放入相应B
  ioq->buf[ioq->head] = byte;
  //写下标向后移动
  ioq->head = next_pos(ioq->head);
  if (ioq->consumer != NULL) {
    //唤醒消费者
    wakeup(&ioq->consumer);
  }
} 
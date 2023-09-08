#ifndef _DEVICE_IOQUEUE_H
#define _DEVICE_IOQUEUE_H

#include "../thread/sync.h"
#include "../thread/thread.h"
#include "../lib/stdint.h"
#include "../kernel/interrupt.h"
#include "../kernel/debug.h"
#include "../kernel/global.h"
#define bufsize 64

//环形队列
struct ioqueue {
  struct lock lock;
  struct task_struct* producer;
  struct task_struct* consumer;
  //缓冲区大小
  char buf[bufsize];
  //队首和队尾，数据往队首写，数据从队尾读
  int32_t head;
  int32_t tail;
  
};

void ioqueue_init(struct ioqueue* ioq);
bool ioq_full(struct ioqueue* ioq);
bool ioq_empty(struct ioqueue* ioq);
char ioq_getchar(struct ioqueue* ioq);
void ioq_putchar(struct ioqueue* ioq, char byte);
uint32_t ioq_length(struct ioqueue* ioq);

#endif
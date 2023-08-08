#ifndef KERNEL_INTERRUPT_H
#define KERNEL_INTERRUPT_H
#include "../lib/stdint.h"
typedef void* intr_handler;
void idt_init();
//定义中断的两种状态
enum intr_status {
  INTR_OFF, //中断关闭
  INTR_ON   //中断打开
};
enum intr_status intr_get_status();
enum intr_status intr_set_status(enum intr_status);
enum intr_status intr_enable();
enum intr_status intr_disable();
#endif
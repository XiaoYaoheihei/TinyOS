#include "init.h"
#include "../lib/kernel/print.h"
#include "interrupt.h"
#include "../device/timer.h"

//负责初始化所有模块
void init_all() {
  put_str("init all\n");
  idt_init();//初始化中断
  timer_init();//初始化PIT8253,将时钟周期设置成100
}
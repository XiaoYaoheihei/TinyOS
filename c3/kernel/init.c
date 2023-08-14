#include "init.h"
#include "../lib/kernel/print.h"
#include "interrupt.h"
#include "../device/timer.h"
#include "../device/console.h"
#include "memory.h"
#include "../thread/thread.h"
#include "../device/keyboard.h"

//负责初始化所有模块
void init_all() {
  put_str("init all\n");
  idt_init();   //初始化中断
  timer_init(); //初始化PIT8253,将时钟周期设置成100
  mem_init();   //初始化内存管理系统
  thread_init();
  console_init();//控制台初始化
  keyboard_init();//键盘初始化
}
//设置时钟中断的频率
//编程8253定时器
#include "timer.h"
#include "../lib/kernel/io.h"
#include "../lib/kernel/print.h"
#include "../thread/thread.h"
#include "../kernel/debug.h"
#include "../kernel/interrupt.h"

//我们自己设置的频率是100Hz
#define IRQ0_FREQUENCY  100
#define INPUT_FREQUENCY 1193180
//计数器0的计数初值
#define COUNTER0_VALUE  INPUT_FREQUENCY/IRQ0_FREQUENCY 
//计数器0对应的端口
#define COUNTER0_PORT   0X40
#define COUNTER0_NO     0
#define COUNTER_MODE    2
#define READ_WRITE_LATCH  3
//控制器端口
#define PIT_CONTROL_PORT  0X43
//此ticks是内核自中断开启以来总共的滴答数
uint32_t ticks;
//操作计数器0,读写锁属性rwl
//计数器模式mode写入控制寄存器并且赋予初值
static void frequency_set(uint8_t counter_port,
                          uint8_t counter_no,
                          uint8_t rwl,
                          uint8_t counter_mode,
                          uint16_t counter_value) {
  //向控制器端口写入控制字
  outb (PIT_CONTROL_PORT, (uint8_t)(counter_no << 6 | rwl << 4 | counter_mode << 1));
  //写入计数初值的低8位
  outb (counter_port, (uint8_t)counter_value);
  //写入高8位
  outb (counter_port, (uint8_t)counter_value >> 8);
}

//时钟中断处理函数
static void intr_timer_handler() {
  //首先拿到当前运行线程的PCB
  struct task_struct* cur_thread = running_thread();
  //检查栈是否溢出
  ASSERT(cur_thread->stack_magic == 0x19870916);
  //此线程占用的cpu时间
  cur_thread->elapsed_ticks++;
  //从内核第一次处理时间中断后开始至今的滴哒数，内核态和用户态总共的嘀哒数
  ticks++;
  if (cur_thread->ticks == 0) {
    //若进程的时间片用完，开始调度新的进程上CPU
    //调度程序
    intr_disable();
    schedule();
  } else {
    cur_thread->ticks--;
  }
}


void timer_init() {
  put_str("timer_init start\n");
  //设置8253的定时周期，也就是发中断的周期
  frequency_set(COUNTER0_PORT,
                COUNTER0_NO,
                READ_WRITE_LATCH,
                COUNTER_MODE,
                COUNTER0_VALUE);
  //注册中断处理函数
  register_handler(0x20, intr_timer_handler);
  put_str("time_init done\n");
}
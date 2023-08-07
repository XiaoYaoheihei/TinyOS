//设置时钟中断的频率
//编程8253定时器
#include "timer.h"
#include "../lib/kernel/io.h"
#include "../lib/kernel/print.h"

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

void timer_init() {
  put_str("timer_init start\n");
  //设置8253的定时周期，也就是发中断的周期
  frequency_set(COUNTER0_PORT,
                COUNTER0_NO,
                READ_WRITE_LATCH,
                COUNTER_MODE,
                COUNTER0_VALUE);
  put_str("time_init done\n");
}
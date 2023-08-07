#ifndef _LIB_IO_H
#define _LIB_IO_H

#include "../stdint.h"

//------机器模式
// b--输出寄存器QImode名称，寄存器的最低8位：[a-d]l 
// w--输出寄存器HImode名称，寄存器中两个B的部分，[a-d]x
// HImode   "half-interger"模式，表示一个两字节的整数
// QImode   "quarter-interger"模式，表示一个一字节的整数

//向端口写入一个B
static inline void outb(uint16_t port, uint8_t data) {
  //通过内联汇编的形式实现
  //源操作数在前，目的操作数在后
  //N 为立即数约束，它表示 0～
  //255 的立即数，也就是 8 位 1 字节所表示的范围，
  //这样把写入的数据限制在 1 字节之内
  asm volatile ("outb %b0, %w1" : : "a" (data), "Nd" (port));
}

//将addr起始的word_cnt个字写入端口port
static inline void outsw(uint16_t port, const void* addr, uint32_t word_cnt) {
  asm volatile ("cld; rep outsw" : "+S" (addr), "+c" (word_cnt) : "d" (port));
}

//从port读取一个B返回
static inline uint8_t inb(uint16_t port) {
  uint8_t data;
  asm volatile ("inb %w1, %b0" : "=a" (data) : "Nd" (port));
  return data;
}

// 从port读入的word_cnt个字写入addr
// 默认段基址为 0，符号地址其实就是偏移地址。
// 指针变量用来存储其他符号的地址,所以将addr 的值约束到 edi 寄存器
// +表示所修饰的约束既做输入（位于 input 中），也做输出（位于 output 中），
// 也就是告诉 gcc 所约束的寄存器或内存先被读入，再被写入
// 由于 insw 指令往内存 es：edi 处写入了数据，所以通知 gcc 这块内存已经改变
static inline void insw(uint16_t port, void * addr, uint32_t word_cnt) {
  // insw 来实现，该指令的功能是从端口 port 处读入的 16 位数据写
  // 入 es：edi 指向的内存，即一次读入 2 字节
  asm volatile ("cld; rep insw" : "+D" (addr), "+c" (word_cnt): "d" (port) : "memory");
}

#endif
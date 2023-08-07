#ifndef KERNEL_INTERRUPT_H
#define KERNEL_INTERRUPT_H
#include "../lib/stdint.h"
typedef void* intr_handler;
void idt_init();
#endif
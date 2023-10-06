#ifndef __DEVICE_KEYBOARD_H
#define __DEVICE_KEYBOARD_H
#include "../lib/kernel/print.h"
#include "../kernel/interrupt.h"
#include "../lib/kernel/io.h"
#include "../kernel/global.h"
#include "ioqueue.h"

void keyboard_init(); 
//保证外面的函数可以访问
extern struct ioqueue kbd_buf;
#endif
#ifndef __DEVICE_CONSOLE_H
#define __DEVICE_CONSOLE_H
#include "../lib/stdint.h"
#include "../lib/kernel/print.h"
#include "../thread/sync.h"
#include "../thread/thread.h"



void console_init(void);
void console_acquire(void);
void console_release(void);
void console_put_str(char* str);
void console_put_char(uint8_t char_asci);
void console_put_int(uint32_t num);
#endif
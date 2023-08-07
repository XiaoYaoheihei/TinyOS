#ifndef _DEVICE_TIMER_H
#define _DEVICE_TIMER_H

#include "../lib/stdint.h"

void timer_init(void);
void mtimer_sleep(uint32_t m_second);

#endif
#ifndef _USERPROG_TSS_H
#define _USERPROG_TSS_H

#include "../thread/thread.h"
void update_tss_esp(struct task_struct* pthread);
void tss_init(void);
#endif
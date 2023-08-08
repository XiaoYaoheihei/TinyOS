#ifndef _KERNEL_MEMORY_H
#define _KERNEL_MEMORY_H

#include "../lib/stdint.h"
#include "../lib/kernel/bitmap.h"

//虚拟地址池，用于虚拟地址管理
struct virtual_addr {
  //虚拟地址使用到的位图，用位图来记录虚拟地址的分配情况
  //虚拟地址也需要通过页来被分配
  struct bitmap virtual_addr_bitmap;
  //虚拟地址起始地址
  uint32_t virtual_addr_start;
};

extern struct pool kernel_pool, user_pool;
void mem_init();

#endif
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

//内存池标记，用于判断使用哪一个内存池
enum pool_flags {
  PF_KERNEL = 1,  //内核物理内存池
  PF_USER = 2     //用户内存池
};
//内存操作中少不了的操作就是修改页表
//最后这些位直接|运算就可以得到相应的值，不用设置为1然后左移右移
#define PG_P_1 1  //此页存在
#define PG_P_0 0  //此页不存在
#define PG_RW_R 0 //此页内存允许读/执行
#define PG_RW_W 2 //读/写/执行
#define PG_US_S 0 //系统级属性位
                  //只允许特权级为0,1,2的程序访问此页内存，不允许3
#define PG_US_U 4 //用户级属性位

extern struct pool kernel_pool, user_pool;
void mem_init();
void* get_kernel_pages(uint32_t pg_cnt);
void* malloc_page(enum pool_flags pf, uint32_t pg_cnt);
void malloc_init(void);
uint32_t* pte_ptr(uint32_t vaddr);
uint32_t* pde_ptr(uint32_t vaddr);
uint32_t addr_v2p(uint32_t vaddr);
void* get_a_page(enum pool_flags pf, uint32_t vaddr);
void* get_user_pages(uint32_t pg_cnt);

#endif
#ifndef _KERNEL_MEMORY_H
#define _KERNEL_MEMORY_H

#include "../lib/stdint.h"
#include "../lib/kernel/bitmap.h"
#include "../lib/kernel/list.h"

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

//内存块
struct mem_block{
  struct list_elem free_elem;
};

//内存块描述符
struct mem_block_desc {
  //内存块大小
  uint32_t block_size;
  //本 arena 中可容纳此 mem_block 的数量
  uint32_t blocks_per_arena;
  //目前可用的 mem_block 链表
  struct list free_list;
};
//内存块描述符个数
#define DESC_CNT 7

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
void block_desc_init(struct mem_block_desc* desc_array);
void* sys_malloc(uint32_t size);
void pfree(uint32_t pg_phy_addr);
void mfree_page(enum pool_flags pf, void* _vaddr, uint32_t pg_cnt);
void sys_free (void* ptr);
void* get_a_page_without_opvaddrbitmap(enum pool_flags pf, uint32_t vaddr);
#endif
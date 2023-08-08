#include "memory.h"
#include "../lib/stdint.h"
#include "../lib/kernel/print.h"
#include "../lib/kernel/bitmap.h"
//一页大小是4096B
#define PG_SIZE 4096
//内存位图基地址，此时对应的是虚拟地址，虚拟地址对应的物理地址是在1MB内部
#define MEM_BITMAP_BASE 0XC009A000
//堆的起始虚拟地址
#define K_HEADP_START   0XC0100000

//物理内存池结构
struct pool {
  //内存池的位图结构，用来管理物理内存
  struct bitmap pool_bitmap;
  //内存池管理的物理内存的起始地址
  uint32_t phy_addr_start;
  //内存池B容量
  uint32_t pool_size;
};
//内核物理内存池和用户物理内存池
struct pool kernel_pool, user_pool;
//用来给内核分配虚拟地址
struct virtual_addr kernel_vaddr;

//根据物理内存容量初始化物理内存池的相关结构
static void mem_pool_init(uint32_t all_mem) {
  put_str("mem_pool_init start\n");
  //记录页目录表和页表占用的B大小
  //总大小=页目录表大小+页表大小
  //页表大小=1页的页目录+第0和第768个页目录项指向的同一个页表+
  //第769-1022个页目录项共指向的254个页表，一共256个
  uint32_t page_table_size = PG_SIZE*256;
  //当前已经使用的内存B=页表大小+低端1M大小
  uint32_t used_mem = page_table_size + 0x100000;
  //存储目前可用的内存B
  uint32_t free_mem = all_mem-used_mem;
  //可用的内存B转化成的物理页数
  uint16_t all_free_pages = free_mem / PG_SIZE;
  
  //内核可用的空闲物理页
  uint16_t kernel_free_pages = all_free_pages / 2;
  //用户可用的空闲物理页
  uint16_t user_free_pages = all_free_pages - kernel_free_pages;
  
  //内核对应位图的长度，位图中的一位表示一页，以B为单位
  uint32_t kernel_bmp_length = kernel_free_pages / 8;
  //用户对应位图的长度
  uint32_t user_bmp_length = user_free_pages / 8;
  
  //内核内存池的起始地址
  uint32_t kernel_pool_start = used_mem;
  //用户内存池的起始地址
  uint32_t user_pool_start = kernel_pool_start + kernel_free_pages*PG_SIZE;

  //内核物理内存池对应的真实物理地址
  kernel_pool.phy_addr_start = kernel_pool_start;
  //用户物理内存池对应的真实物理地址
  user_pool.phy_addr_start = user_pool_start;

  //内核占的所有B数
  kernel_pool.pool_size = kernel_free_pages*PG_SIZE;
  //用户占的所有B数
  user_pool.pool_size = user_free_pages*PG_SIZE;

  //内核物理内存池对应的位图长度
  kernel_pool.pool_bitmap.bimap_bytes_len = kernel_bmp_length;
  //用户物理内存池对应的位图长度
  user_pool.pool_bitmap.bimap_bytes_len = user_bmp_length;

  //内核内存池的位图，此时的位图定在虚拟地址0xc009a000处，对应的物理地址就在0x9a000
  kernel_pool.pool_bitmap.bits = (void*)MEM_BITMAP_BASE;
  //用户内存池的位图，他紧跟着内核内存池位图之后
  user_pool.pool_bitmap.bits = (void*)(MEM_BITMAP_BASE+kernel_bmp_length);
  //输出内存池信息
  put_str("kernel_pool_bitmap_start:");
  put_int((int)kernel_pool.pool_bitmap.bits);
  put_str("kernel_pool_phy_addr_start:");
  put_int(kernel_pool.phy_addr_start);
  put_str("\n");
  put_str("user_pool_bitmap_start:");
  put_int((int)user_pool.pool_bitmap.bits);
  put_str("user_pool_phy_addr_start:");
  put_int(user_pool.phy_addr_start);
  put_str("\n");

  //将1M以下的相关位图置为0
  bitmap_init(&kernel_pool.pool_bitmap);
  bitmap_init(&user_pool.pool_bitmap);

  //初始化内核虚拟地址的位图，按照物理内存大小生成数组
  //用于维护内核堆的虚拟地址，和内核内存池大小一致
  kernel_vaddr.virtual_addr_bitmap.bimap_bytes_len = kernel_bmp_length;
  //位图的数组指向一块未使用的内存，目前紧挨在内核内存池和用户内存池之后
  kernel_vaddr.virtual_addr_bitmap.bits = \
   (void*)(MEM_BITMAP_BASE+kernel_bmp_length+user_bmp_length);
  //设置相应的起始地址
  kernel_vaddr.virtual_addr_start = K_HEADP_START;
  bitmap_init(&kernel_vaddr.virtual_addr_bitmap);
  put_str("mem_pool_init done\n");
}

//内存管理初始化的入口
void mem_init() {
  put_str("mem_init start\n");
  //获取物理内存容量
  uint32_t mem_bytes_total = (*(uint32_t*)(0xb00));
  //初始化内存池
  mem_pool_init(mem_bytes_total);
  put_str("mem_init done\n");
}
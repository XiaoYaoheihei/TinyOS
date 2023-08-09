#include "memory.h"
#include "debug.h"
#include "../lib/stdint.h"
#include "../lib/kernel/print.h"
#include "../lib/kernel/bitmap.h"
//一页大小是4096B
#define PG_SIZE 4096
//内存位图基地址，此时对应的是虚拟地址，虚拟地址对应的物理地址是在1MB内部
#define MEM_BITMAP_BASE 0XC009A000
//堆的起始虚拟地址
#define K_HEADP_START   0XC0100000

//在页目录表中定位PDE，返回虚拟地址的高10位
#define PDE_IDX(addr) ((addr & 0xffc00000) >> 22)
//在页表中定位PTE，返回虚拟地址的中间10位
#define PTE_IDX(addr) ((addr & 0x003ff000) >> 12)

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

//在pf表示的对应的虚拟内存中申请cnt个虚拟页
//成功就返回虚拟页的起始地址，否则返回NULL
static void* vaddr_get(enum pool_flags pf, uint32_t pg_cnt) {
  int vaddr_start = 0, bit_idx_start = -1;
  uint32_t cnt = 0;
  //在内核虚拟地址池中申请地址
  if (pf == PF_KERNEL) {
    //在虚拟内存对应的位图中首先申请连续的pg_cnt个
    bit_idx_start = bitmap_scan(&kernel_vaddr.virtual_addr_bitmap, pg_cnt);
    if (bit_idx_start == -1) {
      return NULL;
    }
    while (cnt < pg_cnt) {
      //将对应的位图置1,表示对应的页已经使用
      bitmap_set(&kernel_vaddr.virtual_addr_bitmap, bit_idx_start+cnt, 1);
      cnt++;
    }
    //目前是直接在内核的堆中进行申请
    //得到虚拟地址的起始值
    vaddr_start = kernel_vaddr.virtual_addr_start+bit_idx_start*PG_SIZE;
  } else {
    //用户内存池，将来实现用户进程的时候再实现
  }
  return (void*)vaddr_start;
}

//得到虚拟地址vaddr对应的pte指针，此时指针的值也是虚拟地址
//我需要通过返回的指针值找到真正的pte物理地址
uint32_t* pte_ptr(uint32_t vaddr) {
  //先访问到页目录表
  //再用页目录项 pde（页目录内页表的索引）作为 pte 的索引访问到页表
  //再用pte的索引作为页内偏移
  //0xffc00000对应的是第1023个页目录项，此页目录项指向的是页目录表物理地址，是0x100000
  uint32_t* pte = (uint32_t*)(0xffc00000 + \
  ((vaddr & 0xffc00000) >> 10) + PTE_IDX(vaddr)*4);
  return pte;
}

//得到虚拟地址vaddr对应的pde指针，指针也是虚拟地址
//我需要通过返回的指针值找到真正的pde物理地址
uint32_t* pde_ptr(uint32_t vaddr) {
  //0xfffff用来访问到页目录表本身所在的地址
  uint32_t* pde = (uint32_t*)((0xfffff000) + PDE_IDX(vaddr)*4);
  return pde;
}

//在各自对应的物理内存池中分配1个物理页
//成功返回页框的物理地址，否则返回null
static void* palloc(struct pool* m_pool) {
  //在相应的位图中找到一个可分配的物理页
  int bit_idx = bitmap_scan(&m_pool->pool_bitmap, 1);
  if (bit_idx == -1) {
    return NULL;
  }
  //在位图中将此位bit_idx置为1
  bitmap_set(&m_pool->pool_bitmap, bit_idx, 1);
  //得到实际的物理页地址
  uint32_t page_phyaddr = ((bit_idx*PG_SIZE) + m_pool->phy_addr_start);
  return (void*)page_phyaddr;
}

//页表中添加虚拟地址vaddr和物理地址page_phyaddr的映射
static void page_table_add(void* _vaddr, void* _page_phyaddr) {
  uint32_t vaddr = (uint32_t)_vaddr, page_phyaddr = (uint32_t)_page_phyaddr;
  //计算虚拟地址应该对应的pde和pte
  uint32_t* pde = pde_ptr(vaddr);
  uint32_t* pte = pte_ptr(vaddr);

  //先在页目录表中判断目录项是否存在
  //页目录项和页表项的第 0 位为 P，若为 1，则表示该表已存在
  if (*pde & 0x00000001) {
    //页目录项存在
    ASSERT(!(*pte & 0x00000001));
    //只要是创建页表，pte就应该不存在
    if (!(*pte & 0x00000001)) {
      *pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
      // US=1,RW=1,P=1
    } else {
      PANIC("pte repeat");
      *pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
      // US=1,RW=1,P=1
    }
  } else {
    //页目录项不存在，所以要先创建页目录项再创建页表项
    //页表中用到的页框一律从内核物理空间分配
    uint32_t pde_phyaddr = (uint32_t)palloc(&kernel_pool);
    *pde = (pde_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
    //分配到的物理页地址对应的物理内存清0,避免里面的陈旧数据变成了页表项
    memset((void*)((int)pte & 0xfffff000), 0, PG_SIZE);
    ASSERT(!(*pte & 0x00000001));
    *pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
    //US=1,RW=1,P=1
  }
}

//分配cnt个页空间，成功则返回虚拟地址，失败返回NULL
void* malloc_page(enum pool_flags pf, uint32_t pg_cnt) {
  ASSERT(pg_cnt > 0 && pg_cnt < 3840);
  //由三部分构成
  //1.在虚拟内存池中申请虚拟地址
  //2.在物理内存池中申请物理地址
  //3.将虚拟地址和物理地址在页表中完成映射
  void* vaddr_start = vaddr_get(pf, pg_cnt);
  if (vaddr_start == NULL) {
    return NULL;
  }

  uint32_t vaddr = (uint32_t)vaddr_start, cnt = pg_cnt;
  struct pool* mem_pool = pf & PF_KERNEL ? &kernel_pool : &user_pool;

  //虚拟地址是连续的，但是物理地址可以不是连续的，所以逐个做映射
  while (cnt-- > 0) {
    void* page_phyaddr = palloc(mem_pool);
    if (page_phyaddr == NULL) {
      return NULL;
    }
    //在页表中做映射
    page_table_add((void*)vaddr, page_phyaddr);
    //下一个虚拟页
    vaddr += PG_SIZE;
  }
  return vaddr_start;
}

//从内核物理内存池申请cnt页内存，成功返回虚拟地址，失败返回null
void* get_kernel_pages(uint32_t pg_cnt) {
  void* vaddr = malloc_page(PF_KERNEL, pg_cnt);
  if (vaddr != NULL) {
    memset(vaddr, 0, pg_cnt*PG_SIZE);
  }
  return vaddr;
}

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
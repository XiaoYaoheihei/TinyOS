#include "memory.h"
#include "debug.h"
#include "../lib/stdint.h"
#include "../lib/kernel/print.h"
#include "../lib/kernel/bitmap.h"
#include "../lib/string.h"
#include "../thread/sync.h"

//一页大小是4096B
#define PG_SIZE 4096
//内存位图基地址，此时对应的是虚拟地址，虚拟地址对应的物理地址是在1MB内部
#define MEM_BITMAP_BASE 0XC009A000
//堆的起始虚拟地址
#define K_HEAP_START   0XC0100000

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
  //保证申请内存的时候互斥
  struct lock lock;
};
//内核物理内存池和用户物理内存池
struct pool kernel_pool, user_pool;
//用来给内核分配虚拟地址
struct virtual_addr kernel_vaddr;

//内存仓库
struct arena {
  //此arena关联的内存块描述符
  struct mem_block_desc* desc;
  //large 为 ture 时，cnt 表示的是页框数
  //否则 cnt 表示空闲 mem_block 数量
  uint32_t cnt;
  bool large;
};
//内核内存块描述符数组
struct mem_block_desc k_block_descs[DESC_CNT];

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
    struct task_struct* cur = running_thread();
    //从用户内存池中获取索引
    bit_idx_start = bitmap_scan(&cur->ueserprog_vaddr.virtual_addr_bitmap, pg_cnt);
    if (bit_idx_start == -1) {
      return NULL;
    }

    while (cnt < pg_cnt) {
      bitmap_set(&cur->ueserprog_vaddr.virtual_addr_bitmap, bit_idx_start+cnt, 1);
      cnt++;
    }
    vaddr_start = cur->ueserprog_vaddr.virtual_addr_start + bit_idx_start*PG_SIZE;
    // (0xc0000000 - PG_SIZE)作为用户 3 级栈已经在 start_process 被分配
    ASSERT((uint32_t)vaddr_start < (0xc0000000 - PG_SIZE));
  }
  return (void*)vaddr_start;
}

//得到虚拟地址vaddr所在的pte的虚拟地址
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

//得到虚拟地址vaddr所在的pde的虚拟地址
//我需要通过返回的指针值找到真正的pde物理地址
uint32_t* pde_ptr(uint32_t vaddr) {
  //0xfffff用来访问到页目录表本身所在的地址
  uint32_t* pde = (uint32_t*)((0xfffff000) + PDE_IDX(vaddr)*4);
  return pde;
}

//在各自对应的物理内存池中分配1个物理页
//成功返回页框的物理地址，否则返回null
static void* palloc(struct pool* m_pool) {
  //扫描或者设置位图要保证原子操作
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
  //计算虚拟地址应该对应的pde虚拟地址和pte虚拟地址
  uint32_t* pde = pde_ptr(vaddr);
  uint32_t* pte = pte_ptr(vaddr);

  //先在页目录表中判断目录项是否存在
  //页目录项和页表项的第 0 位为 P，若为 1，则表示该表已存在
  //c语言的*会自动去所在变量的物理地址处取值
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
    //页表用到的页框一律从内核物理空间分配
    //首先要给页表分配内存
    uint32_t pde_phyaddr = (uint32_t)palloc(&kernel_pool);
    //页目录项中填上对应的页表地址
    *pde = (pde_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
    //分配到的页表内对应的物理内存清0,避免里面的陈旧数据变成了页表项
    memset((void*)((int)pte & 0xfffff000), 0, PG_SIZE);
    ASSERT(!(*pte & 0x00000001));
    //页表项中填充好对应的物理地址
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
  lock_acquire(&kernel_pool.lock);
  void* vaddr = malloc_page(PF_KERNEL, pg_cnt);
  if (vaddr != NULL) {
    memset(vaddr, 0, pg_cnt*PG_SIZE);
  }
  lock_release(&kernel_pool.lock);
  return vaddr;
}

//在用户空间申请4k内存，并且返回其虚拟地址
void* get_user_pages(uint32_t pg_cnt) {
  lock_acquire(&user_pool.lock);
  void* vaddr = malloc_page(PF_USER, pg_cnt);
  memset(vaddr, 0, pg_cnt*PG_SIZE);
  lock_release(&user_pool.lock);
  return vaddr;
}

// 将地址 vaddr 与 pf 池中的物理地址关联，仅支持一页空间分配
void* get_a_page(enum pool_flags pf, uint32_t vaddr) {
  struct pool* mem_pool = pf & PF_KERNEL ? &kernel_pool : &user_pool;
  lock_acquire(&mem_pool->lock);
  //获取当前线程信息
  struct task_struct* cur = running_thread();
  int32_t bit_idx = -1;

  if (cur->pgdir != NULL && pf == PF_USER) {
    //当前用户进程申请内存，修改用户进程自己的虚拟地址位图
    bit_idx = (vaddr - cur->ueserprog_vaddr.virtual_addr_start)/PG_SIZE;
    //bit_idx的地址是可以和位图的地址重合的，下面之前写的是>0意思是不允许分配，但是其实是可以分配的
    //虚拟地址和位图的地址重合的话应该是可以分配的
    ASSERT(bit_idx >= 0);
    bitmap_set(&cur->ueserprog_vaddr.virtual_addr_bitmap, bit_idx, 1);
  } else if (cur->pgdir == NULL && pf == PF_KERNEL) {
    // 如果是内核线程申请内核内存，就修改 kernel_vaddr
    bit_idx = (vaddr - kernel_vaddr.virtual_addr_start)/PG_SIZE;
    //和前面的是一样的道理
    ASSERT(bit_idx >= 0);
    bitmap_set(&kernel_vaddr.virtual_addr_bitmap, bit_idx, 1);
  } else {
    PANIC("get_a_page:not allow kernel alloc userspace or \
    user alloc kernelspace by get_a_page");
  }

  void* page_phyaddr = palloc(mem_pool);
  if (page_phyaddr == NULL) {
    return NULL;
  }
  page_table_add((void*)vaddr, page_phyaddr);
  lock_release(&mem_pool->lock);
  return (void*)vaddr;

}

//得到虚拟地址对应的物理地址
uint32_t addr_v2p(uint32_t vaddr) {
  uint32_t* pte = pte_ptr(vaddr);
  // (*pte)的值是页表所在的物理页框地址
  // 去掉其低 12 位的页表项属性+虚拟地址 vaddr 的低 12 位
  return ((*pte & 0xfffff000) + (vaddr & 0x00000fff));
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

  //初始化锁结构
  lock_init(&kernel_pool.lock);
  lock_init(&user_pool.lock);

  //初始化内核虚拟地址的位图，按照物理内存大小生成数组
  //用于维护内核堆的虚拟地址，和内核内存池大小一致
  kernel_vaddr.virtual_addr_bitmap.bimap_bytes_len = kernel_bmp_length;
  //位图的数组指向一块未使用的内存，目前紧挨在内核内存池和用户内存池之后
  kernel_vaddr.virtual_addr_bitmap.bits = \
   (void*)(MEM_BITMAP_BASE+kernel_bmp_length+user_bmp_length);
  //设置相应的起始地址
  kernel_vaddr.virtual_addr_start = K_HEAP_START;
  bitmap_init(&kernel_vaddr.virtual_addr_bitmap);
  put_str("mem_pool_init done\n");
}

//返回 arena 中第 idx 个内存块的地址
static struct mem_block* arena2block(struct arena* a, uint32_t idx) {
  return (struct mem_block*)((uint32_t)a + sizeof(struct arena) + idx*a->desc->block_size);
}

//返回内存块 b 所在的 arena 地址
static struct arena* block2arena(struct mem_block* b) {
  return (struct arena*)((uint32_t)b & 0xfffff000);
}

//在堆中申请 size 字节内存
void* sys_malloc(uint32_t size) {
  enum pool_flags PF;
  struct pool* mem_pool;
  uint32_t pool_size;
  struct mem_block_desc* descs;
  struct task_struct* cur_thread = running_thread();

  if (cur_thread->pgdir == NULL) {
    //内核线程
    PF = PF_KERNEL;
    pool_size = kernel_pool.pool_size;
    mem_pool = &kernel_pool;
    descs = k_block_descs;
  } else {
    //用户进程 pcb 中的 pgdir 会在为其分配页表时创建
    PF = PF_USER;
    pool_size = user_pool.pool_size;
    mem_pool = &user_pool;
    descs = cur_thread->u_block_desc;
  }

  //若申请的内存不在内存池容量范围内，则直接返回 NULL
  if (!(size > 0) && size < pool_size) {
    return NULL;
  }
  struct arena* a;
  struct mem_block* b;
  //获取内存池的锁
  lock_acquire(&mem_pool->lock);

  //超过最大内存块 1024，就分配页框
  if (size > 1024) {
    uint32_t page_cnt = DIV_ROUND_UP(size + sizeof(struct arena), PG_SIZE);
    a = malloc_page(PF, page_cnt);
    if (a != NULL) {
      //将分配的内存清 0
      memset(a, 0, page_cnt*PG_SIZE);
      
      //对于分配的大块页框，将 desc 置为 NULL
      //cnt 置为页框数，large 置为 true
      a->desc = NULL;
      a->cnt = page_cnt;
      a->large = true;
      lock_release(&mem_pool->lock);
      //跨过 arena 大小，把剩下的内存返回
      return (void*)(a+1);
    } else {
      //释放锁，分配失败
      lock_release(&mem_pool->lock);
      return NULL;
    }
  } else {
    //若申请的内存小于等于 1024
    //可在各种规格的 mem_block_desc 中去适配
    uint8_t desc_idx;
    for (desc_idx = 0; desc_idx < DESC_CNT; desc_idx++) {
      if (size <= descs[desc_idx].block_size) {
        //从小往大后，找到后退出
        break;
      }
    }

    // 若 mem_block_desc 的 free_list 中已经没有可用的 mem_block，
    // 就创建新的 arena 提供 mem_block
    if (list_empty(&descs[desc_idx].free_list)) {
      //分配 1 页框作为 arena
      a = malloc_page(PF, 1);
      if (a == NULL) {
        lock_release(&mem_pool->lock);
        return NULL;
      }
      memset(a, 0, PG_SIZE);

      // 对于分配的小块内存，将 desc 置为相应内存块描述符
      // cnt 置为此 arena 可用的内存块数,large 置为 false
      a->desc = &descs[desc_idx];
      a->large = false;
      a->cnt = descs[desc_idx].blocks_per_arena;
      uint32_t block_idx;

      enum intr_status old_status = intr_disable();
      //开始将 arena 拆分成内存块，并添加到内存块描述符的 free_list 中
      for (block_idx = 0; block_idx < descs[desc_idx].blocks_per_arena; block_idx++) {
        b = arena2block(a, block_idx);
        ASSERT(!elem_find(&a->desc->free_list, &b->free_elem));
        list_append(&a->desc->free_list, &b->free_elem);
      }
      intr_set_status(old_status);
    }
    
    //存在够用的内存块，直接开始分配内存块,不用再申请一页之后分配了
    b = elem2entry(struct mem_block, free_elem, list_pop(&(descs[desc_idx].free_list)));
    memset(b, 0, descs[desc_idx].block_size);

    //获取内存块 b 所在的 arena
    a = block2arena(b);
    //将此 arena 中的空闲内存块数减 1
    a->cnt--;
    lock_release(&mem_pool->lock);
    return (void*)b;
  }
}


//为 malloc 做准备
void block_desc_init(struct mem_block_desc* desc_array) {
  //16,32,64,128,256,512,1024
  uint16_t desc_idx, block_size = 16;
  //初始化每个 mem_block_desc 描述符
  for (desc_idx = 0; desc_idx < DESC_CNT; desc_idx++) {
    desc_array[desc_idx].block_size = block_size;
    //初始化内存块数量
    desc_array[desc_idx].blocks_per_arena = (PG_SIZE-sizeof(struct arena))/block_size;
    list_init(&desc_array[desc_idx].free_list);
    //更新为下一个规格内存块
    block_size *= 2;
  }
}

//将物理地址 pg_phy_addr 回收到物理内存池
void pfree(uint32_t pg_phy_addr) {
  struct pool* mem_pool;
  //物理地址在相应物理内存池中的偏移量
  uint32_t bit_idx = 0;
  if (pg_phy_addr >= user_pool.phy_addr_start) {
    //用户物理内存池
    mem_pool = &user_pool;
    bit_idx = (pg_phy_addr - user_pool.phy_addr_start)/PG_SIZE;
  } else {
    //内核物理内存池
    mem_pool = &kernel_pool;
    bit_idx = (pg_phy_addr - kernel_pool.phy_addr_start)/PG_SIZE;
  }
  // 将位图中该位清 0
  bitmap_set(&mem_pool->pool_bitmap, bit_idx, 0);
}

//去掉页表中虚拟地址 vaddr 的映射，只去掉 vaddr 对应的 pte
static void page_table_pte_remove(uint32_t vaddr) {
  uint32_t* pte = pte_ptr(vaddr);
  // 将页表项 pte 的 P 位置 0
  *pte &= ~PG_P_1;
  //更新 tlb
  asm volatile ("invlpg %0": :"m"(vaddr):"memory");
}

//在虚拟地址池中释放以_vaddr 起始的连续 pg_cnt 个虚拟页地址
static void vaddr_remove(enum pool_flags pf, void* _vaddr, uint32_t pg_cnt) {
  uint32_t bit_idx_start = 0, vaddr = (uint32_t)_vaddr, cnt = 0;
  if (pf == PF_KERNEL) {
    //内核虚拟内存池
    //计算偏移量
    bit_idx_start = (vaddr - kernel_vaddr.virtual_addr_start)/PG_SIZE;
    while (cnt < pg_cnt) {
      bitmap_set(&kernel_vaddr.virtual_addr_bitmap, bit_idx_start+cnt, 0);
      cnt++;
    }
  } else {
    //用户虚拟内存池
    struct task_struct* cur_thread = running_thread();
    bit_idx_start = (vaddr - cur_thread->ueserprog_vaddr.virtual_addr_start)/PG_SIZE;
    while (cnt < pg_cnt) {
      bitmap_set(&cur_thread->ueserprog_vaddr.virtual_addr_bitmap, bit_idx_start+cnt, 0);
      cnt++;
    }
  }
}

//释放以虚拟地址 vaddr 为起始的 cnt 个物理页框
void mfree_page(enum pool_flags pf, void* _vaddr, uint32_t pg_cnt) {
  uint32_t pg_phy_addr;
  uint32_t vaddr = (int32_t)_vaddr, page_cnt = 0;
  ASSERT(pg_cnt >= 1 && vaddr % PG_SIZE == 0);
  //获取虚拟地址 vaddr 对应的物理地址
  pg_phy_addr = addr_v2p(vaddr);

  //确保待释放的物理内存在低端 1MB+1KB 大小的页目录+1KB 大小的页表地址范围外
  ASSERT((pg_phy_addr % PG_SIZE) == 0 && pg_phy_addr >= 0x102000);
  //判断 pg_phy_addr 属于用户物理内存池还是内核物理内存池
  if (pg_phy_addr >= user_pool.phy_addr_start) {
    //位于 user_pool 内存池
    vaddr -= PG_SIZE;
    while (page_cnt < pg_cnt) {
      vaddr += PG_SIZE;
      pg_phy_addr = addr_v2p(vaddr);
      
      //确保物理地址属于用户物理内存池
      ASSERT((pg_phy_addr % PG_SIZE) == 0 && pg_phy_addr >= user_pool.phy_addr_start); 
      // 先将对应的物理页框归还到内存池
      pfree(pg_phy_addr);
      //再从页表中清除此虚拟地址所在的页表项 pte
      page_table_pte_remove(vaddr);
      page_cnt++;
    }
    //清空虚拟地址的位图中的相应位
    vaddr_remove(pf, _vaddr, pg_cnt);
  } else {
    // 位于 kernel_pool 内存池
    vaddr -= PG_SIZE;
    while (page_cnt < pg_cnt) {
      vaddr += PG_SIZE;
      pg_phy_addr = addr_v2p(vaddr);

      //确保待释放的物理内存只属于内核物理内存池
      ASSERT((pg_phy_addr % PG_SIZE) == 0 && \
              pg_phy_addr >= kernel_pool.phy_addr_start && \
              pg_phy_addr < user_pool.phy_addr_start);
      //先将对应的物理页框归还到内存池
      pfree(pg_phy_addr);
      //再从页表中清除此虚拟地址所在的页表项 pte
      page_table_pte_remove(vaddr);
      page_cnt++;
    }

    vaddr_remove(pf, _vaddr, pg_cnt);
  }
}

//回收内存，接收内存指针
void sys_free (void* ptr) {
  ASSERT (ptr != NULL);
  if (ptr != NULL) {
    enum pool_flags PF;
    struct pool* mem_pool;

    //判断是线程或者是进程
    if (running_thread()->pgdir == NULL) {
      ASSERT ((uint32_t)ptr >= K_HEAP_START);
      PF = PF_KERNEL;
      mem_pool = &kernel_pool;
    } else {
      PF = PF_USER;
      mem_pool = &user_pool;
    }

    lock_acquire(&mem_pool->lock);
    struct mem_block* b = ptr;
    //把 mem_block 转换成 arena，获取元信息
    struct arena* a = block2arena(b);

    ASSERT(a->large == 0 || a->large == 1);
    if (a->desc == NULL && a->large == true) {
      //大于 1024 的内存
      mfree_page(PF, a, a->cnt);
    } else {
      // 小于等于 1024 的内存块
      //先将内存块回收到 free_list
      list_append(&a->desc->free_list, &b->free_elem);

      //再判断此 arena 中的内存块是否都是空闲,如果是就释放 arena
      if (++a->cnt == a->desc->blocks_per_arena) {
        uint32_t block_idx;
        for (block_idx = 0; block_idx < a->desc->blocks_per_arena; block_idx++) {
          struct mem_block*  b = arena2block(a, block_idx);
          ASSERT(elem_find(&a->desc->free_list, &b->free_elem));
          list_remove(&b->free_elem);
        }
        mfree_page(PF, a, 1);
      }
    }
    lock_release(&mem_pool->lock);
  }
}

//安装 1 页大小的 vaddr，专门针对 fork 时虚拟地址位图无需操作的情况
void* get_a_page_without_opvaddrbitmap(enum pool_flags pf, uint32_t vaddr) {
  struct pool* mem_pool = pf & PF_KERNEL ? &kernel_pool : &user_pool;
  lock_acquire(&mem_pool->lock);
  void* page_phyaddr = palloc(mem_pool);
  if (page_phyaddr == NULL) {
    lock_release(&mem_pool->lock);
    return NULL;
  }
  page_table_add((void*)vaddr, page_phyaddr);
  lock_release(&mem_pool->lock);
  return (void*)vaddr;
}

//内存管理初始化的入口
void mem_init() {
  put_str("mem_init start\n");
  //获取物理内存容量
  uint32_t mem_bytes_total = (*(uint32_t*)(0xb00));
  //初始化内存池
  mem_pool_init(mem_bytes_total);
  //初始化 mem_block_desc 数组 descs，为 malloc 做准备
  block_desc_init(k_block_descs);
  put_str("mem_init done\n");
}

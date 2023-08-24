#ifndef _DEVICE_IDE_H
#define _DEVICE_IDE_H

#include "../lib/stdint.h"
#include "../thread/sync.h"
#include "../lib/kernel/bitmap.h"

//分区结构
struct partition {
  //起始扇区
  uint32_t start_lba;
  //扇区数
  uint32_t sec_cnt;
  //分区所属的硬盘
  struct disk* my_disk;
  //用于队列中的标记
  struct list_elem part_tag;
  //分区名称
  char name[8];
  //本分区的超级块
  struct super_block* sb;
  //块位图
  struct bitmap block_bitmap;
  //i 结点位图
  struct bitmap inode_bitmap;
  //本分区打开的 i 结点队列
  struct list open_inodes;
};

//硬盘结构
struct disk {
  //本硬盘的名称
  char name[8];
  //此块硬盘归属于哪个 ide 通道
  struct ide_channel* my_channel;
  //本硬盘是主 0，还是从 1
  uint8_t dev_no;
  //主分区顶多是 4 个
  struct partition prim_parts[4];
  //逻辑分区数量无限，但总得有个支持的上限，那就支持 8 个
  struct partition logic_parts[8];
};

//ata通道结构
struct ide_channel {
  //本 ata 通道名称
  char name[8];
  //本通道的起始端口号
  uint16_t port_base;
  //本通道所用的中断号
  //在硬盘的中断处理程序中要根据中断号来判断在哪个通道中操作
  uint8_t irq_no;
  //通道锁
  struct lock lock;
  //表示等待硬盘的中断
  bool expecting_intr;
  //用于阻塞、唤醒驱动程序
  struct semaphore disk_done;
  //一个通道上连接两个硬盘，一主一从
  struct disk devices[2];
};

void ide_init(void);
extern uint8_t channel_cnt;
extern struct ide_channel channels[];
#endif
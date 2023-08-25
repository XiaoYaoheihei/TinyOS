#ifndef _FS_INODE_H
#define _FS_INODE_H
#include "stdint.h"
#include "list.h"

struct inode {
  // inode 编号
  uint32_t i_no;
  //当此 inode 是文件时，i_size 是指文件大小
  //若此 inode 是目录，i_size 是指该目录下所有目录项大小之和
  uint32_t i_size;
  //记录此文件被打开的次数
  uint32_t i_open_cnts;
  //写文件不能并行，进程写文件前检查此标识
  bool write_deny;

  //i_sectors[0-11]是直接块，i_sectors[12]用来存储一级间接块指针
  uint32_t i_sectors[13];
  //每次打开文件的时候都需要从磁盘上加载inode
  //为了避免每次都加载，把inode放到内存缓存中
  struct list_elem inode_tag;
};

#endif
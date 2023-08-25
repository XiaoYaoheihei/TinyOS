#ifndef _FS_DIR_H
#define _FS_DIR_H

#include "stdint.h"
#include "inode.h"
#include "ide.h"
#include "fs.h"
#include "global.h"

// 最大文件名长度
#define MAX_FILE_NAME_LEN 16

//目录结构，他不是磁盘上存在的
//我们在进行目录相关的操作的时候，在内存中创建的数据结构
struct dir {
  struct inode* inode;
  //记录在目录内的偏移
  uint32_t dir_pos;
  //目录的数据缓存,读取目录的时候返回存储的内容
  uint8_t dir_buf[512];
};

//目录项结构
struct dir_entry{
  // 普通文件或目录名称
  char filename[MAX_FILE_NAME_LEN];
  // 普通文件或目录对应的 inode 编号
  uint32_t i_no;
  // 文件类型
  enum file_types f_type;
};

#endif
#ifndef _FS_FILE_H
#define _FS_FILE_H

#include "stdint.h"
#include "ide.h"
#include "dir.h"
// 系统可打开的最大文件数
#define MAX_FILE_OPEN 32
//文件结构，文件表中的内容
struct file {
  //记录当前文件操作的偏移地址，以 0 为起始，最大为文件大小-1
  uint32_t fd_pos;
  //文件操作标识，如 O_RDONLY
  uint32_t fd_flag;
  //inode 指针，用来指向 inode 队列（part-> open_inodes）中的 inode
  struct inode* fd_inode;
};

//标准输入输出描述符
enum std_fd {
  stdin_no,   //0 标准输入
  stdout_no,  //1 标准输出
  stderr_no   //2 标准错误
};

//位图类型
enum bitmap_type {
  INODE_BITMAP, //inode位图
  BLOCK_BITMAP  //块位图
};

extern struct file file_table[MAX_FILE_OPEN];

int32_t inode_bitmap_alloc(struct partition* part);
int32_t block_bitmap_alloc(struct partition* part);
int32_t file_create(struct dir* parent_dir, char* filename, uint8_t flag);
void bitmap_sync(struct partition* part, uint32_t bit_idx, uint8_t btmp);
int32_t get_free_slot_in_global(void);
int32_t pcb_fd_install(int32_t globa_fd_idx);

int32_t file_open(uint32_t inode_no, uint8_t flag);
int32_t file_close(struct file* file);
#endif
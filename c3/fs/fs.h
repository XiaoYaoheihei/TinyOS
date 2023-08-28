#ifndef _FS_FS_H
#define _FS_FS_H
#include "stdint.h"
#include "ide.h"

// 每个分区所支持最大创建的文件数
#define MAX_FILES_PER_PART 4096
// 每扇区的位数
#define BITS_PER_SECTOR 4096
// 扇区字节大小
#define SECTOR_SIZE 512
//块字节大小
#define BLOCK_SIZE SECTOR_SIZE
//路径最大长度
#define MAX_PATH_LEN 512

//文件类型
enum file_types {
  //不支持的文件类型
  FT_UNKNOWN,
  //普通文件
  FT_REGULAR,
  //目录
  FT_DIRECTORY
};

//打开文件的选项
enum oflags {
  O_RDONLY,     //只读
  O_WRONLY,     //只写
  O_RDWR,       //读写
  O_CREAT = 4   //创建
};

//文件读写位置偏移量
enum whence {
  SEEK_SET = 1,
  SEEK_CUR,
  SEEK_END
};


//用来记录查找文件过程中已找到的上级路径
//查找文件过程中“走过的地方”
struct path_search_record{
  //查找处理过程中的父路径
  char searched_path[MAX_PATH_LEN];
  //文件或目录所在的直接父目录
  struct dir* parent_dir;
  //找到的是普通文件，还是目录，找不到将为未知类型(FT_UNKNOWN)
  enum file_types file_type;
};

// 默认情况下操作的分区
extern struct partition* cur_part;

void filesys_init();
int32_t path_depth_cnt(char* pathname);
int32_t sys_open(const char* pathname, uint8_t flags);
int32_t sys_close(int32_t fd);
int32_t sys_write(int32_t fd, const void* buf, uint32_t count);
int32_t sys_read(int32_t fd, void* buf, uint32_t count);
int32_t sys_lseek(int32_t fd, int32_t offset, uint8_t whence);
int32_t sys_unlink(const char* pathname);
int32_t sys_mkdir(const char* pathname);
#endif
#include "file.h"
#include "fs.h"
#include "super_block.h"
#include "inode.h"

#include "stdio_kernel.h"
#include "thread.h"


// file_table 是文件表，也就是文件结构数组
struct file file_table[MAX_FILE_OPEN];

//从文件表 file_table 中获取一个空闲位，成功返回下标，失败返回-1
int32_t get_free_slot_in_global() {
  uint32_t fd_idx = 3;
  while (fd_idx < MAX_FILE_OPEN) {
    if (file_table[fd_idx].fd_inode == NULL) {
      break;
    }
    fd_idx++;
  }
  if (fd_idx == MAX_FILE_OPEN) {
    printk("exceed max open files\n");
    return -1;
  }
  return fd_idx;
}

//将全局描述符下标安装到进程或线程自己的文件描述符数组 fd_table 中
//成功返回下标，失败返回-1
int32_t pcb_fd_install(int32_t globa_fd_idx) {
  struct task_struct* cur = running_thread();
  // 跨过 stdin,stdout,stderr
  uint8_t local_fd_idx = 3;
  while (local_fd_idx < MAX_FILES_OPEN_PER_PROC) {
    if (cur->fd_table[local_fd_idx] == -1) {
      // -1 表示 free_slot，可用
      cur->fd_table[local_fd_idx] = globa_fd_idx;
      break;
    }
    local_fd_idx++;
  }
  if (local_fd_idx == MAX_FILES_OPEN_PER_PROC) {
    printk("exceed max open files_per_proc\n");
    return -1;
  }
  return local_fd_idx;
}

//分配一个 i 结点，返回 i 结点号
int32_t inode_bitmap_alloc(struct partition* part) {
  int32_t bit_idx = bitmap_scan(&part->inode_bitmap, 1);
  if (bit_idx == -1) {
    return -1;
  }
  bitmap_set(&part->inode_bitmap, bit_idx, 1);
  return bit_idx;
}

//分配 1 个扇区，返回其扇区地址
int32_t block_bitmap_alloc(struct partition* part) {
  int32_t bit_idx = bitmap_scan(&part->block_bitmap, 1);
  if (bit_idx == -1) {
    return -1;
  }
  bitmap_set(&part->block_bitmap, bit_idx, 1);
  // 和 inode_bitmap_malloc 不同，此处返回的不是位图索引
  // 而是具体可用的扇区地址
  return (part->sb->data_start_lba+bit_idx);
}

//将内存中 bitmap 第 bit_idx 位所在的 512 字节同步到硬盘
//将块位图或者inode位图对应的字节同步到硬盘中
void bitmap_sync(struct partition* part, uint32_t bit_idx, uint8_t btmp) {
  // 本 i 结点索引相对于位图的扇区偏移量
  uint32_t off_sec = bit_idx / 4096;
  //本 i 结点索引相对于位图的字节偏移量
  uint32_t off_size = off_sec * BLOCK_SIZE;

  uint32_t sec_lba;
  uint8_t* bitmap_off;
  // 需要被同步到硬盘的位图只有 inode_bitmap 和 block_bitmap
  switch (btmp) {
    case INODE_BITMAP:
      sec_lba = part->sb->inode_bitmap_lba+off_sec;
      bitmap_off = part->inode_bitmap.bits+off_size;
      break;

    case BLOCK_BITMAP:
      sec_lba = part->sb->block_bitmap_lba+off_sec;
      bitmap_off = part->block_bitmap.bits+off_size;
      break;
  }
  ide_write(part->my_disk, sec_lba, bitmap_off, 1);
}

//创建文件，若成功则返回文件描述符，否则返回-1
int32_t file_create(struct dir* parent_dir, char* filename, uint8_t flag) {
  //后续操作的公共缓冲区
  void* io_buf = sys_malloc(1024);
  if (io_buf == NULL) {
    printk("in file_creat: sys_malloc for io_buf failed\n");
    return -1;
  }
  //用于操作失败时回滚各资源状态
  uint8_t rollback_step = 0;
  //为新文件分配 inode,从inode位图中申请
  int32_t inode_no = inode_bitmap_alloc(cur_part);
  if (inode_no == -1) {
    printk("in file_creat: allocate inode failed\n");
    return -1;
  }
  //具体的inode内容要从堆中申请内存，不可生成局部变量（函数退出时会释放）
  //因为 file_table 数组中的文件描述符的 inode 指针要指向它
  struct inode* new_file_inode = (struct inode*)sys_malloc(sizeof(struct inode));
  if (new_file_inode == NULL) {
    printk("file_create: sys_malloc for inode failded\n");
    rollback_step = 1;
    goto rollback;
  }
  //初始化 i 结点
  inode_init(inode_no, new_file_inode);
  //返回的是 file_table 数组的下标
  int fd_idx = get_free_slot_in_global();
  if (fd_idx == -1) {
    printk("exceed max open files\n");
    rollback_step = 2;
    goto rollback;
  }
  //
  file_table[fd_idx].fd_inode = new_file_inode;
  file_table[fd_idx].fd_pos = 0;
  file_table[fd_idx].fd_flag = flag;
  file_table[fd_idx].fd_inode->write_deny = false;

  struct dir_entry new_dir_entry;
  memset(&new_dir_entry, 0, sizeof(struct dir_entry));
  create_dir_entry(filename, inode_no, FT_REGULAR, &new_dir_entry);

  //开始同步数据到硬盘中
  //1.在目录 parent_dir 下安装目录项 new_dir_entry，写入硬盘后返回 true，否则 false
  if (!sync_dir_entry(parent_dir, &new_dir_entry, io_buf)) {
    printk("sync dir_entry to disk failed\n");
    rollback_step = 3;
    goto rollback;
  }
  memset(io_buf, 0, 1024);
  //2.将父目录 i 结点的内容同步到硬盘
  inode_sync(cur_part, parent_dir->inode, io_buf);
  memset(io_buf, 0, 1024);
  //3.将新创建文件的 i 结点内容同步到硬盘
  inode_sync(cur_part, new_file_inode, io_buf);
  //4.将 inode_bitmap 位图同步到硬盘
  bitmap_sync(cur_part, inode_no, INODE_BITMAP);
  //5.将创建的文件 i 结点添加到 open_inodes 链表
  list_push(&cur_part->open_inodes, &new_file_inode->inode_tag);
  new_file_inode->i_open_cnts = 1;
  sys_free(io_buf);
  return pcb_fd_install(fd_idx);

  // 创建文件需要创建相关的多个资源,若某步失败则会执行到下面的回滚步骤
  rollback:
    switch (rollback_step) {
      case 3:
        //失败时，将 file_table 中的相应位清空
        memset(&file_table[fd_idx], 0, sizeof(struct file));
      case 2:
        sys_free(new_file_inode);
      case 1:
        //如果新文件在堆中的i 结点创建失败,之前位图中分配的 inode_no 也要恢复
        bitmap_set(&cur_part->inode_bitmap, inode_no, 0);
        break;
    }
    sys_free(io_buf);
    return -1;
}

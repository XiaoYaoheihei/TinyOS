#ifndef _LIB_KERNEL_BITMAP_H
#define _LIB_KERNEL_BITMAP_H

#include "../../kernel/global.h"
//位掩码，逐位进行判断，按位&来判断相应的位是否是1
#define BITMAP_MASK 1
struct bitmap {
  uint32_t bimap_bytes_len;
  //在遍历位图的时候整体是以B为单位
  //细节上是以位为单位
  uint8_t* bits;  //位图指针
};

void bitmap_init(struct bitmap* btmp);
bool bitmap_scan_test(struct bitmap* btmp, uint32_t bit_idx);
int bitmap_scan(struct bitmap* btmp, uint32_t cnt);
void bitmap_set(struct bitmap* btmp, uint32_t bit_idx, int8_t value);
#endif
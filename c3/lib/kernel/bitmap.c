#include "bitmap.h"
#include "../string.h"
#include "../../kernel/debug.h"

//将位图初始化
//根据位图的大小将位图的每一个B用0来填充
void bitmap_init(struct bitmap* btmp) {
  memset(btmp->bits, 0, btmp->bimap_bytes_len);
}

//判断bit_idx位是否为1,若为1返回true否则返回false
bool bitmap_scan_test(struct bitmap* btmp, uint32_t bit_idx) {
  //找到索引数组下标
  uint32_t byte_idx = bit_idx/8;
  //找到在B内的偏移
  uint32_t bit_odd = bit_idx%8;
  //判断固定的位是否为1
  return (btmp->bits[byte_idx] & (BITMAP_MASK << bit_odd));
}

//在位图中申请连续cnt个位，成功返回起始下标，否则返回-1
int bitmap_scan(struct bitmap* btmp, uint32_t cnt) {
  //记录已经使用的B，用来判断空闲B的位置
  uint32_t idx_byte = 0;
  //逐个字节比较
  while ((0xff == btmp->bits[idx_byte]) && (idx_byte < btmp->bimap_bytes_len)) {
    idx_byte++;
  }
  ASSERT(idx_byte < btmp->bimap_bytes_len);
  if(idx_byte == btmp->bimap_bytes_len) {
    //找不到可用空间
    return -1;
  }
  //在位图数组中的某一个B找到了空闲位
  //在该B逐位比较，返回空闲位的索引
  int idx_bit = 0;
  //和btmp->bits[idx_byte]逐个位比较
  while ((uint8_t)(BITMAP_MASK << idx_bit) && btmp->bits[idx_byte]) {
    idx_bit++;
  }

  //空闲位在位图中的下标
  int bit_idx_start = idx_byte*8+idx_bit;
  if (cnt == 1) {
    //如果只是申请一个空闲位的话
    return bit_idx_start;
  }
  //申请多个空闲位
  //记录还有多少位可以判断
  uint32_t bit_left = (btmp->bimap_bytes_len*8-bit_idx_start);
  //记录位图中下一个待查找的位，他是相对于整个位图的位下标
  uint32_t next_bit = bit_idx_start + 1;
  //记录找到空闲位的个数
  uint32_t count = 1;
  //先将其置为−1，默认的情况是找不到
  bit_idx_start = -1;
  while (bit_left-- > 0) {
    //若next_bit==0
    if (!(bitmap_scan_test(btmp, next_bit))) {
      count++;
    } else {
      count=0;
    }
    //找到连续的cnt个空位
    if (count == cnt) {
      bit_idx_start = next_bit - cnt + 1;
      break;
    }
    next_bit++;
  }
  return bit_idx_start;
}

//将位图的bit_idx位设置为value
void bitmap_set(struct bitmap* btmp, uint32_t bit_idx, int8_t value) {
  ASSERT((value == 0) || (value == 1));
  uint32_t byte_idx = bit_idx / 8;
  //在B中的偏移
  uint32_t bit_odd = bit_idx % 8;
  //将1进行任意移动之后再取反或者是先取反再移动，可对0来操作
  if (value) {    //value是1
    btmp->bits[byte_idx] |= (BITMAP_MASK << bit_odd);
  } else {        //value是0
    btmp->bits[byte_idx] &= ~(BITMAP_MASK << bit_odd);
  }
}
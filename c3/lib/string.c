#include "string.h"
#include "../kernel/debug.h"
#include "../kernel/global.h"

//将从dst开始的size个字节置为value
void memset(void* dst_, uint8_t value, uint32_t size) {
  ASSERT(dst_ != NULL);
  uint8_t* dst = (uint8_t*)dst_;
  while (size-- > 0) {
    *dst++ = value;
  }
}

//将从src开始的size个字节复制到dst处
void memcpy(void* dst_, const void* src_, uint32_t size) {
  ASSERT(dst_ != NULL && src_ != NULL);
  uint8_t* dst = (uint8_t*)dst_;
  const uint8_t* src = (uint8_t*)src_;
  while (size-- > 0){
    *dst++ = *src++;
  }
}

//比较以地址a_和地址b_开头的size个字节
//相等就返回0,a_>b_返回1,否则返回-1
int memcmp(const void* a_, const void* b_, uint32_t size) {
  const char* a = a_;
  const char* b = b_;
  ASSERT(a != NULL || b != NULL);
  while (size-- > 0) {
    if (*a != *b) {
      return *a > *b ? 1 :-1;
    }
    a++;
    b++;
  }
  return 0;
}

//字符串复制
char* strcpy(char* dst_,const char* src_) {
  ASSERT(dst_ != NULL && src_ != NULL);
  char* r = dst_;   //返回目的字符串地址
  //脑瘫了，写了一个*src != *dst的判读条件
  //这样肯定会造成缺页异常啊！！！！
  while(*src_) {  
    *dst_ = *src_;
    dst_++;
    src_++; 
  }
  return r;
}

//字符串长度
uint32_t strlen(const char* str) {
  ASSERT(str != NULL);
  const char* p = str;
  while (*p++);
  return (p-str-1);
}

//字符串比较
int8_t strcmp(const char* a, const char* b) {
  ASSERT(a != NULL && b!= NULL);
  while (*a != 0 && *a == *b) {
    a++;
    b++;
  }
  //<的话返回-1
  //否则就是*a>*b表达式的值
  //为真则为1,否则则为0
  return *a < *b ? -1 : *a > *b;
}

//从左往右查找字符
char* strchar(const char* str, const uint8_t ch) {
  ASSERT(str != NULL);
  while (*str != 0) {
    if (*str == ch) {
      return (char*)str;  //强制类型转化成和返回类型一致
    }
    str++;
  }
  return NULL;
}

//从右往左查找字符
char* strrchar(const char* str, const uint8_t ch) {
  ASSERT(str != NULL);
  const char* last = NULL;
  while (*str != 0) {
    if (*str == ch) {
      last = str;
      break;
    }
    str++;
  }
  return (char*)last;
}

//拼接字符串
char* strcat(char* dst_, const char* src_) {
  ASSERT(dst_ != NULL && src_ != NULL);
  char* str = dst_;
  while (*str++);
  --str;//向前移动一位
  //当结尾都被赋予0的时候，while循环结束
  // 当*str被赋值为0时,此时表达式不成立,正好添加了字符串结尾的0.
  while ((*str++ = *src_++));
  return dst_;
}

//字符串中字符出现的次数
uint32_t strchars(const char* str, uint8_t ch) {
  ASSERT(str != NULL);
  uint32_t ch_cnt = 0;
  const char* p = str;
  while (*p != 0) {
    if (*p == ch) {
      ch_cnt++;
    }
    p++;
  }
  return ch_cnt;
}


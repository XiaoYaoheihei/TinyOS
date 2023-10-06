#include "stdio.h"
#include "../kernel/global.h"
#include "string.h"

//把 ap 指向第一个固定参数 v的地址
#define va_start(ap, v) ap = (va_list)&v 
//ap 指向下一个参数并返回其值，栈中的参数占4字节，所以+4
//压入栈中的参数是字符串的地址
#define va_arg(ap, t) *((t*)(ap += 4))
//清除 ap
#define va_end(ap) ap = NULL

//将整型转换成字符
static void itoa(uint32_t value, char** buf_ptr_addr, uint8_t base) {
  // 求模，最先掉下来的是最低位
  uint32_t m = value % base;
  //取整
  uint32_t i = value / base;
  // 如果倍数不为 0，则递归调用
  if (i) {
    itoa(i, buf_ptr_addr, base);
  }
  // 如果余数是 0～9
  if (m < 10) {
    //将数字 0～9 转换为字符'0'～'9'
    //*buf_ptr_addr得到临时变量中的值，是个指针
    //进行++操作，然后再*可以得到该指针指向的值
    *((*buf_ptr_addr)++) = m + '0';
  } else {
    //将数字 A～F 转换为字符'A'～'F'
    *((*buf_ptr_addr)++) = m - 10 + 'A';
  }
}


//将参数ap 按照格式 format 输出到字符串 str，并返回替换后 str 长度
uint32_t vsprintf(char* str, const char* format, va_list ap) {
  char* buf_ptr = str;
  const char* index_ptr = format;
  char index_char = *index_ptr;
  //用来处理整数
  int32_t arg_int;
  //用来处理字符串
  char* arg_str;
  while (index_char) {
    if (index_char != '%') {
      *(buf_ptr++) = index_char;
      index_char = *(++index_ptr);
      continue;
    }
    //得到%后面的字符
    index_char = *(++index_ptr);
    switch (index_char) {
    case 's'://%s
      arg_str = va_arg(ap, char*);
      strcpy(buf_ptr, arg_str);
      buf_ptr += strlen(arg_str);
      index_char = *(++index_ptr);
      break;

    case 'c'://%c
      *(buf_ptr++) = va_arg(ap, char);
      index_char = *(++index_ptr);
      break;

    case 'd'://%d
      arg_int = va_arg(ap, int);
      //若是负数，将其转为正数后，在正数前面输出个负号'-'
      if (arg_int < 0) {
        arg_int = 0-arg_int;
        *buf_ptr++ = '-';
      }
      itoa(arg_int, &buf_ptr, 10);
      index_char = *(++index_ptr);
      break;

    case 'x':
      //具体的值arg_int
      arg_int  = va_arg(ap, int);
      itoa(arg_int, &buf_ptr, 16);
      index_char = *(++index_ptr);
      //跳过格式字符并更新 index_char
      break;
    }
  }
  return strlen(str);
}

//同 printf 不同的地方就是字符串不是写到终端，而是写到 buf 中
uint32_t sprintf(char* buf, const char* format, ...) {
  va_list args;
  uint32_t retval;
  va_start(args, format);
  retval = vsprintf(buf, format, args);
  va_end(args);
  return retval;
}


//格式化输出字符串format
uint32_t printf(const char* format, ...) {
  va_list args;
  //使 args 指向 format
  va_start(args, format);
  //用于存储拼接后的字符串
  char buf[1024] = {0};
  vsprintf(buf, format, args);
  va_end(args);
  return write(1,buf,strlen(buf));
}


#include "print.h"
#include "init.h"

void main() {
  put_str("I am kernel\n");
  // put_int(0);
  // put_char('\n');
  // put_int(9);
  // put_char('\n');
  // put_int(0x00021a3f);
  // put_char('\n');
  // put_int(0x12345678);
  // put_char('\n');
  // put_int(0x00000000);
  init_all();
  asm volatile("sti");
  while(1);
}

//ld main.o -Ttext 0xc0001500 -e main -o kernel.bin
//显示使用-e指明可执行文件的程序入口是main
//因为一般在链接完成之后会有_start的默认地址，他是默认的程序入口
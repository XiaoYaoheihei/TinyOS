#include "print.h"
#include "init.h"
#include "debug.h"
#include "memory.h"

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
  //检验assert函数的正确性
  // ASSERT(1==2);
  // asm volatile("sti");
  void* addr = get_kernel_pages(3);
  put_str("\n get_kernel_page start virtual_addr is:");
  put_int((uint32_t)addr);
  put_str("\n");
  while(1);
}

//ld main.o -Ttext 0xc0001500 -e main -o kernel.bin
//显示使用-e指明可执行文件的程序入口是main
//因为一般在链接完成之后会有_start的默认地址，他是默认的程序入口
// #include "print.h"
void main() {
  // put_char('k');
  // put_char('e');
  // put_char('r');
  // put_char('n');
  // put_char('e');
  // put_char('l');
  // put_char('\n');
  // put_char('1');
  // put_char('2');
  // put_char('\b');
  // put_char('3');
  while(1);
}

//ld main.o -Ttext 0xc0001500 -e main -o kernel.bin
//显示使用-e指明可执行文件的程序入口是main
//因为一般在链接完成之后会有_start的默认地址，他是默认的程序入口
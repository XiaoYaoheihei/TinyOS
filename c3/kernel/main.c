#include "print.h"
#include "init.h"
#include "debug.h"
#include "memory.h"
#include "../thread/thread.h"
#include "../device/console.h"
#include "../device/keyboard.h"
#include "../userprog/process.h"
#include "../userprog/syscall_init.h"
#include "../lib/user/syscall.h"
#include "../lib/stdio.h"
#include "fs.h"
#include "dir.h"
#include "shell.h"


void init(void);

int main(void) {
   put_str("I am kernel\n");
   init_all();
   cls_screen();
   console_put_str("[rabbit@localhost /]$ ");
   while(1);
   return 0;
}

/* init进程 */
void init(void) {
   uint32_t ret_pid = fork();
   if(ret_pid) {  // 父进程
      while(1);
   } else {	  // 子进程
      my_shell();
   }
   PANIC("init: should not be here");
}
//ld main.o -Ttext 0xc0001500 -e main -o kernel.bin
//显示使用-e指明可执行文件的程序入口是main
//因为一般在链接完成之后会有_start的默认地址，他是默认的程序入口
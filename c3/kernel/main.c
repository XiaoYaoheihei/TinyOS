#include "print.h"
#include "init.h"
#include "assert.h"
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
   //os将程序写入hd80中,hd80中包含已经创建好的文件系统
   // uint32_t file_size = 24788;
   // uint32_t sec_cnt = DIV_ROUND_UP(file_size, 512);
   // printf("seccnt:%d ", sec_cnt);
   // struct disk* sda = &channels[0].devices[0];
   // void* prog_buf = sys_malloc(sec_cnt * SECTOR_SIZE);
   // ide_read(sda, 300, prog_buf, sec_cnt);
   // int32_t fd = sys_open("/cat", O_CREAT|O_RDWR);
   // if (fd != -1) {
   //    if (sys_write(fd, prog_buf, file_size) == -1) {
   //       printk("file write error!\n");
   //       while(1);
   //    }
   // }
   // cls_screen();
   // uint32_t fd = sys_open("/file1", O_RDWR);
   // printf("fd:%d\n", fd);
   // sys_write(fd, "hello,world\n", 12);
   // sys_close(fd);
   // printf("%d closed now\n", fd);
   console_put_str("[rabbit@localhost /]$ ");
   thread_exit(running_thread(), true);
   return 0;
}

/* init进程 */
void init(void) {
   uint32_t ret_pid = fork();
   if(ret_pid) {  // 父进程
      int status;
      int child_pid;
      //init 在此处不停地回收僵尸进程
      while (1) {
         child_pid = wait(&status);
         printf("I`m init, My pid is 1, I recieve a child,It`s pid is %d, status is %d\n", child_pid, status);
      }
   } else {	  // 子进程
      my_shell();
   }
   panic("init: should not be here");
}
//ld main.o -Ttext 0xc0001500 -e main -o kernel.bin
//显示使用-e指明可执行文件的程序入口是main
//因为一般在链接完成之后会有_start的默认地址，他是默认的程序入口
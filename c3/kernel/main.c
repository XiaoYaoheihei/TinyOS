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

void k_thread_a(void*);
void k_thread_b(void*);
void u_prog_a(void);
void u_prog_b(void);
// int test_var_a = 0, test_var_b = 0;
// int prog_a_pid = 0, prog_b_pid = 0;

void main() {
  put_str("I am kernel\n");
  init_all();
  //检验assert函数的正确性
  // ASSERT(1==2);
  // asm volatile("sti");
  //检验内存管理
  // void* addr = get_kernel_pages(3);
  // put_str("\n get_kernel_page start virtual_addr is:");
  // put_int((uint32_t)addr);
  // put_str("\n");
  //检验线程
  // enum intr_status old_status = intr_disable();
  //打开中断，使时钟中断起作用
  // intr_set_status(old_status);
  // intr_enable();
  // process_execute(u_prog_a, "user_a");
  // process_execute(u_prog_b, "user_b");
  // thread_start("con_thread_a", 31, k_thread_a, "A_ ");
  // thread_start("con_thread_b", 31, k_thread_b, "B_ ");
  
  sys_open("/file1", O_CREAT);
  
  // printf("/file1 delete %s!\n", sys_unlink("/file1") == 0 ? "done" : "fail");
  
  //想创建目录“/dir1/subdir1”,但是我们上一个版本已经删除目录dir1
  printf("/dir1/subdir1 create %s!\n", sys_mkdir("/dir1/subdir1") == 0 ? "done" : "fail");
  //先创建目录“/dir1”
  printf("/dir1 create %s!\n", sys_mkdir("/dir1") == 0 ? "done" : "fail");
  //重新创建目录“/dir1/subdir1”
  printf("now, /dir1/subdir1 create %s!\n", sys_mkdir("/dir1/subdir1") == 0 ? "done" : "fail");
  
  //现在后面的有bug,创建目录成功但是在目录下创建文件失败了
  //在目录“/dir1/subdir1”下创建文件“file2”
  int fd = sys_open("/dir1/subdir1/file2", O_CREAT|O_RDWR);
  if (fd != -1) {
      printf("/dir1/subdir1/file2 create done!\n");
      // 往文件“/dir1/subdir1/file2”中写数据“Catch me if you can!\n”
      sys_write(fd, "Catch me if you can!\n", 21);
      sys_lseek(fd, 0, SEEK_SET);
      char buf[32] = {0};
      sys_read(fd, buf, 21); 
      printf("/dir1/subdir1/file2 says:\n%s", buf);
      sys_close(fd);
  }  
   while(1);
  return 0;
}

// void k_thread_a(void* a) {
//   //void*来通用表示参数，被调用的函数需要知道自己需要什么类型的参数
//   char* tmp = a;
//   while(1) {
//     //关中断
//     enum intr_status old_status = intr_disable();
//     if (!ioq_empty(&kbd_buf)) {
//       console_put_str(tmp);
//       char byte = ioq_getchar(&kbd_buf);
//       console_put_char(byte);
//     }
//     //开中断
//     intr_set_status(old_status);
//   }
// }

// void k_thread_b(void* b) {
//   char* tmp = b;
//   while (1) {
//     enum intr_status old_status = intr_disable();
//     if (!ioq_empty(&kbd_buf)) {
//       console_put_str(tmp);
//       char byte = ioq_getchar(&kbd_buf);
//       console_put_char(byte);
//     }
//     //开中断
//     intr_set_status(old_status);
//   }
// }

/* 在线程中运行的函数 */
void k_thread_a(void* arg) {     
  void* addr1 = sys_malloc(256);
   void* addr2 = sys_malloc(255);
   void* addr3 = sys_malloc(254);
   console_put_str(" thread_a malloc addr:0x");
   console_put_int((int)addr1);
   console_put_char(',');
   console_put_int((int)addr2);
   console_put_char(',');
   console_put_int((int)addr3);
   console_put_char('\n');

   int cpu_delay = 1000000;
   //用来模拟内核线程虚拟地址空间可以累加的情况
   while(cpu_delay-- > 0);
   sys_free(addr1);
   sys_free(addr2);
   sys_free(addr3);
	while(1);
}

void k_thread_b(void* arg) {     
  void* addr1 = sys_malloc(256);
   void* addr2 = sys_malloc(255);
   void* addr3 = sys_malloc(254);
   console_put_str(" thread_b malloc addr:0x");
   console_put_int((int)addr1);
   console_put_char(',');
   console_put_int((int)addr2);
   console_put_char(',');
   console_put_int((int)addr3);
   console_put_char('\n');

   int cpu_delay = 100000;
   while(cpu_delay-- > 0);
   sys_free(addr1);
   sys_free(addr2);
   sys_free(addr3);
  while(1);
}

/* 测试用户进程 */
void u_prog_a(void) {
  void* addr1 = malloc(256);
   void* addr2 = malloc(255);
   void* addr3 = malloc(254);
   printf(" prog_a malloc addr:0x%x,0x%x,0x%x\n", (int)addr1, (int)addr2, (int)addr3);

   int cpu_delay = 100000;
   while(cpu_delay-- > 0);
   free(addr1);
   free(addr2);
   free(addr3);
	while(1);
}

/* 测试用户进程 */
void u_prog_b() {
  void* addr1 = malloc(256);
  void* addr2 = malloc(255);
  void* addr3 = malloc(254);
  printf(" prog_b malloc addr:0x%x,0x%x,0x%x\n", (int)addr1, (int)addr2, (int)addr3);

  int cpu_delay = 100000;
  while(cpu_delay-- > 0);
  free(addr1);
  free(addr2);
  free(addr3);
	while(1);
}
//ld main.o -Ttext 0xc0001500 -e main -o kernel.bin
//显示使用-e指明可执行文件的程序入口是main
//因为一般在链接完成之后会有_start的默认地址，他是默认的程序入口
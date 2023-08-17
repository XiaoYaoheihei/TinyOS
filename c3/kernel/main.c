#include "print.h"
#include "init.h"
#include "debug.h"
#include "memory.h"
#include "../thread/thread.h"
#include "../device/console.h"
#include "../device/keyboard.h"
#include "../userprog/process.h"

void k_thread_a(void*);
void k_thread_b(void*);
void u_prog_a(void);
void u_prog_b(void);
int test_var_a = 0, test_var_b = 0;

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
  //检验内存管理
  // void* addr = get_kernel_pages(3);
  // put_str("\n get_kernel_page start virtual_addr is:");
  // put_int((uint32_t)addr);
  // put_str("\n");
  //检验线程
  // enum intr_status old_status = intr_disable();
  // thread_start("con_thread_a", 31, k_thread_a, "A_ ");
  // thread_start("con_thread_b", 31, k_thread_b, "B_ ");
  process_execute(u_prog_a, "user_a");
  // process_execute(u_prog_b, "user_b");
  //打开中断，使时钟中断起作用
  // intr_set_status(old_status);
  intr_enable();
  while(1);
  // while(1) {
  //   console_put_str("Main ");
  // }
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
   char* para = arg;
   int n = 0;
   while(1) {
      console_put_str(" v_a:0x");
      console_put_int(test_var_a);
      console_put_str("\n");
   }
}

void k_thread_b(void* arg) {     
   char* para = arg;
   while(1) {
      console_put_str(" v_b:0x");
      console_put_int(test_var_b);
      console_put_str("\n");
   }
}

/* 测试用户进程 */
void u_prog_a(void) {
   while(1) {
      console_put_int(test_var_a);
      test_var_a++;
   }
}

/* 测试用户进程 */
void u_prog_b(void) {
   while(1) {
      test_var_b++;
   }
}
//ld main.o -Ttext 0xc0001500 -e main -o kernel.bin
//显示使用-e指明可执行文件的程序入口是main
//因为一般在链接完成之后会有_start的默认地址，他是默认的程序入口
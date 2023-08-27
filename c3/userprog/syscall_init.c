#include "syscall_init.h"
#include "../lib/user/syscall.h"
#include "../device/console.h"
#include "../lib/string.h"
#include "../kernel/memory.h"
#include "print.h"
#include "thread.h"
#include "fs.h"

#define syscall_nr 32
typedef void* syscall;
syscall syscall_table[syscall_nr];


//打印字符串str，未实现文件系统前的版本
// uint32_t sys_write(char* str) {
//   console_put_str(str);
//   return strlen(str);
// }

uint32_t sys_getpid(void) {
  return running_thread()->pid;  
}

void syscall_init(void) {
  put_str("syscall_init start\n");
  syscall_table[SYS_GETPID] = sys_getpid;
  syscall_table[SYS_WRITE] = sys_write;
  syscall_table[SYS_MALLOC] = sys_malloc;
  syscall_table[SYS_FREE] = sys_free;
  put_str("syscall_init done\n");
}

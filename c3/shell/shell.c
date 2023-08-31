#include "shell.h"
#include "stdint.h"
#include "global.h"
#include "fs.h"
#include "file.h"
#include "syscall.h"
#include "stdio.h"
#include "debug.h"
#include "string.h"

//最大支持键入 128 个字符的命令行输入
#define cmd_len 128
//加上命令名外，最多支持 15 个参数
#define MAX_ARG_NR 16

//存储输入的命令
static char cmd_line[cmd_len] = {0};

//用来记录当前目录，是当前目录的缓存,每次执行 cd 命令时会更新此内容
char cwd_cache[64] = {0};

//输出提示符
void print_promt() {
  printf("[rabbit@localhost %s]$ ", cwd_cache);
}

//从键盘缓冲区中最多读入 count 个字节到 buf
static void readline(char* buf, int32_t count) {
  ASSERT(buf != NULL && count > 0);
  char* pos = buf;
  while (read(stdin_no, pos, 1) != -1 && (pos - buf) < count) {
    //在不出错情况下，直到找到回车符才返回
    switch (*pos) {
    case '\n':
    case '\r':
      //添加 cmd_line 的终止字符 0
      *pos = 0;
      putchar('\n');
      return;

    case '\b':
      if (buf[0] != '\b') {
        // 阻止删除非本次输入的信息
        //退回到缓冲区 cmd_line 中上一个字符
        --pos;
        putchar('\b');
      }
      break;
    //非控制键则输出字符
    default:
      putchar(*pos);
      pos++;
    }
  }
  printf("readline: can't find enter_key in the cmdline, max num of char is 128\n");
}

//简单的 shell
void my_shell() {
  cwd_cache[0] = '/';
  while(1) {
    print_promt();
    memset(cmd_line, 0, cmd_len);
    readline(cmd_line, cmd_len);
    if (cmd_line[0] == 0) {
      //若只键入了一个回车
      continue;
    }
  }
  //出现了bug
  panic("my_shell: should not be here");
}
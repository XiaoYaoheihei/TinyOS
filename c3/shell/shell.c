#include "shell.h"
#include "buildin_cmd.h"
#include "exec.h"
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
// 用于洗路径时的缓冲
char final_path[MAX_PATH_LEN] = {0};      

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

    //ctrl+l 清屏
    case 'l'-'a':
      //1.先将当前的字符'l'-'a'置为 0
      *pos = 0;
      //2.再将屏幕清空
      clear();
      //3.打印提示符
      print_promt();
      //4.将之前键入的内容再次打印
      printf("%s", buf);
      break;

    //ctrl+u 清掉输入
    case 'u'-'a':
      while (buf != pos) {
        putchar('\b');
        *(pos--) = 0;
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

//分析字符串 cmd_str 中以 token 为分隔符的单词,将各单词的指针存入 argv 数组
static int32_t cmd_parse(char* cmd_str, char** argv, char token) {
  ASSERT(cmd_str != NULL);
  int32_t arg_idx = 0;
  while (arg_idx < MAX_ARG_NR) {
    argv[arg_idx] = NULL;
    arg_idx++;
  }

  char* next = cmd_str;
  int32_t argc = 0;
  //外层循环处理整个命令行
  while(*next) {
    //去除命令字或参数之间的空格
    while (*next == token) {
      next++;
    }
    //处理最后一个参数后接空格的情况，如"ls dir2 "
    if (*next == 0) {
      break;
    }
    argv[argc] = next;
    //内层循环处理命令行中的每个命令字及参数
    while (*next && *next != token) {
      //在字符串结束前找单词分隔符
      next++;
    }
    //如果未结束（是 token 字符），使 tocken 变成 0
    if (*next) {
      //将 token 字符替换为字符串结束符 0
      //作为一个单词的结束，并将字符指针 next 指向下一个字符
      *next++ = 0;
    }
    //避免 argv 数组访问越界，参数过多则返回 0
    if (argc > MAX_ARG_NR) {
      return -1;
    }
    argc++;
  }
  return argc;
}


//argv 必须为全局变量，为了以后 exec 的程序可访问参数
char* argv[MAX_ARG_NR];
int32_t argc = -1;

//简单的 shell
void  my_shell() {
  cwd_cache[0] = '/';
  while(1) {
    print_promt();
    memset(final_path, 0, MAX_PATH_LEN);
    memset(cmd_line, 0, MAX_PATH_LEN);
    readline(cmd_line, MAX_PATH_LEN);
    if (cmd_line[0] == 0) {
      //若只键入了一个回车
      continue;
    }
    argc = -1;
    argc = cmd_parse(cmd_line, argv, ' ');
    if (argc == -1) {
      printf("num of arguments exceed %d\n", MAX_ARG_NR);
      continue;
    }

    if (!strcmp("ls", argv[0])) {
      buildin_ls(argc, argv);
    } else if (!strcmp("cd", argv[0])) {
      if (buildin_cd(argc, argv) != NULL) {
        memset(cwd_cache, 0, MAX_PATH_LEN);
        strcpy(cwd_cache, final_path);
      }
    } else if (!strcmp("pwd", argv[0])) {
      buildin_pwd(argc, argv);
    } else if (!strcmp("ps", argv[0])) {
      buildin_ps(argc, argv);
    } else if (!strcmp("clear", argv[0])) {
      buildin_clear(argc, argv);
    } else if (!strcmp("mkdir", argv[0])) {
      buildin_mkdir(argc, argv);
    } else if (!strcmp("rmdir", argv[0])) {
      buildin_rmdir(argc, argv);
    } else if (!strcmp("rm", argv[0])) {
      buildin_rm(argc, argv);
    } else {
      //如果是外部命令，需要从磁盘上加载
      int32_t pid = fork();
      if (pid) {
        // 父进程
        //下面这个 while 必须要加上，否则父进程一般情况下会比子进程先执行，
        //因此会进行下一轮循环将 findl_path 清空,这样子进程将无法从 final_path 中获得参数
        while(1);
      } else {
        //子进程
        //获取可执行文件argv[0]的绝对路径
        make_clear_abs_path(argv[0], final_path);
        argv[0] = final_path;
        //先判断文件是否存在 
        struct stat file_stat;
        memset(&file_stat, 0, sizeof(struct stat));
        if (stat(argv[0], &file_stat) == -1) {
          printf("my_shell: cannot access %s:No such file or directory\n", argv[0]);
        } else {
          execv(argv[0], argv);
        }
        while(1);
      }
    }

    int32_t arg_idx = 0;
    while (arg_idx < MAX_ARG_NR) {
      argv[arg_idx] = NULL;
      arg_idx++;
    }
  }
  //出现了bug
  PANIC("my_shell: should not be here");
}
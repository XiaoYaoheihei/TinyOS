#include "buildin_cmd.h"
#include "syscall.h"
#include "string.h"
#include "stdio.h"
#include "fs.h"
#include "global.h"
#include "dir.h"
#include "shell.h"
#include "debug.h"

//将路径 old_abs_path 中的..和.转换为实际路径后存入 new_abs_path
//原理是调用函数 path_parse 从左到右解析 old_abs_path 路径中的每一层，
//若解析出来的目录名不是“..”，就将其连接到 new_abs_path，
//若是“..”, 就将 new_abs_path 的最后一层目录去掉
static void wash_path(char* old_abs_path, char* new_abs_path) {
  ASSERT(old_abs_path[0] == '/');
  char name[MAX_PATH_LEN] = {0};
  char* sub_path = old_abs_path;
  sub_path = path_parse(sub_path, name);
  if (name[0] == 0) { 
    //若只键入了"/",直接将"/"存入 new_abs_path 后返回
    new_abs_path[0] = '/';
    new_abs_path[1] = 0;
    return;
  }
  //避免传给 new_abs_path 的缓冲区不干净
  new_abs_path[0] = 0;
  strcat(new_abs_path, "/");
  while (name[0]) {
    //如果是上一级目录“..”
    if (!strcmp("..", name)) {
      // 如果未到 new_abs_path 中的顶层目录，就将最右边的'/'替换为 0，
      // 这样便去除了 new_abs_path 中最后一层路径，相当于到了上一级目录
      char* slash_ptr = strrchar(new_abs_path, '/');
      if (slash_ptr != new_abs_path) {
        // 如 new_abs_path 为“/a/b”，".."之后则变为“/a”
        *slash_ptr = 0;
      } else {
        //如 new_abs_path 为"/a"，".."之后则变为"/"
        //若 new_abs_path 中只有 1 个'/'，即表示已经到了顶层目录,就将下一个字符置为结束符 0
        *(slash_ptr+1) = 0;
      }
    } else if (strcmp(".", name)) {
      //如果路径不是‘.’，就将 name 拼接到 new_abs_path
      if (strcmp(new_abs_path, "/")) {
        //如果 new_abs_path 不是"/"
        //就拼接一个"/",此处的判断是为了避免路径开头变成这样"//"
        strcat(new_abs_path, "/"); 
      }
      //若 name 为当前目录"."，无需处理 new_abs_path
      strcat(new_abs_path, name);
    }
    
    //继续遍历下一层路径
    memset(name, 0, MAX_FILE_NAME_LEN);
    if (sub_path) {
      sub_path = path_parse(sub_path, name);
    }
  }
}

//将 path 处理成不含..和.的绝对路径，存储在 final_path
//path 是用户键入的路径，可能是相对路径，也可能是绝对路径，
//也可能包含“.”和“..”的相对路径或绝对路径，
void make_clear_abs_path(char* path, char* final_path) {
  char abs_path[MAX_PATH_LEN] = {0};
  //先判断是否输入的是否是绝对路径
  if (path[0] != '/') {
    //不是绝对路径的话，拼接成绝对路径
    memset(abs_path, 0, MAX_PATH_LEN);
    //获得当前工作目录的绝对路径
    if (getcwd(abs_path, MAX_PATH_LEN) != NULL) {
      if (!((abs_path[0] == '/') && (abs_path[1] == 0))) {
        //若 abs_path 表示的当前目录不是根目录/
        strcat(abs_path, "/");
      }
    }
  }
  //将用户输入的目录 path 追加到工作目录之后形成绝对目录 abs_path
  strcat(abs_path, path);
  wash_path(abs_path, final_path);
}

//pwd 命令的内建函数
void buildin_pwd(uint32_t argc, char** argv UNUSED) {
  if (argc != 1) {
    printf("pwd: no argument support!\n");
    return;
  } else {
    if (getcwd(final_path, MAX_PATH_LEN) != NULL) {
      printf("%s\n", final_path);
    } else {
      printf("pwd: get current work directory failed.\n");
    }
  }
}

//cd 命令的内建函数
char* buildin_cd(uint32_t argc, char** argv) {
  if (argc > 2) {
    printf("cd: only support 1 argument!\n");
    return NULL;
  }

  //若是只键入 cd 而无参数，直接返回到根目录
  if (argc == 1) {
    final_path[0] = '/';
    final_path[1] = 0;
  } else {
    make_clear_abs_path(argv[1], final_path);
  }

  if (chdir(final_path) == -1) {
    printf("cd: no such directory %s\n", final_path);
    return NULL;
  }
  return final_path;
}

//ls 命令的内建函数
void buildin_ls(uint32_t argc, char** argv) {
  char* pathname = NULL;
  struct stat file_stat;
  memset(&file_stat, 0, sizeof(struct stat));
  //是否是-l参数的标志
  bool long_info = false;
  uint32_t arg_path_nr = 0;
  // 跨过 argv[0]，argv[0]是字符串“ls”
  uint32_t arg_idx = 1;
  while (arg_idx < argc) {
    if (argv[arg_idx][0] == '-') {
      // 如果是选项，单词的首字符是-
      if (!strcmp("-l", argv[arg_idx])) {
        //如果是参数-l
        long_info = true;
      } else if (!strcmp("-h", argv[arg_idx])) {
        //参数-h
        printf("usage: -l list all infomation about the file.\n-h for \
        help\nlist all files in the current dirctory if no option\n");
        return;
      } else {
        // 只支持-h -l 两个选项
        printf("ls: invalid option %s\nTry `ls -h' for more \
        information.\n", argv[arg_idx]);
        return;
      }
    } else {
      // ls 的路径参数
      if (arg_path_nr == 0) {
        pathname = argv[arg_idx];
        arg_path_nr = 1;
      } else {
        printf("ls: only support one path\n");
        return;
      }
    }
    arg_idx++;
  }

  //若只输入了 ls 或 ls -l
  if (pathname == NULL) {
    //没有输入操作路径，默认以当前路径的绝对路径为参数
    if (getcwd(final_path, MAX_PATH_LEN) != NULL) {
      pathname = final_path;
    } else {
      printf("ls: getcwd for default path failed\n");
      return;
    }
  } else {
    make_clear_abs_path(pathname, final_path);
    pathname = final_path;
  }

  if (stat(pathname, &file_stat) == -1) {
    printf("ls: cannot access %s: No such file or directory\n", pathname);
    return;
  }
  //显示的信息是目录的话
  if (file_stat.st_filetype == FT_DIRECTORY) {
    struct dir* dir = opendir(pathname);
    struct dir_entry* dir_e = NULL;
    char sub_pathname[MAX_PATH_LEN] = {0};
    uint32_t pathname_len = strlen(pathname);
    uint32_t last_char_idx = pathname_len - 1;
    memcpy(sub_pathname, pathname, pathname_len);
    if (sub_pathname[last_char_idx] != '/') {
      sub_pathname[pathname_len] = '/';
      pathname_len++;
    }
    rewinddir(dir);
    if (long_info) {
      char ftype;
      printf("total: %d\n", file_stat.st_size);
      while ((dir_e = readdir(dir))) {
        ftype = 'd';
        if (dir_e->f_type == FT_REGULAR) {
          ftype = '-';
        }
        sub_pathname[pathname_len] = 0;
        strcat(sub_pathname, dir_e->filename);
        memset(&file_stat, 0, sizeof(struct stat));
        if (stat(sub_pathname, &file_stat) == -1) {
          printf("ls: cannot access %s: No such file or directory\n",dir_e->filename);
          return;
        }
        printf("%c %d %d %s\n", ftype, dir_e->i_no, file_stat.st_size,dir_e->filename);
      }
    } else {
      while((dir_e = readdir(dir))) {
        printf("%s ", dir_e->filename);
      }
      printf("\n");
    }
    closedir(dir);
  } else {
    if (long_info) {
      printf("- %d %d %s\n",file_stat.st_ino,file_stat.st_size,pathname);
    } else {
      printf("%s\n", pathname);
    }
  }
}

//ps 命令内建函数
void buildin_ps(uint32_t argc, char** argv UNUSED) {
  if (argc != 1) {
    printf("ps: no argument support!\n");
    return;
  }
  ps();
}

//clear 命令内建函数
void buildin_clear(uint32_t argc, char** argv UNUSED) {
  if (argc != 1) {
    printf("clear: no argument support!\n");
    return;
  }
  clear();
}

//mkdir 命令内建函数
int32_t buildin_mkdir(uint32_t argc, char** argv) {
  int32_t ret = -1;
  if (argc != 2) {
    printf("mkdir: only support 1 argument!\n");
  } else {
    make_clear_abs_path(argv[1], final_path);
    //若创建的不是根目录
    if (strcmp("/", final_path)) {
      if (mkdir(final_path) == 0) {
        ret = 0;
      } else {
        printf("mkdir: create directory %s failed.\n", argv[1]);
      }
    }
  }
  return ret;
}

//rmdir 命令内建函数
int32_t buildin_rmdir(uint32_t argc, char** argv) {
  int32_t ret = -1;
  if (argc != 2) {
    printf("rmdir: only support 1 argument!\n");
  } else {
    make_clear_abs_path(argv[1], final_path);
    //若删除的不是根目录
    if (strcmp("/", final_path)) {
      if (rmdir(final_path) == 0) {
        ret = 0;
      } else {
        printf("rmdir: remove %s failed.\n", argv[1]);
      }
    }
  }
  return ret;
}

//rm 命令内建函数
int32_t buildin_rm(uint32_t argc, char** argv) {
  int32_t ret = -1;
  if (argc != 2) {
    printf("rm: only support 1 argument!\n");
  } else {
    make_clear_abs_path(argv[1], final_path);
    //若删除的不是根目录
    if (strcmp("/", final_path)) {
      if (unlink(final_path) == 0) {
        ret = 0;
      } else {
        printf("rm: delete %s failed.\n", argv[1]);
      }
    } 
  }
  return ret;
}

/* 显示内建命令列表 */
void buildin_help(uint32_t argc, char** argv UNUSED) {
  help();
}


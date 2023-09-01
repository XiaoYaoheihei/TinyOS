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
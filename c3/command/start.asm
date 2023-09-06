[bits 32]
extern main
extern exit 
section .text
global _start
_start:
  ;下面这两个要和 execv 中 load 之后指定的寄存器一致
  ;压入 argv
  push ebx
  ;压入 argc
  push ecx 
  call main

  ;将 main 的返回值通过栈传给 exit，gcc 用 eax 存储返回值，这是 ABI 规定的
  push eax
  call exit 
  ;exit 不会返回
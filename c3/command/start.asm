[bits 32]
extern main
section .text
global _start
_start:
  ;下面这两个要和 execv 中 load 之后指定的寄存器一致
  ;压入 argv
  push ebx
  ;压入 argc
  push ecx 
  call main
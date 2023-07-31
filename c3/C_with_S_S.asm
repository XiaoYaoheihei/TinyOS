section .data
str: db "asm_print says hello world!", 0xa, 0
str_len equ $-str 

section .text
;为了在汇编文件中引用外部的函数（未必是 C 代码中的）
;需要用 extern 来声明所需要的函数名
extern c_print
global _start
_start:
  ;调用c代码中的c_print
  push str        ;push的str是个地址，不是整个字符串
  call c_print
  add esp, 4
  ;退出程序
  mov eax, 1
  int 0x80 

global asm_print  ;相当于asm_print(str,size)
asm_print:
  push ebp
  mov ebp, esp
  mov eax, 4
  mov ebx, 1      ;此项固定为文件描述符 1，标准输出（stdout）指向屏幕
  mov ecx, [ebp+8]
  mov edx, [ebp+12]
  int 0x80
  pop ebp
  ret
section .data
;0xa是LF的ASCII码
str_c_lib: db "c library says: hello world!", 0xa
str_c_lib_len equ $-str_c_lib

str_syscall: db "syscall says: hello world!", 0xa
str_syscall_len equ $-str_syscall

section .text
global _start
_start:
  ;方式1,模拟c中的库函数write
  push str_c_lib_len
  push str_c_lib
  push 1
  call simu_write
  add esp, 12         ;回收栈空间

  ;方式2,跨过库函数，直接进行系统调用
  mov eax, 4          ;4号子功能
  mov ebx, 1
  mov ecx, str_syscall
  mov edx, str_syscall_len
  int 0x80

  ;退出程序
  mov eax, 1    ;1号子功能是exit
  int 0x80

  ;自定义的函数来模拟c库中的系统调用write
simu_write:
  push ebp
  mov ebp, esp
  mov eax, 4        ;4号子功能是write系统调用
  mov ebx, [ebp+8]  ;第1个参数
  mov ecx, [ebp+12] ;第2个参数
  mov edx, [ebp+16] ;第3个参数
  int 0x80          ;发起中断，linux内核完成调用
  pop ebp
  ret

;ld -m elf_i386 -o syscall_write.bin syscall_write.o
;在x86_64位机上编译和链接32位程序的时候，将仿真设置为elf_i386来提供正确的elf格式
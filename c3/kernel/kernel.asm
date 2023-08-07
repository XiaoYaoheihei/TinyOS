[bits 32]
%define ERROR_CODE nop  ;若在相关的异常中CPU已经自动压入了错误码
                        ;为了保证栈中格式的统一，这里不做任何操作
%define ZERO push 0     ;若在相关的异常中CPU没有压入错误码，
                        ;为了统一栈中格式，手工压入0

extern idt_table
extern put_str      ;声明外部函数
section .data
intr_str db "interrupt occur!", 0xa,0

global intr_entry_table
intr_entry_table:
  %macro VECTOR 2   ;宏的定义
  section .data
  dd intr%1entry    ;存储各个中断入口程序的地址
                    ;形成intr_entry_table数组

  section .text
  intr%1entry:      ;每个中断处理程序都需要压入中断向量号
                    ;一个中断类型一个中断处理程序，自己本身需要知道自己的中断向量号
    %2
    push ds   ;保存上下文环境
    push es
    push fs
    push gs
    pushad    ;压入32位寄存器，入栈顺序：EAX,ECX,EDX,EBX,ESP,EBP,ESI,EDI,EAX 最先入栈
    ; push intr_str
    ; call put_str
    ; add esp, 4      ;跳过压入的地址参数
    ;如果是从片上进入的中断，除了往从片上发送 EOI 外，还要往主片上发送 EOI
    mov al, 0x20    ;中断结束命令EOI
    out 0xa0, al    ;向从片发送
    out 0x20, al    ;向主片发送

    push %1         ;不管 idt_table 中的目标程序是否需要参数
                    ;都一律压入中断向量号
    call [idt_table + %1*4]
    jmp intr_exit   ;跳转到恢复上下文环境函数
    ; add esp, 4      ;跳过压入的0或者是错误码
    ; iret
  %endmacro

  section .text
  global intr_exit
  intr_exit:      ;以下是恢复上下文环境
    add esp, 4    ;跳过中断号
    popad
    pop gs
    pop fs
    pop es
    pop ds
    add esp, 4    ;跳过压入的0或者是错误码
    iret

  VECTOR 0X00, ZERO
  VECTOR 0x01, ZERO
  VECTOR 0x02, ZERO
  VECTOR 0x03,ZERO 
  VECTOR 0x04,ZERO
  VECTOR 0x05,ZERO
  VECTOR 0x06,ZERO
  VECTOR 0x07,ZERO 
  VECTOR 0x08,ERROR_CODE
  VECTOR 0x09,ZERO
  VECTOR 0x0a,ERROR_CODE
  VECTOR 0x0b,ERROR_CODE 
  VECTOR 0x0c,ZERO
  VECTOR 0x0d,ERROR_CODE
  VECTOR 0x0e,ERROR_CODE
  VECTOR 0x0f,ZERO 
  VECTOR 0x10,ZERO
  VECTOR 0x11,ERROR_CODE
  VECTOR 0x12,ZERO
  VECTOR 0x13,ZERO 
  VECTOR 0x14,ZERO
  VECTOR 0x15,ZERO
  VECTOR 0x16,ZERO
  VECTOR 0x17,ZERO 
  VECTOR 0x18,ERROR_CODE
  VECTOR 0x19,ZERO
  VECTOR 0x1a,ERROR_CODE
  VECTOR 0x1b,ERROR_CODE 
  VECTOR 0x1c,ZERO
  VECTOR 0x1d,ERROR_CODE
  VECTOR 0x1e, ERROR_CODE
  VECTOR 0x1f, ZERO
  VECTOR 0x20, ZERO
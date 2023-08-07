%line 1+1 kernel.asm
[bits 32]





[extern put_str]
[section .data]
intr_str db "interrupt occur!", 0xa, 0

[global intr_entry_table]
intr_entry_table:

[section .data]
 dd intr0X00entry


[section .text]
 intr0X00entry:

 push 0
 push intr_str
 call put_str
 add esp, 4

 mov al, 0x20
 out 0xa0, al
 out 0x20, al

 add esp, 4
 iret
%line 14+1 kernel.asm
[section .data]
 dd intr0x01entry


[section .text]
 intr0x01entry:

 push 0
 push intr_str
 call put_str
 add esp, 4

 mov al, 0x20
 out 0xa0, al
 out 0x20, al

 add esp, 4
 iret
%line 14+1 kernel.asm
[section .data]
 dd intr0x02entry


[section .text]
 intr0x02entry:

 push 0
 push intr_str
 call put_str
 add esp, 4

 mov al, 0x20
 out 0xa0, al
 out 0x20, al

 add esp, 4
 iret
%line 14+1 kernel.asm
[section .data]
 dd intr0x03entry


[section .text]
 intr0x03entry:

 push 0
 push intr_str
 call put_str
 add esp, 4

 mov al, 0x20
 out 0xa0, al
 out 0x20, al

 add esp, 4
 iret
%line 14+1 kernel.asm
[section .data]
 dd intr0x04entry


[section .text]
 intr0x04entry:

 push 0
 push intr_str
 call put_str
 add esp, 4

 mov al, 0x20
 out 0xa0, al
 out 0x20, al

 add esp, 4
 iret
%line 14+1 kernel.asm
[section .data]
 dd intr0x05entry


[section .text]
 intr0x05entry:

 push 0
 push intr_str
 call put_str
 add esp, 4

 mov al, 0x20
 out 0xa0, al
 out 0x20, al

 add esp, 4
 iret
%line 14+1 kernel.asm
[section .data]
 dd intr0x06entry


[section .text]
 intr0x06entry:

 push 0
 push intr_str
 call put_str
 add esp, 4

 mov al, 0x20
 out 0xa0, al
 out 0x20, al

 add esp, 4
 iret
%line 14+1 kernel.asm
[section .data]
 dd intr0x07entry


[section .text]
 intr0x07entry:

 push 0
 push intr_str
 call put_str
 add esp, 4

 mov al, 0x20
 out 0xa0, al
 out 0x20, al

 add esp, 4
 iret
%line 14+1 kernel.asm
[section .data]
 dd intr0x08entry


[section .text]
 intr0x08entry:

 nop
 push intr_str
 call put_str
 add esp, 4

 mov al, 0x20
 out 0xa0, al
 out 0x20, al

 add esp, 4
 iret
%line 14+1 kernel.asm
[section .data]
 dd intr0x09entry


[section .text]
 intr0x09entry:

 push 0
 push intr_str
 call put_str
 add esp, 4

 mov al, 0x20
 out 0xa0, al
 out 0x20, al

 add esp, 4
 iret
%line 14+1 kernel.asm
[section .data]
 dd intr0x0aentry


[section .text]
 intr0x0aentry:

 nop
 push intr_str
 call put_str
 add esp, 4

 mov al, 0x20
 out 0xa0, al
 out 0x20, al

 add esp, 4
 iret
%line 14+1 kernel.asm
[section .data]
 dd intr0x0bentry


[section .text]
 intr0x0bentry:

 nop
 push intr_str
 call put_str
 add esp, 4

 mov al, 0x20
 out 0xa0, al
 out 0x20, al

 add esp, 4
 iret
%line 14+1 kernel.asm
[section .data]
 dd intr0x0centry


[section .text]
 intr0x0centry:

 push 0
 push intr_str
 call put_str
 add esp, 4

 mov al, 0x20
 out 0xa0, al
 out 0x20, al

 add esp, 4
 iret
%line 14+1 kernel.asm
[section .data]
 dd intr0x0dentry


[section .text]
 intr0x0dentry:

 nop
 push intr_str
 call put_str
 add esp, 4

 mov al, 0x20
 out 0xa0, al
 out 0x20, al

 add esp, 4
 iret
%line 14+1 kernel.asm
[section .data]
 dd intr0x0eentry


[section .text]
 intr0x0eentry:

 nop
 push intr_str
 call put_str
 add esp, 4

 mov al, 0x20
 out 0xa0, al
 out 0x20, al

 add esp, 4
 iret
%line 14+1 kernel.asm
[section .data]
 dd intr0x0fentry


[section .text]
 intr0x0fentry:

 push 0
 push intr_str
 call put_str
 add esp, 4

 mov al, 0x20
 out 0xa0, al
 out 0x20, al

 add esp, 4
 iret
%line 14+1 kernel.asm
[section .data]
 dd intr0x10entry


[section .text]
 intr0x10entry:

 push 0
 push intr_str
 call put_str
 add esp, 4

 mov al, 0x20
 out 0xa0, al
 out 0x20, al

 add esp, 4
 iret
%line 14+1 kernel.asm
[section .data]
 dd intr0x11entry


[section .text]
 intr0x11entry:

 nop
 push intr_str
 call put_str
 add esp, 4

 mov al, 0x20
 out 0xa0, al
 out 0x20, al

 add esp, 4
 iret
%line 14+1 kernel.asm
[section .data]
 dd intr0x12entry


[section .text]
 intr0x12entry:

 push 0
 push intr_str
 call put_str
 add esp, 4

 mov al, 0x20
 out 0xa0, al
 out 0x20, al

 add esp, 4
 iret
%line 14+1 kernel.asm
[section .data]
 dd intr0x13entry


[section .text]
 intr0x13entry:

 push 0
 push intr_str
 call put_str
 add esp, 4

 mov al, 0x20
 out 0xa0, al
 out 0x20, al

 add esp, 4
 iret
%line 14+1 kernel.asm
[section .data]
 dd intr0x14entry


[section .text]
 intr0x14entry:

 push 0
 push intr_str
 call put_str
 add esp, 4

 mov al, 0x20
 out 0xa0, al
 out 0x20, al

 add esp, 4
 iret
%line 14+1 kernel.asm
[section .data]
 dd intr0x15entry


[section .text]
 intr0x15entry:

 push 0
 push intr_str
 call put_str
 add esp, 4

 mov al, 0x20
 out 0xa0, al
 out 0x20, al

 add esp, 4
 iret
%line 14+1 kernel.asm
[section .data]
 dd intr0x16entry


[section .text]
 intr0x16entry:

 push 0
 push intr_str
 call put_str
 add esp, 4

 mov al, 0x20
 out 0xa0, al
 out 0x20, al

 add esp, 4
 iret
%line 14+1 kernel.asm
[section .data]
 dd intr0x17entry


[section .text]
 intr0x17entry:

 push 0
 push intr_str
 call put_str
 add esp, 4

 mov al, 0x20
 out 0xa0, al
 out 0x20, al

 add esp, 4
 iret
%line 14+1 kernel.asm
[section .data]
 dd intr0x18entry


[section .text]
 intr0x18entry:

 nop
 push intr_str
 call put_str
 add esp, 4

 mov al, 0x20
 out 0xa0, al
 out 0x20, al

 add esp, 4
 iret
%line 14+1 kernel.asm
[section .data]
 dd intr0x19entry


[section .text]
 intr0x19entry:

 push 0
 push intr_str
 call put_str
 add esp, 4

 mov al, 0x20
 out 0xa0, al
 out 0x20, al

 add esp, 4
 iret
%line 14+1 kernel.asm
[section .data]
 dd intr0x1aentry


[section .text]
 intr0x1aentry:

 nop
 push intr_str
 call put_str
 add esp, 4

 mov al, 0x20
 out 0xa0, al
 out 0x20, al

 add esp, 4
 iret
%line 14+1 kernel.asm
[section .data]
 dd intr0x1bentry


[section .text]
 intr0x1bentry:

 nop
 push intr_str
 call put_str
 add esp, 4

 mov al, 0x20
 out 0xa0, al
 out 0x20, al

 add esp, 4
 iret
%line 14+1 kernel.asm
[section .data]
 dd intr0x1centry


[section .text]
 intr0x1centry:

 push 0
 push intr_str
 call put_str
 add esp, 4

 mov al, 0x20
 out 0xa0, al
 out 0x20, al

 add esp, 4
 iret
%line 14+1 kernel.asm
[section .data]
 dd intr0x1dentry


[section .text]
 intr0x1dentry:

 nop
 push intr_str
 call put_str
 add esp, 4

 mov al, 0x20
 out 0xa0, al
 out 0x20, al

 add esp, 4
 iret
%line 14+1 kernel.asm
[section .data]
 dd intr0x1eentry


[section .text]
 intr0x1eentry:

 nop
 push intr_str
 call put_str
 add esp, 4

 mov al, 0x20
 out 0xa0, al
 out 0x20, al

 add esp, 4
 iret
%line 14+1 kernel.asm
[section .data]
 dd intr0x1fentry


[section .text]
 intr0x1fentry:

 push 0
 push intr_str
 call put_str
 add esp, 4

 mov al, 0x20
 out 0xa0, al
 out 0x20, al

 add esp, 4
 iret
%line 14+1 kernel.asm
[section .data]
 dd intr0x20entry


[section .text]
 intr0x20entry:

 push 0
 push intr_str
 call put_str
 add esp, 4

 mov al, 0x20
 out 0xa0, al
 out 0x20, al

 add esp, 4
 iret

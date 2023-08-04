;直接绝对远转移

section call_test vstart=0x900
jmp 0:start
times 128 db 0
start:
    mov ax, 0x1234
    jmp $
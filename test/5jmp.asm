;间接绝对远转移
;操作数只能放在内存中，必须使用关键字far

section call_test vstart=0x900
jmp far [addr]
times 128 db 0
addr dw start, 0
start:
    mov ax, 0x1234
    jmp $
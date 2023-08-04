;间接近调用

SECTION call_test vstart=0x900
;伪指令word表示CPU在读取数字的时候一次性读取2字节内容，可能会造成数据强制类型转化
mov word [addr], near_proc
call [addr]
mov ax, near_proc
call ax
jmp $
addr dd 4
near_proc:
    mov ax, 0x1234
    ret
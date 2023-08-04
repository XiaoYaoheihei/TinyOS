;直接绝对远调用
;两个数都是一个立即数

SECTION call_test vstart=0x900
call 0: far_proc
jmp $
far_proc:
  mov ax, 0x1234
  retf

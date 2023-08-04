;间接绝对远调用
;只支持内存寻址，不支持寄存器寻址，一定要记得加far

SECTION call_test vstart=0x900
call far [addr]
jmp $
addr dw far_proc, 0
far_proc:
  mov ax, 0x1234
  retf
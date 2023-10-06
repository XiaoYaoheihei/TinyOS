
;主引导程序
SECTION MBR vstart=0x7c00
  mov ax,cs
  mov ds,ax
  mov es,ax
  mov ss,ax
  mov fs,ax
  mov sp,0x7c00
  mov ax,0xb800
  mov gs,ax

;清屏利用0x06号功能，
; -----------------------------------------------------------
;INT 0x10   功能号:0x06   功能描述:上卷窗口
;------------------------------------------------------
;输入:
;AH 功能号= 0x06
;AL = 上卷的行数(如果为 0,表示全部)
;BH = 上卷行属性
;(CL,CH) = 窗口左上角的(X,Y)位置
;(DL,DH) = 窗口右下角的(X,Y)位置
;无返回值:
  mov ax,0x0600
  mov bx,0x0700
  mov cx,0        ;左上角（0.0）
  mov dx,0x184f   ;右下角（80,25）
                  ;VGA文本模式中，一行只能容纳80个字符，一共25行
                  ;下标从0开始，0x18=24,0x4f=79
  int 0x10

;输出背景色为绿色，前景色为红色，并且是闪烁的字符串
;显存文本模式中，内存地址是0xb8000

  mov byte [gs:0x00],'1'    ;把字符1的ascii码写入以gs:0x00为起始，大小为1字节的内存中
  mov byte [gs:0x01],0xa4   ;a表示绿色背景闪烁，4表示前景色为红色

  mov byte [gs:0x02],' '
  mov byte [gs:0x03],0xa4

  mov byte [gs:0x04],'M'
  mov byte [gs:0x05],0xa4

  mov byte [gs:0x06],'B'
  mov byte [gs:0x07],0xa4

  mov byte [gs:0x08],'R'
  mov byte [gs:0x09],0xa4

  jmp $           ;使程序悬浮在这里

  message db "1 MBR"
  times 510-($-$$) db 0
  db 0x55,0xaa
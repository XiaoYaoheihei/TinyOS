
;主引导程序
%include "boot.inc"
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

  mov eax, LOADER_START_SECTOR  ;起始扇区lba的地址
  mov bx, LOADER_BASE_ADDR      ;写入的地址
  mov cx, 4                     ;待写入的扇区数
                                ;加载loader.bin的扇区数变为4
  call rd_disk_m_16             ;读取程序的起始部分

  jmp LOADER_BASE_ADDR + 0x300

;功能：读取硬盘的n个扇区
rd_disk_m_16:
                                ;eax=lba扇区号
                                ;bx=将数据写入的内存地址
                                ;cx=读入的扇区数
    mov esi,eax ;备份eax和cx
    mov di,cx
;读写硬盘
;第一步，设置要读取的扇区数
    mov dx,0x1f2
    mov al,cl
    out dx,al

    mov eax,esi

;第二步，将LBA地址存入0x1f3 ~ 0x1f6这几个端口中
    ;将0--7位写入端口0x1f3
    mov dx,0x1f3
    out dx,al

    ;将8--15位写入0x1f4
    mov cl,8
    shr eax,cl  ;右移8位
    mov dx,0x1f4
    out dx,al

    ;将16--23位写入0x1f5
    shr eax,cl  ;右移8位
    mov dx,0x1f5
    out dx,al

    shr eax,cl  ;再右移8位
    and al,0x0f ;LBA的24--27位设置
    or al,0xe0  ;与此同时设置4--7位为1110,表示lba模式
    mov dx,0x1f6
    out dx,al

;第三步，向0x1f7端口写入读命令
    mov dx,0x1f7
    mov al,0x20
    out dx,al

;第四步，检测硬盘状态
;同一个端口地址，写时表示表示写入命令字，读时表示读入硬盘状态
  .not_ready:
    nop
    in al,dx
    and al,0x88   ;第4位为1表示硬盘控制器已经准备好数据传输
                  ;第7位为1表示硬盘忙
    cmp al,0x08
    jnz .not_ready  ;如果没有准备好继续等待

;第五步，从0x1f0端口读取数据
    mov ax,di       ;di是要读取的扇区数
    mov dx,256      ;一个扇区有512个字节，每次读入一个字
    mul dx          ;共需要读取di*512/2次,所以di*256
    mov cx,ax       ;因为结果比较小，不用管在dx寄存器中的高16位
                    ;直接使用ax表示循环的次数
    mov dx,0x1f0

  .go_on_ready:
    in ax,dx
    mov [bx],ax
    add bx,2
    loop .go_on_ready
  ret

  ;jmp $           ;使程序悬浮在这里

  ;message db "1 MBR"
  times 510-($-$$) db 0
  db 0x55,0xaa
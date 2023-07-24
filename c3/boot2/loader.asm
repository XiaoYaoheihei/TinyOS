%include "boot.inc"
section loader vstart=LOADER_BASE_ADDR
LOADER_STACK_TOP equ LOADER_BASE_ADDR
jmp loader_start

;构建GDT以及其内部的描述符
GDT_BASE:   dd 0x00000000
            dd 0x00000000

CODE_DESC:  dd 0x0000FFFF         ;低4字节内容
            dd DESC_CODE_HIGH4    ;高4字节内容

DATA_STACK_DESC:  dd 0x0000FFFF
                  dd DESC_DATA_HIGH4

VIDEO_DESC: dd 0x80000007       ;limit=(0xbffff-0xb8000)/4k=0x7
            dd DESC_VIDEO_HIGH4 ;dpl=0

GDT_SIZE  equ $ - GDT_BASE
;GDT的界限
GDT_LIMIT equ GDT_SIZE - 1 
;dq用来定义（4字）8字节数据,define quad-word
times 60 dq 0     ;预留60个段描述符的空位
;选择子结构进行赋值
SELECTOR_CODE equ (0x0001<<3) + TI_GDT + RPL0
SELECTOR_DATA equ (0x0002<<3) + TI_GDT + RPL0
SELECTOR_VIDEO equ (0x0003<<3) + TI_GDT + RPL0

;接下来是gdt的指针，前两个字节是GDT的界限，后四字节是GDT的起始地址
gdt_ptr   dw  GDT_LIMIT
          dd  GDT_BASE
loadermsg db '2 loader in real.'


loader_start:
;------------------------------------------------------------
;INT 0x10   功能号:0x13   功能描述:打印字符串
;------------------------------------------------------------
;输入:
;AH 子功能号=13H
;BH = 页码
;BL = 属性(若 AL=00H 或 01H)
;CX=字符串长度
;(DH､ DL)=坐标(行､ 列)
;ES:BP=字符串地址
;AL=显示输出方式
; 0—字符串中只含显示字符，其显示属性在 BL 中
        ;显示后，光标位置不变
; 1—字符串中只含显示字符，其显示属性在 BL 中
        ;显示后，光标位置改变
; 2—字符串中含显示字符和显示属性。显示后，光标位置不变
; 3—字符串中含显示字符和显示属性。显示后，光标位置改变
;无返回值
mov sp, LOADER_BASE_ADDR
mov bp, loadermsg     ; ES:BP = 字符串地址
mov cx, 17            ; CX = 字符串长度
mov ax, 0x1301        ; AH = 13, AL = 01h
mov bx, 0x001f        ; 页号为 0(BH = 0) 蓝底粉红字(BL = 1fh)
mov dx, 0x1800        ; 行数 dh 为 0x18，列数 dl 为 0x00
                      ; 由于文本模式下的行数是25行，所以此表示最后一行
int 0x10              ; 10h 号中断

;--------------------准备进入保护模式----------------------------
;打开A20
in al, 0x92
or al, 0000_0010b
out 0x92,al

;加载GDT到gdtr寄存器
lgdt [gdt_ptr]

;cr0的第0位置1
mov eax, cr0
or eax, 0x00000001
mov cr0, eax

;刷新流水线
jmp dword SELECTOR_CODE:p_mode_start

[bits 32]
p_mode_start:
  mov ax, SELECTOR_DATA
  mov ds, ax
  mov es, ax
  mov ss, ax
  mov esp, LOADER_STACK_TOP
  mov ax, SELECTOR_VIDEO
  mov gs, ax

  mov byte [gs:160], 'P';利用选择子进行寻址的过程需要清楚！！！
  ;gs中选择子的低两位是RPL，第2位是TL=0，表示是在GDT中索引段描述符
  ;用gs的高18位在GDT中进行索引，找到段描述符之后，计算出相应的段基址，与偏移地址相加
  ;用所得的地址作为访存的地址
  jmp $

mov byte [gs:0x00],'2'
mov byte [gs:0x01],0x99   ;闪烁的蓝色背景，高亮的前景蓝色

mov byte [gs:0x02],' '
mov byte [gs:0x03],0x99

mov byte [gs:0x04],'L'
mov byte [gs:0x05],0x99

mov byte [gs:0x06],'O'
mov byte [gs:0x07],0x99

mov byte [gs:0x08],'A'
mov byte [gs:0x09],0x99

mov byte [gs:0x0a],'D'
mov byte [gs:0x0b],0x99

mov byte [gs:0x0c],'E'
mov byte [gs:0x0d],0x99

mov byte [gs:0x0e],'R'
mov byte [gs:0x0f],0x99

jmp $
%include "boot.inc"
section loader vstart=LOADER_BASE_ADDR
LOADER_STACK_TOP equ LOADER_BASE_ADDR
;jmp loader_start

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

;用来保存内存容量，按照字节为单位
;当前偏移文件头0x200，加载地址又是0x900
;所以此变量在内存中的地址是0xb00
total_mem_bytes dd 0

;接下来是gdt的指针，前两个字节是GDT的界限，后四字节是GDT的起始地址
gdt_ptr   dw  GDT_LIMIT
          dd  GDT_BASE
;loadermsg db '2 loader in real.'

;人工对齐:total_mem_bytes4+gdt_ptr6+ards_buf244+ards_nr2，共 256 字节
ards_buf times 244 db 0
;记录ARDS结构体数量
ards_nr  dw 0


loader_start:

;获取内存布局
;int 15h        eax=0000e820h   edx=534d4150h('SWAP')

xor ebx, ebx							;第一次调用，ebx的值=0
mov edx, 0x534d4150
mov di, ards_buf					;结构缓冲区地址
.e820_mem_get_loop:				;循环获取每一个ARDS内存范围描述符
	mov eax, 0x0000e820			;执行 int 0x15 后，eax 值变为 0x534d4150，
													;所以每次执行 int 前都要更新为子功能号
	mov ecx, 20							;每一个结构大小是20字节
	int 0x15
	;如果cf=1则有错误发生，尝试e801功能
	jc .e820_failed_so_try_e801
	
	add di, cx							;di增加20字节指向新的缓冲区地址
	inc word [ards_nr]			;记录ARDS数量
	cmp ebx, 0							;若ebx为0且cf不为1，这说明ards全部返回
													;当前是最后一个ARDS
	;循环进行检测
	jnz .e820_mem_get_loop

;在所有的ARDS结构中，找出(base_add_low + length_low)的最大值，即内存的容量
	mov cx, [ards_nr]				;循环次数是ARDS的数量
	mov ebx, ards_buf
	xor edx, edx						;edx记录最大的内存容量，首先清0

.find_max_mem_area:				;冒泡排序找最大值
	mov eax, [ebx]					;ARDS结构中的base
	add eax, [ebx+8]				;加上length部分
	add ebx, 20							;指向缓冲区的下一个ARDS结构
	cmp edx, eax						;找出最大值，edx始终是最大的值
	jge .next_ards					;>=的时候跳转到.next
	mov edx, eax						;<的话交换赋值
.next_ards:
	loop .find_max_mem_area
	jmp .mem_get_ok					;获取成功进行跳转

;--int 15h  ax = E801h 获取内存大小，最大支持 4G 
; 返回后, ax cx 值一样,以 KB 为单位，bx dx 值一样，以 64KB 为单位
; 在 ax 和 cx 寄存器中为低 16MB，在 bx 和 dx 寄存器中为 16MB 到 4GB
.e820_failed_so_try_e801:
	mov ax, 0xe801
	int 0x15
	;执行完之后，ax和cx的值是一样的
	;bx和cx的值是一样的
	;若当前 e801 方法失败，就尝试 0x88 方法
	jc .e801_failed_so_try88

;1 先算出低15MB的内存
;在16位乘法中，mul 指令固定的乘数是寄存器 AX，积的高16位在dx寄存器，低16位在ax寄存器
	mov cx, 0x400					;cx和ax值一样，cx用作乘数，为1024
	mul cx
	shl edx, 16
	and eax, 0x0000ffff		;ax只是15M，需要+1M
	or edx, eax						;得到完整的32位积
	add edx, 0x100000
	mov esi, edx					;将小部分的内存数量存入esi寄存器备份

;2 再将16MB以上的内存转换为byte为单位
	xor eax, eax					;高16位清0
	mov ax, bx						;初始化低16位
	mov ecx, 0x10000			;用作乘数，是64K
	mul ecx								;32 位乘法，默认的被乘数是 eax，积为 64 位
												;高 32 位存入 edx，低 32 位存入 eax
	;此方法只能测出4G以内的内存，所以eax足够了，而且edx肯定是0
	add esi, eax					;添加剩余部分的内存

	mov edx, esi					;edx是总内存大小
	jmp .mem_get_ok


;-- int 15h ah = 0x88 获取内存大小，只能获取 64MB 之内 
.e801_failed_so_try88:
	mov ah, 0x88					;int 15 后，ax 存入的是以 KB 为单位的内存容量
	int 15h
	jc .error_hlt					;这个error函数是内置的吗
	and eax, 0x0000ffff
	
;16 位乘法，被乘数是 ax，积为 32 位。积的高 16 位在 dx 中
;积的低 16 位在 ax 中
	mov cx, 0x400					;0x400等于1024,将ax中的内存容量换为以byte为单位
	mul cx
	shl edx, 16						;把dx移动到高16位
	or edx, eax						;低16位组合到edx
	add edx, 0x100000			;0x88 子功能只会返回 1MB 以上的内存
												;故实际内存大小要加上 1MB

;将内存换为byte单位之后存入到total_mem_bytes处
.mem_get_ok:
	mov [total_mem_bytes], edx
	
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
; mov sp, LOADER_BASE_ADDR
; mov bp, loadermsg     ; ES:BP = 字符串地址
; mov cx, 17            ; CX = 字符串长度
; mov ax, 0x1301        ; AH = 13, AL = 01h
; mov bx, 0x001f        ; 页号为 0(BH = 0) 蓝底粉红字(BL = 1fh)
; mov dx, 0x1800        ; 行数 dh 为 0x18，列数 dl 为 0x00
;                       ; 由于文本模式下的行数是25行，所以此表示最后一行
; int 0x10              ; 10h 号中断

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
;这里的dword其实也是可以去掉的,具体原因可以看P173
jmp dword SELECTOR_CODE:p_mode_start

;出错则挂起
.error_hlt:
	hlt

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
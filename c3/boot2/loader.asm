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

;-------------准备进入保护模式
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

;----------加载内核
;----------把内核文件从硬盘上加载到内存中
	;内存位置是0xcde
	mov eax, KERNEL_START_SECTOR	;kernel在的扇区号
	mov ebx, KERNEL_BIN_BASE_ADDR	;kernel加载到的内存地址
	;从disk读出之后，写入到ebx指定的地址
	mov ecx, 200									;读入的扇区数
	call rd_disk_m_32


;----------创建页目录以及页表并且初始化页内存位图
	call setup_page		
	
	sgdt [gdt_ptr]		;将GDTR中存储的地址以及偏移量写入内存gdt_ptr中
										;存储到原来gdt的位置
	;将gdt描述符中视频段描述符中的段基址+0xc0000000
	mov ebx, [gdt_ptr + 2]	;GDT的起始位置放入到ebx中
	or dword [ebx+0x18+4], 0xc0000000	;是第三个段描述符，每个描述符8字节，是0x18
																		;段描述符的高 4 字节的最高位是段基址的第 31～24 位
	;将GDT基址也改到内核中去
	add dword [gdt_ptr+2], 0xc0000000	;将gdt的基址加上0xc0000000,使得成为内核所在的高地址
	add esp, 0xc0000000								;将栈指针同样映射到内核地址
																		;此时esp=0xc0000900

	mov eax, PAGE_DIR_TABLE_POS				;把页目录地址赋给cr3
	mov cr3, eax

	mov eax, cr0											;打开cr0的PG位
	or eax, 0x80000000
	mov cr0, eax

	lgdt [gdt_ptr]										;现在已经正式启动分页机制
																		;开启分页之后，重新加载gdt的地址

	mov byte [gs:160], 'V'						;视频段基址已经被更新
																		;此时我们访问的所有地址都是虚拟地址
																		;首先便是gdt的虚拟地址，其此就是视频段的虚拟地址
																		;cpu会自动将虚拟地址转化成真实的物理地址进行访问
	mov byte [gs:162], 'I'
	mov byte [gs:164], 'R'
	mov byte [gs:166], 'T'
	mov byte [gs:168], 'U'
	mov byte [gs:170], 'A'
	mov byte [gs:172], 'L'
	;显示已经进入虚拟地址，以后访问的地址都是虚拟地址

;为了以防万一，还是强制刷新流水线吧
	jmp SELECTOR_CODE:enter_kernel		;强制刷新流水线，更新gdt
enter_kernel:
	mov byte [gs:320], 'k'     				;视频段基址已经被更新
  mov byte [gs:322], 'e'     
  mov byte [gs:324], 'r'     
  mov byte [gs:326], 'n'     
  mov byte [gs:328], 'e'     
  mov byte [gs:330], 'l'     
	;地址是0xd9c
  ; mov byte [gs:480], 'w'     
  ; mov byte [gs:482], 'h'     
  ; mov byte [gs:484], 'i'     
  ; mov byte [gs:486], 'l'     
  ; mov byte [gs:488], 'e'     
  ; mov byte [gs:490], '('     
  ; mov byte [gs:492], '1'     
  ; mov byte [gs:494], ')'     
  ; mov byte [gs:496], ';'     
	call kernel_init
	mov esp, 0xc009f000								;重新设置内核的栈地址
	jmp KERNEL_ENTRY_POINT						;使用虚拟地址0xc0001500来访问
																		;虚拟地址对应的真实地址是物理内存地址0x1500

;---------初始化内核，进行相应的复制
;---------将kernel.bin 中的 segment 拷贝到编译的地址
kernel_init:
	xor eax, eax
	xor ebx, ebx		;ebx记录 程序头表 地址
	xor ecx, ecx		;cx记录 程序头表 中program header的数量
	xor edx, edx		;dx记录 program header的大小，即e_phentsize
									;每个用来描述段信息的数据结构的字节大小
	mov dx, [KERNEL_BIN_BASE_ADDR+42]		;偏移文件42字节处的属性是 e_phentsize，表示program header大小
	mov ebx, [KERNEL_BIN_BASE_ADDR+28]	;偏移文件开始部分28字节的地方是 e_phoff
																			;程序头表在文件中的偏移量，因为程序头表其实就是一个数组
																			;所以也表示第1个program header在文件中的偏移量
																			;其实该值是 0x34，不过还是谨慎一点，这里来读取实际值
	;因为我们需要的是物理地址，但是此时ebx中存储的还是偏移地址
	;所以加上内核的加载地址，这样得到的才是程序头表的物理地址
	add ebx, KERNEL_BIN_BASE_ADDR				;此时ebx指向的是第1个program header
	mov cx, [KERNEL_BIN_BASE_ADDR + 44]	;偏移文件开始部分44字节的地方是e_phnum，表示有几个program header
	
.each_segment:
	cmp byte [ebx + 0], PT_NULL		;若p_type等于PT_NULL说明此program header未使用
	je .PTNULL
	
	;为函数压入参数，参数是从右往左依次压入
	;函数原型类似于memcpy(dst, src, size)
	push dword [ebx + 16]					;program header中偏移16字节的地方是 p_filesz
																;首先压入第3个参数size
																;表示本段在文件中的大小
	mov eax, [ebx + 4]						;距程序头偏移量为 4 字节的位置是 p_offset
																;表示本段在文件中的起始偏移字节
	add eax, KERNEL_BIN_BASE_ADDR	;加上kernel.bin被加载到的物理地址，eax为该段的物理地址
	push eax											;压入第2个参数源地址
	push dword [ebx + 8]					;压入第一个参数目标地址
																;偏移程序头8字节的位置是p_vaddr是目的地址
																;表示本段在内存中的起始虚拟地址
	call mem_cpy		;调用完成段复制
	add esp, 12			;3个参数总共占用了12字节，栈顶跨过他们，清理栈中压入的三个参数
	
.PTNULL:
	add ebx, edx		;edx为program header大小,即 e_phentsize
									;在此ebx指向下一个program header
	loop .each_segment
	ret

;----------逐个字节进行拷贝
;输入：栈中的三个参数(dst, src, size)
;输出：无	
;使用call指令进行函数调用的时候，CPU会自动在栈中压入返回地址
mem_cpy:
	cld 					;将方向标志位置0,在执行后续的字符串指令的时候
								;esi和edi会自动  加上  搬运数据的大小
	push ebp			;访问栈中的参数是基于ebp来访问的，通常要将esp的值赋给ebp
								;由于不知道ebp的值是否重要，所以提前将ebp备份起来
	mov ebp, esp	;将esp的值赋给ebp
	push ecx							;rep指令用到了ecx
												;但是ecx对于外层段的循环由作用，先入栈备份
	mov edi, [ebp + 8]		;dst,栈中每一个单元占用4个字节
	mov esi, [ebp + 12]		;src
	mov ecx, [ebp + 16]		;size
	rep movsb							;逐个字节拷贝

	;恢复环境
	pop ecx
	pop ebp
	ret

	
	;jmp $

;-------读取硬盘n个扇区空间
rd_disk_m_32:
	;eax=扇区号
	;ebx=将数据写入的内存地址
	;ecx=读入的扇区数
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
  mov [ebx],ax
  add ebx,2
  loop .go_on_ready
  ret


;-------创建页目录以及页表
setup_page:
	mov ecx, 4096
	mov esi, 0
;把页目录项占用的空间逐个字节清0
;循环次数4096
.clear_page_dir:
	mov byte [PAGE_DIR_TABLE_POS + esi], 0
	inc esi
	loop .clear_page_dir

;开始创建页目录项PDE(Page Directory Entry)
.create_pde:
	mov eax, PAGE_DIR_TABLE_POS
	;此是eax指向的是第一个页表的位置，eax越过页目录所有的地址
	add eax, 0x1000
	;此处为 ebx赋值，是为.create_pte 做准备，ebx为基址
	mov ebx, eax

;每一个页表表示4M的内存
;这样 0xc03fffff 以下的地址和 0x003fffff 以下的地址都指向相同的页表
;这是为将地址映射为内核地址做准备
	;表示用户属性，所有特权级别都可以访问
	or eax, PG_US_U | PG_RW_W | PG_P	;第一个页表的位置是0x101000属性是7
	;第一个页表的地址放在页目录项0中
	;页目录项中高20存储的物理地址，后12位存储的都是属性
	mov [PAGE_DIR_TABLE_POS + 0x0], eax
	;一个页表项占用4个字节
	;0xc00表示第768个页表占用的目录项，0xc00以上的目录项用于内核空间
	;也就是说页表的0xc0000000--0xffffffff共计1G属于内核
	;0x0--0xbfffffff共计3G属于用户进程
	;页目录项0xc00也存放第一个页表的地址
	mov [PAGE_DIR_TABLE_POS + 0xc00], eax
	
	sub eax, 0x1000												;eax指向了页目录地址
	mov [PAGE_DIR_TABLE_POS + 4092], eax	;页目录项中的最后一个目录项指向的是页目录表自己地址

;开始创建页表项PTE
;此页表是第0个页表项对应的页表，他用来分配物理地址范围0--0x3fffff
;一个页表表示的内存容量是4MB，但我们目前只用到了第1个1MB空间
;所以我们只为这1MB空间对应的页表项分配物理页
	mov ecx, 256	;每个物理页是4K，所以1M空间只需要256个页表项
	mov esi, 0
	mov edx, PG_US_U | PG_RW_W | PG_P		;属性为7
.creade_pte:						;创建每一个Page Table Entry
												;之前的ebx已经赋值为0x101000,是第一个页表的位置
	mov [ebx+esi*4], edx	;edx是物理页的页表项
	add edx, 4096					;每次都加4K，物理地址是连续分配的
	inc esi
	loop .creade_pte

;创建内核其他页表的PDE
	mov eax, PAGE_DIR_TABLE_POS
	add eax, 0x2000				;此是eax指向第二个页表的位置
	or eax, PG_US_U | PG_RW_W | PG_P	;属性是7
	mov ebx, PAGE_DIR_TABLE_POS
	mov ecx, 254					;范围为第 769～1022 的所有目录项数量
	mov esi, 769					;数量是254
.create_kernel_pde:
	mov [ebx+esi*4], eax
	inc esi								;偏移量+1
	add eax, 0x1000				;页表数+1
	loop .create_kernel_pde
	;最后从2--256都是内核对应的页表空间
	;此时还没有为页表中的具体PTE分配物理页，还不算真正的内存空间
	ret


  ; mov byte [gs:160], 'P';利用选择子进行寻址的过程需要清楚！！！
  ; ;gs中选择子的低两位是RPL，第3位是TL=0，gs中表示的是在GDT中索引段描述符
  ; ;用gs的高18位在GDT中进行索引，找到段描述符之后，计算出相应的段基址，与偏移地址相加
  ; ;用所得的地址作为访存的地址
  ; jmp $

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
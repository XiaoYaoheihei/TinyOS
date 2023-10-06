TI_GDT  equ 0
RPL0    equ 0
SELECTOR_VIDEO equ (0x0003<<3)+TI_GDT+RPL0

section .data
put_int_buffer dq 0   ;定义了8字节的缓冲区用于转化

[bits 32]
section .text
;------------实现put_int的功能
;将小端字节序的数字变成对应的ASCII之后，倒置
;输入：栈中参数待打印的数字
;输出:在屏幕上打印16进制数字，并且不会打印0x前缀

global put_int
put_int:
  pushad            ;8个寄存器
  mov ebp, esp
  mov eax, [ebp+4*9]  ;8个寄存器和一个返回地址
  mov edx, eax        ;拿到数字的值
  mov edi, 7          ;指定在buffer中的初始偏移量
  mov ecx, 8          ;32位数字中，有8个16进制的数字
  mov ebx, put_int_buffer ;buffer的起始地址
  ;将32位数字按照16进制的形式从低位到高位逐个处理
  ;一共处理8个16进制数字
  .16based_4bits:
  ;遍历每一位16进制数字
    and edx, 0x0000000f ;解析16进制中数字的每一位
                        ;and操作之后只有低4位有效
    cmp edx, 9          ;数字0-9和a-f需要分别处理成对应的字符
    jg .is_A2F         
    add edx, '0'        ;ASCII大小是8位，add之后edx只有低8位有效
    jmp .store
  .is_A2F:
    sub edx, 10         ;a-f的话-10得到的差+字符A的ASCII,便是a-f
                        ;的ASCII值
    add edx, 'A'

  .store:
    ;每一位数字转化成字符之后，按照类似大端的顺序
    ;高位字符放在低地址，低位字符放在高地址
    mov [ebx+edi], dl   ;此时dl中是数字对应的字符ASCII值
    dec edi
    shr eax, 4          ;处理下一个16进制，右移的方式
    mov edx, eax        ;方便下一次处理
    loop .16based_4bits

  ;现在buffer中都是字符，打印之前，需要把高位连续的字符去掉
  ;比如把字符000123变成123
  .ready_to_print:
    inc edi       ;edi需要+1
  .skip_prefix_0:
    cmp edi, 8    ;偏移地址已经到第8个了，说明是第9位，偏移地址最多是7
                  ;第一次就==8的话说明在buffer存储了0位
    je .full0     ;buffer中前面的全是0
  .go_on_skip:
    mov cl, [put_int_buffer+edi]
    inc edi       ;指向了下一个偏移地址
    cmp cl, '0'   ;如果刚好此时只存储了1位，这一位还恰好是0，还得处理
    je .skip_prefix_0
    dec edi       ;如果上面判断的字符不是0的话，edi-1恢复到当前字符
    jmp .put_each_num

  .full0:
    mov cl, '0'   ;输入的字符全为0,只打印一个0
  .put_each_num:
    push ecx
    call put_char
    add esp, 4
    inc edi
    mov cl, [put_int_buffer+edi]
    cmp edi, 8
    jl .put_each_num
    popad
    ret


;--------实现put_str功能，本质上就是对put_char的调用
;--------put_str通过put_char来打印以0结尾的字符串
;输入：栈中参数为打印的字符串
;输出：无
global put_str
put_str:            ;此函数只用到了ebx和ecx两个寄存器
  push ebx
  push ecx
  xor ecx, ecx      ;用ecx存储字符参数，必须清空
  mov ebx, [esp+12] ;从栈中得到待打印的字符串首地址
                    ;编译器会给字符串常量分配一块内存存储各个字符的ASCII码
                    ;并且会为其在结尾后面自动补'\0'
                    ;另外编译器在将该字符串作为参数时会传入字符串的内存首地址
.goon:
  mov cl, [ebx]     ;cl中是字符的ASCII码
  cmp cl, 0
  jz .str_over
  push ecx          ;为put_char传递参数，
                    ;32位保护模式下的栈压入16位操作数和32位操作数
  call put_char
  add esp, 4        ;回收字符所占的空间
  inc ebx
  jmp .goon

.str_over:
  pop ecx
  pop ebx
  ret


;----------实现put_char功能
;----------把栈中的第一个字符写入光标所在处

global put_char
put_char: 
  pushad            ;该指令压入所有双字长的寄存器，
                    ;总共8个，备份32位寄存器环境
  mov ax, SELECTOR_VIDEO  
  mov gs, ax        ;gs中是视频段选择子

  ;获取当前光标位置
  ;一个字符占2个字节，低字节是任意字符的ASCII码
  ;高字节是前景色和背景色属性
  ;先获取光标坐标的高8位
  mov dx, 0x03d4    ;索引寄存器的端口号
  mov al, 0x0e      ;寄存器的索引号
                    ;用于提供光标位置的高 8 位
  out dx, al
  mov dx, 0x03d5    ;通过读写端口0x3d5的寄存器来获得或设置光标位置
  in al, dx         ;得到光标的高8位
  mov ah, al
  ;再获取低8位
  mov dx, 0x03d4
  mov al, 0x0f      ;寄存器的索引号
  out dx, al
  mov dx, 0x03d5
  in al, dx
  ;将光标存入到bx中
  mov bx, ax          ;习惯使用bx做基址寻址
  mov ecx, [esp + 36] ;在栈中获取待打印的字符
                      ;pushad压入的是32个字节
                      ;加上主调函数4字节的返回地址
  cmp cl, 0xd         ;回车符CR 是 0x0d
  jz .is_carriage_return 
  cmp cl, 0xa         ;换行符LF 是 0x0a
  jz .is_line_feed

  cmp cl, 0x8         ;退回符BS(backspace)的 asc 码是 8
  jz .is_backspace
  jmp .put_other

.is_backspace:
  ;退回符的相应处理逻辑
  ; 当为 backspace 时，本质上只要将光标移向前一个显存位置即可.后面再输入的字符自然会覆盖此处的字符
  ; 但有可能在键入 backspace 后并不再键入新的字符，这时光标已经向前移动到待删除的字符位置，但字符还在原处
  ; 这就显得好怪异，所以此处添加了空格或空字符 0
  dec bx          ;光标坐标向前移动1位，指向前一个字符
  shl bx, 1       ;✖2,表示光标对应的显存中的偏移地址

  mov byte [gs:bx], 0x20  ;处于低字节处，待删除的字节位置补为0或者空格
  inc bx
  mov byte [gs:bx], 0x07  ;高字节处，表示黑屏白字
  shr bx, 1               ;由显存中的相对地址恢复成光标地址
  jmp .set_cursor

.put_other:
  ;处理可见字符的代码
  shl bx, 1               ;将光标值✖2，得到在显存中的偏移地址
  mov [gs:bx], cl         ;低字节是ASCII码本身
  inc bx
  mov byte  [gs:bx], 0x07
  shr bx, 1               ;恢复光标值
  inc bx                  ;下一个光标
  cmp bx, 2000
  jl .set_cursor          ;光标坐标<2000，还未写到显存的最后，去设置新的光标值
                          ;若超出屏幕字符数大小，换行处理

.is_line_feed:
  ;是换行符的处理
.is_carriage_return:
  ;是回车符的处理
  ;把光标移动到本行行首就行
  xor dx, dx              ;被除数的高16位，清0
  mov ax, bx              ;被除数的低16位，
  mov si, 80              ;每一行是80充当除数
  div si                  ;除以80再-余数就是行首位置
  sub bx, dx              ;经过div之后dx寄存器中存的是余数

.is_carriage_return_end:
  add bx, 80              ;直接移动到下一行行首
  cmp bx, 2000
.is_line_feed_end:
  jl .set_cursor

;屏幕行范围是 0～24，滚屏的原理是将屏幕的第 1～24 行搬运到第 0～23 行
;再将第 24 行用空格填充
.roll_screen:
  ;滚屏操作
  cld                 ;清除方向位，标志位DF=0
  mov ecx, 960        ;2000-80=1920个字符需要搬运，一共3840个字节
                      ;一次搬运4字节，一共需要3840/4=960次
  mov esi, 0xc00b808a ;第一行行首位置
  mov edi, 0xc00b8000 ;第0行行首位置
  rep movsd
  ;将最后一行填充为空白
  mov ebx, 3840       ;字节偏移值是1920✖2=3840
  mov ecx, 80         ;一行是80个字符，每一清空1个字符，一行需要移动80次

  .cls:
  mov word [gs:ebx], 0x0720 ;黑底白字的空格键
  add ebx, 2
  loop .cls
  mov bx, 1920        ;将光标重置为1920，最后一行的首字符
  
  .set_cursor:      
  ;重新设置光标值为bx
  mov dx, 0x03d4  ;索引寄存器的端口号
  mov al, 0xe     ;用于提供高8位的寄存器索引
  out dx, al
  mov dx, 0x03d5  ;通过读写端口来获得值
  mov al, bh      ;把bx的值更新到光标坐标器的高8位
  out dx, al
  ;再设置低8位
  mov dx, 0x03d4
  mov al, 0x0f
  out dx, al
  mov dx, 0x03d5
  mov al, bl      ;把bx的值更新到光标坐标器的低8位
  out dx, al
.put_char_done:
  popad
  ret

global cls_screen
cls_screen:
  pushad
  ;由于用户程序的 cpl 为 3，显存段的 dpl 为 0
  ;故用于显存段的选择子 gs 在低于自己特权的环境中为 0
  ;导致用户程序再次进入中断后，gs 为 0
  ;直接在 put_str 中每次都为 gs 赋值
  mov ax, SELECTOR_VIDEO
  mov gs, ax

  mov ebx, 0
  mov ecx, 80*25

.cls:
  ;0x0720 是黑底白字的空格键
  mov word[gs:ebx], 0x0720
  add ebx, 2
  loop .cls
  mov ebx, 0

.set_cursor:
  ;;;;;;; 1 先设置高8位 ;;;;;;;;
  mov dx, 0x03d4			  ;索引寄存器
  mov al, 0x0e				  ;用于提供光标位置的高8位
  out dx, al
  mov dx, 0x03d5			  ;通过读写数据端口0x3d5来获得或设置光标位置 
  mov al, bh
  out dx, al

;;;;;;; 2 再设置低8位 ;;;;;;;;;
  mov dx, 0x03d4
  mov al, 0x0f
  out dx, al
  mov dx, 0x03d5 
  mov al, bl
  out dx, al
  popad
  ret


global set_cursor
set_cursor:
   pushad
   mov bx, [esp+36]
;;;;;;; 1 先设置高8位 ;;;;;;;;
   mov dx, 0x03d4			  ;索引寄存器
   mov al, 0x0e				  ;用于提供光标位置的高8位
   out dx, al
   mov dx, 0x03d5			  ;通过读写数据端口0x3d5来获得或设置光标位置 
   mov al, bh
   out dx, al

;;;;;;; 2 再设置低8位 ;;;;;;;;;
   mov dx, 0x03d4
   mov al, 0x0f
   out dx, al
   mov dx, 0x03d5 
   mov al, bl
   out dx, al
   popad
   ret
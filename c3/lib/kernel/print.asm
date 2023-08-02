TI_GDT  equ 0
RPL0    equ 0
SELECTOR_VIDEO equ (0x0003<<3)+TI_GDT+RPL0

[bits 32]
section .text
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
  jl .set_cursor          ;光标值<2000，还未写到显存的最后，去设置新的光标值
                          ;若超出屏幕字符数大小，换行处理

.is_line_feed:
  ;是换行符的处理
.is_carriage_return:
  ;是回车符的处理
  ;把光标移动到行首就行
  xor dx, dx
  mov ax, bx
  mov si, 80

  div si

  sub bx, dx

.is_carriage_return_end:
  add bx, 80
  cmp bx, 2000
.is_line_feed_end:
  jl .set_cursor


.roll_screen:
  cld
  mov ecx, 960
  
  mov esi, 0xc00b808a
  mov edi, 0xc00b8000
  rep movsd

  mov ebx, 3840
  mov ecx, 80

.cls:
  mov word [gs:ebx], 0x0720
  add ebx, 2
  loop .cls
  mov bx, 1920

.set_cursor:
  mov dx, 0x03d4
  mov al, 0xe
  out dx, al
  mov dx, 0x03d5
  mov al, bh
  out dx, al

  mov dx, 0x03d4
  mov al, 0x0f
  out dx, al
  mov dx, 0x03d5
  mov al, bl
  out dx, al
.put_char_done:
  popad
  ret
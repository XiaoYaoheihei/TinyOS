;---loader 和 kernel

LOADER_BASE_ADDR equ 0x900    ;loader（加载器）在内存中的位置
LOADER_START_SECTOR equ 0x2   ;loader在硬盘上的逻辑扇区地址,也就是LBA地址，第二块扇区

KERNEL_START_SECTOR  equ 0x9         ;kernel.bin 所在的扇区号
KERNEL_BIN_BASE_ADDR equ 0x70000     ;kernel.bin加载的内存地址
KERNEL_ENTRY_POINT   equ 0xc0001500

PAGE_DIR_TABLE_POS equ 0x100000 ;页表目录项的物理地址

;gdt中段描述符属性
DESC_G_4K   equ  1_00000000000000000000000b
DESC_D_32   equ   1_0000000000000000000000b
DESC_L      equ    0_000000000000000000000b
;64 位代码标记,此处标记为 0 便可
DESC_AVL    equ  0_00000000000000000000b
DESC_LIMIT_CODE2  equ 1111_0000000000000000b  ;代码段段界限的第二部分
DESC_LIMIT_DATA2  equ DESC_LIMIT_CODE2
DESC_LIMIT_VIDEO2 equ 0000_000000000000000b
DESC_P            equ 1_000000000000000b
DESC_DPL_0  equ 00_0000000000000b   ;特权级级别
DESC_DPL_1  equ 01_0000000000000b
DESC_DPL_2  equ 10_0000000000000b
DESC_DPL_3  equ 11_0000000000000b
DESC_S_CODE equ 1_000000000000b
DESC_S_DATA equ DESC_S_CODE
DESC_S_sys  equ 0_000000000000b

DESC_TYPE_CODE  equ 1000_00000000b  ;代码段的type字段
;x=1,c=0,r=0,a=0 代码段是可执行的，非一致性，不可读，已访问位 a 清 0
DESC_TYPE_DATA  equ 0010_00000000b
;x=0,e=0,w=1,a=0 数据段是不可执行的，向上扩展的，可写，已访问位 a 清 0
;代码段的高4字节内容
DESC_CODE_HIGH4 equ (0x00 << 24) + DESC_G_4K + DESC_D_32 + \
DESC_L + DESC_AVL + DESC_LIMIT_CODE2 + \
DESC_P+DESC_DPL_0 + DESC_S_CODE +\
DESC_TYPE_CODE + 0x00
;数据段的高4字节内容
DESC_DATA_HIGH4 equ (0x00 << 24) + DESC_G_4K + DESC_D_32 +\
DESC_L + DESC_AVL + DESC_LIMIT_DATA2 + \
DESC_P + DESC_DPL_0 + DESC_S_DATA + \
DESC_TYPE_DATA + 0x00
;显存段的高4字节内容
DESC_VIDEO_HIGH4 equ (0x00 << 24) + DESC_G_4K + DESC_D_32 +\
DESC_L + DESC_AVL + DESC_LIMIT_VIDEO2 + DESC_P + \
DESC_DPL_0 + DESC_S_DATA + DESC_TYPE_DATA + 0x0b

;--------------选择子属性 ---------------
RPL0 equ 00b  ;特权级为0,最高
RPL1 equ 01b  ;特权级为1,依次类推
RPL2 equ 10b
RPL3 equ 11b
TI_GDT equ 000b
TI_LDT equ 100b

;--------------页表相关属性
PG_P equ 1b
PG_RW_R equ 00b
PG_RW_W equ 10b
PG_US_S equ 000b
PG_US_U equ 100b

;-------------program type 定义
PT_NULL equ 0
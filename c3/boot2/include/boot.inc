;---loader 和 kernel

LOADER_BASE_ADDR equ 0x900    ;loader（加载器）在内存中的位置
LOADER_START_SECTOR equ 0x2   ;loader在硬盘上的逻辑扇区地址，第二块扇区
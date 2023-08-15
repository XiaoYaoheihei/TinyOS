#include "tss.h"
#include "../lib/stdint.h"
#include "../kernel/global.h"
#include "../lib/string.h"
#include "../lib/kernel/print.h"

//任务状态段tss结构
struct tss {
  uint32_t backlink;
  uint32_t* esp0;
  uint32_t ss0;
  uint32_t* esp1;
  uint32_t ss1;
  uint32_t* esp2;
  uint32_t ss2;
  uint32_t cr3;
  uint32_t (*eip) (void);
  uint32_t eflags;
  uint32_t eax;
  uint32_t ecx;
  uint32_t edx;
  uint32_t ebx;
  uint32_t esp;
  uint32_t ebp;
  uint32_t esi;
  uint32_t edi;
  uint32_t es;
  uint32_t cs;
  uint32_t ss;
  uint32_t ds;
  uint32_t fs;
  uint32_t gs;
  uint32_t ldt;
  uint32_t trace;
  uint32_t io_base;
};

static struct tss tss;

// 更新 tss 中 esp0 字段的值为 pthread 的 0 级线
void update_tss_esp(struct task_struct* pthread) {
  tss.esp0 = (uint32_t*)((uint32_t)pthread + PG_SIZE);
}

//创建gdt描述符
static struct gdt_desc make_gdt_desc(uint32_t* desc_addr,
                                     uint32_t limit,
                                     uint8_t  attr_low,
                                     uint8_t  attr_high) {
  //段基址的内容
  uint32_t desc_base = (uint32_t)desc_addr;
  struct gdt_desc desc;
  //limit是段界限值
  desc.limit_low_word = limit & 0x0000ffff;
  desc.base_low_word = desc_base & 0x0000ffff;
  desc.base_mid_byte = ((desc_base & 0x00ff0000) >> 16);
  desc.attr_low_byte = (uint8_t)(attr_low);
  desc.limit_high_attr_high = (((limit & 0x000f0000) >> 16) + (uint8_t)(attr_high));
  desc.base_high_byte = desc_base >> 24;
  return desc;
}

//在gdt中创建tss并且加载gdt
void tss_init() {
  put_str("tss_init start\n");
  uint32_t tss_size = sizeof(tss);
  memset(&tss, 0, tss_size);
  //0级栈段的选择子
  tss.ss0 = SELECTOR_K_STACK;
  tss.io_base = tss_size;

  // gdt 段基址为 0x900，把 tss 放到第 4 个位置，也就是 0x900+0x20 的位置
  //在 gdt 中添加 dpl 为 0 的 TSS 描述符
  //将0x00000920地址强制转化为指向gdt_desc的指针；
  //这个指针处的内容是函数的返回值
  *((struct gdt_desc*)0x00000920) = make_gdt_desc((uint32_t*)&tss, tss_size-1, TSS_ATTR_LOW, TSS_ATTR_HIGH);
  //在GDT中安装了两个供用户进程使用的描述符
  //一个是供用户使用的代码段
  //一个是供用户使用的数据段
  *((struct gdt_desc*)0xc0000928) = make_gdt_desc((uint32_t*)0, 0xfffff, GDT_CODE_ATTR_LOW_DPL3, GDT_ATTR_HIGH);
  *((struct gdt_desc*)0xc0000930) = make_gdt_desc((uint32_t*)0, 0xfffff, GDT_DATA_ATTR_LOE_DPL3, GDT_ATTR_HIGH);
  //7个描述符大小
  //定义gdt_operand为ldgt指令的操作数
  //操作数是“16 位表界限&32 位表的起始地址”
  //操作数中低16位是表界限，一共7个描述符，所以表界限值是7*8-1
  //操作数中高32位是GDT起始地址
  uint64_t gdt_operand = ((8*7-1) | ((uint64_t)(uint32_t)0xc0000900 << 16));
  //重新加载GDT
  asm volatile ("lgdt %0": : "m"(gdt_operand));
  //将TSS加载到TR寄存器
  asm volatile ("ltr %w0": : "r"(SELECTOR_TSS));
  put_str("tss_init and ltr done");

}
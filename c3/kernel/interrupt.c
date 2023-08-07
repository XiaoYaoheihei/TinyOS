#include "global.h"
#include "interrupt.h"
#include "../lib/kernel/io.h"
#include "../lib/stdint.h"

#define IDT_DESC_CNT 0X21   //目前一共支持33种中断
#define PIC_M_CTRL   0X20   //主片的控制端口是0x20
#define PIC_M_DATA   0X21   //数据端口是0x21
#define PIC_S_CTRL   0XA0   //从片的控制端口是0xa0
#define PIC_S_DATA   0XA1   //数据端口是0xa1

//中断门描述符结构体，总共8B大小
struct gate_desc {
  uint16_t func_offset_low_word;//低32位中断处理程序在目标代码段内的偏移量0-15
  uint16_t selector;    //目标代码段描述符选择子
  uint8_t dcount;       //此项为双字计数字段,是门描述符中的第4字节
                        //此项固定值不用考虑
  uint8_t attribute;
  uint16_t func_offset_high_word;//高32位偏移量16-31
};

extern intr_handler intr_entry_table[IDT_DESC_CNT]; //kernel.asm中的中断处理函数入口数组
//静态函数声明
static void make_idt_desc(struct gate_desc* p_gdesc, uint8_t attr, intr_handler function);
//idt是属于全局数据结构
static struct gate_desc idt[IDT_DESC_CNT];  //声明中断描述符表

//创建中断门描述符
//中断门描述符的指针，中断描述符内的属性，中断描述符对应的中断处理函数
static void make_idt_desc(struct gate_desc* p_gdesc, uint8_t attr, intr_handler function) {
  p_gdesc->func_offset_low_word = (uint32_t)function & 0x0000ffff;
  p_gdesc->selector = SELECTOR_K_CODE;
  p_gdesc->dcount = 0;
  p_gdesc->attribute = attr;
  p_gdesc->func_offset_high_word = ((uint32_t)function & 0xffff0000) >> 16;
}

//初始化中断描述符
static void idt_desc_init(void) {
  for (int i = 0; i < IDT_DESC_CNT; i++) {
    make_idt_desc(&idt[i], IDT_DESC_ATTR_DPL0, intr_entry_table[i]);
  }
  put_str("idt_desc_init done\n");
}

//初始化可编程中断控制器 8259A
static void pic_init(void) {
  //初始化主片
  outb (PIC_M_CTRL, 0X11);  //icw1,边沿触发,级联 8259, 需要 ICW4
  outb (PIC_M_DATA, 0x20);  //icw2,起始中断向量号为 0x20,也就是 IR[0-7] 为 0x20 ～ 0x27
  outb (PIC_M_DATA, 0x04);  //icw3,IR2 接从片
  outb (PIC_M_DATA, 0x01);  //icw4,8086 模式, 正常 EOI

  //初始化从片
  outb (PIC_S_CTRL, 0x11);  //icw1,边沿触发,级联 8259, 需要 ICW4
  outb (PIC_S_DATA, 0x28);  //icw2,起始中断向量号为 0x28,也就是 IR[8-15]为 0x28 ～ 0x2F
  outb (PIC_S_DATA, 0x02);  //icw3,设置从片连接到主片的 IR2 引脚
  outb (PIC_S_DATA, 0x01);  //icw4,8086 模式, 正常 EOI
  //打开主片上的IR0,目前只是接收时钟产生的中断，屏蔽掉其他所有的中断
  //此时发送的任何数据都是操作控制字，即是ocw
  //0表示不屏蔽，1表示屏蔽
  outb (PIC_M_DATA, 0xfe);  //主片上第0位表示不屏蔽时钟中断，其他位都屏蔽
  outb (PIC_S_DATA, 0xff);  //从片上的所有外设都屏蔽
  put_str(" pic_init done\n");

}

//完成有关中断的所有初始化工作
void idt_init() {
  put_str("idt_init start\n");
  idt_desc_init();  //初始化中断描述符
  pic_init();       //初始化8259A
  
  //加载idt
  //首先得到idt的段界限limit，用作低16位
  uint64_t idt_operand = ((sizeof(idt) - 1) | ((uint64_t)((uint32_t)idt << 16)));
  //通过内存获取48位操作数
  asm volatile("lidt %0" : : "m" (idt_operand));
  put_str("idt_init done\n");
}

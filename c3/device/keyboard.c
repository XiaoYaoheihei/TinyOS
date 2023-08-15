#include "keyboard.h"
//键盘buffer寄存器的端口号是0x60
#define KBD_BUF_PORT 0X60
//使用转义字符定义部分控制字符
//控制字符的ASCII码
#define esc '\033'    //八进制表示符
#define backspace '\b'
#define tab '\t'
#define enter '\r'
#define delete '\177' //八进制表示符

//不可见字符的ASCII一律定义为0
#define char_invisible 0
#define ctrl_l_char char_invisible
#define ctrl_r_char char_invisible
#define shift_l_char char_invisible
#define shift_r_char char_invisible
#define alt_l_char char_invisible
#define alt_r_char char_invisible
#define caps_lock_char char_invisible

//控制字符的通码和断码
//操作控制键的扫描码
#define shift_l_make 0x2a
#define shift_r_make 0x36
#define alt_l_make 0x38
#define alt_r_make 0xe038
#define alt_r_break 0xe0b8
#define ctrl_l_make 0x1d
#define ctrl_r_make 0xe01d
#define ctrl_r_break 0xe09d
#define caps_lock_make 0x3a

//定义以下变量记录相应键是否先前已经被按下
//因为每次的键盘中断处理程序只处理1B，所以当扫描码是多B或者有组合键的时候，要有额外的变量来记录
//ext标记为扩展标记
static bool ctrl_status, shift_status, alt_status, caps_lock_status, ext_scancode;

// 以通码 make_code 为索引的二维数组
static char keymap[][2] = {
/* 扫描码   未与shift组合  与shift组合*/
/* ---------------------------------- */
/* 0x00 */	{0,	0},		
/* 0x01 */	{esc,	esc},		
/* 0x02 */	{'1',	'!'},		
/* 0x03 */	{'2',	'@'},		
/* 0x04 */	{'3',	'#'},		
/* 0x05 */	{'4',	'$'},		
/* 0x06 */	{'5',	'%'},		
/* 0x07 */	{'6',	'^'},		
/* 0x08 */	{'7',	'&'},		
/* 0x09 */	{'8',	'*'},		
/* 0x0A */	{'9',	'('},		
/* 0x0B */	{'0',	')'},		
/* 0x0C */	{'-',	'_'},		
/* 0x0D */	{'=',	'+'},		
/* 0x0E */	{backspace, backspace},	
/* 0x0F */	{tab,	tab},		
/* 0x10 */	{'q',	'Q'},		
/* 0x11 */	{'w',	'W'},		
/* 0x12 */	{'e',	'E'},		
/* 0x13 */	{'r',	'R'},		
/* 0x14 */	{'t',	'T'},		
/* 0x15 */	{'y',	'Y'},		
/* 0x16 */	{'u',	'U'},		
/* 0x17 */	{'i',	'I'},		
/* 0x18 */	{'o',	'O'},		
/* 0x19 */	{'p',	'P'},		
/* 0x1A */	{'[',	'{'},		
/* 0x1B */	{']',	'}'},		
/* 0x1C */	{enter,  enter},
/* 0x1D */	{ctrl_l_char, ctrl_l_char},
/* 0x1E */	{'a',	'A'},		
/* 0x1F */	{'s',	'S'},		
/* 0x20 */	{'d',	'D'},		
/* 0x21 */	{'f',	'F'},		
/* 0x22 */	{'g',	'G'},		
/* 0x23 */	{'h',	'H'},		
/* 0x24 */	{'j',	'J'},		
/* 0x25 */	{'k',	'K'},		
/* 0x26 */	{'l',	'L'},		
/* 0x27 */	{';',	':'},		
/* 0x28 */	{'\'',	'"'},		
/* 0x29 */	{'`',	'~'},		
/* 0x2A */	{shift_l_char, shift_l_char},	
/* 0x2B */	{'\\',	'|'},		
/* 0x2C */	{'z',	'Z'},		
/* 0x2D */	{'x',	'X'},		
/* 0x2E */	{'c',	'C'},		
/* 0x2F */	{'v',	'V'},		
/* 0x30 */	{'b',	'B'},		
/* 0x31 */	{'n',	'N'},		
/* 0x32 */	{'m',	'M'},		
/* 0x33 */	{',',	'<'},		
/* 0x34 */	{'.',	'>'},		
/* 0x35 */	{'/',	'?'},
/* 0x36	*/	{shift_r_char, shift_r_char},	
/* 0x37 */	{'*',	'*'},    	
/* 0x38 */	{alt_l_char, alt_l_char},
/* 0x39 */	{' ',	' '},		
/* 0x3A */	{caps_lock_char, caps_lock_char}
/*其它按键暂不处理*/
};

//定义键盘缓冲区
struct ioqueue kbd_buf;

//键盘中断处理程序
static void intr_keyboard_handler() {
  // put_char('k');
  //必须要读取输出缓冲区寄存器，否则 8042 不再继续响应键盘中断
  // uint8_t scancode = inb(KBD_BUF_PORT);
  // put_int(scancode);
  // return;
  //前一次中断中，三个键是否被按下
  bool ctrl_down_last = ctrl_status;
  bool shift_down_last = shift_status;
  bool caps_lock_last = caps_lock_status;
  bool break_code;
  
  uint16_t scancode = inb(KBD_BUF_PORT);
  //若扫描码 scancode 是 e0 开头的，表示此键的按下将产生多个扫描码
  //所以马上结束此次中断处理函数，等待下一个扫描码进来
  if (scancode == 0xe0) {
    //打开e0标记
    ext_scancode = true;
    return;
  }
  //如果上次是以 0xe0 开头的，将扫描码合并
  if (ext_scancode)  {
    scancode = ((0xe000) | scancode);
    //关闭e0标记
    ext_scancode = false;
  }
  //合并为完整的扫描码之后用于后续处理
  //扫描码可以是通码，也可以是断码，目前不清楚到底是哪种类型
  //判断是否为break_code
  //断码的第8位为1
  break_code = ((scancode & 0x0080) != 0);
  if (break_code) {
    // put_str("duan ma");   //调试信息
    //若此次的扫描码是断码
    //得到其make_code
    //由于 ctrl_r 和 alt_r 的 make_code 和 break_code 都是两字节
    //通过&操作获取make_code
    uint16_t make_code = (scancode &= 0xff7f);
    //三个任意的键弹起了，状态设置为false
    if (make_code == ctrl_l_make || make_code == ctrl_r_make) {
      ctrl_status = false;
    } else if (make_code == shift_l_make || make_code == shift_r_make) {
      shift_status = false;
    } else if (make_code == alt_l_make || make_code == alt_r_make) {
      alt_status = false;
    }
    //由于 caps_lock 不是弹起后关闭，所以需要单独处理
    return;

  } else if ((scancode > 0x00 && scancode < 0x3b) || (scancode == alt_r_make) || (scancode == ctrl_r_make)) {
    // put_str("tong ma");  //调试信息
    //若此次扫描码是通码
    //判断是否与shift结合
    bool shift = false;
    //首先判断是否是双字符键
    if ((scancode < 0x0e) || (scancode == 0x29) || (scancode == 0x1a) ||
        (scancode == 0x1b) || (scancode == 0x2b) || (scancode == 0x27) ||
        (scancode == 0x28) || (scancode == 0x33) || (scancode == 0x34) ||
        (scancode == 0x35)) {
        // 0x0e 数字'0'～'9',字符'-',字符'='
        // 0x29 字符'`'
        // 0x1a 字符'['
        // 0x1b 字符']'
        // 0x2b 字符'\\'
        // 0x27 字符';'
        // 0x28 字符'\''
        // 0x33 字符','
        // 0x34 字符'.'
        // 0x35 字符'/'
      if (shift_down_last) {
        //同时按下了shift键
        shift = true;
      }
    } else {
      //默认为字母键
      if (shift_down_last && caps_lock_last) {
        //shift和capslock同时按下
        shift = false;
      } else if (shift_down_last || caps_lock_last) {
        //任意被按下
        shift = true;
      } else {
        shift = false;
      }
    }
    //扫描码的高字节置0
    uint8_t index = (scancode &= 0x00ff);
    char cur_char = keymap[index][shift];
    //只处理ASCII码不为0的键
    //可显示字符或者格式控制字符
    if (cur_char) {
      //如果此时的键盘缓冲区中未满
      if (!ioq_full(&kbd_buf)) {
        put_char(cur_char);
        //临时操作，为了显示问题
        ioq_putchar(&kbd_buf, cur_char);
      }
      // put_char(cur_char);
      return;
    }
    //记录本次是否按下了下面几类控制键之一，供下次键入时判断组合键
    if (scancode == ctrl_l_make || scancode == ctrl_r_make) {
      ctrl_status = true;
    } else if (scancode == shift_l_make || scancode == shift_r_make) {
      shift_status = true;
    } else if (scancode == alt_l_make || scancode == alt_r_make) {
      alt_status = true;
    } else if (scancode == caps_lock_make) {
      // 不管之前是否有按下 caps_lock 键，当再次按下时则状态取反，
      //  即已经开启时，再按下同样的键是关闭。关闭时按下表示开启
      caps_lock_status = !caps_lock_status;
    }
  } else {
    put_str("unknow key\n");
  } 
}

//键盘初始化
void keyboard_init() {
  put_str("keyboard init start\n");
  ioqueue_init(&kbd_buf); //初始化缓冲区
  register_handler(0x21,intr_keyboard_handler);
  put_str("keyboard init down\n");
}
#ifndef _KERNEL_DEBUG_H
#define _KERNEL_DEBUG_H

void panic_spin(char* filename, int line, const char* func, const char* condition);
#define PANIC(...) panic_spin(__FILE__, __LINE__, __func__, __VA_ARGS__);

//如果定义了NDEBUG，就执行NDEBUG下面的语句
#ifdef NDEBUG 
#define ASSERT(CONDITION) ((void)0)
#else
#define ASSERT(CONDITION)\
if(CONDITION){}else{\
/* 符号#让编译器将宏的参数转化为字符串字面量 */ \
PANIC(#CONDITION); \
}
#endif

#endif
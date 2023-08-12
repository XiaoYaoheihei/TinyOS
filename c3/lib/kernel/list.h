#ifndef _LIB_KERNEL_LIST_H
#define _LIB_KERNEL_LIST_H

#include "../../kernel/global.h"
#include "../../kernel/interrupt.h"

#define offset(struct_type,member) (int)(&((struct_type*)0)->member)
//elem_ptr是待转化的地址，属于某一个结构体中某一个成员的地址
//member_name是elem_ptr对应的名字
//type是elem_ptr所属的结构体的类型
// elem_ptr 的地址减去 elem_ptr 在结构体 struct_type 中的偏移量
//此地址差便是结构体 struct_type 的起始地址
#define elem2entry(struct_type, struct_member_name, elem_ptr) \
        (struct_type*)((int)elem_ptr - offset(struct_type, struct_member_name))

//链表节点
struct list_elem {
  struct list_elem* prev;
  struct list_elem* next;
};

//链表结构
struct list {
  struct list_elem head;
  struct list_elem tail;
};

typedef bool (function)(struct list_elem*, int arg);

void list_init(struct list*);
void list_insert_before(struct list_elem* berfore, struct list_elem*);
void list_push(struct list*, struct list_elem*);
// void list_iterate(struct list*);
void list_append(struct list*, struct list_elem*);
void list_remove(struct list_elem*);
struct list_elem* list_pop(struct list*);
bool list_empty(struct list*);
uint32_t list_len(struct list*);
struct list_elem* list_traversal(struct list*, function, int);
bool elem_find(struct list*, struct list_elem*);

#endif
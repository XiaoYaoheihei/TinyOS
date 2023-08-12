#include "list.h"

//初始化双向链表
void list_init(struct list* plist) {
  plist->head.prev = NULL;
  plist->head.next = &plist->tail;
  plist->tail.prev = &plist->head;
  plist->tail.next = NULL;
}

//把链表元素插入到元素before之前
void list_insert_before(struct list_elem* before, struct list_elem* elem) {
  //在关中断的前提下进行原子操作
  enum intr_status old_status = intr_disable();

  before->prev->next = elem;
  elem->prev = before->prev;
  elem->next = before;

  before->prev = elem;
  intr_set_status(old_status);
}

//将元素添加到plist首部
void list_push(struct list* plist, struct list_elem* elem) {
  list_insert_before(plist->head.next, elem);
}

// void list_iterate(struct list*);

//追加元素到list尾部
void list_append(struct list* plist, struct list_elem* elem) {
  //在队尾的前面插入
  list_insert_before(&plist->tail, elem);
}

//元素脱离plist
void list_remove(struct list_elem* elem) {
  enum intr_status old_status = intr_disable();

  elem->prev->next = elem->next;
  elem->next->prev = elem->prev;

  intr_set_status(old_status);
}

//将list的第一个元素弹出并且返回
struct list_elem* list_pop(struct list* plist) {
  struct list_elem* elem = plist->head.next;
  list_remove(elem);
  return elem;
}

//判断是否为空
bool list_empty(struct list* plist) {
  return (plist->head.next == &plist->tail ? true : false);
}

//返回list的长度
uint32_t list_len(struct list* plist) {
  struct list_elem* elem = plist->head.next;
  uint32_t length = 0;
  while (elem != &plist->tail) {
    length++;
    elem = elem->next;
  }
  return length;
}

/* 把列表 plist 中的每个元素 elem 和 arg 传给回调函数 func，
* arg 给 func 用来判断 elem 是否符合条件．
* 本函数的功能是遍历列表内所有元素，逐个判断是否有符合条件的元素。
* 找到符合条件的元素返回元素指针，否则返回 NULL */
struct list_elem* list_traversal(struct list* plist, function func, int arg) {
  struct list_elem* elem = plist->head.next;
  //如果plist为空，就没有符合的条件
  if (list_empty(plist)) {
    return NULL;
  }
  while (elem != &plist->tail) {
    if (func(elem, arg)) {
      //回调函数返回true，则证明该元素在回调函数中符合条件，命中停止遍历
      return elem;
    }
    elem = elem->next;
  }
  return NULL;
}

//查找元素
bool elem_find(struct list* plist, struct list_elem* obj) {
  struct list_elem* elem = plist->head.next;
  while (elem != &plist->tail) {
    if (elem == obj) {
      return true;
    }
    elem = elem->next;
  }
  return false;
}
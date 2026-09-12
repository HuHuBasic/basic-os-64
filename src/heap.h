/*
 * heap.h - 内核堆分配器
 * 基于首次适配 + 相邻块合并的简单空闲链表分配器。
 * 完全自主实现，不依赖任何第三方库。
 */
#ifndef HEAP_H
#define HEAP_H

#include "types.h"

void  heap_init(void);
void *kmalloc(size_t size);
void *kzalloc(size_t size);
void  kfree(void *ptr);

/* 统计信息（字节） */
uint64_t heap_total(void);
uint64_t heap_used(void);
uint64_t heap_free(void);

#endif /* HEAP_H */

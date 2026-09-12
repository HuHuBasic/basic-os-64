/*
 * heap.c - 内核堆分配器实现
 *
 * 采用连续内存块 + 隐式空闲链表：每个块头部记录 size/free，
 * 通过 size 直接跳到物理相邻的下一个块，分配时首次适配并切分，
 * 释放时与前后相邻空闲块合并，抑制碎片。
 */
#include "heap.h"
#include "string.h"

#define HEAP_SIZE   (256 * 1024)
#define HEAP_MAGIC  0x48454150u   /* "HEAP" */
#define ALIGN8(x)  (((x) + 7) & ~(uint64_t)7)

typedef struct block {
    uint64_t size;   /* 有效载荷大小（不含头部） */
    uint32_t free;   /* 1 = 空闲, 0 = 已占用 */
    uint32_t magic;  /* 完整性校验 */
} block_t;

#define HDR_SIZE   (ALIGN8(sizeof(block_t)))
#define MIN_SPLIT  (HDR_SIZE + 16)

static uint8_t  heap_area[HEAP_SIZE] __attribute__((aligned(16)));
static block_t *heap_head = (block_t *)0;

static inline block_t *block_next(block_t *b)
{
    return (block_t *)((uint8_t *)b + HDR_SIZE + b->size);
}

void heap_init(void)
{
    heap_head = (block_t *)heap_area;
    heap_head->size  = HEAP_SIZE - HDR_SIZE;
    heap_head->free  = 1;
    heap_head->magic = HEAP_MAGIC;
}

void *kmalloc(size_t size)
{
    if (size == 0 || heap_head == (block_t *)0)
        return (void *)0;

    size = ALIGN8(size);

    for (block_t *b = heap_head; ; b = block_next(b)) {
        if (b->free && b->size >= size) {
            /* 若剩余空间足够，切分出一个新空闲块 */
            if (b->size >= size + MIN_SPLIT) {
                block_t *nb = (block_t *)((uint8_t *)b + HDR_SIZE + size);
                nb->size  = b->size - size - HDR_SIZE;
                nb->free  = 1;
                nb->magic = HEAP_MAGIC;
                b->size   = size;
            }
            b->free = 0;
            return (uint8_t *)b + HDR_SIZE;
        }

        block_t *n = block_next(b);
        if ((uint8_t *)n >= heap_area + HEAP_SIZE)
            break;
    }
    return (void *)0;   /* 内存不足 */
}

void *kzalloc(size_t size)
{
    void *p = kmalloc(size);
    if (p)
        memset(p, 0, size);
    return p;
}

void kfree(void *ptr)
{
    if (!ptr || heap_head == (block_t *)0)
        return;

    block_t *b = (block_t *)((uint8_t *)ptr - HDR_SIZE);
    if (b->magic != HEAP_MAGIC)
        return;   /* 非法指针，忽略 */

    b->free = 1;

    /* 向后合并相邻空闲块 */
    block_t *n = block_next(b);
    if ((uint8_t *)n < heap_area + HEAP_SIZE && n->magic == HEAP_MAGIC && n->free) {
        b->size += HDR_SIZE + n->size;
    }

    /* 向前合并：从头遍历找到前驱 */
    if (b != heap_head) {
        block_t *p = heap_head;
        while (block_next(p) < b)
            p = block_next(p);
        if (p->free && block_next(p) == b) {
            p->size += HDR_SIZE + b->size;
        }
    }
}

uint64_t heap_total(void)
{
    return HEAP_SIZE;
}

uint64_t heap_used(void)
{
    uint64_t used = 0;
    for (block_t *b = heap_head; (uint8_t *)b < heap_area + HEAP_SIZE; b = block_next(b)) {
        if (b->magic != HEAP_MAGIC)
            break;
        if (!b->free)
            used += b->size + HDR_SIZE;
        if ((uint8_t *)block_next(b) <= (uint8_t *)b)
            break;
    }
    return used;
}

uint64_t heap_free(void)
{
    return heap_total() - heap_used();
}

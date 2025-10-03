#ifndef MEMORY_H
#define MEMORY_H

#include "../types.h"

// Memory layout constants
#define KERNEL_START      0x10000   // 64KB - where kernel is loaded
#define KERNEL_HEAP_START 0x100000  // 1MB - start of kernel heap
#define KERNEL_HEAP_SIZE  0x400000  // 4MB - size of kernel heap

// Page size
#define PAGE_SIZE 4096

// Memory block structure for simple heap allocator
typedef struct mem_block {
    size_t size;
    int free;
    struct mem_block* next;
} mem_block_t;

// Function prototypes
void memory_init(void);
void* kmalloc(size_t size);
void kfree(void* ptr);
size_t get_free_memory(void);
size_t get_total_memory(void);

// Memory utility functions
void* memset(void* dest, int val, size_t count);
void* memcpy(void* dest, const void* src, size_t count);
int memcmp(const void* ptr1, const void* ptr2, size_t count);

#endif // MEMORY_H
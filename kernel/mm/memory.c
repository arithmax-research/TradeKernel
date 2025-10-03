#include "memory.h"

static mem_block_t* heap_start = NULL;
static size_t total_memory = KERNEL_HEAP_SIZE;
static size_t used_memory = 0;

void memory_init(void) {
    heap_start = (mem_block_t*)KERNEL_HEAP_START;
    heap_start->size = KERNEL_HEAP_SIZE - sizeof(mem_block_t);
    heap_start->free = 1;
    heap_start->next = NULL;
    used_memory = sizeof(mem_block_t);
}

void* kmalloc(size_t size) {
    if (size == 0) return NULL;
    
    // Align size to 8-byte boundary
    size = (size + 7) & ~7;
    
    mem_block_t* current = heap_start;
    
    while (current != NULL) {
        if (current->free && current->size >= size) {
            // Split block if it's much larger than needed
            if (current->size > size + sizeof(mem_block_t) + 8) {
                mem_block_t* new_block = (mem_block_t*)((char*)current + sizeof(mem_block_t) + size);
                new_block->size = current->size - size - sizeof(mem_block_t);
                new_block->free = 1;
                new_block->next = current->next;
                
                current->size = size;
                current->next = new_block;
            }
            
            current->free = 0;
            used_memory += current->size;
            return (char*)current + sizeof(mem_block_t);
        }
        current = current->next;
    }
    
    return NULL; // Out of memory
}

void kfree(void* ptr) {
    if (ptr == NULL) return;
    
    mem_block_t* block = (mem_block_t*)((char*)ptr - sizeof(mem_block_t));
    block->free = 1;
    used_memory -= block->size;
    
    // Merge with next block if it's free
    if (block->next && block->next->free) {
        block->size += block->next->size + sizeof(mem_block_t);
        block->next = block->next->next;
    }
    
    // Find previous block and merge if it's free
    mem_block_t* current = heap_start;
    while (current && current->next != block) {
        current = current->next;
    }
    
    if (current && current->free) {
        current->size += block->size + sizeof(mem_block_t);
        current->next = block->next;
    }
}

size_t get_free_memory(void) {
    return total_memory - used_memory;
}

size_t get_total_memory(void) {
    return total_memory;
}

// Memory utility functions
void* memset(void* dest, int val, size_t count) {
    unsigned char* ptr = (unsigned char*)dest;
    while (count--) {
        *ptr++ = (unsigned char)val;
    }
    return dest;
}

void* memcpy(void* dest, const void* src, size_t count) {
    unsigned char* dst = (unsigned char*)dest;
    const unsigned char* source = (const unsigned char*)src;
    while (count--) {
        *dst++ = *source++;
    }
    return dest;
}

int memcmp(const void* ptr1, const void* ptr2, size_t count) {
    const unsigned char* p1 = (const unsigned char*)ptr1;
    const unsigned char* p2 = (const unsigned char*)ptr2;
    while (count--) {
        if (*p1 != *p2) {
            return *p1 - *p2;
        }
        p1++;
        p2++;
    }
    return 0;
}
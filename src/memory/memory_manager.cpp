#include "memory.h"
#include "types.h"

namespace TradeKernel {
namespace Memory {

// Simple kernel heap implementation
constexpr size_t KERNEL_HEAP_SIZE = 16 * 1024 * 1024; // 16MB
static u8 kernel_heap_memory[KERNEL_HEAP_SIZE] __attribute__((aligned(64)));
static size_t heap_offset = 0;

// Simple memory pool implementation
struct SimplePool {
    u8* base;
    size_t size;
    size_t block_size;
    size_t num_blocks;
    volatile u32* free_bitmap;
    volatile u32 free_count;
};

// Memory pools for different allocation sizes
static SimplePool small_pool;  // 64-byte blocks
static SimplePool medium_pool; // 256-byte blocks  
static SimplePool large_pool;  // 1KB blocks

// Global memory manager instance
NumaMemoryManager* g_memory_manager = nullptr;

// Simple heap allocator for kernel
void* kernel_alloc(size_t size) {
    // Align to 8 bytes
    size = (size + 7) & ~7;
    
    size_t old_offset = __sync_fetch_and_add(&heap_offset, size);
    
    if (old_offset + size > KERNEL_HEAP_SIZE) {
        return nullptr; // Out of memory
    }
    
    return kernel_heap_memory + old_offset;
}

void kernel_free(void* /*ptr*/) {
    // Simple heap doesn't support individual frees
    // In production, would use a more sophisticated allocator
}

// Initialize a memory pool
bool init_pool(SimplePool& pool, size_t block_size, size_t num_blocks) {
    pool.block_size = block_size;
    pool.num_blocks = num_blocks;
    pool.size = block_size * num_blocks;
    
    // Allocate memory for the pool
    pool.base = static_cast<u8*>(kernel_alloc(pool.size));
    if (!pool.base) {
        return false;
    }
    
    // Allocate bitmap (4 bytes per 32 blocks)
    size_t bitmap_size = (num_blocks + 31) / 32 * sizeof(u32);
    pool.free_bitmap = static_cast<volatile u32*>(kernel_alloc(bitmap_size));
    if (!pool.free_bitmap) {
        return false;
    }
    
    // Initialize bitmap (all blocks free)
    for (size_t i = 0; i < (num_blocks + 31) / 32; i++) {
        pool.free_bitmap[i] = 0xFFFFFFFF;
    }
    
    pool.free_count = num_blocks;
    return true;
}

// Allocate from a pool using lock-free algorithm
void* pool_alloc(SimplePool& pool) {
    while (pool.free_count > 0) {
        // Find a free block
        for (u32 word_idx = 0; word_idx < (pool.num_blocks + 31) / 32; word_idx++) {
            u32 word = pool.free_bitmap[word_idx];
            if (word == 0) continue; // No free blocks in this word
            
            // Find first set bit
            u32 bit_idx = __builtin_ctz(word);
            u32 block_idx = word_idx * 32 + bit_idx;
            
            if (block_idx >= pool.num_blocks) continue;
            
            // Try to claim this block atomically
            u32 mask = 1U << bit_idx;
            if (__sync_bool_compare_and_swap(&pool.free_bitmap[word_idx], word, word & ~mask)) {
                __sync_fetch_and_sub(&pool.free_count, 1);
                return pool.base + block_idx * pool.block_size;
            }
        }
    }
    
    return nullptr; // Pool exhausted
}

// Free to a pool
void pool_free(SimplePool& pool, void* ptr) {
    if (!ptr || ptr < pool.base || ptr >= pool.base + pool.size) {
        return; // Invalid pointer
    }
    
    size_t offset = static_cast<u8*>(ptr) - pool.base;
    if (offset % pool.block_size != 0) {
        return; // Not aligned to block boundary
    }
    
    u32 block_idx = offset / pool.block_size;
    u32 word_idx = block_idx / 32;
    u32 bit_idx = block_idx % 32;
    u32 mask = 1U << bit_idx;
    
    // Mark block as free atomically
    __sync_or_and_fetch(&pool.free_bitmap[word_idx], mask);
    __sync_fetch_and_add(&pool.free_count, 1);
}

// LockFreePool implementation (simplified)
LockFreePool::LockFreePool(const PoolConfig& config) 
    : free_head(nullptr)
    , memory_start(nullptr)
    , block_size(config.block_size)
    , total_blocks(config.num_blocks)
    , alignment(config.alignment) {
    
    size_t total_size = block_size * total_blocks;
    memory_start = static_cast<u8*>(kernel_alloc(total_size));
    
    if (memory_start) {
        // Initialize free list
        for (size_t i = 0; i < total_blocks; i++) {
            Block* block = reinterpret_cast<Block*>(memory_start + i * block_size);
            if (i < total_blocks - 1) {
                block->next = reinterpret_cast<Block*>(memory_start + (i + 1) * block_size);
            } else {
                block->next = nullptr;
            }
        }
        free_head = reinterpret_cast<Block*>(memory_start);
    }
}

LockFreePool::~LockFreePool() {
    if (memory_start) {
        kernel_free(memory_start);
    }
}

size_t LockFreePool::available_blocks() const {
    size_t count = 0;
    Block* current = free_head;
    while (current) {
        count++;
        current = current->next;
    }
    return count;
}

bool LockFreePool::is_pool_memory(void* ptr) const {
    return ptr >= memory_start && ptr < memory_start + (block_size * total_blocks);
}

// NumaMemoryManager implementation
NumaMemoryManager::NumaMemoryManager() 
    : num_nodes(1)
    , current_cpu_node(0) {
    
    // Initialize single node for simplicity
    nodes[0].node_id = 0;
    nodes[0].total_memory = KERNEL_HEAP_SIZE;
    nodes[0].available_memory = KERNEL_HEAP_SIZE;
    
    for (size_t i = 0; i < 16; i++) {
        nodes[0].pools[i] = nullptr;
    }
}

NumaMemoryManager::~NumaMemoryManager() {
    for (u32 i = 0; i < num_nodes; i++) {
        for (size_t j = 0; j < 16; j++) {
            delete nodes[i].pools[j];
        }
    }
}

bool NumaMemoryManager::initialize() {
    // Pools are initialized globally, so just return success
    return true;
}

void* NumaMemoryManager::allocate(size_t size, u32 /*numa_node*/) {
    // Choose appropriate pool based on size
    if (size <= 64) {
        return pool_alloc(small_pool);
    } else if (size <= 256) {
        return pool_alloc(medium_pool);
    } else if (size <= 1024) {
        return pool_alloc(large_pool);
    } else {
        // Large allocation - use kernel heap directly
        return kernel_alloc(size);
    }
}

void NumaMemoryManager::deallocate(void* ptr) {
    if (!ptr) return;
    
    // Determine which pool this belongs to
    if (ptr >= small_pool.base && ptr < small_pool.base + small_pool.size) {
        pool_free(small_pool, ptr);
    } else if (ptr >= medium_pool.base && ptr < medium_pool.base + medium_pool.size) {
        pool_free(medium_pool, ptr);
    } else if (ptr >= large_pool.base && ptr < large_pool.base + large_pool.size) {
        pool_free(large_pool, ptr);
    } else {
        // Large allocation - kernel heap doesn't support individual frees
        kernel_free(ptr);
    }
}

void NumaMemoryManager::set_cpu_affinity(u32 /*cpu*/) {
    // Simplified - no actual affinity setting
}

u32 NumaMemoryManager::detect_numa_topology() {
    return 1; // Single node
}

void NumaMemoryManager::setup_pools_for_node(u32 /*node_id*/) {
    // Simplified - already set up in initialize()
}

// DMAMemoryRegion implementation (simplified)
DMAMemoryRegion::DMAMemoryRegion(size_t size, bool coherent) 
    : virtual_addr(nullptr)
    , physical_addr(0)
    , size(size)
    , is_coherent(coherent) {
    
    virtual_addr = kernel_alloc(size);
    physical_addr = reinterpret_cast<u64>(virtual_addr); // Simplified
}

DMAMemoryRegion::~DMAMemoryRegion() {
    if (virtual_addr) {
        kernel_free(virtual_addr);
    }
}

bool DMAMemoryRegion::map_for_device() {
    return true; // Simplified
}

void DMAMemoryRegion::sync_for_cpu() {
    // Simplified - no actual sync
}

void DMAMemoryRegion::sync_for_device() {
    // Simplified - no actual sync
}

// Global functions
bool initialize_memory_subsystem() {
    if (g_memory_manager) {
        return false; // Already initialized
    }
    
    // Create memory manager instance using kernel allocation
    g_memory_manager = static_cast<NumaMemoryManager*>(kernel_alloc(sizeof(NumaMemoryManager)));
    if (!g_memory_manager) {
        return false;
    }
    
    // Initialize memory pools first
    if (!init_pool(small_pool, 64, 1024)) {
        return false;
    }
    
    if (!init_pool(medium_pool, 256, 512)) {
        return false;
    }
    
    if (!init_pool(large_pool, 1024, 256)) {
        return false;
    }
    
    return true;
}

void shutdown_memory_subsystem() {
    if (g_memory_manager) {
        g_memory_manager->~NumaMemoryManager();
        g_memory_manager = nullptr;
    }
}

MemoryStats get_memory_stats() {
    MemoryStats stats = {};
    stats.total_allocated = heap_offset;
    stats.peak_allocated = heap_offset;
    stats.num_allocations = 0;
    stats.num_deallocations = 0;
    stats.avg_alloc_time = 0;
    stats.max_alloc_time = 0;
    return stats;
}

} // namespace Memory
} // namespace TradeKernel

#if !defined(BASE_H)
#define byte_offset(base, offset) ((u8*)(base) + (offset))

struct Memory_Pool
{
    memory_index size;
    memory_index used;
    u8 *base;

    memory_index alloc_history[500];
    u32 alloc_history_index;
};

#define push_struct(type) (type *)push_size_((&mempool), sizeof(type))
#define push_array(length, type) (type *)push_size_((&mempool), (length)*sizeof(type))
#define mempool_pop_last pop_size_((&mempool))
inline void *
push_size_(Memory_Pool *pool, memory_index size)
{
    Assert(size);
    Assert((pool->used + size) < pool->size);

    void *result = pool->base + pool->used;
    pool->used += size;

    pool->alloc_history[pool->alloc_history_index++] = size;
    Assert(pool->alloc_history_index < 500);
    return(result);
}
inline void pop_size_(Memory_Pool *pool) { pool->used -= pool->alloc_history[pool->alloc_history_index--]; }

struct Image
{
    u32 width;
    u32 height;
    u32 n_channels;
    u8 *bytes;
};

global Memory_Pool mempool;

#define BASE_H
#endif

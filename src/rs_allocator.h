#pragma once

/*
 * Memory allocators
 */

#include <stdlib.h>
#include "rs_base.h"
#include "rs_logger.h"

#define Kilobyte(v) ((i64)(v)*(i64)1024)
#define Megabyte(v) ((i64)Kilobyte(v)*(i64)1024)
#define Gigabyte(v) ((i64)Megabyte(v)*(i64)1024)

#define ALLOC(...) __alloc(__FILE__, __LINE__, ##__VA_ARGS__)
#define reallocate(...) __realloc(__FILE__, __LINE__, ##__VA_ARGS__)
#define dealloc(...) __dealloc(__FILE__, __LINE__, ##__VA_ARGS__)

struct IAllocator;

struct MemBlock
{
    void* ptr = nullptr;
    u64 size = 0;
    void* notaligned = nullptr;
    IAllocator* allocator = nullptr;

    inline u64 unalignedSize() const {
        return size + ((intptr_t)notaligned - (intptr_t)ptr);
    }

    inline void __dealloc(const char* filename, i32 line);
};

#define NULL_MEMBLOCK MemBlock{}

// Allocator interface
struct IAllocator
{
    virtual void release() {}
    virtual MemBlock __alloc(const char* filename, i32 line, u64 size, u8 alignment = 0) = 0;
    virtual MemBlock __realloc(const char* filename, i32 line, MemBlock block, u64 size, u8 alignment = 0) = 0;
    virtual void __dealloc(const char* filename, i32 line, MemBlock block) = 0;
    virtual bool owns(MemBlock block) const = 0;
};

inline void MemBlock::__dealloc(const char* filename, i32 line)
{
    if(allocator && ptr) {
        allocator->__dealloc(filename, line, *this);
    }
}

// Allocator using malloc
struct Mallocator: public IAllocator
{
    SINGLETON_IMP(Mallocator)

    i32 _allocCount = 0;

    MemBlock __alloc(const char* filename, i32 line, u64 size, u8 alignment = 0);
    MemBlock __realloc(const char* filename, i32 line, MemBlock block, u64 size, u8 alignment = 0);
    void __dealloc(const char* filename, i32 line, MemBlock block);

    inline bool owns(MemBlock block) const {
        return true;
    }
};

#define MEM_MALLOCATOR (Mallocator::get())

struct AllocatorStack: public IAllocator
{
    MemBlock _block = NULL_MEMBLOCK;
    u64 _topStackMarker = 0;
    u64 _highestMarker = 0;
    IAllocator* _fallback = &MEM_MALLOCATOR;

    AllocatorStack() = default;
    explicit AllocatorStack(MemBlock block) {
        init(block);
    }

    /**
     * @brief Allocate the global block of memory
     * @param size
     */
    void init(MemBlock block);

    /**
     * @brief Allocate
     * @param size
     * @return allocated block
     */
    MemBlock __alloc(const char* filename, i32 line, u64 size, u8 alignment, u64* out_mk);
    MemBlock __alloc(const char* filename, i32 line, u64 size, u8 alignment = 0);

    /**
     * @brief Realloc block. /!\ WILL dealloc to block start. /!\
     * @param filename
     * @param line
     * @param block
     * @param size
     * @param alignment
     * @return
     */
    MemBlock __realloc(const char* filename, i32 line, MemBlock block, u64 size, u8 alignment = 0);

    /**
     * @brief Dealloc TO block start
     * @param filename
     * @param line
     * @param block
     */
    void __dealloc(const char* filename, i32 line, MemBlock block);

    /**
     * @brief Dealloc TO marker
     * @param filename
     * @param line
     * @param block
     */
    void deallocTo(u64 marker, const char* filename = "unknown_file", i32 line = -1);

    /**
     * @brief Returns top marker. Doubles as a "how much memory are we using"
     * @return
     */
    inline u64 getTopMarker() const {
        return _topStackMarker;
    }

    inline u64 getHighestMarker() const {
        return _highestMarker;
    }

    /**
     * @brief Returns malloc'd size
     * @return
     */
    inline u64 size() const {
        return _block.size;
    }

    inline bool owns(MemBlock block) const {
        return block.ptr >= _block.ptr &&
                (intptr_t)block.ptr < ((intptr_t)_block.ptr + (intptr_t)_block.size);
    }
};

/**
 * @brief AllocatorBucket
 * - alloc blocks of bucketSize
 * - 1 byte overhead per block (TODO: make it one bit?)
 * - faster and fragment less that heap allocation
 * - realloc can be really fast
 */
struct AllocatorBucket: public IAllocator
{
    MemBlock _memBlock = NULL_MEMBLOCK;
    void* _bucketsPtr = nullptr;
    u8* _bytefield = nullptr;
    u32 _bucketSize = 1;
    u32 _bucketMaxCount = 0;
    u8 _bucketAlignment = 0;

    AllocatorBucket() = default;

    AllocatorBucket(MemBlock fromBlock, u32 bucketCount, u64 bucketSize, u8 alignment = 0) {
        init(fromBlock, bucketCount, bucketSize, alignment);
    }

    void init(MemBlock fromBlock, u32 bucketCount, u64 bucketSize, u8 alignment = 0);

    MemBlock __alloc(const char *filename, i32 line, u64 size, u8 alignment = 0);
    MemBlock __realloc(const char *filename, i32 line, MemBlock block, u64 sisze, u8 alignment);
    void __dealloc(const char *filename, i32 line, MemBlock block);
    inline bool owns(MemBlock block) const {
        return ((intptr_t)block.ptr >= (intptr_t)_bucketsPtr ||
                (intptr_t)block.ptr < (intptr_t)_bucketsPtr + (_bucketSize * _bucketMaxCount));
    }
};

/**
 * @brief Pool Allocator
 *	- very fast
 *	- limited to elementSize alllocations
 */
struct AllocatorPool: IAllocator
{
    MemBlock _block = NULL_MEMBLOCK;
    intptr_t _start = 0;
    u32 _elementSize = 0;
    u32 _elementCapacity = 0;

    struct FreeList {
        FreeList* next;
    };

    FreeList* _freeListTop = nullptr;

    void init(MemBlock block, u32 elementSize, u8 alignment = 0);

    MemBlock __alloc(const char* filename, i32 line, u64 size, u8 alignment = 0);
    MemBlock __realloc(const char* filename, i32 line, MemBlock block, u64 size, u8 alignment = 0);
    void __dealloc(const char* filename, i32 line, MemBlock block);

    inline bool owns(MemBlock block) const {
        return ((intptr_t)block.notaligned >= (intptr_t)_block.ptr &&
                (intptr_t)block.notaligned < (intptr_t)_block.ptr + (intptr_t)_block.size);
    }

    inline bool isFull() const {
        return _freeListTop == nullptr;
    }
};

/**
 * @brief Step allocator: use a different allocator based of allocation size
 */
struct AllocatorStep: IAllocator
{
    static constexpr i32 _MAX_ALLOCATORS = 32;
    IAllocator* _pAllocs[_MAX_ALLOCATORS];
    u64 _maxAllocSize[_MAX_ALLOCATORS];
    u32 _allocatorCount = 0;

    /**
     * @brief init
     * @param pAllocators array of allocator pointers
     * @param maxAllocSize array of maximum size to alloc for each allocator
     * @param count allocator count
     */
    void init(IAllocator** pAllocators, u64* maxAllocSize, u32 count);
    void release();

    MemBlock __alloc(const char* filename, i32 line, u64 size, u8 alignment = 0);
    MemBlock __realloc(const char* filename, i32 line, MemBlock block, u64 size, u8 alignment = 0);
    void __dealloc(const char* filename, i32 line, MemBlock block);
    bool owns(MemBlock block) const;
};

struct RingAllocator: IAllocator
{
    MemBlock _block = NULL_MEMBLOCK;
    intptr_t _cursor = 0;

    void init(MemBlock block, u8 alignment = 0);

    MemBlock __alloc(const char* filename, i32 line, u64 size, u8 alignment = 0);
    MemBlock __realloc(const char* filename, i32 line, MemBlock block, u64 size, u8 alignment = 0);
    void __dealloc(const char* filename, i32 line, MemBlock block);
};

struct MemoryContext
{
    IAllocator* allocator = &Mallocator::get();
    IAllocator* _oldAllocator = allocator;
    AllocatorStack* tempAllocator = nullptr;

    MemoryContext();

    inline void pushAllocator(IAllocator* pAlloc) {
        _oldAllocator = allocator;
        allocator = pAlloc;
    }

    inline void popAllocator() {
        allocator = _oldAllocator;
    }
};

MemoryContext& getMemoryContext();
inline void resetTempAllocator()
{
    getMemoryContext().tempAllocator->deallocTo(0);
}

#define MEM_CONTEXT (getMemoryContext())

inline MemBlock __mem_alloc(const char* filename, i32 line, u64 size, u8 alignment = 0)
{
    return MEM_CONTEXT.allocator->__alloc(filename, line, size, alignment);
}

#define MEM_ALLOC(...) __mem_alloc(__FILE__, __LINE__, ##__VA_ARGS__)

inline void __mem_dealloc(const char* filename, i32 line, MemBlock& mb)
{
    if(mb.allocator) {
        mb.allocator->__dealloc(filename, line, mb);
    }
}

#define MEM_DEALLOC(mb) __mem_dealloc(__FILE__, __LINE__, mb)

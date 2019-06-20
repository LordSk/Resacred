#include "rs_allocator.h"
#include <memory.h>
#include <string.h>

Logger& getMemLogger()
{
    static Logger memLog("memory_log");
    return memLog;
}


#if defined(CONF_DEBUG) && defined(SACRED_LOG_MEM_ALLOC)
    #define LOG_MEM(format, ...) getMemLogger().logf(LOG_COLOR_MAGENTA,\
    __FILE__, __LINE__, format, ##__VA_ARGS__)
#else
    #define LOG_MEM
#endif


inline i32 alignAdjust(intptr_t addr, u8 alignment)
{
    if(alignment == 0) return 0;
    assert(alignment >= 1 && alignment <= 128);
    assert((alignment & (alignment - 1)) == 0); // power of 2

    intptr_t misalignment = addr % alignment;

    // realign
    if(misalignment > 0) {
        return alignment - misalignment;
    }
    return 0;
}

MemBlock Mallocator::__alloc(const char* filename, i32 line, u64 size, u8 alignment)
{
    LOG_MEM("[Mallocator] %s:%d alloc(%d)", filename, line, size);
    assert(size > 0);
    size += alignment;

    void* ptr = malloc(size);
    assert(ptr);
    memset(ptr, 0, size);

    ++_allocCount;

    i32 adjust = alignAdjust((intptr_t)ptr, alignment);

    MemBlock block;
    block.ptr = (void*)((intptr_t)ptr + adjust);
    block.notaligned = ptr;
    block.size = size - adjust;
    block.allocator = this;
    return block;
}

MemBlock Mallocator::__realloc(const char* filename, i32 line, MemBlock block, u64 size, u8 alignment)
{
    assert(size > 0);
    LOG_MEM("[Mallocator] %s:%d reallocate(%#x, %d, %d)", filename, line, block.ptr, block.size, size);

    size += alignment;

    void* ptr = realloc(block.notaligned, size);
    assert(ptr);
    if(size > block.size) {
        memset((char*)ptr + block.size, 0, size - block.size);
    }

    i32 adjust = alignAdjust((intptr_t)ptr, alignment);

    MemBlock nb;
    nb.ptr = (void*)((intptr_t)ptr + adjust);
    nb.notaligned = ptr;
    nb.size = size - adjust;
    nb.allocator = this;
    return nb;
}

void Mallocator::__dealloc(const char* filename, i32 line, MemBlock block)
{
    if(!block.ptr || !block.notaligned) return;
    LOG_MEM("[Mallocator] %s:%d dealloc(%#x, %d)", filename, line, block.ptr, block.size);
    free(block.notaligned);
    --_allocCount;
}

void AllocatorStack::init(MemBlock block)
{
    assert(block.ptr);
    _block = block;
    _topStackMarker = 0;
}

MemBlock AllocatorStack::__alloc(const char* filename, i32 line, u64 size, u8 alignment, u64* out_mk)
{
    assert(size > 0);
    LOG_MEM("[AllocatorStack] %s:%d alloc(%d)", filename, line, size);

    i32 adjust = alignAdjust((intptr_t)_block.ptr + _topStackMarker, alignment);
    size += adjust;

    if(_topStackMarker + size > _block.size) {
        if(_fallback) {
            LOG_MEM("[AllocatorStack] allocator full, using fallback instead");
            return _fallback->ALLOC(size, alignment);
        }
        return NULL_MEMBLOCK;
    }

    if(out_mk) {
        *out_mk = _topStackMarker;
    }

    void* addr = (void*)((intptr_t)_block.ptr + _topStackMarker); // start of the block
    _topStackMarker += size; // end of the block
    memset(addr, 0, size);

    MemBlock block;
    block.ptr = (void*)((intptr_t)addr + adjust);
    block.notaligned = addr;
    block.size = size - adjust;
    block.allocator = this;

    _highestMarker = _topStackMarker > _highestMarker ? _topStackMarker : _highestMarker;
    return block;
}

MemBlock AllocatorStack::__alloc(const char* filename, i32 line, u64 size, u8 alignment)
{
    return __alloc(filename, line, size, alignment, nullptr);
}

MemBlock AllocatorStack::__realloc(const char* filename, i32 line, MemBlock block, u64 size, u8 alignment)
{
    assert(size > 0);

    LOG_MEM("[AllocatorStack] %s:%d reallocate(%#x, %d, %d)", filename,
            line, block.ptr, block.size, size);

    // the block we're trying to reallocate is the last one allocated
    if(((intptr_t)_block.ptr + _topStackMarker) == ((intptr_t)block.notaligned + block.unalignedSize())
       && owns(block)) {
        u64 newMk = (intptr_t)block.notaligned - (intptr_t)_block.ptr;

        i32 adjust = alignAdjust((intptr_t)_block.ptr + _topStackMarker, alignment);
        size += adjust;

        if(newMk + size > _block.size) {
            if(_fallback) {
                LOG_WARN("[AllocatorStack] %s:%d allocator full, using fallback instead", filename, line);
                MemBlock b = _fallback->ALLOC(size, alignment);
                if(b.ptr) {
                    memmove(b.ptr, block.ptr, block.size);
                }
                return b;
            }
            return NULL_MEMBLOCK;
        }

        void* addr = (void*)((intptr_t)_block.ptr + newMk); // start of the block
        _topStackMarker = newMk + size; // end of the block

        u64 ulsize = block.unalignedSize();
        if(size > ulsize) {
            memset((void*)((intptr_t)addr + ulsize), 0, size - ulsize);
        }

        MemBlock nb;
        nb.ptr = (void*)((intptr_t)addr + adjust);
        nb.notaligned = addr;
        nb.size = size - adjust;
        nb.allocator = this;

        _highestMarker = _topStackMarker > _highestMarker ? _topStackMarker : _highestMarker;
        return nb;
    }


    MemBlock b = ALLOC(size, alignment);
    if(b.ptr) {
        memmove(b.ptr, block.ptr, block.size);
    }
    return b;
}

void AllocatorStack::__dealloc(const char* filename, i32 line, MemBlock block)
{
    if(!block.ptr || !block.notaligned ||
       (intptr_t)block.notaligned + block.unalignedSize() != (intptr_t)_block.ptr + _topStackMarker) {
        return;
    }

    LOG_MEM("[AllocatorStack] %s:%d dealloc(%#x, %d)", filename, line, block.ptr, block.size);

    u64 marker = (intptr_t)block.notaligned - (intptr_t)_block.ptr;
    assert(marker < _block.size);
    _topStackMarker = marker;
}

void AllocatorStack::deallocTo(u64 marker, const char* filename, i32 line)
{
    assert(marker < _block.size);
    LOG_MEM("[AllocatorStack] %s:%d deallocTo(%llu)", filename, line, marker);
    _topStackMarker = marker;
}

//
// Bucket Allocator
//
void AllocatorBucket::init(MemBlock fromBlock, u64 bucketSize, u8 alignment)
{
    AllocatorStack stack(fromBlock);
	stack.setNoFallback();

	i32 maxBuckets = fromBlock.size / bucketSize;
	while(maxBuckets > 0) {
		MemBlock bytefield = stack.ALLOC(maxBuckets);
		MemBlock fsBlocks = stack.ALLOC(maxBuckets * bucketSize, alignment);

		if(bytefield.isValid() && fsBlocks.isValid()) {
			_bytefield = (u8*)bytefield.ptr;
			_bucketsPtr = fsBlocks.ptr;
			break;
		}

		maxBuckets--;
		stack.deallocTo(0);
	}

	assert_msg(maxBuckets > 0, "AllocatorBucket bucket count is invalid");

    _memBlock = fromBlock;
	_bucketMaxCount = maxBuckets;
    _bucketSize = bucketSize;
    _bucketAlignment = alignment;
}

MemBlock AllocatorBucket::__alloc(const char* filename, i32 line, u64 size, u8 alignment)
{
    assert(size > 0);

    LOG_MEM("[AllocatorBucket] %s:%d ALLOC(size=%d align=%d bucketAlign=%d)",
                filename, line, size, alignment, _bucketAlignment);

    u8 shouldAlign = 0;
    if(alignment > 0 && (alignment > _bucketAlignment || _bucketAlignment % alignment != 0)) {
        size += alignment;
        shouldAlign = 1;
    }

    // First fit
    // small allocation
    if(size <= _bucketSize) {
        for(u32 i = 0; i < _bucketMaxCount; ++i) {
            if(_bytefield[i] == 0) {
                _bytefield[i] = 1;
                MemBlock block;
                block.allocator = this;
                block.notaligned = (void*)((intptr_t)_bucketsPtr + (_bucketSize * i));
                i32 adjust = alignAdjust((intptr_t)block.notaligned, alignment * shouldAlign);
                block.ptr = (void*)((intptr_t)block.notaligned + adjust);
                block.size = _bucketSize - (alignment * shouldAlign);
                memset(block.ptr, 0, block.size);
                return block;
            }
        }
    }
    // big allocation
    else {
        i32 chain = 0;
        i32 chainStartBid = 0;
        const i32 chainGoal = ceil(size / (f64)_bucketSize);

        for(i32 i = 0; i < _bucketMaxCount; ++i) {
            if(_bytefield[i]) {
                chain = 0;
                chainStartBid = i+1;
            }
            else if(_bytefield[i] == 0) {
                chain++;
                if(chain == chainGoal) {
                    for(u32 j = 0; j < chainGoal; ++j) {
                        _bytefield[i-j] = 1;
                    }

                    MemBlock block;
                    block.allocator = this;
                    block.notaligned = (void*)((intptr_t)_bucketsPtr + (_bucketSize * chainStartBid));
                    i32 adjust = alignAdjust((intptr_t)block.notaligned, alignment * shouldAlign);
                    block.ptr = (void*)((intptr_t)block.notaligned + adjust);
                    block.size = (_bucketSize * chainGoal) - (alignment * shouldAlign);
                    memset(block.ptr, 0, block.size);
                    return block;
                }
            }
        }
    }

	assert_msg(0, "AllocatorBucket allocation failed");

	if(_fallback) {
		return _fallback->__alloc(filename, line, size, alignment);
	}

	assert(0); // should never be reached
    return NULL_MEMBLOCK;
}

MemBlock AllocatorBucket::__realloc(const char* filename, i32 line, MemBlock block, u64 size, u8 alignment)
{
    assert(size > 0);
    assert(size != block.size);

    LOG_MEM("[AllocatorBucket] %s:%d reallocate(%#x, %d, %d)", filename,
            line, block.ptr, block.size, size);

    if(!owns(block) || !block.ptr || block.size == 0) {
        return ALLOC(size, alignment);
    }

    u8 shouldAlign = 0;
    if(alignment > 0 && (alignment > _bucketAlignment || _bucketAlignment % alignment != 0)) {
        size += alignment;
        shouldAlign = 1;
    }

    u64 uaSize = block.unalignedSize();
    i32 bucketCount = uaSize / _bucketSize;
    i32 bid = ((intptr_t)block.notaligned - (intptr_t)_bucketsPtr) / _bucketSize;

    // shrink
    if(size < uaSize) {
        i32 newBucketCount = ceil(size / (f64)_bucketSize);
        i32 diff = bucketCount - newBucketCount;

        for(i32 i = diff-1; i >= 0; --i) {
            _bytefield[bid + i] = 0;
        }

        // if alignment changed, slide data
        i32 adjust = alignAdjust((intptr_t)block.notaligned, alignment);
        if((intptr_t)block.ptr != (intptr_t)block.notaligned + adjust) {
            void* oldPtr = block.ptr;
            block.ptr = (void*)((intptr_t)block.notaligned + adjust);
            memmove(block.ptr, oldPtr, block.size);
        }
        block.size = (newBucketCount * _bucketSize) - adjust;
        return block;
    }
    // try to expand
    else {
        i32 chain = bucketCount;
        const i32 chainGoal = ceil(size / (f64)_bucketSize);

        for(u32 i = bid+bucketCount; i < _bucketMaxCount; ++i) {
            if(_bytefield[i]) {
                break;
            }
            else if(_bytefield[i] == 0) {
                chain++;
                if(chain == chainGoal) {
                    for(u32 j = 0; j < chainGoal; ++j) {
                        _bytefield[bid+j] = 1;
                    }

                    // if alignment changed, slide data
                    i32 adjust = alignAdjust((intptr_t)block.notaligned, alignment);
                    if((intptr_t)block.ptr != (intptr_t)block.notaligned + adjust) {
                        void* oldPtr = block.ptr;
                        block.ptr = (void*)((intptr_t)block.notaligned + adjust);
                        memmove(block.ptr, oldPtr, block.size);
                    }

                    memset((void*)((intptr_t)block.ptr + block.size), 0,
                           (chainGoal * _bucketSize) - block.size);
                    block.size = (chainGoal * _bucketSize) - adjust;
                    return block;
                }
            }
        }
    }

    MemBlock newBlock = ALLOC(size, alignment);
    assert(newBlock.ptr);
    memmove(newBlock.ptr, block.ptr, block.size);
    dealloc(block);
    return newBlock;
}

void AllocatorBucket::__dealloc(const char* filename, i32 line, MemBlock block)
{
    if(!block.ptr || !block.notaligned || block.size == 0 || !owns(block)) {
        return;
    }

    u64 uaSize = block.unalignedSize();

    LOG_MEM("[AllocatorBucket] %s:%d dealloc(%#x, %d)", filename, line, block.notaligned, uaSize);

    if(uaSize <= _bucketSize) {
        i32 bid = ((intptr_t)block.notaligned - (intptr_t)_bucketsPtr) / _bucketSize;
        _bytefield[bid] = 0;
    }
    else {
        i32 bucketCount = uaSize / _bucketSize;
        i32 bid = ((intptr_t)block.notaligned - (intptr_t)_bucketsPtr) / _bucketSize;
        for(i32 i = 0; i < bucketCount; ++i) {
            _bytefield[bid + i] = 0;
        }
    }
}

void AllocatorBucket::getFillInfo(u64* allocatedSpace, u64* freeSpace)
{
    const i32 count = _bucketMaxCount;
    const i32 bucketSize = _bucketSize;
    u64 allocatedSpace_ = 0;
    u64 freeSpaced_ = 0;

    for(u32 i = 0; i < count; ++i) {
        const u8 b = _bytefield[i];
        allocatedSpace_ += bucketSize * b;
        freeSpaced_ += bucketSize * (!b);
    }

    *allocatedSpace = allocatedSpace_;
    *freeSpace = freeSpaced_;
}



//
// Pool Allocator
//
void AllocatorPool::init(MemBlock block, u32 elementSize, u8 alignment)
{
    assert(block.ptr && elementSize > sizeof(intptr_t));
    _block = block;
    i32 adjust = alignAdjust((intptr_t)block.ptr, alignment);
    _start = (intptr_t)_block.ptr + adjust;
    _elementSize = elementSize;
    _elementCapacity = (block.size - adjust) / elementSize;

    // populate freelist
    _freeListTop = nullptr;
    for(u32 i = 0; i < _elementCapacity; ++i) {
        FreeList* cur = (FreeList*)(_start + i * _elementSize);
        cur->next = _freeListTop;
        _freeListTop = cur;
    }
}

MemBlock AllocatorPool::__alloc(const char* filename, i32 line, u64 size, u8 alignment)
{
    if(!_freeListTop) {
        return NULL_MEMBLOCK;
    }

    intptr_t top = (intptr_t)_freeListTop;
    i32 adjust = alignAdjust(top, alignment);

    if(size + adjust > _elementSize) {
        return NULL_MEMBLOCK;
    }

    _freeListTop = _freeListTop->next; // pop it
    memset((void*)top, 0, _elementSize);

    MemBlock block;
    block.ptr = (void*)((intptr_t)top + adjust);
    block.notaligned = (void*)top;
    block.size = _elementSize - adjust;
    block.allocator = this;
    return block;
}

MemBlock AllocatorPool::__realloc(const char* filename, i32 line, MemBlock block, u64 size, u8 alignment)
{
    i32 adjust = alignAdjust((intptr_t)block.ptr, alignment);
    if(size + adjust <= _elementSize) {
        return block;
    }
    return NULL_MEMBLOCK;
}

void AllocatorPool::__dealloc(const char* filename, i32 line, MemBlock block)
{
    if(!block.ptr || !block.notaligned || !owns(block)) return;

    intptr_t ptr = (intptr_t)block.notaligned;
    if((ptr - _start) % _elementSize != 0) {
        LOG_MEM("lsk_PoolAllocator::__dealloc: tried to dealloc an invalid block");
        return;
    }

    FreeList* cur = (FreeList*)ptr;
    cur->next = _freeListTop;
    _freeListTop = cur;
}

void AllocatorStep::init(IAllocator** pAllocators, u64* maxAllocSize, u32 count)
{
    assert(pAllocators && maxAllocSize && count > 0);
    memmove(_pAllocs, pAllocators, sizeof(IAllocator*) * count);
    memmove(_maxAllocSize, maxAllocSize, sizeof(u64) * count);
    _allocatorCount = count;

    // bubble sort
    bool sorting = true;
    while(sorting) {
        sorting = false;

        for(i32 i = 1; i < _allocatorCount; ++i) {
            if(_maxAllocSize[i-1] > _maxAllocSize[i]) {
                swap(_maxAllocSize[i-1], _maxAllocSize[i]);
                swap(_pAllocs[i-1], _pAllocs[i]);
                sorting = true;
            }
        }
    }
}

void AllocatorStep::release()
{
    for(i32 i = 0; i < _allocatorCount; ++i) {
        _pAllocs[i]->release();
    }
}

MemBlock AllocatorStep::__alloc(const char* filename, i32 line, u64 size, u8 alignment)
{
    for(i32 i = 0; i < _allocatorCount; ++i) {
        if(size <= _maxAllocSize[i]) {
            MemBlock block = _pAllocs[i]->__alloc(filename, line, size, alignment);
            if(block.ptr) {
                return block;
            }
        }
    }

    assert_msg(false, "lsk_StepAllocator size doesn't fit any allocator or all allocators are full");
    return NULL_MEMBLOCK;
}

MemBlock AllocatorStep::__realloc(const char* filename, i32 line, MemBlock block, u64 size, u8 alignment)
{
    for(i32 i = 0; i < _allocatorCount; ++i) {
        if(_pAllocs[i]->owns(block)) {
            MemBlock reblock = _pAllocs[i]->reallocate(block, size, alignment);
            if(reblock.ptr) {
                return reblock;
            }
            else {
                _pAllocs[i]->dealloc(block);
            }
            break;
        }
    }

    MemBlock newBlock = __alloc(filename, line, size, alignment);
    memmove(newBlock.ptr, block.ptr, block.size);
    return newBlock;
}

void AllocatorStep::__dealloc(const char* filename, i32 line, MemBlock block)
{
    for(i32 i = 0; i < _allocatorCount; ++i) {
        if(_pAllocs[i]->owns(block)) {
            _pAllocs[i]->__dealloc(filename, line, block);
            return;
        }
    }
}

bool AllocatorStep::owns(MemBlock block) const
{
    for(i32 i = 0; i < _allocatorCount; ++i) {
        if(_pAllocs[i]->owns(block)) {
            return true;
        }
    }
    return false;
}


void AllocatorRing::init(MemBlock block, u8 alignment)
{
    assert(block.ptr);
    _block = block;
    _block.ptr = (void*)((intptr_t)_block.ptr + alignAdjust((intptr_t)_block.ptr, alignment));
}

MemBlock AllocatorRing::__alloc(const char* filename, i32 line, u64 size, u8 alignment)
{
    assert(size > 0);
    LOG_MEM("[RingAllocator] %s:%d alloc(%d)", filename, line, size);
    const i64 maxSize = size += alignment;

    if(_cursor + maxSize > _block.size) {
        _cursor = 0;
        assert(_cursor + maxSize <= _block.size);
    }

    void* addr = (void*)((intptr_t)_block.ptr + _cursor); // start of the block
    i32 adjust = 0;
    if(alignment) {
        adjust = alignAdjust((intptr_t)addr, alignment);
        size += adjust;
    }

    _cursor += size;
    memset(addr, 0, size);

    MemBlock block;
    block.ptr = (void*)((intptr_t)addr + adjust);
    block.notaligned = addr;
    block.size = size - adjust;
    block.allocator = this;
    return block;
}

MemBlock AllocatorRing::__realloc(const char* filename, i32 line, MemBlock block, u64 size, u8 alignment)
{
    // TODO: improve this
    return __alloc(filename, line, size, alignment);
}

void AllocatorRing::__dealloc(const char* filename, i32 line, MemBlock block)
{
    return;
}

#ifndef SACRED_TEMP_STACK_SIZE
    #define SACRED_TEMP_STACK_SIZE 10000
#endif

MemoryContext::MemoryContext()
{
	thread_local u8 __tempStack[SACRED_TEMP_STACK_SIZE];
    thread_local AllocatorStack tempStackAllocator;

    MemBlock block;
    block.allocator = nullptr;
    block.ptr = __tempStack;
    block.notaligned = __tempStack;
    block.size = SACRED_TEMP_STACK_SIZE;

    tempStackAllocator.init(block);
    tempAllocator = &tempStackAllocator;

    LOG_MEM("[MemoryContext] initialized (thid=%d temp_stack_size=%d)",
            threadGetId(), SACRED_TEMP_STACK_SIZE);
}

MemoryContext& getMemoryContext()
{
    thread_local MemoryContext context;
    return context;
}

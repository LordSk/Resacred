#pragma once
#include <io.h>
#include "rs_base.h"
#include "rs_string.h"
#include "rs_allocator.h"

/**
 * @brief Basic itertor class (for things like range-fors)
 */
template<typename T>
struct Iter
{
    T* val;

    Iter() = default;
    Iter(T* val_) {
        val = val_;
    }

    inline bool operator!=(const Iter& o) {
        return val != o.val;
    }

    inline Iter& operator++() {
        ++val;
        return *this;
    }

    inline T& operator*() {
        return *val;
    }
};

template<typename T, u32 staticCount = 1>
struct Array
{
    IAllocator* allocator = MEM_CONTEXT.allocator;
    T _stackData[staticCount] alignas(T);
    T* _data = _stackData;
    i32 _capacity = staticCount;
    i32 _count = 0;
    MemBlock _memBlock = NULL_MEMBLOCK;

    Array() = default;

    template<u32 from_staticCount>
    Array(const Array<T, from_staticCount>& from) {
        copy(from);
    }

    // deep copy
    template<u32 from_staticCount>
    void copy(const Array<T, from_staticCount>& from) {
        _count = from._count;
        if(_count >= _capacity) {
            reserve(from._capacity);
        }
        for(i32 i = 0; i < _count; ++i) {
            new(_data + i) T(from._data[i]); // copy data over
        }
    }

    ~Array() {
        destroy();
    }

    inline void destroy() {
        for(u32 i = 0; i < _count; ++i) {
            _data[i].~T();
        }
        if(allocator) {
            _memBlock.dealloc();
            _memBlock = NULL_MEMBLOCK;
            allocator = nullptr;
        }
    }

    void reserve(u32 newCapacity) {
        if(newCapacity <= _capacity) return;
        MemBlock memblock = allocator->reallocate(_memBlock, newCapacity * sizeof(T), alignof(T));
        assert_msg(memblock.ptr, "Out of memory");

        T* oldData = _data;
        _data = (T*)memblock.ptr;
        _capacity = newCapacity;

        if(oldData == _stackData) {
            for(i32 i = 0; i < _count; ++i) {
                new(_data + i) T(oldData[i]); // copy data over
                oldData[i].~T(); // destruct old data
            }
        }

        _memBlock = memblock;
    }

    T& push(const T& elt) {
        if(_count + 1 > _capacity) {
            reserve(_capacity * 2);
        }
        new(_data + _count) T((T&&)elt);
        _count++;
        return _data[_count-1];
    }

    T& push(const T* elements, const u32 eltCount = 1) {
        if(_count + eltCount > _capacity) {
            reserve(max(_capacity * 2, _count + eltCount));
        }

        u32 start = _count;
        _count += eltCount;
        for(u32 i = start; i < _count; ++i) {
            new(_data + i) T((T&&)elements[i]);
        }
        return _data[start];
    }

    T& pushPOD(const T* elements, const u32 eltCount = 1) {
        if(_count + eltCount >= _capacity) {
            reserve(max(_capacity * 2, _count + eltCount));
        }

        u32 start = _count;
        _count += eltCount;
        memmove(_data + start, elements, eltCount * sizeof(T));
        return _data[start];
    }

    inline void remove(i32 id) {
        assert_msg(_count > 0 && id < _count, "Element out of range");
        _data[id].~T();
        memmove(_data + id, _data + (_count -1), sizeof(T));
        --_count;
    }

    inline void remove(const T& elt) {
        const T* eltPtr = &elt;
        assert_msg(eltPtr >= _data && eltPtr < _data + _count, "Element out of range");

        u32 id = eltPtr - _data;
        _data[id].~T();
        memmove(_data + id, _data + (_count -1), sizeof(T));
        --_count;
    }

    inline void clear() {
        for(u32 i = 0; i < _count; ++i) {
            _data[i].~T();
        }
        _count = 0;
    }

    inline void clearPOD() {
        _count = 0;
    }

    inline u32 capacity() const {
        return _capacity;
    }

    inline u32 count() const {
        return _count;
    }

    inline u32 dataSize() const {
        return _count * sizeof(T);
    }

    inline T* data() {
        return _data;
    }

    inline T* data() const {
        return _data;
    }

    inline T& operator[](u32 index) {
        assert(index < _count);
        return _data[index];
    }

    inline const T& operator[](u32 index) const {
        assert(index < _count);
        return _data[index];
    }

    inline Iter<T> begin() {
        return Iter<T>(_data);
    }

    inline Iter<T> end() {
        return Iter<T>(_data + _count);
    }

    inline void shrinkTo(u32 num) {
        assert(num < _count);
        _count = num;
    }
};

/**
 *	Dynamically growing HashMap
 *	- _hashFunction: hash function used to hash keys
 *	- _growth: grow rate (default: 2)
 *	- jumpLimit: maximum number of jumps to find a spot, will grow if limit is reached
 *	- iteration is not O(1)
 */
template<typename KeyT, typename ValueT, u32 stackCount = 1>
struct HashMap
{
    u8 _stack_occupied[stackCount];
    u32 _stack_keyHash[stackCount] alignas(u32);
    ValueT _stack_value[stackCount] alignas(ValueT);

    IAllocator* allocator = mem_context.allocator;
    u8* _occupied = _stack_occupied;
    u32* _keyHash = _stack_keyHash;
    ValueT* _value = _stack_value;

    u32 _capacity = stackCount;
    u32 jumpLimit = 5;
    f32 _growth = 2.f;
    MemBlock _memBlock;
    bool constValues = false;

    // can use generic lambda here instead of template
    typedef u32 (*FUNC_HASH)(const KeyT& key);
    FUNC_HASH hashFunction = [](const KeyT& key) {
        return hash32_fnv1a(&key, sizeof(KeyT));
    };

    HashMap() {
        memset(_occupied, 0, sizeof(u8) * _capacity);
    }

    ~HashMap() {
        destroy();
    }

    inline void _destroyValue(i32 id) {
        if(!constValues) {
            _value[id].~ValueT();
        }
    }

    void destroy() {
        if(allocator) {
            clear();
            _memBlock.dealloc();
            allocator = nullptr;
        }
    }

    void clear() {
        for(u32 i = 0; i < _capacity; ++i) {
            if(_occupied[i]) {
                _destroyValue(i);
                _occupied[i] = false;
                _keyHash[i] = 0;
            }
        }
    }

    void reserve(u32 newCapacity) {
        if(newCapacity <= _capacity) return;

        MemBlock newBlock = allocator->alloc(newCapacity * (sizeof(u8) + sizeof(u32) + sizeof(ValueT))
                                             + 128);
        assert_msg(newBlock.ptr, "Out of memory");

        AllocatorStack stack;
        stack.init(newBlock);

        MemBlock new_occupiedBlock = stack.alloc(newCapacity * sizeof(u8), alignof(u8));
        assert_msg(new_occupiedBlock.ptr, "Out of memory");
        u8* new_occupied = (u8*)new_occupiedBlock.ptr;
        MemBlock new_keyHashBlock = stack.alloc(newCapacity * sizeof(u32), alignof(u32));
        assert_msg(new_keyHashBlock.ptr, "Out of memory");
        u32* new_keyHash = (u32*)new_keyHashBlock.ptr;
        MemBlock new_dataBlock = stack.alloc(newCapacity * sizeof(ValueT), alignof(ValueT));
        assert_msg(new_dataBlock.ptr, "Out of memory");
        ValueT* new_data = (ValueT*)new_dataBlock.ptr;

        for(u32 i = 0; i < _capacity; ++i) {
            if(_occupied[i]) {
                u32 id = _keyHash[i] % newCapacity;
                u32 jumps = 0;
                while(new_occupied[id]) {
                    ++id;
                    ++jumps;
                    if(id == newCapacity) {
                        id = 0;
                    }
                    // we hit a jump limit again, regrow (should rarely happen)
                    if(jumps == jumpLimit) {
                        newBlock.dealloc();
                        reserve(newCapacity * _growth);
                        return;
                    }
                }

                new_occupied[id] = 1;
                new_keyHash[id] = _keyHash[i];
                new(&new_data[id]) ValueT(_value[i]); // copy data
            }
        }

        _memBlock.dealloc();
        _memBlock = newBlock;

        _occupied = new_occupied;
        _keyHash = new_keyHash;
        _value = new_data;

        _capacity = newCapacity;
    }

    // warning: can be slow if it doesn't find a spot with the jump limit (will grow)
    ValueT& seth(u32 keyHash, const ValueT& value) {
        assert_msg(_capacity > 0, "HashMap not initialized"); // init array first
        i32 id = keyHash % _capacity;
        u32 jumps = 0;
        while(_occupied[id] && _keyHash[id] != keyHash) {
            ++id;
            ++jumps;
            if(id == _capacity) {
                id = 0;
            }

            if(jumps == jumpLimit) {
                // expand
                reserve(_capacity * _growth);
                return seth(keyHash, value);
            }
        }

        if(_occupied[id] && _keyHash[id] == keyHash) {
            _destroyValue(id);
        }
        _keyHash[id] = keyHash;
        new(_value + id) ValueT((ValueT&&)value);
        _occupied[id] = true;
        return _value[id];
    }

    inline ValueT& set(const KeyT& key, const ValueT& value) {
        u32 keyHash = hashFunction(key);
        return seth(keyHash, value);
    }

    void removeh(u32 keyHash) {
        u32 id = keyHash % _capacity;
        u32 jumps = 0;
        while(jumps++ < jumpLimit) {
            if(_occupied[id] && _keyHash[id] == keyHash) {
                _occupied[id] = false;
                _keyHash[id] = 0;
                _destroyValue(id);
                return;
            }
            ++id;
            if(id == _capacity) {
                id = 0;
            }
        }
    }

    inline void remove(const KeyT& key) {
        u32 keyHash = hashFunction(key);
        return removeh(keyHash);
    }

    ValueT* geth(u32 keyHash) const {
        u32 id = keyHash % _capacity;
        u32 jumps = 0;
        while(jumps++ < jumpLimit) {
            if(_occupied[id] && _keyHash[id] == keyHash) {
                return &_value[id];
            }
            ++id;
            if(id == _capacity) {
                id = 0;
            }
        }
        return nullptr;
    }

    inline ValueT* get(const KeyT& key) const {
        u32 keyHash = hashFunction(key);
        return geth(keyHash);
    }

    // Warning: hashmap[key] = val -> double construct, one destroy
    // use set() instead
    // TODO: fix?
    inline ValueT& operator[](const KeyT& key) {
        ValueT* val = get(key);
        if(val) return *val;
        return set(key, ValueT());
    }

    // TODO: iteration
};

#define H(str) const_strHash32_fnv1a(str)

/**
 * String HashMap
 *	- prefer using geth(H("MyString")) for compile-time hash
 */
template<typename ValueT, u32 stackCount = 1>
struct StrHashMap: HashMap<const char*, ValueT, stackCount>
{
    StrHashMap(): HashMap() {
        hashFunction = [](auto key) -> u32 {
            return hash32_fnv1a(key, strLen(key));
        };
    }
};

// T is a SparseArray
template<typename T>
struct Ref
{
    u32 _id = 0;
    T* _from = nullptr;

    Ref() = default;

    inline auto& get() {
        assert(_from);
        return _from->get(_id);
    }

    inline auto* operator->() {
        assert(_from);
        return &_from->get(_id);
    }

    inline const auto* operator->() const {
        assert(_from);
        return &_from->get(_id);
    }

    inline void clear() {
        _id = 0;
        _from = nullptr;
    }

    inline bool valid() {
        return _from != nullptr;
    }
};

/**
 * Dynamic sparse array
 * - Grows dynamically
 * - Packed data
 * - No pointer invalidation (kind of)
 * - O(1) push (when not expanding), remove, iteration
 *
 * note: doesn't play well with AllocatorStack (nothing will get deallocated)
 */
template<typename T, u32 stackCount = 1>
struct SparseArray
{
    typedef Ref<SparseArray<T,stackCount>> RefT;

    // FreeList is saved in-place, where _ref is empty (negative i64s)
    struct FreeList {
        intptr_t _next = 0;
        inline FreeList* next() const {
            return (FreeList*)-_next;
        }
    };

    IAllocator* allocator = mem_context.allocator;
    FreeList* _freeListTop = nullptr;

    T _stack_data[stackCount] alignas(T);
    i64 _stack_refId[stackCount] alignas(i64); // TODO: make this i32
    i64* _stack_refIdPtr[stackCount] alignas(i64*);

    T* _data = _stack_data;
    i64* _refId = _stack_refId;
    i64** _refIdPtr = _stack_refIdPtr;

    MemBlock _block;

    u32 _capacity = stackCount;
    u32 _count = 0;
    f32 _growth = 2.f;

    SparseArray() {
        _freeListTop = nullptr;
        for(u32 i = 0; i < _capacity; ++i) {
            FreeList* cur = (FreeList*)&_refId[i];
            cur->_next = -(intptr_t)_freeListTop;
            _freeListTop = cur;
        }
    }

    ~SparseArray() {
        destroy();
    }

    SparseArray(const SparseArray& other) = delete;
    SparseArray& operator=(const SparseArray& other) = delete;

    inline void destroy() {
        if(!allocator) return;
        for(u32 i = 0; i < _count; ++i) {
            _data[i].~T();
        }
        _block.dealloc();
        _block = NULL_MEMBLOCK;
        allocator = nullptr;
    }

    void clear() {
        for(u32 i = 0; i < _count; ++i) {
            _data[i].~T();
        }
        _count = 0;

        _freeListTop = nullptr;
        for(u32 i = 0; i < _capacity; ++i) {
            FreeList* cur = (FreeList*)&_refId[i];
            cur->_next = -(intptr_t)_freeListTop;
            _freeListTop = cur;
        }
    }

    void reserve(u32 newCapacity) {
        if(newCapacity <= _capacity) return;

        MemBlock block = allocator->alloc(newCapacity * (sizeof(T) + sizeof(i64) + sizeof(i64*)) + 128,
                                          alignof(T));
        AllocatorStack stack;
        stack.init(block);

        MemBlock dataBlock = stack.alloc(newCapacity * sizeof(T), alignof(T));
        assert_msg(dataBlock.ptr, "SparseArray::push() out of memory");
        MemBlock refBlock = stack.alloc(newCapacity * sizeof(i64), alignof(i64));
        assert_msg(refBlock.ptr, "SparseArray::push() out of memory");
        MemBlock refPtrBlock = stack.alloc(newCapacity * sizeof(i64*), alignof(i64*));
        assert_msg(refPtrBlock.ptr, "SparseArray::push() out of memory");

        T* oldData = _data;
        intptr_t oldRef = (intptr_t)_refId;
        _data = (T*)dataBlock.ptr;

        // copy data
        for(i32 i = 0; i < _count; ++i) {
            new(_data + i) T(oldData[i]); // copy data over
            oldData[i].~T(); // destruct old data
        }

        memmove(refBlock.ptr, _refId, _capacity * sizeof(i64));
        memmove(refPtrBlock.ptr, _refIdPtr, _capacity * sizeof(i64*));
        _refId = (i64*)refBlock.ptr;
        _refIdPtr = (i64**)refPtrBlock.ptr;

        // fix pointers offsets
        for(u32 i = 0; i < _count; ++i) {
            _refIdPtr[i] = (i64*)((intptr_t)_refIdPtr[i] - oldRef + (intptr_t)_refId);
        }
        // make refIds -1
        for(u32 i = _capacity; i < newCapacity; ++i) {
            _refId[i] = -1;
        }

        _capacity = newCapacity;

        // remake freelist
        _freeListTop = nullptr;
        for(u32 i = 0; i < _capacity; ++i) {
            if(_refId[i] < 0) {
                FreeList* cur = (FreeList*)&_refId[i];
                cur->_next = -(intptr_t)_freeListTop;
                _freeListTop = cur;
            }
        }

        _block.dealloc();
        _block = block;
    }

    RefT push(const T& elt) {
        assert_msg(_capacity > 0, "Array not initialized"); // init array first
        // expand
        if(_count >= _capacity) {
            reserve(_capacity * _growth);
        }

        u32 did = _count++;
        // _data is not initialized (constructed), assignement operator doesn't copy vtable
        // so use placement new and tries to move it
        new(_data + did) T((T&&)elt);

        // should never happen
        assert_msg(_freeListTop, "SparseArray::push() couldn't find a free redirect slot");

        // pop from freelist
        u32 freeSlotId = ((intptr_t)_freeListTop - (intptr_t)_refId) / sizeof(i64);
        _freeListTop = _freeListTop->next();
        _refId[freeSlotId] = did;
        _refIdPtr[did] = &_refId[freeSlotId];

        RefT ref;
        ref._id = freeSlotId;
        ref._from = this;
        return ref;
    }

    void remove(const T& elt) {
        // shrink
        // TODO: shrink

        const T* eltPtr = &elt;
        assert_msg((char*)eltPtr >= (char*)_data && (char*)eltPtr < ((char*)_data + _count * sizeof(T)),
                   "SparseArray::remove() ptr out of bounds");
        u32 did = eltPtr - _data;

        i64& oldref = *_refIdPtr[did];

        // swap to last
        _data[did].~T();
        _data[did] = _data[_count-1];
        _refIdPtr[did] = _refIdPtr[_count-1];
        *_refIdPtr[did] = did;
        --_count;

        // push to free list
        oldref = -1;
        FreeList* newNext = (FreeList*)&oldref;
        newNext->_next = -(intptr_t)_freeListTop;
        _freeListTop = newNext;
    }

    inline void remove(RefT& ref) {
        assert(ref._id < _capacity && _refId[ref._id] >= 0 && _refId[ref._id] < _count);
        remove(get(ref._id));
        ref.clear();
    }

    inline void removeByRefId(u32 refId) {
        assert(refId < _capacity && _refId[refId] >= 0 && _refId[refId] < _count);
        remove(get(refId));
    }

    inline Iter<T> begin() const {
        return Iter<T>(_data);
    }

    inline Iter<T> end() const {
        return Iter<T>(&_data[_count]);
    }

    inline u32 capacity() const {
        return _capacity;
    }

    inline u32 count() const {
        return _count;
    }

    inline T* data() const {
        return _data;
    }

    inline T& data(u32 did) {
        assert(did < _count);
        return _data[did];
    }

    inline const T& data(u32 did) const {
        assert(did < _count);
        return _data[did];
    }

    inline T& get(u32 refIndex) {
        assert(refIndex < _capacity && _refId[refIndex] >= 0 && _refId[refIndex] < _count);
        return _data[_refId[refIndex]];
    }

    inline const T& get(u32 refIndex) const {
        assert(refIndex < _capacity && _refId[refIndex] >= 0 && _refId[refIndex] < _count);
        return _data[_refId[refIndex]];
    }

    inline RefT getRef(const T& elt) {
        const T* eltPtr = &elt;
        assert_msg((char*)eltPtr >= (char*)_data && (char*)eltPtr < ((char*)_data + _count * sizeof(T)),
                   "SparseArray::getRef() ptr out of bounds");
        u32 did = eltPtr - _data;
        RefT ref;
        ref._id = (u32)(_refIdPtr[did] - _refId);
        ref._from = this;
        return ref;
    }
};

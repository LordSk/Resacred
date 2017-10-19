#pragma once
#include "rs_base.h"
#include <intrin.h>
#include <functional>

typedef volatile long int vli32;

struct MutexSpin
{
    vli32 _inUse = 0;

    void lock() {
        while(_InterlockedExchange(&_inUse, 1) == 1) {
            _mm_pause();
        }
    }

    void unlock() {
        _InterlockedExchange(&_inUse, 0);
    }
};

struct AtomicCounter
{
    vli32 _count = 0;

    inline void increment() {
        _InterlockedIncrement(&_count);
    }

    inline void decrement() {
        _InterlockedDecrement(&_count);
    }

    inline vli32 get() const {
        return _count;
    }
};


typedef unsigned long(*ThreadFuncType)(void*);
void* threadCreate(ThreadFuncType func, void* pUserData);
void threadClose(void* thread);
void threadWaitForClose(void** threads, i32 count = 1);
void threadSleep(i32 milliseconds);
i32 threadGetId();

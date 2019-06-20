#pragma once
#include "rs_base.h"
#include <intrin.h>
#include <functional>
#include <stdio.h>

typedef volatile long int vli32;

/*#define lock() __lock(__FILE__, __LINE__)
#define unlock() __unlock(__FILE__, __LINE__)*/

struct MutexSpin
{
    vli32 _inUse = 0;

	void lock(/*const char* filename, i32 line*/) {
		//printf("{{{ lock %s:%d\n", filename, line); fflush(stdout);
        while(_InterlockedExchange(&_inUse, 1) == 1) {
            _mm_pause();
        }
    }

	void unlock(/*const char* filename, i32 line*/) {
        _InterlockedExchange(&_inUse, 0);
		//printf("}}} unlock %s:%d\n", filename, line); fflush(stdout);
    }
};

struct Mutex
{
#ifdef CONF_WINDOWS
    CRITICAL_SECTION criticalSection;

    Mutex() {
        InitializeCriticalSection(&criticalSection);
    }

	void lock(/*const char* filename, i32 line*/) {
		//printf("{{{ lock %s:%d\n", filename, line); fflush(stdout);
        EnterCriticalSection(&criticalSection);
    }

	void unlock(/*const char* filename, i32 line*/) {
        LeaveCriticalSection(&criticalSection);
		//printf("}}} unlock %s:%d\n", filename, line); fflush(stdout);
    }
#endif
};

struct AtomicCounter
{
    vli32 _count = 0;

    inline i32 increment() {
        _InterlockedIncrement(&_count);
        return (i32)_count;
    }

    inline i32 decrement() {
        _InterlockedDecrement(&_count);
        return (i32)_count;
    }

    inline void set(i32 val) {
        _InterlockedExchange(&_count, val);
    }

	inline void reset() {
		_InterlockedExchange(&_count, 0);
	}

    inline vli32 get() const {
        return _count;
    }
};

struct ConditionVariable
{
#ifdef CONF_WINDOWS
	CONDITION_VARIABLE var;
#endif

	ConditionVariable() {
#ifdef CONF_WINDOWS
		InitializeConditionVariable(&var);
#endif
	}

	inline void wakeOne() {
#ifdef CONF_WINDOWS
		WakeConditionVariable(&var);
#endif
	}

	inline void wakeAll() {
#ifdef CONF_WINDOWS
		WakeAllConditionVariable(&var);
#endif
	}

	inline void SleepUntilWokenUp(Mutex* pMutex) {
#ifdef CONF_WINDOWS
		SleepConditionVariableCS(&var, &pMutex->criticalSection, INFINITE);
#endif
	}

	inline void SleepUntilWokenUpTime(Mutex* pMutex, u64 milliseconds) {
#ifdef CONF_WINDOWS
		SleepConditionVariableCS(&var, &pMutex->criticalSection, milliseconds);
#endif
	}
};

#ifdef CONF_WINDOWS
typedef HANDLE ThreadHandle;
#endif

typedef unsigned long(*ThreadFuncType)(void*);
ThreadHandle threadCreate(ThreadFuncType func, void* pUserData);
void threadClose(ThreadHandle thread);
void threadWaitForClose(ThreadHandle* threads, i32 count = 1);
void threadSleep(i32 milliseconds);
i32 threadGetId();
i32 threadGetLogicalProcessorCount();
void threadSetProcessorAffinity(ThreadHandle threadHandle, i32 procId);

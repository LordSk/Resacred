#include "rs_thread.h"
#include "rs_logger.h"
#include <thread>

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#endif

ThreadHandle threadCreate(ThreadFuncType func, void* pUserData)
{
#ifdef _WIN32
    return CreateThread(0, 0, (LPTHREAD_START_ROUTINE)func, pUserData, 0, nullptr);
#endif
}

void threadClose(ThreadHandle thread)
{
#ifdef _WIN32
    TerminateThread(thread, 0);
#endif
}

void threadWaitForClose(ThreadHandle *threads, i32 count)
{
#ifdef _WIN32
    WaitForMultipleObjects(count, threads, true, INFINITE);
#endif
}

void threadSleep(i32 milliseconds)
{
#ifdef _WIN32
    Sleep(milliseconds);
#endif
}

i32 threadGetId()
{
#ifdef _WIN32
    return GetCurrentThreadId();
#endif
}

i32 threadGetLogicalProcessorCount()
{
    return std::thread::hardware_concurrency();
}

void threadSetProcessorAffinity(ThreadHandle threadHandle, i32 procId)
{
#ifdef _WIN32
    DWORD_PTR mask = 1 << procId;
    SetThreadAffinityMask((HANDLE)threadHandle, mask);
#endif
}

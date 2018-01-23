#include "rs_thread.h"
#include "rs_logger.h"
#include <thread>

#ifdef _WIN32
    #include <windows.h>
#endif


void* threadCreate(ThreadFuncType func, void* pUserData)
{
#ifdef _WIN32
    return CreateThread(0, 0, (LPTHREAD_START_ROUTINE)func, pUserData, 0, nullptr);
#endif
}

void threadClose(void* thread)
{
#ifdef _WIN32
    TerminateThread(thread, 0);
#endif
}

void threadWaitForClose(void** threads, i32 count)
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

void threadSetProcessorAffinity(void* threadHandle, i32 procId)
{
#ifdef _WIN32
    DWORD_PTR mask = 1 << procId;
    SetThreadAffinityMask((HANDLE)threadHandle, mask);
#endif
}

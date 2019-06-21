#pragma once
#include "rs_thread.h"

typedef void (*JobFunc)(void*);
struct JobSystem;

void jobSystemInit(i32 workerCount);
void jobSystemShutdown();

i32 jobRun(void* pUserData, JobFunc func, AtomicCounter* pFinishSignal);
// will deallocate passed data on completion
i32 jobRunEx(void* pUserData, i32 sizeOfData, JobFunc func, AtomicCounter* pFinishSignal);
void jobWait(AtomicCounter* pFinishSignal);

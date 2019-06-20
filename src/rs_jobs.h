#pragma once
#include "rs_thread.h"

typedef void (*JobFunc)(void*);
struct JobSystem;

void jobSystemInit(i32 workerCount);
void jobSystemShutdown();

i32 jobRun(void* pUserData, JobFunc func, AtomicCounter* pFinishSignal);
void jobWait(AtomicCounter* pFinishSignal);

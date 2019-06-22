#include "rs_jobs.h"
#include "rs_array.h"
#include <stdio.h>
#include <Tracy.hpp>
#include <common/TracySystem.hpp>

struct Job
{
	i32 id;
	JobFunc func;
	AtomicCounter* pSignal;
	void* pUserData;
	MemBlock dataBlock;
};

struct JobSystem
{
	void* threads[64];
	i32 workerCount = 0;
	i32 nextJobId = 0;
	TracyLockable(Mutex, queueMutex);
	TracyLockable(Mutex, tempAllocMutex);
	Array<Job, 1024> queue;
	ConditionVariable queueCv;
	AllocatorBucket tempAlloc;
	bool running = true;
};

JobSystem* g_pJobSystem = nullptr;

struct WorkerThreadData
{
	i32 id;
};

unsigned long workerThread(void* pData)
{
	const WorkerThreadData wtd = *(WorkerThreadData*)pData;
	const i32 tid = threadGetId();
	JobSystem& js = *g_pJobSystem;

	while(js.running) {
		js.queueMutex.lock();
		//LOG_THREAD("wid=%d tid=%x SLEEPING", wtd.id, tid);
		js.queueCv.SleepUntilWokenUp((Mutex*)&js.queueMutex);
		//LOG_THREAD("wid=%d tid=%x WOKEN UP", wtd.id, tid);

		if(!js.running) {
			js.queueMutex.unlock();
			return 0;
		}

		while(js.queue.count() > 0) {
			// pop last
			Job task = *js.queue.last();
			const i32 newCount = --js.queue._count;
			js.queueMutex.unlock();
			if(newCount > 0) {
				js.queueCv.wakeOne(); // there is job to do, wake another worker thread
			}

			//LOG_THREAD("wid=%d tid=%x jobId=%d> Job start...", wtd.id, tid, task.id);

			{
				ProfileBlock("Task exec");
				task.func(task.pUserData);
			}

			if(task.pSignal) {
				task.pSignal->decrement();
			}

			if(task.dataBlock.isValid()) {
				js.tempAllocMutex.lock();
				MEM_DEALLOC(task.dataBlock);
				js.tempAllocMutex.unlock();
			}

			//LOG_THREAD("wid=%d tid=%x jobId=%d> Job done. queue=%d", wtd.id, tid, task.id, js.queue.count());

			if(js.queue.count() > 0) {
				js.queueMutex.lock();
			}
		}

		js.queueMutex.unlock();
	}

	return 0;
}

void jobSystemInit(i32 workerCount)
{
	static JobSystem js = {};
	g_pJobSystem = &js;

	js.workerCount = workerCount;
	const i32 procCount = threadGetLogicalProcessorCount();

	static WorkerThreadData wtds[64];

	for(i32 i = 0; i < workerCount; i++) {
		wtds[i] = { i };
		js.threads[i] = threadCreate(workerThread, &wtds[i]);
		threadSetProcessorAffinity(js.threads[i], procCount-1-i);

		wchar_t nameBuff[256];
		_snwprintf(nameBuff, arr_count(nameBuff), L"WorkerThread %d", i);
		SetThreadDescription(
			js.threads[i],
			nameBuff
		);

#ifdef _WIN32
#pragma pack(push, 8)
		struct ThreadName
		{
			DWORD  type;
			LPCSTR name;
			DWORD  id;
			DWORD  flags;
		};
#pragma pack(pop)
		char nameBuff2[256];
		_snprintf(nameBuff2, arr_count(nameBuff2), "WorkerThread %d", i);
		ThreadName tn;
		tn.type  = 0x1000;
		tn.name  = nameBuff2;
		tn.id    = GetThreadId(js.threads[i]);
		tn.flags = 0;

		__try
		{
			RaiseException(0x406d1388
					, 0
					, sizeof(tn)/4
					, reinterpret_cast<ULONG_PTR*>(&tn)
					);
		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
		}
#endif

		tracy::SetThreadName(std::thread::native_handle_type(js.threads[i]), nameBuff2);
	}

	js.tempAlloc.init(MEM_MALLOCATOR.ALLOC(Megabyte(10)), 1024);
}

void jobSystemShutdown()
{
	g_pJobSystem->running = false;
	g_pJobSystem->queueCv.wakeAll();
	threadWaitForCloseTimeout(g_pJobSystem->threads, g_pJobSystem->workerCount, 1000); // give the workers a second to close
	for(i32 i = 0; i < g_pJobSystem->workerCount; i++) {
		threadClose(g_pJobSystem->threads[i]);
	}
	g_pJobSystem->tempAlloc.release();
}

i32 jobRun(void *pUserData, JobFunc func, AtomicCounter *pFinishSignal)
{
	return jobRunEx(pUserData, 0, func, pFinishSignal);
}

i32 jobRunEx(void *pUserData, i32 sizeOfData, JobFunc func, AtomicCounter *pFinishSignal)
{
	assert(g_pJobSystem);
	if(pFinishSignal) {
		pFinishSignal->increment();
	}

	Job task;
	task.id = g_pJobSystem->nextJobId++;
	task.func = func;
	task.pSignal = pFinishSignal;

	if(sizeOfData > 0) {
		g_pJobSystem->tempAllocMutex.lock();
		task.dataBlock = g_pJobSystem->tempAlloc.ALLOC(sizeOfData);
		g_pJobSystem->tempAllocMutex.unlock();

		assert(task.dataBlock.isValid());
		task.pUserData = task.dataBlock.ptr;
		memmove(task.pUserData, pUserData, sizeOfData);
	}
	else {
		task.pUserData = pUserData;
		task.dataBlock = NULL_MEMBLOCK;
	}

	g_pJobSystem->queueMutex.lock();
	g_pJobSystem->queue.pushPOD(&task);
	g_pJobSystem->queueMutex.unlock();
	g_pJobSystem->queueCv.wakeOne();
	return task.id;
}

void jobWait(AtomicCounter *pFinishSignal)
{
	while(pFinishSignal->get() != 0) {
		_mm_pause();
	}
}

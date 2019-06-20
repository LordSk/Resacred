#include "rs_jobs.h"
#include "rs_array.h"

struct Job
{
	i32 id;
	JobFunc func;
	AtomicCounter* pSignal;
	void* pUserData;
};

struct JobSystem
{
	void* threads[64];
	i32 workerCount = 0;
	i32 nextJobId = 0;
	Mutex queueMutex;
	Array<Job, 1024> queue;
	ConditionVariable queueCv;
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
		js.queueCv.SleepUntilWokenUp(&js.queueMutex);

		if(js.queue.count() > 0) {
			// pop last
			Job task = *js.queue.last();
			js.queue._count--;
			js.queueMutex.unlock();
			js.queueCv.wakeOne();

			LOG("[JobSystem] wid=%d tid=%x jobId=%d", wtd.id, tid, task.id);

			task.func(task.pUserData);
			task.pSignal->decrement();

			LOG_SUCC("[JobSystem] wid=%d tid=%x jobId=%d JOB DONE", wtd.id, tid, task.id);
		}
		else {
			js.queueMutex.unlock();
		}
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
	}
}

void jobSystemShutdown()
{
	g_pJobSystem->running = false;
	threadSleep(1000); // give the workers a second to close
	for(i32 i = 0; i < g_pJobSystem->workerCount; i++) {
		threadClose(g_pJobSystem->threads[i]);
	}
}

i32 jobRun(void *pUserData, JobFunc func, AtomicCounter *pFinishSignal)
{
	assert(g_pJobSystem);
	pFinishSignal->increment();

	Job task;
	task.id = g_pJobSystem->nextJobId++;
	task.func = func;
	task.pUserData = pUserData;
	task.pSignal = pFinishSignal;

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

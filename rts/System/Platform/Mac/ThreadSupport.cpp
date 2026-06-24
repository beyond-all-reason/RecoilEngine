/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#include "System/Platform/Threading.h"

#include <memory>
#include <pthread/qos.h>

namespace Threading {

void SetupCurrentThreadControls(std::shared_ptr<ThreadControls>& threadCtls)
{
	threadCtls.reset(new Threading::ThreadControls());
	threadCtls->handle = pthread_self();

	// macOS has no pthread_setaffinity_np equivalent. Scheduling locality is
	// expressed with QoS hints instead; this asks Apple Silicon to prefer the
	// performance cluster for worker threads that enter through ThreadStart.
	pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0);
}

void ThreadStart(
	std::function<void()> taskFunc,
	std::shared_ptr<ThreadControls>* threadCtls,
	ThreadControls* tempCtls
) {
	if (threadCtls != nullptr)
		SetupCurrentThreadControls(*threadCtls);

	{
		std::lock_guard<spring::mutex> lock(tempCtls->mutSuspend);
		tempCtls->condInitialized.notify_one();
	}

	taskFunc();
}

SuspendResult ThreadControls::Suspend()
{
	return Threading::THREADERR_NOT_RUNNING;
}

SuspendResult ThreadControls::Resume()
{
	return Threading::THREADERR_NONE;
}

} // namespace Threading

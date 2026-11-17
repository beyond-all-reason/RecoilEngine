#include "StreamBuffer.h"

#include "VBO.h"

#include <algorithm>
#include <cstdlib>

#include "System/Log/ILog.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GlobalRenderingInfo.h"

#include "System/Misc/TracyDefs.h"


//////////////////////////////////////////////////////////////////////


// To make sure that you don't stomp
// all over data that hasn't been used yet, you can insert a fence right after the
// last command that might read from a buffer, and then issue a call to
// glClientWaitSync() right before you write into the buffer.
void IStreamBufferConcept::PutBufferLocks()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (lockList.empty())
		return;

	for (auto* fence : lockList)
		PutBufferLock(*fence);
	lockList.clear();
}

void IStreamBufferConcept::PutBufferLock(BufferFence& fence)
{
	const GLsync sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	if (sync == nullptr) {
		LOG_L(L_FATAL, "[IStreamBuffer::PutBufferLock] Failed to fence GPU buffer use");
		std::abort();
	}
	if (fence.sync != nullptr)
		glDeleteSync(fence.sync);
	fence.sync = sync;
	fence.queued = false;
}

void IStreamBufferConcept::CancelBufferLock(BufferFence& fence)
{
	if (!fence.queued)
		return;
	std::erase(lockList, &fence);
	fence.queued = false;
}

IStreamBufferConcept::IStreamBufferConcept(StreamBufferCreationParams p, std::string_view bufferTypeName)
	: name{ p.name }
	, target{ p.target }
	, id{ 0 }
	, numElements { p.numElems }
	, byteSize{ 0 }
	, allocIdx{ 0 }
	, mapElemOffet{ 0 }
	, mapElemCount{ 0 }
	, optimizeForStreaming{ p.optimizeForStreaming }
{
	if (reportType)
		LOG_L(L_INFO, "[StreamBuffer::%s] Created StreamBuffer name %s type %s", __func__, name.c_str(), bufferTypeName.data());
}

IStreamBufferConcept::~IStreamBufferConcept()
{
	// Derived Kill has already waited for the GPU. No reservation may outlive
	// its host storage, even when initialization failed or no frame was swapped.
	ClearBufferFences();
}

void IStreamBufferConcept::InitBufferFences(uint32_t count)
{
	assert(fences.empty());
	assert(count > 0);
	fences.resize(count);
	allocIdx = 0;
	boundIdx = INVALID_BUFFER_IDX;
	// Fresh storage has no GPU reader; fences are queued only when used.
}

void IStreamBufferConcept::ClearBufferFences()
{
	for (auto& fence : fences) {
		CancelBufferLock(fence);
		assert(fence.sync == nullptr);
	}
	fences.clear();
	allocIdx = 0;
	boundIdx = INVALID_BUFFER_IDX;
}

void IStreamBufferConcept::QueueLockBuffer(BufferFence& fence) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (fence.queued)
		return;
	lockList.emplace_back(&fence);
	fence.queued = true;
}

void IStreamBufferConcept::WaitBuffer(BufferFence& fence) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (fence.queued) {
		// A slot can wrap or be destroyed before the frame-end fence pass.
		// Cover its latest issued reads before granting the CPU write access.
		CancelBufferLock(fence);
		PutBufferLock(fence);
	}
	if (fence.sync == nullptr)
		return;

	uint32_t gWaitCount = 0;
	while (true) {
		GLenum waitReturn = glClientWaitSync(fence.sync, GL_SYNC_FLUSH_COMMANDS_BIT, 1);
		if (waitReturn == GL_ALREADY_SIGNALED || waitReturn == GL_CONDITION_SATISFIED)
			break;

		if (waitReturn != GL_TIMEOUT_EXPIRED) {
			// Completion is unknown. Never delete this fence and let Map reuse
			// GPU-owned storage, and do not throw through buffer destructors.
			LOG_L(L_FATAL, "[IStreamBuffer::WaitBuffer] Fence wait failed (0x%x) for buffer %u (%s)", waitReturn, id, name.c_str());
			std::abort();
		}
		gWaitCount++;
	}
	glDeleteSync(fence.sync);
	fence.sync = nullptr;

	if (gWaitCount > 0)
		LOG_L(L_DEBUG, "[IStreamBuffer::WaitBuffer] Detected non-zero (%u) wait spins on stream buffer (%u, %s). Consider increasing numBuffers", gWaitCount, id, name.c_str());
}

void IStreamBufferConcept::CreateBuffer(uint32_t byteBufferSize, uint32_t newUsage)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (id == 0)
		glGenBuffers(1, &id);

	Bind();
	glBufferData(target, byteBufferSize, nullptr, newUsage);
	Unbind();

	assert(glIsBuffer(id));
}

void IStreamBufferConcept::CreateBufferStorage(uint32_t byteBufferSize, uint32_t flags)
{
	RECOIL_DETAILED_TRACY_ZONE;
	glGenBuffers(1, &id);

	Bind();
	glBufferStorage(target, byteBufferSize, nullptr, flags);
	Unbind();

	assert(glIsBuffer(id));
}

void IStreamBufferConcept::DeleteBuffer()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (glIsBuffer(id))
		glDeleteBuffers(1, &id);

	id = 0;
}

uint32_t IStreamBufferConcept::GetAlignedByteSize(uint32_t byteSizeRaw)
{
	RECOIL_DETAILED_TRACY_ZONE;
	return VBO::GetAlignedSize(target, byteSizeRaw);
}

void IStreamBufferConcept::Bind(uint32_t bindTarget) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	glBindBuffer(bindTarget > 0 ? bindTarget : target, id);
}

void  IStreamBufferConcept::Unbind(uint32_t bindTarget) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	glBindBuffer(bindTarget > 0 ? bindTarget : target, 0);
}

void IStreamBufferConcept::BindBufferRange(GLuint index, uint32_t bindTarget) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	glBindBufferRange(bindTarget > 0 ? bindTarget : this->target, index, id, allocIdx * this->byteSize, this->byteSize);
	boundIdx = allocIdx;
	MarkBoundBufferUsed();
}

void IStreamBufferConcept::UnbindBufferRange(GLuint index, uint32_t bindTarget) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	glBindBufferRange(bindTarget > 0 ? bindTarget : this->target, index, 0u, allocIdx * this->byteSize, this->byteSize);
	boundIdx = INVALID_BUFFER_IDX;
}

void IStreamBufferConcept::MarkBoundBufferUsed() const
{
	if (boundIdx < fences.size())
		QueueLockBuffer(fences[boundIdx]);
}

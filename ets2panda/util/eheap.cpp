/**
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "util/eheap.h"
#include "libarkbase/mem/pool_manager.h"
#include "util/es2pandaMacros.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace ark::es2panda {

void ScopedAllocatorsManager::Initialize()
{
    constexpr size_t ARENA_POOL_SIZE = 1_GB;

    mem::MemConfig::Initialize(0, 0, ARENA_POOL_SIZE, 0, 0, 0);
    PoolManager::Initialize(PoolType::MALLOC);
}

void ScopedAllocatorsManager::Finalize()
{
    PoolManager::Finalize();
    mem::MemConfig::Finalize();
}

bool ScopedAllocatorsManager::IsInitialized()
{
    return mem::MemConfig::IsInitialized();
}

EHeap::EHeapSpace *EHeap::gEHeapSpace {};

void EHeap::InitializeEHeapSpace()
{
    ES2PANDA_ASSERT(gEHeapSpace == nullptr);

    constexpr size_t EHEAP_SIZE = sizeof(void *) <= 4U ? 1_GB : 32_GB;

    // pointer compression
    static_assert(EHEAP_SIZE <= (4_GB * EHeap::EHeapSpace::ALLOC_ALIGNMENT));

    gEHeapSpace = new EHeapSpace(EHEAP_SIZE);

    // consume the "nullptr" so the pointer compression may re-use it
    [[maybe_unused]] void *unused = Alloc(1);
}

void EHeap::FinalizeEHeapSpace()
{
    delete gEHeapSpace;
    gEHeapSpace = nullptr;
}

[[noreturn]] __attribute__((noinline)) void EHeap::OOMAction()
{
    std::cerr << "es2panda heap is out of memory, aborting" << std::endl;
    std::abort();
}

#ifdef _WIN32

[[noreturn]] static void MappingFailureAction(char const *msg)
{
    std::cerr << msg << ", code=" << GetLastError() << std::endl;
    std::abort();
}

static void *ReserveMapping(size_t size)
{
    void *mem = VirtualAlloc(nullptr, size, MEM_RESERVE, PAGE_READWRITE);
    if (mem == nullptr) {
        MappingFailureAction("Failed to reserve heap mapping");
    }
    return mem;
}

static void CommitMapping(void *mem, size_t size)
{
    if (UNLIKELY(VirtualAlloc(mem, size, MEM_COMMIT, PAGE_READWRITE) == nullptr)) {
        MappingFailureAction("Failed to commit heap mapping");
    }
}

static void ReleaseMapping(void *mem, size_t size)
{
    (void)size;
    [[maybe_unused]] auto res = VirtualFree(mem, 0, MEM_RELEASE);
    ES2PANDA_ASSERT(res != 0);
}

#else

[[noreturn]] static void MappingFailureAction(char const *msg)
{
    perror(msg);
    std::abort();
}

static void *ReserveMapping(size_t size)
{
    void *mem = mmap(nullptr, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (mem == MAP_FAILED) {
        MappingFailureAction("Failed to reserve heap mapping");
    }
    return mem;
}

static void CommitMapping(void *mem, size_t size)
{
    if (UNLIKELY(mprotect(mem, size, PROT_READ | PROT_WRITE) < 0)) {
        MappingFailureAction("Failed to commit heap mapping");
    }
}

static void ReleaseMapping(void *mem, size_t size)
{
    [[maybe_unused]] int res = munmap(mem, size);
    ES2PANDA_ASSERT(res == 0);
}

#endif

EHeap::EHeapSpace::EHeapSpace(size_t size) : bufferSize_(size)
{
    buffer_ = ReserveMapping(bufferSize_);
    current_ = ToUintPtr(buffer_);
    top_ = current_ + bufferSize_;
    currentCommitted_ = current_;

    ES2PANDA_ASSERT(IsAligned(current_, ALLOC_ALIGNMENT));
}

EHeap::EHeapSpace::~EHeapSpace()
{
    if (currentCommitted_ > ToUintPtr(buffer_)) {
        ASAN_UNPOISON_MEMORY_REGION(buffer_, currentCommitted_ - ToUintPtr(buffer_));
    }
    ReleaseMapping(buffer_, bufferSize_);
}

[[nodiscard]] __attribute__((returns_nonnull)) void *EHeap::EHeapSpace::Alloc(size_t sz)
{
    constexpr size_t COMMIT_GRANNULARITY = 256_KB;
    static_assert(COMMIT_GRANNULARITY >= PAGE_SIZE);

    uintptr_t updCurrent = current_ + AlignUp(sz, ALLOC_ALIGNMENT);
    if (UNLIKELY(updCurrent > currentCommitted_)) {
        if (UNLIKELY(updCurrent > top_)) {
            OOMAction();
            ES2PANDA_UNREACHABLE();
        }
        uintptr_t updComitted = std::min(RoundUp(updCurrent, COMMIT_GRANNULARITY), top_);
        size_t commitSize = updComitted - currentCommitted_;
        CommitMapping(ToVoidPtr(currentCommitted_), commitSize);
        ASAN_POISON_MEMORY_REGION(ToVoidPtr(currentCommitted_), commitSize);
        currentCommitted_ = updComitted;
    }
    uintptr_t res = current_;
    current_ = updCurrent;
    ASAN_UNPOISON_MEMORY_REGION(ToVoidPtr(res), sz);
    return ToVoidPtr(res);
}

void EHeap::BrokenEHeapPointerAction(void const *ptr)
{
    std::cerr << "Broken es2panda heap pointer: " << ptr << std::endl;
    ES2PANDA_UNREACHABLE();
}

size_t EHeap::AllocatedSize()
{
    return gEHeapSpace == nullptr ? 0 : gEHeapSpace->AllocatedSize();
}

size_t EHeap::FreedSize()
{
    return gEHeapSpace == nullptr ? 0 : gEHeapSpace->FreedSize();
}

// #32069 - there should be a single scope created within the single es2panda context
static long g_eheapScopeCount = 0;

EHeap::Scope::Scope()
{
    if (g_eheapScopeCount++ == 0) {
        InitializeEHeapSpace();
    } else {
        ES2PANDA_ASSERT(IsEHeapInitialized());
    }
}

EHeap::Scope::~Scope()
{
    if (--g_eheapScopeCount == 0) {
        FinalizeEHeapSpace();
    } else {
        ES2PANDA_ASSERT(IsEHeapInitialized());
    }
}

}  // namespace ark::es2panda

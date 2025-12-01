/*
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

EHeap::EHeapSpace *EHeap::gEHeapSpace {};

void EHeap::Initialize()
{
    constexpr size_t ARENA_POOL_SIZE = 2_GB;

    mem::MemConfig::Initialize(0, 0, ARENA_POOL_SIZE, 0, 0, 0);
    PoolManager::Initialize(PoolType::MALLOC);
}

void EHeap::Finalize()
{
    PoolManager::Finalize();
    mem::MemConfig::Finalize();
}

bool EHeap::IsInitialized()
{
    return mem::MemConfig::IsInitialized();
}

void EHeap::InitializeEHeapSpace()
{
    ES2PANDA_ASSERT(gEHeapSpace == nullptr);

    constexpr size_t EHEAP_SIZE = sizeof(void *) <= 4U ? 2_GB : 32_GB;

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

void EHeap::ForceInitializeEHeapSpace()
{
    InitializeEHeapSpace();
}

void EHeap::ForceFinalizeEHeapSpace()
{
    FinalizeEHeapSpace();
}

[[nodiscard]] __attribute__((returns_nonnull)) void *EHeap::Alloc(size_t sz)
{
    return gEHeapSpace->Alloc(sz);
}

[[noreturn]] __attribute__((noinline)) void EHeap::OOMAction()
{
    std::cerr << "es2panda heap is out of memory, aborting" << std::endl;
    ES2PANDA_UNREACHABLE();
}

EHeap::EHeapSpace::EHeapSpace(size_t size) : bufferSize_(size)
{
#ifndef _WIN32
    buffer_ = mmap(nullptr, bufferSize_, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (buffer_ == MAP_FAILED) {
        EHeap::OOMAction();
    }
#else
    buffer_ = VirtualAlloc(nullptr, bufferSize_, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (buffer_ == nullptr) {
        EHeap::OOMAction();
    }
#endif
    current_ = ToUintPtr(buffer_);
    top_ = current_ + bufferSize_;

    ES2PANDA_ASSERT(IsAligned(current_, ALLOC_ALIGNMENT));
}

EHeap::EHeapSpace::~EHeapSpace()
{
#ifndef _WIN32
    [[maybe_unused]] int res = munmap(buffer_, bufferSize_);
    ES2PANDA_ASSERT(res != -1);
#else
    [[maybe_unused]] auto res = VirtualFree(buffer_, 0, MEM_RELEASE);
    ES2PANDA_ASSERT(res != 0);
#endif
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

// while the allocator management in *es2panda_lib* is completely chaotic
// the reasonable way to track the usage of heap is to count the number of active allocators
static size_t gAllocatorCount = 0;

EAllocator::EAllocator()
{
    if (gAllocatorCount++ == 0 && !EHeap::IsEHeapInitialized()) {
        EHeap::InitializeEHeapSpace();
    }
}

EAllocator::~EAllocator()
{
    if (--gAllocatorCount == 0 && EHeap::IsEHeapInitialized()) {
        EHeap::FinalizeEHeapSpace();
    }
}

}  // namespace ark::es2panda

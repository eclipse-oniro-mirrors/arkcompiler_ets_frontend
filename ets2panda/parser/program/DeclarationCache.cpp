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

#include "DeclarationCache.h"
#include <mutex>
#include <shared_mutex>
#include <thread>
#include "util/es2pandaMacros.h"

namespace ark::es2panda::parser {

UniqueSpinMutex::~UniqueSpinMutex()
{
    // Atomic with relaxed order reason: read of field
    ES2PANDA_ASSERT(spin_.load(std::memory_order_relaxed) == LOCK_OFF);
};

// CC-OFFNXT(G.NAM.03-CPP) project code style
void UniqueSpinMutex::lock()
{
    std::int64_t test = LOCK_OFF;
    // Atomic with acquire order reason: other threads should see correct value
    while (!spin_.compare_exchange_weak(test, LOCK_SET, std::memory_order_acquire, std::memory_order_relaxed)) {
        test = LOCK_OFF;
        std::this_thread::yield();
    }
}

// CC-OFFNXT(G.NAM.03-CPP) project code style
void UniqueSpinMutex::unlock()
{
    // Atomic with release order reason: write to field, other threads should see correct value
    spin_.store(LOCK_OFF, std::memory_order_release);
}

// CC-OFFNXT(G.NAM.03-CPP) project code style
bool UniqueSpinMutex::try_lock()
{
    std::int64_t test = LOCK_OFF;
    // Atomic with acquire order reason: other threads should see correct value
    return spin_.compare_exchange_strong(test, LOCK_SET, std::memory_order_acquire, std::memory_order_relaxed);
}

// CC-OFFNXT(G.NAM.03-CPP) project code style
void ReadWriteSpinMutex::lock_shared()
{
    // Atomic with relaxed order reason: read of field
    while (spin_.load(std::memory_order_relaxed) < LOCK_OFF ||
           // Atomic with acquire order reason: other threads should see correct value
           spin_.fetch_add(1, std::memory_order_acquire) < LOCK_OFF) {
        std::this_thread::yield();
    }
}

// CC-OFFNXT(G.NAM.03-CPP) project code style
void ReadWriteSpinMutex::unlock_shared()
{
    // Atomic with release order reason: write to field, other threads should see correct value
    spin_.fetch_sub(1, std::memory_order_release);
}

// CC-OFFNXT(G.NAM.03-CPP) project code style
bool ReadWriteSpinMutex::try_lock_shared()
{
    // Atomic with relaxed order reason: read of field
    return spin_.load(std::memory_order_relaxed) >= LOCK_OFF &&
           // Atomic with acquire order reason: other threads should see correct value
           spin_.fetch_add(1, std::memory_order_acquire) >= LOCK_OFF;
}

DeclarationCache::~DeclarationCache()
{
    ClearAll();
}

DeclarationCache::DeclarationCache([[maybe_unused]] Tag &&tag) {}

//--------------------------------------------------------------------------------------------------------//
//  Creates the new instance of DeclarationCache class (a singleton object)
//--------------------------------------------------------------------------------------------------------//
DeclarationCache &DeclarationCache::Instance()
{
    if (!DeclarationCache::globalDeclarationCache_) {
        DeclarationCache::globalDeclarationCache_ = std::make_unique<DeclarationCache>(DeclarationCache::Tag {});
    }
    return *DeclarationCache::globalDeclarationCache_;
}

void DeclarationCache::ClearAll() noexcept
{
    std::scoped_lock<ReadWriteSpinMutex> lock(dataGuard_);
    declarations_.clear();
}

void DeclarationCache::RemoveDeclaration(std::string const &fileName) noexcept
{
    std::scoped_lock<ReadWriteSpinMutex> lock(dataGuard_);
    declarations_.erase(fileName);
}

DeclarationType DeclarationCache::GetDeclaration(std::string const &fileName) const noexcept
{
    std::shared_lock<ReadWriteSpinMutex> lock(dataGuard_);
    auto const it = declarations_.find(fileName);
    return (it != declarations_.end()) ? it->second : ABSENT;
}

//--------------------------------------------------------------------------------------------------------//
//  Adds specified declaration file to the cache (or replaces the existing one).
//--------------------------------------------------------------------------------------------------------//
DeclarationType DeclarationCache::AddDeclaration(std::string fileName, DeclarationType decl)
{
    std::scoped_lock<ReadWriteSpinMutex> lock(dataGuard_);
    auto [it, _] = declarations_.insert_or_assign(std::move(fileName), std::move(decl));
    return it->second;
}

}  // namespace ark::es2panda::parser

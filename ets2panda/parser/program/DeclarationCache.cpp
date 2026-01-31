/**
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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
#include "util/importPathManager.h"
#include "util/es2pandaMacros.h"

namespace ark::es2panda::parser {

std::shared_mutex DeclarationCache::globalGuard_;

DeclarationCache *DeclarationCache::globalDeclarationCache_;

UniqueSpinMutex::~UniqueSpinMutex()
{
    // Atomic with relaxed order reason: read of field
    ES2PANDA_ASSERT(spin_.load(std::memory_order_relaxed) == LOCK_OFF);
}

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
           spin_.fetch_add(1, std::memory_order_acquire) <= LOCK_OFF) {
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
           spin_.fetch_add(1, std::memory_order_acquire) > LOCK_OFF;
}

DeclarationCache::~DeclarationCache()
{
    ClearAll();
}

constexpr std::size_t INITIAL_CACHE_SIZE = 256U;

DeclarationCache::DeclarationCache()
{
    declarations_.reserve(INITIAL_CACHE_SIZE);
}

//--------------------------------------------------------------------------------------------------------//
//  Creates the new instance of DeclarationCache class (a singleton object)
//--------------------------------------------------------------------------------------------------------//
void DeclarationCache::ActivateCache()
{
    std::scoped_lock<std::shared_mutex> lock(globalGuard_);
    if (DeclarationCache::globalDeclarationCache_ == nullptr) {
        globalDeclarationCache_ = new DeclarationCache();
    }
}

bool DeclarationCache::IsCacheActivated() noexcept
{
    std::shared_lock<std::shared_mutex> lock(globalGuard_);
    return static_cast<bool>(DeclarationCache::globalDeclarationCache_);
}

DeclarationCache *DeclarationCache::Instance() noexcept
{
    std::shared_lock<std::shared_mutex> lock(globalGuard_);
    return DeclarationCache::globalDeclarationCache_;
}

void DeclarationCache::ClearAll() noexcept
{
    if (auto cache = DeclarationCache::Instance(); static_cast<bool>(cache)) {
        cache->Clear();
    }
}

void DeclarationCache::GetFromCache(DeclarationCache::CacheReference *importData) noexcept
{
    ES2PANDA_ASSERT(importData->Kind() == util::ModuleKind::UNKNOWN);
    if (auto cache = DeclarationCache::Instance(); cache != nullptr) {
        cache->Get(importData);
    }
}

std::string_view DeclarationCache::PromoteExistingEntryToLowdeclaration(
    const DeclarationCache::CacheReference &importData, std::string &&text)
{
    auto cache = DeclarationCache::Instance();
    ES2PANDA_ASSERT(cache != nullptr);
    return cache->Add<util::ModuleKind::ETSCACHE_DECL, true>(importData, "", std::move(text)).Text();
}

void DeclarationCache::Clear() noexcept
{
    std::scoped_lock lock(dataGuard_);
    declarations_.clear();
}

void DeclarationCache::Get(DeclarationCache::CacheReference *importData) const noexcept
{
    std::shared_lock lock(dataGuard_);
    const auto it = declarations_.find(std::string(importData->Key()));
    if (it != declarations_.end()) {
        importData->Set(it->second);
    }
}

bool DeclarationCache::CacheReference::CanBePromoted() const
{
    return kind_ != util::ModuleKind::ETSCACHE_DECL;
}

bool DeclarationCache::CacheReference::Absent() const
{
    return kind_ == util::ModuleKind::UNKNOWN;
}

}  // namespace ark::es2panda::parser

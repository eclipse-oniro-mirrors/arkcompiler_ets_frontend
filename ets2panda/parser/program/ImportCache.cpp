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

#include "ImportCache.h"
#include <mutex>
#include <shared_mutex>
#include <thread>
#include "util/importPathManager.h"
#include "util/es2pandaMacros.h"

namespace ark::es2panda::parser {

template <CacheType TYPE>
std::shared_mutex ImportCache<TYPE>::globalGuard_;

template <CacheType TYPE>
ImportCache<TYPE> *ImportCache<TYPE>::globalCache_;

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

template <CacheType TYPE>
ImportCache<TYPE>::~ImportCache()
{
    ClearAll();
}

constexpr std::size_t INITIAL_CACHE_SIZE = 256U;

template <CacheType TYPE>
ImportCache<TYPE>::ImportCache()
{
    cache_.reserve(INITIAL_CACHE_SIZE);
}

//--------------------------------------------------------------------------------------------------------//
//  Creates the new instance of ImportCache class (a singleton object)
//--------------------------------------------------------------------------------------------------------//
template <CacheType TYPE>
void ImportCache<TYPE>::ActivateCache()
{
    std::scoped_lock<std::shared_mutex> lock(globalGuard_);
    if (ImportCache::globalCache_ == nullptr) {
        globalCache_ = new ImportCache();
    }
}

template <CacheType TYPE>
bool ImportCache<TYPE>::IsCacheActivated() noexcept
{
    std::shared_lock<std::shared_mutex> lock(globalGuard_);
    return static_cast<bool>(globalCache_);
}

template <CacheType TYPE>
ImportCache<TYPE> *ImportCache<TYPE>::Instance() noexcept
{
    std::shared_lock<std::shared_mutex> lock(globalGuard_);
    return globalCache_;
}

template <CacheType TYPE>
void ImportCache<TYPE>::ClearAll() noexcept
{
    if (auto cache = ImportCache::Instance(); static_cast<bool>(cache)) {
        cache->Clear();
    }
}

template <CacheType TYPE>
void ImportCache<TYPE>::GetFromCache(CacheReference<> *importInfo) noexcept
{
    ES2PANDA_ASSERT(importInfo->Kind() == util::ModuleKind::UNKNOWN);
    if (auto cache = ImportCache::Instance(); cache != nullptr) {
        cache->Get(importInfo);
    }
}

template <CacheType TYPE>
SelectCacheDataType<TYPE> ImportCache<TYPE>::PromoteExistingEntryToLowdeclaration(const CacheReference<> &importInfo,
                                                                                  SelectCacheDataType<TYPE, true> data)
{
    if constexpr (TYPE == CacheType::SOURCES) {
        auto cache = ImportCache::Instance();
        ES2PANDA_ASSERT(cache != nullptr);
        return cache->template Add<util::ModuleKind::ETSCACHE_DECL, true>(importInfo, "", std::move(data)).Data();
    } else {
        return nullptr;
    }
}

template <CacheType TYPE>
void ImportCache<TYPE>::UpdateOnFileModification(std::string_view key, std::string textSource, std::string &&text,
                                                 util::ModuleKind kind) noexcept
{
    if constexpr (TYPE == CacheType::SOURCES) {
        if (auto cache = ImportCache::Instance(); cache != nullptr) {
            cache->UpdateModifyfile(key, std::move(textSource), std::move(text), kind);
        }
    }
}

template <CacheType TYPE>
void ImportCache<TYPE>::Clear() noexcept
{
    std::scoped_lock lock(dataGuard_);
    cache_.clear();
}

template <CacheType TYPE>
void ImportCache<TYPE>::Get(CacheReference<> *importInfo) const noexcept
{
    std::shared_lock lock(dataGuard_);
    const auto it = cache_.find(std::string(importInfo->Key()));
    if (it != cache_.end()) {
        importInfo->Set(it->second);
    }
}

template <CacheType TYPE>
void ImportCache<TYPE>::UpdateModifyfile(std::string_view key, std::string textSource, std::string &&text,
                                         util::ModuleKind kind)
{
    if constexpr (TYPE == CacheType::SOURCES) {
        std::scoped_lock lock(dataGuard_);
        const auto &newElem = dataStorage_.emplace_back(
            std::make_unique<NamedData>(std::string(key), std::move(textSource), std::move(text)));

        CacheReference<SelectCacheDataType<TYPE>> ref {};
        std::tie(ref.key_, ref.textSource_, ref.data_) = *newElem;
        ref.kind_ = kind;
        cache_[std::string(key)] = ref;
    }
}

template <typename D>
bool CacheReference<D>::CanBePromoted() const
{
    return kind_ != util::ModuleKind::ETSCACHE_DECL;
}

template <typename D>
bool CacheReference<D>::Absent() const
{
    return kind_ == util::ModuleKind::UNKNOWN;
}

template class ImportCache<CacheType::SOURCES>;
template class ImportCache<CacheType::METADATA>;

template struct CacheReference<SelectCacheDataType<CacheType::SOURCES>>;
template struct CacheReference<SelectCacheDataType<CacheType::METADATA>>;

}  // namespace ark::es2panda::parser

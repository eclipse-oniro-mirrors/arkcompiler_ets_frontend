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

#ifndef ES2PANDA_IMPORT_CACHE_H
#define ES2PANDA_IMPORT_CACHE_H

#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>

#include "util/es2pandaMacros.h"

namespace ark::es2panda::parser {
enum class CacheType : uint8_t { SOURCES, METADATA };
}  // namespace ark::es2panda::parser

namespace ark::es2panda::util {

enum class ModuleKind : uint8_t {
    UNKNOWN,

    PACKAGE,
    MODULE,
    SOURCE_DECL,
    ETSCACHE_DECL,
    DECLLESS_DYNAMIC,

    SIMULT_MAIN,

    METADATA_DECL
};

class ImportInfo;
}  // namespace ark::es2panda::util

namespace ark::es2panda::parser {

using SourceDeclType = std::string &&;
using SourceDeclCacheType = std::string_view;
using MetadataType = std::vector<uint8_t>;
using MetadataCacheType = std::vector<uint8_t> *;
using AnyCacheType = std::variant<SourceDeclCacheType, MetadataCacheType>;

template <CacheType TYPE, bool IS_INITIAL_TYPE = false>
using SelectCacheDataType =
    std::conditional_t<TYPE == CacheType::METADATA,
                       std::conditional_t<IS_INITIAL_TYPE, MetadataType, MetadataCacheType>,
                       std::conditional_t<IS_INITIAL_TYPE, SourceDeclType, SourceDeclCacheType>>;

//--------------------------------------------------------------------------------------------------------------------//
//  Constants used in spin-lock classes.
//--------------------------------------------------------------------------------------------------------------------//
constexpr std::int64_t LOCK_OFF = 0;
constexpr std::int64_t LOCK_SET = std::numeric_limits<std::int64_t>::min();

//--------------------------------------------------------------------------------------------------------------------//
//  Class to provide lightweight non-blocking read/write access for multiple threads
//  using the 'spinning' interlocked mutex that meets Lockable requirements.
//      Note 1. Each '...Lock()' call should always have the corresponding '...Unlock()'.
//      Note 2. Write lock cannot be used in recursive calls.
//      Note 3. Standard template guards like 'scoped_lock' and 'unique_lock' can be used.
//--------------------------------------------------------------------------------------------------------------------//
class UniqueSpinMutex {
public:
    UniqueSpinMutex() = default;
    virtual ~UniqueSpinMutex();

    NO_COPY_SEMANTIC(UniqueSpinMutex);
    NO_MOVE_SEMANTIC(UniqueSpinMutex);

    //  Standard library 'Lockable' requirements implementation
    void lock();      // CC-OFF(G.NAM.03-CPP) project code style // NOLINT(readability-identifier-naming)
    void unlock();    // CC-OFF(G.NAM.03-CPP) project code style // NOLINT(readability-identifier-naming)
    bool try_lock();  // CC-OFF(G.NAM.03-CPP) project code style // NOLINT(readability-identifier-naming)

protected:
    std::atomic_int64_t spin_ {LOCK_OFF};  // NOLINT(misc-non-private-member-variables-in-classes)
};

//--------------------------------------------------------------------------------------------------------------------//
//  Class to provide lightweight non-blocking simultaneous read access for multiple threads and blocking write access
//  for a single thread using the 'spinning' interlocked mutex that meets the SharedMutex requirements.
//      Note 1. Each '...Lock()' call should always have the corresponding '...Unlock()'.
//      Note 2. Write lock cannot be used in recursive calls.
//      Note 3. Standard template guards like 'scoped_lock' and 'shared_lock' can be used.
//--------------------------------------------------------------------------------------------------------------------//
class ReadWriteSpinMutex final : public UniqueSpinMutex {
public:
    ReadWriteSpinMutex() = default;
    ~ReadWriteSpinMutex() override = default;

    NO_COPY_SEMANTIC(ReadWriteSpinMutex);
    NO_MOVE_SEMANTIC(ReadWriteSpinMutex);

    //  Standard library 'SharedLockable' requirements implementation
    void lock_shared();      // CC-OFF(G.NAM.03-CPP) project code style // NOLINT(readability-identifier-naming)
    void unlock_shared();    // CC-OFF(G.NAM.03-CPP) project code style // NOLINT(readability-identifier-naming)
    bool try_lock_shared();  // CC-OFF(G.NAM.03-CPP) project code style // NOLINT(readability-identifier-naming)
};

template <CacheType TYPE>
class ImportCache;

template <typename T = AnyCacheType>
struct CacheReference {
public:
    T Data() const
    {
        static_assert(!std::is_same_v<T, AnyCacheType>,
                      "Cache type is unknown at compile-time, use `DataFor` to pick data for required cache type");
        return data_;
    }

    template <CacheType TYPE>
    SelectCacheDataType<TYPE> DataFor() const
    {
        if constexpr (std::is_same_v<T, AnyCacheType>) {
            return std::get<SelectCacheDataType<TYPE>>(data_);
        } else {
            static_assert(std::is_same_v<T, SelectCacheDataType<TYPE>>);
            return data_;
        }
    }

    std::string_view TextSource() const
    {
        return textSource_;
    }

    std::string_view Key() const
    {
        return key_;
    }

    auto Kind() const
    {
        return kind_;
    }

protected:
    template <typename T1>
    void Set(CacheReference<T1> ref)
    {
        // `Set` can only be invoked to set cache info for the import based on source decl cache info or metadata one
        static_assert(std::is_same_v<T, AnyCacheType>);
        static_assert(std::is_same_v<T1, SourceDeclCacheType> || std::is_same_v<T1, MetadataCacheType>);
        ES2PANDA_ASSERT(Key() == ref.Key());
        kind_ = ref.Kind();
        textSource_ = ref.TextSource();
        data_ = std::variant<SourceDeclCacheType, MetadataCacheType>(ref.Data());
    }

    // NOTE(dkofanov): 'key_' should be immutable after packages are eliminated.
    void SetKey(std::string_view key)
    {
        key_ = key;
    }

    bool CanBePromoted() const;

    bool Absent() const;

private:
    // CC-OFFNXT(G.NAM.03-CPP) project codestyle
    std::string_view key_ {};
    // CC-OFFNXT(G.NAM.03-CPP) project codestyle
    util::ModuleKind kind_ {};
    // CC-OFFNXT(G.NAM.03-CPP) project codestyle
    T data_ {};
    // CC-OFFNXT(G.NAM.03-CPP) project codestyle
    std::string_view textSource_ {};

    friend ImportCache<CacheType::SOURCES>;
    friend ImportCache<CacheType::METADATA>;
};

template <CacheType TYPE>
class ImportCache final {
private:
    ImportCache();

public:
    ~ImportCache();

    NO_COPY_SEMANTIC(ImportCache);
    NO_MOVE_SEMANTIC(ImportCache);

    static void ActivateCache();

    static bool IsCacheActivated() noexcept;

    static void ClearAll() noexcept;

    static void GetFromCache(CacheReference<> *importInfo) noexcept;

    template <util::ModuleKind KIND>
    static constexpr bool AssertCacheType()
    {
        return (KIND >= util::ModuleKind::PACKAGE && KIND <= util::ModuleKind::SIMULT_MAIN &&
                TYPE == CacheType::SOURCES) ||
               (KIND == util::ModuleKind::METADATA_DECL && TYPE == CacheType::METADATA);
    }

    template <util::ModuleKind KIND, bool SHOULD_CACHE>
    static CacheReference<SelectCacheDataType<TYPE>> StoreContents(const CacheReference<> &importInfo,
                                                                   std::string textSource,
                                                                   SelectCacheDataType<TYPE, true> data)
    {
        static_assert(AssertCacheType<KIND>());
        auto cache = Instance();
        ES2PANDA_ASSERT(cache != nullptr);
        return cache->template Add<KIND, SHOULD_CACHE>(importInfo, textSource, std::move(data));
    }

    static SelectCacheDataType<TYPE> PromoteExistingEntryToLowdeclaration(const CacheReference<> &importInfo,
                                                                          SelectCacheDataType<TYPE, true> data);

private:
    static ImportCache *Instance() noexcept;

    void Clear() noexcept;

    void Get(CacheReference<> *importInfo) const noexcept;

    template <util::ModuleKind KIND, bool SHOULD_CACHE>
    CacheReference<SelectCacheDataType<TYPE>> Add(const CacheReference<> &importInfo, const std::string &textSource,
                                                  SelectCacheDataType<TYPE, true> data)
    {
        std::scoped_lock lock(dataGuard_);
        const auto &newElem = dataStorage_.emplace_back(
            std::make_unique<NamedData>(std::string(importInfo.Key()), textSource, std::move(data)));
        CacheReference<SelectCacheDataType<TYPE>> ref {};
        if constexpr (TYPE == CacheType::METADATA) {
            ref.key_ = std::get<0>(*newElem);
            ref.textSource_ = std::get<1>(*newElem);
            constexpr auto dataIndex = 2;
            ref.data_ = &(std::get<dataIndex>(*newElem));
        } else {
            std::tie(ref.key_, ref.textSource_, ref.data_) = *newElem;
        }
        ref.kind_ = KIND;

        if constexpr (SHOULD_CACHE) {
            if (auto it = cache_.find(std::string(importInfo.Key())); it != cache_.end()) {
                if (!it->second.CanBePromoted() || ref.CanBePromoted()) {
                    return ref;
                }
                if (ref.TextSource().empty()) {
                    ref.textSource_ = it->second.textSource_;
                }
            }
            cache_[std::string(ref.Key())] = ref;
        }
        return ref;
    }

    static std::shared_mutex globalGuard_;
    // NOLINTNEXTLINE(fuchsia-statically-constructed-objects)
    static ImportCache *globalCache_;

    //  Synchronization object to control access to cached data:
    mutable std::shared_mutex dataGuard_ {};
    using NamedData = std::tuple<std::string, std::string, std::decay_t<SelectCacheDataType<TYPE, true>>>;
    std::vector<std::unique_ptr<NamedData>> dataStorage_ {};
    // mapping from importInfo
    std::unordered_map<std::string, CacheReference<SelectCacheDataType<TYPE>>> cache_ {};
};

}  // namespace ark::es2panda::parser

#endif /* ES2PANDA_IMPORT_CACHE_H */

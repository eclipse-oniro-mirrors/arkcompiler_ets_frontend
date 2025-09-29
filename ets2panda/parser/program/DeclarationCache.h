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

#ifndef ES2PANDA_DECLARATION_CACHE_H
#define ES2PANDA_DECLARATION_CACHE_H

#include <atomic>
#include <limits>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>

#include "util/es2pandaMacros.h"

namespace ark::es2panda::util {
class ImportMetadata;
enum class ModuleKind : uint8_t;
}  // namespace ark::es2panda::util

namespace ark::es2panda::parser {

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

class DeclarationCache final {
private:
    DeclarationCache();

public:
    struct CacheReference {
    public:
        std::string_view Text() const
        {
            return text_;
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
        template <util::ModuleKind KIND, bool SHOULD_CACHE>
        void Set(std::string textSource, std::string &&contents)
        {
            Set(DeclarationCache::StoreContents<KIND, SHOULD_CACHE>(*this, textSource, std::move(contents)));
            ES2PANDA_ASSERT(KIND == Kind());
        }

        void Set(CacheReference ref)
        {
            ES2PANDA_ASSERT(Key() == ref.Key());
            kind_ = ref.kind_;
            textSource_ = ref.textSource_;
            text_ = ref.text_;
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
        std::string_view text_ {};
        // CC-OFFNXT(G.NAM.03-CPP) project codestyle
        std::string_view textSource_ {};

        friend DeclarationCache;
    };

    ~DeclarationCache();

    NO_COPY_SEMANTIC(DeclarationCache);
    NO_MOVE_SEMANTIC(DeclarationCache);

    static void ActivateCache();

    static bool IsCacheActivated() noexcept;

    static void ClearAll() noexcept;

    static void GetFromCache(CacheReference *importData) noexcept;

    template <util::ModuleKind KIND, bool SHOULD_CACHE>
    static CacheReference StoreContents(const CacheReference &importData, std::string textSource, std::string &&text)
    {
        auto cache = DeclarationCache::Instance();
        ES2PANDA_ASSERT(cache != nullptr);
        return cache->Add<KIND, SHOULD_CACHE>(importData, textSource, std::move(text));
    }

    static std::string_view PromoteExistingEntryToLowdeclaration(const CacheReference &importData, std::string &&text);

private:
    static DeclarationCache *Instance() noexcept;

    void Clear() noexcept;

    void Get(CacheReference *importData) const noexcept;

    template <util::ModuleKind KIND, bool SHOULD_CACHE>
    CacheReference Add(const CacheReference &importData, std::string textSource, std::string &&text)
    {
        std::scoped_lock lock(dataGuard_);
        const auto &newElem = textsStorage_.emplace_back(
            std::make_unique<NamedText>(std::string(importData.Key()), textSource, std::move(text)));
        DeclarationCache::CacheReference ref {};
        std::tie(ref.key_, ref.textSource_, ref.text_) = *newElem;
        ref.kind_ = KIND;

        if constexpr (SHOULD_CACHE) {
            if (auto it = declarations_.find(std::string(importData.Key())); it != declarations_.end()) {
                if (!it->second.CanBePromoted() || ref.CanBePromoted()) {
                    return ref;
                }
                if (ref.TextSource().empty()) {
                    ref.textSource_ = it->second.textSource_;
                }
            }
            declarations_[std::string(ref.Key())] = ref;
        }
        return ref;
    }

private:
    static std::shared_mutex globalGuard_;
    // NOLINTNEXTLINE(fuchsia-statically-constructed-objects)
    static DeclarationCache *globalDeclarationCache_;

private:
    //  Synchronization object to control access to cached data:
    mutable std::shared_mutex dataGuard_ {};
    using NamedText = std::tuple<std::string, std::string, std::string>;
    std::vector<std::unique_ptr<NamedText>> textsStorage_ {};
    // mapping from importMetadata
    std::unordered_map<std::string, CacheReference> declarations_ {};
};

}  // namespace ark::es2panda::parser

#endif /* ES2PANDA_PDECLARATION_CACHE_H */

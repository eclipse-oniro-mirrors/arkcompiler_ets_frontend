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

#ifndef ES2PANDA_DECLARATION_CACHE_H
#define ES2PANDA_DECLARATION_CACHE_H

#include <atomic>
#include <limits>
#include <memory>
#include <shared_mutex>
#include <string>
#include <unordered_map>

#include "libpandabase/macros.h"

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
    void lock();      // CC-OFF(G.NAM.03-CPP) project code style
    void unlock();    // CC-OFF(G.NAM.03-CPP) project code style
    bool try_lock();  // CC-OFF(G.NAM.03-CPP) project code style

protected:
    std::atomic_int64_t spin_ {LOCK_OFF};
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
    void lock_shared();      // CC-OFF(G.NAM.03-CPP) project code style
    void unlock_shared();    // CC-OFF(G.NAM.03-CPP) project code style
    bool try_lock_shared();  // CC-OFF(G.NAM.03-CPP) project code style
};

using DeclarationType = std::shared_ptr<std::string>;

class DeclarationCache final {
    struct Tag {};

public:
    inline static DeclarationType const ABSENT {};

    DeclarationCache() = delete;
    ~DeclarationCache();

    NO_COPY_SEMANTIC(DeclarationCache);
    NO_MOVE_SEMANTIC(DeclarationCache);

    explicit DeclarationCache(Tag &&tag);

    static void ActivateCache();

    [[nodiscard]] static bool IsCacheActivated() noexcept;

    static void ClearAll() noexcept;

    static void RemoveFromCache(std::string const &fileName) noexcept;

    [[nodiscard]] static DeclarationType GetFromCache(std::string const &fileName) noexcept;

    static DeclarationType CacheIfPossible(std::string fileName, DeclarationType decl);

private:
    static std::shared_ptr<DeclarationCache> Instance() noexcept;

    void Clear() noexcept;
    void Remove(std::string const &fileName) noexcept;

    DeclarationType Get(std::string const &fileName) const noexcept;
    DeclarationType Add(std::string fileName, DeclarationType decl);

private:
private:
    inline static std::shared_mutex globalGuard_ {};
    inline static std::shared_ptr<DeclarationCache> globalDeclarationCache_ = nullptr;

private:
    //  Synchronization object to control access to cached data:
    mutable ReadWriteSpinMutex dataGuard_ {};
    std::unordered_map<std::string, DeclarationType> declarations_ {};
};

}  // namespace ark::es2panda::parser

#endif /* ES2PANDA_PDECLARATION_CACHE_H */

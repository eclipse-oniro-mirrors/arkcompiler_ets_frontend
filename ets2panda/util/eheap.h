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

#ifndef ES2PANDA_UTIL_EHEAP_H
#define ES2PANDA_UTIL_EHEAP_H

#include "es2pandaMacros.h"
#include "libarkbase/mem/arena_allocator.h"
#include "libarkbase/mem/arena_allocator_stl_adapter.h"

#include <vector>
#include <queue>
#include <deque>
#include <stack>
#include <list>
#include <forward_list>
#include <set>
#include <map>
#include <unordered_map>
#include <unordered_set>

namespace ark::es2panda {

class EAllocator;
using SArenaAllocator = ark::ArenaAllocator;

class EHeap {
public:
    static void Initialize();
    static void Finalize();

    // legacy API
    static bool IsInitialized();

    static inline EAllocator CreateAllocator();
    static inline std::unique_ptr<EAllocator> NewAllocator();

    static SArenaAllocator CreateScopedAllocator();
    static std::unique_ptr<SArenaAllocator> NewScopedAllocator();

    [[nodiscard]] __attribute__((returns_nonnull)) static void *Alloc(size_t sz)
    {
        return gEHeapSpace->Alloc(sz);
    }

    ALWAYS_INLINE static void Free([[maybe_unused]] void *ptr, [[maybe_unused]] size_t sz)
    {
#ifndef NDEBUG
        if (LIKELY(gEHeapSpace != nullptr)) {
            gEHeapSpace->Free(ptr, sz);
        }
#endif
    }

    static size_t AllocatedSize();
    static size_t FreedSize();

    // legacy API
    static bool IsEHeapInitialized()
    {
        return EHeap::gEHeapSpace != nullptr;
    }

    static void ForceInitializeEHeapSpace();
    static void ForceFinalizeEHeapSpace();

    template <typename T>
    class EPtr {
    public:
        EPtr(T *ptr) : raw_(compress(ptr))
        {
            ES2PANDA_ASSERT(decompress(raw_) == ptr);
        }
        EPtr() : EPtr(nullptr) {}

        DEFAULT_COPY_SEMANTIC(EPtr);
        DEFAULT_MOVE_SEMANTIC(EPtr);

        T *operator->()
        {
            return decompress(raw_);
        }

        T *operator->() const
        {
            return decompress(raw_);
        }

        operator T *()
        {
            return decompress(raw_);
        }

        operator T *() const
        {
            return decompress(raw_);
        }

    private:
        static T *decompress(uint32_t raw)
        {
            if (raw == 0) {
                return nullptr;
            }
            return reinterpret_cast<T *>(ToUintPtr(gEHeapSpace->BaseAddr()) + (raw << EHeapSpace::ALLOC_LOG_ALIGNMENT));
        }

        static uint32_t compress(T *ptr)
        {
            if (ptr == nullptr) {
                return 0;
            }
            gEHeapSpace->AssertInRange(static_cast<void const *>(ptr));
            return (ToUintPtr(ptr) - ToUintPtr(gEHeapSpace->BaseAddr())) >> EHeapSpace::ALLOC_LOG_ALIGNMENT;
        }

        uint32_t raw_;
    };

private:
    EHeap() = delete;
    ~EHeap() = delete;

    static void InitializeEHeapSpace();
    static void FinalizeEHeapSpace();

    [[noreturn]] __attribute__((noinline)) static void OOMAction();
    [[noreturn]] __attribute__((noinline)) static void BrokenEHeapPointerAction(void const *ptr);

    class EHeapSpace {
    public:
        explicit EHeapSpace(size_t size);
        ~EHeapSpace();

        [[nodiscard]] __attribute__((returns_nonnull)) void *Alloc(size_t sz);

        ALWAYS_INLINE void Free([[maybe_unused]] void *ptr, size_t sz)
        {
            ASAN_POISON_MEMORY_REGION(ptr, sz);
            freedSize_ += sz;
        }

        ALWAYS_INLINE void *BaseAddr()
        {
            return buffer_;
        }

        ALWAYS_INLINE void AssertInRange(void const *ptr)
        {
            if (!(ToUintPtr(ptr) >= ToUintPtr(buffer_) && ToUintPtr(ptr) < current_)) {
                BrokenEHeapPointerAction(ptr);
            }
        }

        size_t AllocatedSize() const
        {
            return current_ - ToUintPtr(buffer_);
        }

        size_t FreedSize() const
        {
            return freedSize_;
        }

        static constexpr auto ALLOC_LOG_ALIGNMENT = LOG_ALIGN_3;
        static constexpr auto ALLOC_ALIGNMENT = GetAlignmentInBytes(ALLOC_LOG_ALIGNMENT);

    private:
        void *buffer_ {};
        size_t bufferSize_ {};

        uintptr_t current_ {};
        uintptr_t top_ {};
        uintptr_t current_committed_ {};

        size_t freedSize_ {};
    };

    friend class EAllocator;

    static EHeapSpace *gEHeapSpace;
};

template <typename T>
using EPtr = EHeap::EPtr<T>;

template <typename T>
class EAllocatorAdapter;

class EAllocator {
public:
    EAllocator();
    ~EAllocator();

    NO_COPY_SEMANTIC(EAllocator);
    NO_MOVE_SEMANTIC(EAllocator);

    [[nodiscard]] static void *Alloc(size_t size)
    {
        return EHeap::Alloc(size);
    }

    static void Free(void *ptr, size_t sz)
    {
        return EHeap::Free(ptr, sz);
    }

    template <typename T, typename... Args>
    [[nodiscard]] static std::enable_if_t<!std::is_array_v<T>, T *> New(Args &&...args)
    {
        auto p = reinterpret_cast<void *>(Alloc(sizeof(T)));
        if (UNLIKELY(p == nullptr)) {
            ES2PANDA_UNREACHABLE();
        }
        new (p) T(std::forward<Args>(args)...);
        return reinterpret_cast<T *>(p);
    }

    template <typename T>
    [[nodiscard]] static std::enable_if_t<is_unbounded_array_v<T>, std::remove_extent_t<T> *> New(size_t size)
    {
        using ElementType = std::remove_extent_t<T>;
        void *p = Alloc(sizeof(ElementType) * size);
        if (UNLIKELY(p == nullptr)) {
            ES2PANDA_UNREACHABLE();
        }
        auto const data = ToNativePtr<ElementType>(ToUintPtr(p));
        for (size_t i = 0; i < size; ++i) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            new (&data[i]) ElementType();
        }
        return data;
    }

    EAllocatorAdapter<void> Adapter();
};

inline EAllocator EHeap::CreateAllocator()
{
    return EAllocator {};
}

inline std::unique_ptr<EAllocator> EHeap::NewAllocator()
{
    return std::make_unique<EAllocator>();
}

inline SArenaAllocator EHeap::CreateScopedAllocator()
{
    return SArenaAllocator {SpaceType::SPACE_TYPE_COMPILER, nullptr, true};
}

inline std::unique_ptr<SArenaAllocator> EHeap::NewScopedAllocator()
{
    return std::make_unique<SArenaAllocator>(SpaceType::SPACE_TYPE_COMPILER, nullptr, true);
}

template <>
class EAllocatorAdapter<void> {
public:
    using value_type = void;             // NOLINT(readability-identifier-naming)
    using pointer = void *;              // NOLINT(readability-identifier-naming)
    using const_pointer = const void *;  // NOLINT(readability-identifier-naming)

    template <typename U>
    struct Rebind {
        // NOLINTNEXTLINE(readability-identifier-naming)
        using other = EAllocatorAdapter<U>;
    };

    template <typename U>
    using rebind = Rebind<U>;  // NOLINT(readability-identifier-naming)

    explicit EAllocatorAdapter() = default;
    explicit EAllocatorAdapter([[maybe_unused]] EAllocator *allocator) {}
    template <typename U>
    // NOLINTNEXTLINE(google-explicit-constructor)
    EAllocatorAdapter([[maybe_unused]] const EAllocatorAdapter<U> &other)
    {
    }
    EAllocatorAdapter(const EAllocatorAdapter &) = default;
    EAllocatorAdapter &operator=(const EAllocatorAdapter &) = default;
    EAllocatorAdapter(EAllocatorAdapter &&) = default;
    EAllocatorAdapter &operator=(EAllocatorAdapter &&) = default;
    ~EAllocatorAdapter() = default;

private:
    template <typename U>
    friend class EAllocatorAdapter;
};

template <typename T>
class EAllocatorAdapter {
public:
    using value_type = T;               // NOLINT(readability-identifier-naming)
    using pointer = T *;                // NOLINT(readability-identifier-naming)
    using reference = T &;              // NOLINT(readability-identifier-naming)
    using const_pointer = const T *;    // NOLINT(readability-identifier-naming)
    using const_reference = const T &;  // NOLINT(readability-identifier-naming)
    using size_type = size_t;           // NOLINT(readability-identifier-naming)
    using difference_type = ptrdiff_t;  // NOLINT(readability-identifier-naming)

    template <typename U>
    struct Rebind {
        // NOLINTNEXTLINE(readability-identifier-naming)
        using other = EAllocatorAdapter<U>;
    };

    template <typename U>
    using rebind = Rebind<U>;  // NOLINT(readability-identifier-naming)

    explicit EAllocatorAdapter() = default;
    explicit EAllocatorAdapter([[maybe_unused]] EAllocator *allocator) {}
    template <typename U>
    // NOLINTNEXTLINE(google-explicit-constructor)
    EAllocatorAdapter([[maybe_unused]] const EAllocatorAdapter<U> &other)
    {
    }
    EAllocatorAdapter(const EAllocatorAdapter &) = default;
    EAllocatorAdapter &operator=(const EAllocatorAdapter &) = default;
    EAllocatorAdapter([[maybe_unused]] EAllocatorAdapter &&other) noexcept {}
    EAllocatorAdapter &operator=([[maybe_unused]] EAllocatorAdapter &&other) noexcept
    {
        return *this;
    }
    ~EAllocatorAdapter() = default;

    // NOLINTNEXTLINE(readability-identifier-naming)
    size_type max_size() const
    {
        return static_cast<size_type>(-1) / sizeof(T);
    }

    // NOLINTNEXTLINE(readability-identifier-naming)
    pointer address(reference x) const
    {
        return &x;
    }
    // NOLINTNEXTLINE(readability-identifier-naming)
    const_reference address(const_reference x) const
    {
        return &x;
    }

    // NOLINTNEXTLINE(readability-identifier-naming)
    pointer allocate(size_type n, [[maybe_unused]] typename EAllocatorAdapter<void>::pointer ptr = nullptr)
    {
        ES2PANDA_ASSERT(n <= max_size());
        return static_cast<T *>(EHeap::Alloc(sizeof(T) * n));
    }

    // NOLINTNEXTLINE(readability-identifier-naming)
    void deallocate([[maybe_unused]] pointer p, [[maybe_unused]] size_type n)
    {
        EHeap::Free(p, n);
    }

    template <typename U, typename... Args>
    void construct(U *p, Args &&...args)  // NOLINT(readability-identifier-naming)
    {
        ES2PANDA_ASSERT(p != nullptr);
        ::new (static_cast<void *>(p)) U(std::forward<Args>(args)...);
    }
    template <typename U>
    void destroy(U *p)  // NOLINT(readability-identifier-naming)
    {
        p->~U();
    }

private:
    template <typename U>
    friend class EAllocatorAdapter;

    template <typename U>
    // NOLINTNEXTLINE(readability-redundant-declaration)
    friend inline bool operator==(const EAllocatorAdapter<U> &lhs, const EAllocatorAdapter<U> &rhs);
};

template <typename T>
inline bool operator==([[maybe_unused]] const EAllocatorAdapter<T> &lhs,
                       [[maybe_unused]] const EAllocatorAdapter<T> &rhs)
{
    return true;
}

template <typename T>
inline bool operator!=(const EAllocatorAdapter<T> &lhs, const EAllocatorAdapter<T> &rhs)
{
    return !(lhs == rhs);
}

inline EAllocatorAdapter<void> EAllocator::Adapter()
{
    return EAllocatorAdapter<void>(this);
}

}  // namespace ark::es2panda

namespace ark::es2panda {
namespace eallocator_replacer {
using ArenaAllocator = EAllocator;

template <class T>
using ArenaVector = std::vector<T, EAllocatorAdapter<T>>;
template <class T>
using ArenaDeque = std::deque<T, EAllocatorAdapter<T>>;
template <class T, class ArenaContainer = ArenaDeque<T>>
using ArenaStack = std::stack<T, ArenaContainer>;
template <class T, class ArenaContainer = ArenaDeque<T>>
using ArenaQueue = std::queue<T, ArenaContainer>;
template <class T>
using ArenaList = std::list<T, EAllocatorAdapter<T>>;
template <class T>
using ArenaForwardList = std::forward_list<T, EAllocatorAdapter<T>>;
template <class Key, class Compare = std::less<Key>>
using ArenaSet = std::set<Key, Compare, EAllocatorAdapter<Key>>;
template <class Key, class T, class Compare = std::less<Key>>
using ArenaMap = std::map<Key, T, Compare, EAllocatorAdapter<std::pair<const Key, T>>>;
template <class Key, class T, class Compare = std::less<Key>>
using ArenaMultiMap = std::multimap<Key, T, Compare, EAllocatorAdapter<std::pair<const Key, T>>>;
template <class Key, class T, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>>
using ArenaUnorderedMultiMap =
    std::unordered_multimap<Key, T, Hash, KeyEqual, EAllocatorAdapter<std::pair<const Key, T>>>;
template <class Key, class T, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>>
using ArenaUnorderedMap = std::unordered_map<Key, T, Hash, KeyEqual, EAllocatorAdapter<std::pair<const Key, T>>>;
template <class Key1, class Key2, class T>
using ArenaDoubleUnorderedMap =
    ArenaUnorderedMap<Key1, ArenaUnorderedMap<Key2, T, std::hash<Key2>, std::equal_to<Key2>>, std::hash<Key1>,
                      std::equal_to<Key1>>;
template <class Key, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>>
using ArenaUnorderedSet = std::unordered_set<Key, Hash, KeyEqual, EAllocatorAdapter<Key>>;
using ArenaString = std::basic_string<char, std::char_traits<char>, EAllocatorAdapter<char>>;

template <class T>
using SArenaVector = std::vector<T, ark::ArenaAllocatorAdapter<T, false>>;
template <class Key, class T, class Compare = std::less<Key>>
using SArenaMap = std::map<Key, T, Compare, ark::ArenaAllocatorAdapter<std::pair<const Key, T>, false>>;
template <class Key, class T, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>>
using SArenaUnorderedMap =
    std::unordered_map<Key, T, Hash, KeyEqual, ark::ArenaAllocatorAdapter<std::pair<const Key, T>, false>>;
template <class T>
using SArenaList = std::list<T, ark::ArenaAllocatorAdapter<T, false>>;
using SArenaString = std::basic_string<char, std::char_traits<char>, ark::ArenaAllocatorAdapter<char>>;

}  // namespace eallocator_replacer

using namespace ark::es2panda::eallocator_replacer;
}  // namespace ark::es2panda

#endif  // ES2PANDA_UTIL_EHEAP_H

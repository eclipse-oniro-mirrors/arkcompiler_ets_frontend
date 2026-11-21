/**
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

// Comprehensive gtest coverage for `util/eheap.h`.
//
// The suite covers:
//   * EHeap::Alloc / EHeap::Free / accounting primitives,
//   * EHeap::Scope lifetime, nesting, re-initialization and DeprecatedId(),
//   * EAllocator (object/array construction) and EAllocatorAdapter,
//   * every arena container alias from `eallocator_replacer`,
//   * EHeap::EPtr in both the raw-pointer (debug) and the compressed (release) flavour,
//   * multi-threaded stress tests.
//
// Threading note: `EHeap::Alloc` bumps a plain `uintptr_t` cursor and is therefore not
// synchronized internally.  Every heap mutation performed by a worker thread in this file
// is serialized through `HeapMutationGuard`; what the threaded tests validate is the
// cross-thread invariants (non-overlapping blocks, pattern integrity, size accounting,
// lifetime of scopes created by worker threads), not lock-free allocation.

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <forward_list>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <numeric>
#include <set>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "util/eheap.h"

namespace ark::es2panda {
namespace {

// EHeapSpace::ALLOC_ALIGNMENT is private, keep the value in sync with `eheap.h`.
constexpr size_t ALLOC_ALIGNMENT = 8U;
constexpr size_t HUGE_ALLOCATION_SIZE = 1ULL << 40U;

constexpr size_t AlignUpBytes(size_t value)
{
    return (value + ALLOC_ALIGNMENT - 1U) & ~(ALLOC_ALIGNMENT - 1U);
}

// `EHeap::Free` only records sizes when assertions are enabled, see `EHeapSpace::Free`.
#ifdef NDEBUG
constexpr bool FREE_ACCOUNTING_ENABLED = false;
#else
constexpr bool FREE_ACCOUNTING_ENABLED = true;
#endif

constexpr size_t ExpectedFreedSize(size_t size)
{
    return FREE_ACCOUNTING_ENABLED ? size : 0U;
}

inline uintptr_t Addr(const void *ptr)
{
    return reinterpret_cast<uintptr_t>(ptr);
}

inline bool IsAligned(const void *ptr)
{
    return (Addr(ptr) & (ALLOC_ALIGNMENT - 1U)) == 0U;
}

inline bool RangesOverlap(const void *lhs, size_t lhsSize, const void *rhs, size_t rhsSize)
{
    return Addr(lhs) < (Addr(rhs) + rhsSize) && Addr(rhs) < (Addr(lhs) + lhsSize);
}

inline void FillPattern(void *ptr, size_t size, uint64_t seed)
{
    auto *bytes = static_cast<uint8_t *>(ptr);
    for (size_t i = 0; i < size; ++i) {
        bytes[i] = static_cast<uint8_t>((seed * 131U) + (i * 17U) + 0x5AU);
    }
}

inline bool PatternMatches(const void *ptr, size_t size, uint64_t seed)
{
    auto *bytes = static_cast<const uint8_t *>(ptr);
    for (size_t i = 0; i < size; ++i) {
        if (bytes[i] != static_cast<uint8_t>((seed * 131U) + (i * 17U) + 0x5AU)) {
            return false;
        }
    }
    return true;
}

inline void FillByte(void *ptr, size_t size, uint8_t value)
{
    auto *bytes = static_cast<uint8_t *>(ptr);
    for (size_t i = 0; i < size; ++i) {
        bytes[i] = value;
    }
}

inline bool AllBytes(const void *ptr, size_t size, uint8_t value)
{
    auto *bytes = static_cast<const uint8_t *>(ptr);
    for (size_t i = 0; i < size; ++i) {
        if (bytes[i] != value) {
            return false;
        }
    }
    return true;
}

// `EHeap::Alloc` is not re-entrant: serialize every worker-thread mutation of the heap.
std::mutex &HeapMutationMutex()
{
    static std::mutex mutex;
    return mutex;
}

class HeapMutationGuard {
public:
    HeapMutationGuard() : lock_(HeapMutationMutex()) {}

    NO_COPY_SEMANTIC(HeapMutationGuard);
    NO_MOVE_SEMANTIC(HeapMutationGuard);

private:
    std::lock_guard<std::mutex> lock_;
};

template <typename Function>
void RunThreads(size_t threadCount, Function function)
{
    std::vector<std::thread> threads;
    threads.reserve(threadCount);
    for (size_t index = 0; index < threadCount; ++index) {
        threads.emplace_back(function, index);
    }
    for (auto &thread : threads) {
        thread.join();
    }
}

class SpinBarrier {
public:
    explicit SpinBarrier(uint32_t parties) : parties_(parties) {}

    NO_COPY_SEMANTIC(SpinBarrier);
    NO_MOVE_SEMANTIC(SpinBarrier);

    void Wait()
    {
        // Atomic with acquire order reason: acquire pairs with the release publication
        const uint32_t generation = generation_.load(std::memory_order_acquire);
        // Atomic with acq_rel order reason: read-modify-write of the arrival counter
        if (waiting_.fetch_add(1U, std::memory_order_acq_rel) + 1U == parties_) {
            // Atomic with relaxed order reason: reset is observed via the release fetch_add
            waiting_.store(0U, std::memory_order_relaxed);
            // Atomic with release order reason: publish the new generation to the waiters
            generation_.fetch_add(1U, std::memory_order_release);
            return;
        }
        // Atomic with acquire order reason: acquire pairs with the release fetch_add
        while (generation_.load(std::memory_order_acquire) == generation) {
            std::this_thread::yield();
        }
    }

private:
    const uint32_t parties_;
    std::atomic<uint32_t> waiting_ {0U};
    std::atomic<uint32_t> generation_ {0U};
};

struct BlockRecord {
    void *ptr {};
    size_t size {};
    uint64_t seed {};
};

class ScopedAllocatorsManagerSession {
public:
    ScopedAllocatorsManagerSession()
    {
        ScopedAllocatorsManager::Initialize();
    }

    ~ScopedAllocatorsManagerSession()
    {
        ScopedAllocatorsManager::Finalize();
    }

    NO_COPY_SEMANTIC(ScopedAllocatorsManagerSession);
    NO_MOVE_SEMANTIC(ScopedAllocatorsManagerSession);
};

struct DefaultConstructibleProbe {
    static std::atomic<size_t> constructed;
    static std::atomic<size_t> destroyed;

    int value {42};
    bool defaultConstructed {true};

    DefaultConstructibleProbe()
    {
        ++constructed;
    }

    explicit DefaultConstructibleProbe(int initial) : value(initial), defaultConstructed(false)
    {
        ++constructed;
    }

    ~DefaultConstructibleProbe()
    {
        ++destroyed;
    }

    NO_COPY_SEMANTIC(DefaultConstructibleProbe);
};

std::atomic<size_t> DefaultConstructibleProbe::constructed {0};
std::atomic<size_t> DefaultConstructibleProbe::destroyed {0};

struct ValueProbe {
    int first {0};
    int second {0};

    ValueProbe(int lhs, int rhs) : first(lhs), second(rhs) {}
};

struct LargeProbe {
    std::array<uint8_t, 4096> payload {};

    LargeProbe() = default;

    explicit LargeProbe(uint8_t seed)
    {
        payload.fill(seed);
    }
};

class EHeapTestBase : public ::testing::Test {
public:
    EHeapTestBase()
    {
        ScopedAllocatorsManager::Initialize();
    }

    ~EHeapTestBase() override
    {
        ScopedAllocatorsManager::Finalize();
    }

    NO_COPY_SEMANTIC(EHeapTestBase);
    NO_MOVE_SEMANTIC(EHeapTestBase);
};

class EHeapTest : public EHeapTestBase {
protected:
    EHeap::Scope scope_;
};

// Fixture that owns a dedicated arena allocator for the `SArena*` aliases.
class EHeapSArenaTest : public EHeapTest {
protected:
    EHeapSArenaTest() : arena_(ScopedAllocatorsManager::NewAllocator()) {}

    SArenaAllocator *Allocator()
    {
        return arena_.get();
    }

private:
    std::unique_ptr<SArenaAllocator> arena_;
};
// ---------------------------------------------------------------------------
// Compile time interface contract
// ---------------------------------------------------------------------------
static_assert(!std::is_copy_constructible_v<EHeap>);
static_assert(!std::is_move_constructible_v<EHeap>);
static_assert(!std::is_copy_constructible_v<EHeap::Scope>);
static_assert(!std::is_move_constructible_v<EHeap::Scope>);
static_assert(!std::is_copy_constructible_v<EAllocator>);
static_assert(!std::is_move_constructible_v<EAllocator>);
static_assert(!std::is_copy_constructible_v<ScopedAllocatorsManager>);
static_assert(!std::is_move_constructible_v<ScopedAllocatorsManager>);

static_assert(std::is_same_v<EAllocatorAdapter<int>::value_type, int>);
static_assert(std::is_same_v<EAllocatorAdapter<int>::pointer, int *>);
static_assert(std::is_same_v<EAllocatorAdapter<int>::reference, int &>);
static_assert(std::is_same_v<EAllocatorAdapter<int>::const_pointer, const int *>);
static_assert(std::is_same_v<EAllocatorAdapter<int>::const_reference, const int &>);
static_assert(std::is_same_v<EAllocatorAdapter<int>::size_type, size_t>);
static_assert(std::is_same_v<EAllocatorAdapter<int>::difference_type, ptrdiff_t>);
static_assert(std::is_same_v<EAllocatorAdapter<int>::rebind<double>::other, EAllocatorAdapter<double>>);

static_assert(std::is_same_v<EAllocatorAdapter<void>::value_type, void>);
static_assert(std::is_same_v<EAllocatorAdapter<void>::pointer, void *>);
static_assert(std::is_same_v<EAllocatorAdapter<void>::const_pointer, const void *>);
static_assert(std::is_same_v<EAllocatorAdapter<void>::rebind<int>::other, EAllocatorAdapter<int>>);

static_assert(std::is_same_v<eallocator_replacer::ArenaAllocator, EAllocator>);
static_assert(std::is_same_v<ArenaVector<int>, std::vector<int, EAllocatorAdapter<int>>>);
static_assert(std::is_same_v<ArenaDeque<int>, std::deque<int, EAllocatorAdapter<int>>>);
static_assert(std::is_same_v<ArenaList<int>, std::list<int, EAllocatorAdapter<int>>>);
static_assert(std::is_same_v<ArenaForwardList<int>, std::forward_list<int, EAllocatorAdapter<int>>>);
static_assert(std::is_same_v<ArenaSet<int>, std::set<int, std::less<int>, EAllocatorAdapter<int>>>);
static_assert(std::is_same_v<ArenaMap<int, int>,
                             std::map<int, int, std::less<int>, EAllocatorAdapter<std::pair<const int, int>>>>);
static_assert(std::is_same_v<ArenaUnorderedSet<int>,
                             std::unordered_set<int, std::hash<int>, std::equal_to<int>, EAllocatorAdapter<int>>>);
static_assert(
    std::is_same_v<ArenaUnorderedMap<int, int>, std::unordered_map<int, int, std::hash<int>, std::equal_to<int>,
                                                                   EAllocatorAdapter<std::pair<const int, int>>>>);
static_assert(std::is_same_v<ArenaString, std::basic_string<char, std::char_traits<char>, EAllocatorAdapter<char>>>);

#if defined(NDEBUG) || defined(PANDA_FAST_VERIFY)
// Pointer compression is enabled: `EPtr<T>` stores a 32-bit heap-relative offset.
static_assert(sizeof(EPtr<int>) == sizeof(uint32_t));
static_assert(sizeof(EPtr<LargeProbe>) == sizeof(uint32_t));
static_assert(alignof(EPtr<int>) == alignof(uint32_t));
#else
// Pointer compression is disabled: `EPtr<T>` is a plain pointer.
static_assert(std::is_same_v<EPtr<int>, int *>);
static_assert(std::is_same_v<EHeap::EPtr<LargeProbe>, LargeProbe *>);
static_assert(sizeof(EPtr<int>) == sizeof(int *));
#endif

static_assert(std::is_same_v<EPtr<int>, EHeap::EPtr<int>>);
TEST(EHeapInterfaceTest, ScopeIsNotCopyable)
{
    EXPECT_FALSE(std::is_copy_constructible_v<EHeap::Scope>);
    EXPECT_FALSE(std::is_copy_assignable_v<EHeap::Scope>);
    EXPECT_FALSE(std::is_move_constructible_v<EHeap::Scope>);
    EXPECT_FALSE(std::is_move_assignable_v<EHeap::Scope>);
}
TEST(EHeapInterfaceTest, EHeapIsNotInstantiable)
{
    EXPECT_FALSE(std::is_default_constructible_v<EHeap>);
    EXPECT_FALSE(std::is_copy_constructible_v<EHeap>);
    EXPECT_FALSE(std::is_destructible_v<EHeap>);
}
TEST(EHeapInterfaceTest, AllocatorIsNotCopyable)
{
    EXPECT_FALSE(std::is_copy_constructible_v<EAllocator>);
    EXPECT_FALSE(std::is_copy_assignable_v<EAllocator>);
    EXPECT_FALSE(std::is_move_constructible_v<EAllocator>);
    EXPECT_FALSE(std::is_move_assignable_v<EAllocator>);
}
TEST(EHeapInterfaceTest, ScopedAllocatorsManagerIsNotCopyable)
{
    EXPECT_FALSE(std::is_copy_constructible_v<ScopedAllocatorsManager>);
    EXPECT_FALSE(std::is_default_constructible_v<ScopedAllocatorsManager>);
}
TEST(EHeapInterfaceTest, AdapterTraitsMatchAllocatorRequirements)
{
    using Adapter = EAllocatorAdapter<int>;
    using Traits = std::allocator_traits<Adapter>;
    EXPECT_FALSE(Traits::propagate_on_container_copy_assignment::value);
    EXPECT_FALSE(Traits::propagate_on_container_move_assignment::value);
    EXPECT_FALSE(Traits::propagate_on_container_swap::value);
    EXPECT_TRUE(Traits::is_always_equal::value);
    EXPECT_EQ(Traits::max_size(Adapter {}), static_cast<size_t>(-1) / sizeof(int));
}
TEST(EHeapInterfaceTest, VoidAdapterRebindKeepsVoidSpecializationUsable)
{
    EAllocatorAdapter<void> voidAdapter;
    EAllocatorAdapter<void>::rebind<int>::other intAdapter;
    EAllocatorAdapter<int> converted {intAdapter};
    EAllocatorAdapter<void> convertedBack {intAdapter};
    EXPECT_TRUE(voidAdapter == convertedBack);
    EXPECT_EQ(intAdapter.max_size(), static_cast<size_t>(-1) / sizeof(int));
    EXPECT_EQ(converted.max_size(), static_cast<size_t>(-1) / sizeof(int));
}
// ---------------------------------------------------------------------------
// Allocation size matrix
// ---------------------------------------------------------------------------
class EHeapAllocSizeTest : public EHeapTest {};
TEST_F(EHeapAllocSizeTest, AllocSize_000000)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(0);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(0));
    if (0 != 0U) {
        FillPattern(block, 0, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 0, 0x1234U));
    }
    void *next = EHeap::Alloc(0 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(0));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000001)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(1);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(1));
    if (1 != 0U) {
        FillPattern(block, 1, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 1, 0x1234U));
    }
    void *next = EHeap::Alloc(1 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(1));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000002)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(2);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(2));
    if (2 != 0U) {
        FillPattern(block, 2, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 2, 0x1234U));
    }
    void *next = EHeap::Alloc(2 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(2));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000003)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(3);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(3));
    if (3 != 0U) {
        FillPattern(block, 3, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 3, 0x1234U));
    }
    void *next = EHeap::Alloc(3 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(3));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000004)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(4);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(4));
    if (4 != 0U) {
        FillPattern(block, 4, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 4, 0x1234U));
    }
    void *next = EHeap::Alloc(4 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(4));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000005)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(5);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(5));
    if (5 != 0U) {
        FillPattern(block, 5, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 5, 0x1234U));
    }
    void *next = EHeap::Alloc(5 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(5));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000006)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(6);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(6));
    if (6 != 0U) {
        FillPattern(block, 6, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 6, 0x1234U));
    }
    void *next = EHeap::Alloc(6 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(6));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000007)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(7);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(7));
    if (7 != 0U) {
        FillPattern(block, 7, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 7, 0x1234U));
    }
    void *next = EHeap::Alloc(7 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(7));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000008)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(8);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(8));
    if (8 != 0U) {
        FillPattern(block, 8, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 8, 0x1234U));
    }
    void *next = EHeap::Alloc(8 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(8));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000009)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(9);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(9));
    if (9 != 0U) {
        FillPattern(block, 9, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 9, 0x1234U));
    }
    void *next = EHeap::Alloc(9 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(9));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000010)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(10);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(10));
    if (10 != 0U) {
        FillPattern(block, 10, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 10, 0x1234U));
    }
    void *next = EHeap::Alloc(10 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(10));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000011)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(11);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(11));
    if (11 != 0U) {
        FillPattern(block, 11, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 11, 0x1234U));
    }
    void *next = EHeap::Alloc(11 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(11));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000012)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(12);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(12));
    if (12 != 0U) {
        FillPattern(block, 12, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 12, 0x1234U));
    }
    void *next = EHeap::Alloc(12 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(12));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000013)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(13);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(13));
    if (13 != 0U) {
        FillPattern(block, 13, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 13, 0x1234U));
    }
    void *next = EHeap::Alloc(13 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(13));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000014)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(14);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(14));
    if (14 != 0U) {
        FillPattern(block, 14, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 14, 0x1234U));
    }
    void *next = EHeap::Alloc(14 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(14));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000015)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(15);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(15));
    if (15 != 0U) {
        FillPattern(block, 15, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 15, 0x1234U));
    }
    void *next = EHeap::Alloc(15 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(15));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000016)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(16);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(16));
    if (16 != 0U) {
        FillPattern(block, 16, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 16, 0x1234U));
    }
    void *next = EHeap::Alloc(16 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(16));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000017)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(17);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(17));
    if (17 != 0U) {
        FillPattern(block, 17, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 17, 0x1234U));
    }
    void *next = EHeap::Alloc(17 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(17));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000023)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(23);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(23));
    if (23 != 0U) {
        FillPattern(block, 23, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 23, 0x1234U));
    }
    void *next = EHeap::Alloc(23 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(23));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000024)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(24);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(24));
    if (24 != 0U) {
        FillPattern(block, 24, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 24, 0x1234U));
    }
    void *next = EHeap::Alloc(24 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(24));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000025)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(25);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(25));
    if (25 != 0U) {
        FillPattern(block, 25, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 25, 0x1234U));
    }
    void *next = EHeap::Alloc(25 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(25));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000031)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(31);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(31));
    if (31 != 0U) {
        FillPattern(block, 31, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 31, 0x1234U));
    }
    void *next = EHeap::Alloc(31 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(31));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000032)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(32);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(32));
    if (32 != 0U) {
        FillPattern(block, 32, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 32, 0x1234U));
    }
    void *next = EHeap::Alloc(32 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(32));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000033)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(33);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(33));
    if (33 != 0U) {
        FillPattern(block, 33, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 33, 0x1234U));
    }
    void *next = EHeap::Alloc(33 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(33));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000047)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(47);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(47));
    if (47 != 0U) {
        FillPattern(block, 47, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 47, 0x1234U));
    }
    void *next = EHeap::Alloc(47 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(47));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000048)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(48);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(48));
    if (48 != 0U) {
        FillPattern(block, 48, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 48, 0x1234U));
    }
    void *next = EHeap::Alloc(48 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(48));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000049)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(49);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(49));
    if (49 != 0U) {
        FillPattern(block, 49, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 49, 0x1234U));
    }
    void *next = EHeap::Alloc(49 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(49));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000063)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(63);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(63));
    if (63 != 0U) {
        FillPattern(block, 63, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 63, 0x1234U));
    }
    void *next = EHeap::Alloc(63 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(63));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000064)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(64);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(64));
    if (64 != 0U) {
        FillPattern(block, 64, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 64, 0x1234U));
    }
    void *next = EHeap::Alloc(64 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(64));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000065)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(65);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(65));
    if (65 != 0U) {
        FillPattern(block, 65, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 65, 0x1234U));
    }
    void *next = EHeap::Alloc(65 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(65));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000079)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(79);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(79));
    if (79 != 0U) {
        FillPattern(block, 79, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 79, 0x1234U));
    }
    void *next = EHeap::Alloc(79 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(79));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000080)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(80);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(80));
    if (80 != 0U) {
        FillPattern(block, 80, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 80, 0x1234U));
    }
    void *next = EHeap::Alloc(80 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(80));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000081)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(81);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(81));
    if (81 != 0U) {
        FillPattern(block, 81, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 81, 0x1234U));
    }
    void *next = EHeap::Alloc(81 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(81));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000095)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(95);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(95));
    if (95 != 0U) {
        FillPattern(block, 95, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 95, 0x1234U));
    }
    void *next = EHeap::Alloc(95 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(95));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000096)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(96);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(96));
    if (96 != 0U) {
        FillPattern(block, 96, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 96, 0x1234U));
    }
    void *next = EHeap::Alloc(96 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(96));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000097)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(97);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(97));
    if (97 != 0U) {
        FillPattern(block, 97, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 97, 0x1234U));
    }
    void *next = EHeap::Alloc(97 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(97));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000100)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(100);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(100));
    if (100 != 0U) {
        FillPattern(block, 100, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 100, 0x1234U));
    }
    void *next = EHeap::Alloc(100 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(100));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000111)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(111);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(111));
    if (111 != 0U) {
        FillPattern(block, 111, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 111, 0x1234U));
    }
    void *next = EHeap::Alloc(111 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(111));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000112)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(112);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(112));
    if (112 != 0U) {
        FillPattern(block, 112, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 112, 0x1234U));
    }
    void *next = EHeap::Alloc(112 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(112));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000113)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(113);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(113));
    if (113 != 0U) {
        FillPattern(block, 113, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 113, 0x1234U));
    }
    void *next = EHeap::Alloc(113 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(113));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000127)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(127);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(127));
    if (127 != 0U) {
        FillPattern(block, 127, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 127, 0x1234U));
    }
    void *next = EHeap::Alloc(127 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(127));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000128)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(128);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(128));
    if (128 != 0U) {
        FillPattern(block, 128, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 128, 0x1234U));
    }
    void *next = EHeap::Alloc(128 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(128));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000129)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(129);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(129));
    if (129 != 0U) {
        FillPattern(block, 129, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 129, 0x1234U));
    }
    void *next = EHeap::Alloc(129 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(129));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000150)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(150);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(150));
    if (150 != 0U) {
        FillPattern(block, 150, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 150, 0x1234U));
    }
    void *next = EHeap::Alloc(150 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(150));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000191)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(191);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(191));
    if (191 != 0U) {
        FillPattern(block, 191, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 191, 0x1234U));
    }
    void *next = EHeap::Alloc(191 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(191));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000192)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(192);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(192));
    if (192 != 0U) {
        FillPattern(block, 192, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 192, 0x1234U));
    }
    void *next = EHeap::Alloc(192 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(192));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000193)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(193);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(193));
    if (193 != 0U) {
        FillPattern(block, 193, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 193, 0x1234U));
    }
    void *next = EHeap::Alloc(193 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(193));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000200)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(200);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(200));
    if (200 != 0U) {
        FillPattern(block, 200, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 200, 0x1234U));
    }
    void *next = EHeap::Alloc(200 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(200));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000223)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(223);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(223));
    if (223 != 0U) {
        FillPattern(block, 223, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 223, 0x1234U));
    }
    void *next = EHeap::Alloc(223 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(223));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000224)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(224);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(224));
    if (224 != 0U) {
        FillPattern(block, 224, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 224, 0x1234U));
    }
    void *next = EHeap::Alloc(224 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(224));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000225)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(225);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(225));
    if (225 != 0U) {
        FillPattern(block, 225, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 225, 0x1234U));
    }
    void *next = EHeap::Alloc(225 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(225));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000255)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(255);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(255));
    if (255 != 0U) {
        FillPattern(block, 255, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 255, 0x1234U));
    }
    void *next = EHeap::Alloc(255 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(255));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000256)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(256);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(256));
    if (256 != 0U) {
        FillPattern(block, 256, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 256, 0x1234U));
    }
    void *next = EHeap::Alloc(256 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(256));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000257)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(257);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(257));
    if (257 != 0U) {
        FillPattern(block, 257, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 257, 0x1234U));
    }
    void *next = EHeap::Alloc(257 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(257));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000300)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(300);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(300));
    if (300 != 0U) {
        FillPattern(block, 300, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 300, 0x1234U));
    }
    void *next = EHeap::Alloc(300 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(300));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000383)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(383);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(383));
    if (383 != 0U) {
        FillPattern(block, 383, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 383, 0x1234U));
    }
    void *next = EHeap::Alloc(383 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(383));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000384)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(384);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(384));
    if (384 != 0U) {
        FillPattern(block, 384, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 384, 0x1234U));
    }
    void *next = EHeap::Alloc(384 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(384));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000385)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(385);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(385));
    if (385 != 0U) {
        FillPattern(block, 385, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 385, 0x1234U));
    }
    void *next = EHeap::Alloc(385 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(385));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000400)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(400);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(400));
    if (400 != 0U) {
        FillPattern(block, 400, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 400, 0x1234U));
    }
    void *next = EHeap::Alloc(400 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(400));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000511)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(511);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(511));
    if (511 != 0U) {
        FillPattern(block, 511, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 511, 0x1234U));
    }
    void *next = EHeap::Alloc(511 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(511));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000512)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(512);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(512));
    if (512 != 0U) {
        FillPattern(block, 512, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 512, 0x1234U));
    }
    void *next = EHeap::Alloc(512 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(512));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000513)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(513);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(513));
    if (513 != 0U) {
        FillPattern(block, 513, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 513, 0x1234U));
    }
    void *next = EHeap::Alloc(513 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(513));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000600)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(600);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(600));
    if (600 != 0U) {
        FillPattern(block, 600, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 600, 0x1234U));
    }
    void *next = EHeap::Alloc(600 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(600));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000767)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(767);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(767));
    if (767 != 0U) {
        FillPattern(block, 767, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 767, 0x1234U));
    }
    void *next = EHeap::Alloc(767 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(767));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000768)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(768);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(768));
    if (768 != 0U) {
        FillPattern(block, 768, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 768, 0x1234U));
    }
    void *next = EHeap::Alloc(768 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(768));
}
TEST_F(EHeapAllocSizeTest, AllocSize_000769)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(769);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(769));
    if (769 != 0U) {
        FillPattern(block, 769, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 769, 0x1234U));
    }
    void *next = EHeap::Alloc(769 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(769));
}
TEST_F(EHeapAllocSizeTest, AllocSize_001000)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(1000);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(1000));
    if (1000 != 0U) {
        FillPattern(block, 1000, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 1000, 0x1234U));
    }
    void *next = EHeap::Alloc(1000 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(1000));
}
TEST_F(EHeapAllocSizeTest, AllocSize_001023)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(1023);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(1023));
    if (1023 != 0U) {
        FillPattern(block, 1023, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 1023, 0x1234U));
    }
    void *next = EHeap::Alloc(1023 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(1023));
}
TEST_F(EHeapAllocSizeTest, AllocSize_001024)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(1024);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(1024));
    if (1024 != 0U) {
        FillPattern(block, 1024, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 1024, 0x1234U));
    }
    void *next = EHeap::Alloc(1024 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(1024));
}
TEST_F(EHeapAllocSizeTest, AllocSize_001025)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(1025);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(1025));
    if (1025 != 0U) {
        FillPattern(block, 1025, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 1025, 0x1234U));
    }
    void *next = EHeap::Alloc(1025 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(1025));
}
TEST_F(EHeapAllocSizeTest, AllocSize_001500)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(1500);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(1500));
    if (1500 != 0U) {
        FillPattern(block, 1500, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 1500, 0x1234U));
    }
    void *next = EHeap::Alloc(1500 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(1500));
}
TEST_F(EHeapAllocSizeTest, AllocSize_002047)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(2047);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(2047));
    if (2047 != 0U) {
        FillPattern(block, 2047, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 2047, 0x1234U));
    }
    void *next = EHeap::Alloc(2047 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(2047));
}
TEST_F(EHeapAllocSizeTest, AllocSize_002048)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(2048);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(2048));
    if (2048 != 0U) {
        FillPattern(block, 2048, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 2048, 0x1234U));
    }
    void *next = EHeap::Alloc(2048 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(2048));
}
TEST_F(EHeapAllocSizeTest, AllocSize_002049)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(2049);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(2049));
    if (2049 != 0U) {
        FillPattern(block, 2049, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 2049, 0x1234U));
    }
    void *next = EHeap::Alloc(2049 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(2049));
}
TEST_F(EHeapAllocSizeTest, AllocSize_003000)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(3000);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(3000));
    if (3000 != 0U) {
        FillPattern(block, 3000, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 3000, 0x1234U));
    }
    void *next = EHeap::Alloc(3000 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(3000));
}
TEST_F(EHeapAllocSizeTest, AllocSize_004095)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(4095);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(4095));
    if (4095 != 0U) {
        FillPattern(block, 4095, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 4095, 0x1234U));
    }
    void *next = EHeap::Alloc(4095 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(4095));
}
TEST_F(EHeapAllocSizeTest, AllocSize_004096)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(4096);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(4096));
    if (4096 != 0U) {
        FillPattern(block, 4096, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 4096, 0x1234U));
    }
    void *next = EHeap::Alloc(4096 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(4096));
}
TEST_F(EHeapAllocSizeTest, AllocSize_004097)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(4097);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(4097));
    if (4097 != 0U) {
        FillPattern(block, 4097, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 4097, 0x1234U));
    }
    void *next = EHeap::Alloc(4097 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(4097));
}
TEST_F(EHeapAllocSizeTest, AllocSize_006000)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(6000);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(6000));
    if (6000 != 0U) {
        FillPattern(block, 6000, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 6000, 0x1234U));
    }
    void *next = EHeap::Alloc(6000 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(6000));
}
TEST_F(EHeapAllocSizeTest, AllocSize_008191)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(8191);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(8191));
    if (8191 != 0U) {
        FillPattern(block, 8191, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 8191, 0x1234U));
    }
    void *next = EHeap::Alloc(8191 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(8191));
}
TEST_F(EHeapAllocSizeTest, AllocSize_008192)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(8192);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(8192));
    if (8192 != 0U) {
        FillPattern(block, 8192, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 8192, 0x1234U));
    }
    void *next = EHeap::Alloc(8192 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(8192));
}
TEST_F(EHeapAllocSizeTest, AllocSize_008193)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(8193);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(8193));
    if (8193 != 0U) {
        FillPattern(block, 8193, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 8193, 0x1234U));
    }
    void *next = EHeap::Alloc(8193 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(8193));
}
TEST_F(EHeapAllocSizeTest, AllocSize_012000)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(12000);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(12000));
    if (12000 != 0U) {
        FillPattern(block, 12000, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 12000, 0x1234U));
    }
    void *next = EHeap::Alloc(12000 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(12000));
}
TEST_F(EHeapAllocSizeTest, AllocSize_016383)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(16383);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(16383));
    if (16383 != 0U) {
        FillPattern(block, 16383, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 16383, 0x1234U));
    }
    void *next = EHeap::Alloc(16383 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(16383));
}
TEST_F(EHeapAllocSizeTest, AllocSize_016384)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(16384);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(16384));
    if (16384 != 0U) {
        FillPattern(block, 16384, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 16384, 0x1234U));
    }
    void *next = EHeap::Alloc(16384 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(16384));
}
TEST_F(EHeapAllocSizeTest, AllocSize_016385)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(16385);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(16385));
    if (16385 != 0U) {
        FillPattern(block, 16385, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 16385, 0x1234U));
    }
    void *next = EHeap::Alloc(16385 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(16385));
}
TEST_F(EHeapAllocSizeTest, AllocSize_020000)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(20000);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(20000));
    if (20000 != 0U) {
        FillPattern(block, 20000, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 20000, 0x1234U));
    }
    void *next = EHeap::Alloc(20000 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(20000));
}
TEST_F(EHeapAllocSizeTest, AllocSize_032767)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(32767);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(32767));
    if (32767 != 0U) {
        FillPattern(block, 32767, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 32767, 0x1234U));
    }
    void *next = EHeap::Alloc(32767 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(32767));
}
TEST_F(EHeapAllocSizeTest, AllocSize_032768)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(32768);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(32768));
    if (32768 != 0U) {
        FillPattern(block, 32768, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 32768, 0x1234U));
    }
    void *next = EHeap::Alloc(32768 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(32768));
}
TEST_F(EHeapAllocSizeTest, AllocSize_032769)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(32769);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(32769));
    if (32769 != 0U) {
        FillPattern(block, 32769, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 32769, 0x1234U));
    }
    void *next = EHeap::Alloc(32769 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(32769));
}
TEST_F(EHeapAllocSizeTest, AllocSize_050000)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(50000);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(50000));
    if (50000 != 0U) {
        FillPattern(block, 50000, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 50000, 0x1234U));
    }
    void *next = EHeap::Alloc(50000 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(50000));
}
TEST_F(EHeapAllocSizeTest, AllocSize_065535)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(65535);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(65535));
    if (65535 != 0U) {
        FillPattern(block, 65535, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 65535, 0x1234U));
    }
    void *next = EHeap::Alloc(65535 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(65535));
}
TEST_F(EHeapAllocSizeTest, AllocSize_065536)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(65536);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(65536));
    if (65536 != 0U) {
        FillPattern(block, 65536, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 65536, 0x1234U));
    }
    void *next = EHeap::Alloc(65536 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(65536));
}
TEST_F(EHeapAllocSizeTest, AllocSize_065537)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(65537);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(65537));
    if (65537 != 0U) {
        FillPattern(block, 65537, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 65537, 0x1234U));
    }
    void *next = EHeap::Alloc(65537 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(65537));
}
TEST_F(EHeapAllocSizeTest, AllocSize_100000)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(100000);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(100000));
    if (100000 != 0U) {
        FillPattern(block, 100000, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 100000, 0x1234U));
    }
    void *next = EHeap::Alloc(100000 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(100000));
}
TEST_F(EHeapAllocSizeTest, AllocSize_131071)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(131071);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(131071));
    if (131071 != 0U) {
        FillPattern(block, 131071, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 131071, 0x1234U));
    }
    void *next = EHeap::Alloc(131071 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(131071));
}
TEST_F(EHeapAllocSizeTest, AllocSize_131072)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(131072);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(131072));
    if (131072 != 0U) {
        FillPattern(block, 131072, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 131072, 0x1234U));
    }
    void *next = EHeap::Alloc(131072 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(131072));
}
TEST_F(EHeapAllocSizeTest, AllocSize_131073)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(131073);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(131073));
    if (131073 != 0U) {
        FillPattern(block, 131073, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 131073, 0x1234U));
    }
    void *next = EHeap::Alloc(131073 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(131073));
}
TEST_F(EHeapAllocSizeTest, AllocSize_196608)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(196608);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(196608));
    if (196608 != 0U) {
        FillPattern(block, 196608, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 196608, 0x1234U));
    }
    void *next = EHeap::Alloc(196608 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(196608));
}
TEST_F(EHeapAllocSizeTest, AllocSize_262144)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(262144);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(262144));
    if (262144 != 0U) {
        FillPattern(block, 262144, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 262144, 0x1234U));
    }
    void *next = EHeap::Alloc(262144 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(262144));
}
TEST_F(EHeapAllocSizeTest, AllocSize_300000)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(300000);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(300000));
    if (300000 != 0U) {
        FillPattern(block, 300000, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 300000, 0x1234U));
    }
    void *next = EHeap::Alloc(300000 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(300000));
}
TEST_F(EHeapAllocSizeTest, AllocSize_400000)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(400000);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(400000));
    if (400000 != 0U) {
        FillPattern(block, 400000, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 400000, 0x1234U));
    }
    void *next = EHeap::Alloc(400000 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(400000));
}
TEST_F(EHeapAllocSizeTest, AllocSize_500000)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(500000);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(500000));
    if (500000 != 0U) {
        FillPattern(block, 500000, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 500000, 0x1234U));
    }
    void *next = EHeap::Alloc(500000 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(500000));
}
TEST_F(EHeapAllocSizeTest, AllocSize_1000000)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(1000000);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(1000000));
    if (1000000 != 0U) {
        FillPattern(block, 1000000, 0x1234U);
        EXPECT_TRUE(PatternMatches(block, 1000000, 0x1234U));
    }
    void *next = EHeap::Alloc(1000000 + 1U);
    ASSERT_NE(next, nullptr);
    EXPECT_GE(Addr(next), Addr(block) + AlignUpBytes(1000000));
}
// ---------------------------------------------------------------------------
// Byte pattern and neighbour integrity matrix
// ---------------------------------------------------------------------------
class EHeapPatternTest : public EHeapTest {};
TEST_F(EHeapPatternTest, BytePattern_00_0001)
{
    const size_t size = 1;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0x00U);
    EXPECT_TRUE(AllBytes(target, size, 0x00U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0x00U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_00_0003)
{
    const size_t size = 3;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0x00U);
    EXPECT_TRUE(AllBytes(target, size, 0x00U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0x00U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_00_0008)
{
    const size_t size = 8;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0x00U);
    EXPECT_TRUE(AllBytes(target, size, 0x00U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0x00U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_00_0017)
{
    const size_t size = 17;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0x00U);
    EXPECT_TRUE(AllBytes(target, size, 0x00U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0x00U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_00_0064)
{
    const size_t size = 64;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0x00U);
    EXPECT_TRUE(AllBytes(target, size, 0x00U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0x00U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_00_0257)
{
    const size_t size = 257;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0x00U);
    EXPECT_TRUE(AllBytes(target, size, 0x00U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0x00U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_00_1024)
{
    const size_t size = 1024;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0x00U);
    EXPECT_TRUE(AllBytes(target, size, 0x00U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0x00U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_01_0001)
{
    const size_t size = 1;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0x01U);
    EXPECT_TRUE(AllBytes(target, size, 0x01U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0x01U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_01_0003)
{
    const size_t size = 3;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0x01U);
    EXPECT_TRUE(AllBytes(target, size, 0x01U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0x01U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_01_0008)
{
    const size_t size = 8;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0x01U);
    EXPECT_TRUE(AllBytes(target, size, 0x01U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0x01U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_01_0017)
{
    const size_t size = 17;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0x01U);
    EXPECT_TRUE(AllBytes(target, size, 0x01U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0x01U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_01_0064)
{
    const size_t size = 64;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0x01U);
    EXPECT_TRUE(AllBytes(target, size, 0x01U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0x01U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_01_0257)
{
    const size_t size = 257;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0x01U);
    EXPECT_TRUE(AllBytes(target, size, 0x01U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0x01U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_01_1024)
{
    const size_t size = 1024;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0x01U);
    EXPECT_TRUE(AllBytes(target, size, 0x01U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0x01U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_7F_0001)
{
    const size_t size = 1;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0x7FU);
    EXPECT_TRUE(AllBytes(target, size, 0x7FU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0x7FU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_7F_0003)
{
    const size_t size = 3;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0x7FU);
    EXPECT_TRUE(AllBytes(target, size, 0x7FU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0x7FU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_7F_0008)
{
    const size_t size = 8;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0x7FU);
    EXPECT_TRUE(AllBytes(target, size, 0x7FU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0x7FU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_7F_0017)
{
    const size_t size = 17;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0x7FU);
    EXPECT_TRUE(AllBytes(target, size, 0x7FU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0x7FU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_7F_0064)
{
    const size_t size = 64;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0x7FU);
    EXPECT_TRUE(AllBytes(target, size, 0x7FU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0x7FU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_7F_0257)
{
    const size_t size = 257;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0x7FU);
    EXPECT_TRUE(AllBytes(target, size, 0x7FU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0x7FU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_7F_1024)
{
    const size_t size = 1024;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0x7FU);
    EXPECT_TRUE(AllBytes(target, size, 0x7FU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0x7FU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_80_0001)
{
    const size_t size = 1;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0x80U);
    EXPECT_TRUE(AllBytes(target, size, 0x80U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0x80U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_80_0003)
{
    const size_t size = 3;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0x80U);
    EXPECT_TRUE(AllBytes(target, size, 0x80U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0x80U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_80_0008)
{
    const size_t size = 8;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0x80U);
    EXPECT_TRUE(AllBytes(target, size, 0x80U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0x80U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_80_0017)
{
    const size_t size = 17;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0x80U);
    EXPECT_TRUE(AllBytes(target, size, 0x80U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0x80U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_80_0064)
{
    const size_t size = 64;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0x80U);
    EXPECT_TRUE(AllBytes(target, size, 0x80U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0x80U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_80_0257)
{
    const size_t size = 257;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0x80U);
    EXPECT_TRUE(AllBytes(target, size, 0x80U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0x80U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_80_1024)
{
    const size_t size = 1024;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0x80U);
    EXPECT_TRUE(AllBytes(target, size, 0x80U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0x80U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_FF_0001)
{
    const size_t size = 1;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0xFFU);
    EXPECT_TRUE(AllBytes(target, size, 0xFFU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0xFFU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_FF_0003)
{
    const size_t size = 3;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0xFFU);
    EXPECT_TRUE(AllBytes(target, size, 0xFFU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0xFFU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_FF_0008)
{
    const size_t size = 8;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0xFFU);
    EXPECT_TRUE(AllBytes(target, size, 0xFFU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0xFFU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_FF_0017)
{
    const size_t size = 17;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0xFFU);
    EXPECT_TRUE(AllBytes(target, size, 0xFFU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0xFFU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_FF_0064)
{
    const size_t size = 64;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0xFFU);
    EXPECT_TRUE(AllBytes(target, size, 0xFFU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0xFFU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_FF_0257)
{
    const size_t size = 257;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0xFFU);
    EXPECT_TRUE(AllBytes(target, size, 0xFFU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0xFFU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_FF_1024)
{
    const size_t size = 1024;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0xFFU);
    EXPECT_TRUE(AllBytes(target, size, 0xFFU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0xFFU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_A5_0001)
{
    const size_t size = 1;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0xA5U);
    EXPECT_TRUE(AllBytes(target, size, 0xA5U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0xA5U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_A5_0003)
{
    const size_t size = 3;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0xA5U);
    EXPECT_TRUE(AllBytes(target, size, 0xA5U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0xA5U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_A5_0008)
{
    const size_t size = 8;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0xA5U);
    EXPECT_TRUE(AllBytes(target, size, 0xA5U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0xA5U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_A5_0017)
{
    const size_t size = 17;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0xA5U);
    EXPECT_TRUE(AllBytes(target, size, 0xA5U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0xA5U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_A5_0064)
{
    const size_t size = 64;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0xA5U);
    EXPECT_TRUE(AllBytes(target, size, 0xA5U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0xA5U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_A5_0257)
{
    const size_t size = 257;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0xA5U);
    EXPECT_TRUE(AllBytes(target, size, 0xA5U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0xA5U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_A5_1024)
{
    const size_t size = 1024;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0xA5U);
    EXPECT_TRUE(AllBytes(target, size, 0xA5U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0xA5U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_5A_0001)
{
    const size_t size = 1;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0x5AU);
    EXPECT_TRUE(AllBytes(target, size, 0x5AU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0x5AU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_5A_0003)
{
    const size_t size = 3;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0x5AU);
    EXPECT_TRUE(AllBytes(target, size, 0x5AU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0x5AU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_5A_0008)
{
    const size_t size = 8;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0x5AU);
    EXPECT_TRUE(AllBytes(target, size, 0x5AU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0x5AU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_5A_0017)
{
    const size_t size = 17;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0x5AU);
    EXPECT_TRUE(AllBytes(target, size, 0x5AU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0x5AU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_5A_0064)
{
    const size_t size = 64;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0x5AU);
    EXPECT_TRUE(AllBytes(target, size, 0x5AU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0x5AU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_5A_0257)
{
    const size_t size = 257;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0x5AU);
    EXPECT_TRUE(AllBytes(target, size, 0x5AU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0x5AU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_5A_1024)
{
    const size_t size = 1024;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0x5AU);
    EXPECT_TRUE(AllBytes(target, size, 0x5AU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0x5AU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_3C_0001)
{
    const size_t size = 1;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0x3CU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0x3CU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_3C_0003)
{
    const size_t size = 3;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0x3CU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0x3CU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_3C_0008)
{
    const size_t size = 8;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0x3CU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0x3CU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_3C_0017)
{
    const size_t size = 17;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0x3CU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0x3CU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_3C_0064)
{
    const size_t size = 64;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0x3CU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0x3CU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_3C_0257)
{
    const size_t size = 257;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0x3CU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0x3CU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_3C_1024)
{
    const size_t size = 1024;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0x3CU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0x3CU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_C3_0001)
{
    const size_t size = 1;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0xC3U);
    EXPECT_TRUE(AllBytes(target, size, 0xC3U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0xC3U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_C3_0003)
{
    const size_t size = 3;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0xC3U);
    EXPECT_TRUE(AllBytes(target, size, 0xC3U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0xC3U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_C3_0008)
{
    const size_t size = 8;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0xC3U);
    EXPECT_TRUE(AllBytes(target, size, 0xC3U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0xC3U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_C3_0017)
{
    const size_t size = 17;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0xC3U);
    EXPECT_TRUE(AllBytes(target, size, 0xC3U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0xC3U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_C3_0064)
{
    const size_t size = 64;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0xC3U);
    EXPECT_TRUE(AllBytes(target, size, 0xC3U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0xC3U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_C3_0257)
{
    const size_t size = 257;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0xC3U);
    EXPECT_TRUE(AllBytes(target, size, 0xC3U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0xC3U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_C3_1024)
{
    const size_t size = 1024;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0xC3U);
    EXPECT_TRUE(AllBytes(target, size, 0xC3U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0xC3U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_0F_0001)
{
    const size_t size = 1;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0x0FU);
    EXPECT_TRUE(AllBytes(target, size, 0x0FU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0x0FU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_0F_0003)
{
    const size_t size = 3;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0x0FU);
    EXPECT_TRUE(AllBytes(target, size, 0x0FU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0x0FU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_0F_0008)
{
    const size_t size = 8;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0x0FU);
    EXPECT_TRUE(AllBytes(target, size, 0x0FU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0x0FU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_0F_0017)
{
    const size_t size = 17;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0x0FU);
    EXPECT_TRUE(AllBytes(target, size, 0x0FU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0x0FU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_0F_0064)
{
    const size_t size = 64;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0x0FU);
    EXPECT_TRUE(AllBytes(target, size, 0x0FU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0x0FU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_0F_0257)
{
    const size_t size = 257;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0x0FU);
    EXPECT_TRUE(AllBytes(target, size, 0x0FU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0x0FU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_0F_1024)
{
    const size_t size = 1024;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0x0FU);
    EXPECT_TRUE(AllBytes(target, size, 0x0FU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0x0FU));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_F0_0001)
{
    const size_t size = 1;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0xF0U);
    EXPECT_TRUE(AllBytes(target, size, 0xF0U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0xF0U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_F0_0003)
{
    const size_t size = 3;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0xF0U);
    EXPECT_TRUE(AllBytes(target, size, 0xF0U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0xF0U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_F0_0008)
{
    const size_t size = 8;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0xF0U);
    EXPECT_TRUE(AllBytes(target, size, 0xF0U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0xF0U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_F0_0017)
{
    const size_t size = 17;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0xF0U);
    EXPECT_TRUE(AllBytes(target, size, 0xF0U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0xF0U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_F0_0064)
{
    const size_t size = 64;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0xF0U);
    EXPECT_TRUE(AllBytes(target, size, 0xF0U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0xF0U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_F0_0257)
{
    const size_t size = 257;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0xF0U);
    EXPECT_TRUE(AllBytes(target, size, 0xF0U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0xF0U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, BytePattern_F0_1024)
{
    const size_t size = 1024;
    const size_t before = EHeap::AllocatedSize();
    void *target = EHeap::Alloc(size);
    ASSERT_NE(target, nullptr);
    void *canary = EHeap::Alloc(64U);
    ASSERT_NE(canary, nullptr);
    FillByte(canary, 64U, 0xCCU);
    FillByte(target, size, 0xF0U);
    EXPECT_TRUE(AllBytes(target, size, 0xF0U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U);
    void *trailer = EHeap::Alloc(32U);
    ASSERT_NE(trailer, nullptr);
    FillByte(trailer, 32U, 0x3CU);
    EXPECT_TRUE(AllBytes(target, size, 0xF0U));
    EXPECT_TRUE(AllBytes(canary, 64U, 0xCCU));
    EXPECT_TRUE(AllBytes(trailer, 32U, 0x3CU));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size) + 64U + 32U);
}
TEST_F(EHeapPatternTest, SeedPattern_0001_0001)
{
    const size_t size = 1;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    FillPattern(block, size, 1U);
    EXPECT_TRUE(PatternMatches(block, size, 1U));
    EXPECT_FALSE(PatternMatches(block, size, 1U + 1U));
    void *neighbour = EHeap::Alloc(size + 8U);
    ASSERT_NE(neighbour, nullptr);
    FillPattern(neighbour, size + 8U, 1U + 4096U);
    EXPECT_TRUE(PatternMatches(block, size, 1U));
    EXPECT_TRUE(PatternMatches(neighbour, size + 8U, 1U + 4096U));
    EXPECT_FALSE(RangesOverlap(block, size, neighbour, size + 8U));
    EXPECT_TRUE(IsAligned(block));
    EXPECT_TRUE(IsAligned(neighbour));
}
TEST_F(EHeapPatternTest, SeedPattern_0001_0003)
{
    const size_t size = 3;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    FillPattern(block, size, 1U);
    EXPECT_TRUE(PatternMatches(block, size, 1U));
    EXPECT_FALSE(PatternMatches(block, size, 1U + 1U));
    void *neighbour = EHeap::Alloc(size + 8U);
    ASSERT_NE(neighbour, nullptr);
    FillPattern(neighbour, size + 8U, 1U + 4096U);
    EXPECT_TRUE(PatternMatches(block, size, 1U));
    EXPECT_TRUE(PatternMatches(neighbour, size + 8U, 1U + 4096U));
    EXPECT_FALSE(RangesOverlap(block, size, neighbour, size + 8U));
    EXPECT_TRUE(IsAligned(block));
    EXPECT_TRUE(IsAligned(neighbour));
}
TEST_F(EHeapPatternTest, SeedPattern_0001_0008)
{
    const size_t size = 8;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    FillPattern(block, size, 1U);
    EXPECT_TRUE(PatternMatches(block, size, 1U));
    EXPECT_FALSE(PatternMatches(block, size, 1U + 1U));
    void *neighbour = EHeap::Alloc(size + 8U);
    ASSERT_NE(neighbour, nullptr);
    FillPattern(neighbour, size + 8U, 1U + 4096U);
    EXPECT_TRUE(PatternMatches(block, size, 1U));
    EXPECT_TRUE(PatternMatches(neighbour, size + 8U, 1U + 4096U));
    EXPECT_FALSE(RangesOverlap(block, size, neighbour, size + 8U));
    EXPECT_TRUE(IsAligned(block));
    EXPECT_TRUE(IsAligned(neighbour));
}
TEST_F(EHeapPatternTest, SeedPattern_0001_0017)
{
    const size_t size = 17;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    FillPattern(block, size, 1U);
    EXPECT_TRUE(PatternMatches(block, size, 1U));
    EXPECT_FALSE(PatternMatches(block, size, 1U + 1U));
    void *neighbour = EHeap::Alloc(size + 8U);
    ASSERT_NE(neighbour, nullptr);
    FillPattern(neighbour, size + 8U, 1U + 4096U);
    EXPECT_TRUE(PatternMatches(block, size, 1U));
    EXPECT_TRUE(PatternMatches(neighbour, size + 8U, 1U + 4096U));
    EXPECT_FALSE(RangesOverlap(block, size, neighbour, size + 8U));
    EXPECT_TRUE(IsAligned(block));
    EXPECT_TRUE(IsAligned(neighbour));
}
TEST_F(EHeapPatternTest, SeedPattern_0001_0064)
{
    const size_t size = 64;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    FillPattern(block, size, 1U);
    EXPECT_TRUE(PatternMatches(block, size, 1U));
    EXPECT_FALSE(PatternMatches(block, size, 1U + 1U));
    void *neighbour = EHeap::Alloc(size + 8U);
    ASSERT_NE(neighbour, nullptr);
    FillPattern(neighbour, size + 8U, 1U + 4096U);
    EXPECT_TRUE(PatternMatches(block, size, 1U));
    EXPECT_TRUE(PatternMatches(neighbour, size + 8U, 1U + 4096U));
    EXPECT_FALSE(RangesOverlap(block, size, neighbour, size + 8U));
    EXPECT_TRUE(IsAligned(block));
    EXPECT_TRUE(IsAligned(neighbour));
}
TEST_F(EHeapPatternTest, SeedPattern_0001_0257)
{
    const size_t size = 257;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    FillPattern(block, size, 1U);
    EXPECT_TRUE(PatternMatches(block, size, 1U));
    EXPECT_FALSE(PatternMatches(block, size, 1U + 1U));
    void *neighbour = EHeap::Alloc(size + 8U);
    ASSERT_NE(neighbour, nullptr);
    FillPattern(neighbour, size + 8U, 1U + 4096U);
    EXPECT_TRUE(PatternMatches(block, size, 1U));
    EXPECT_TRUE(PatternMatches(neighbour, size + 8U, 1U + 4096U));
    EXPECT_FALSE(RangesOverlap(block, size, neighbour, size + 8U));
    EXPECT_TRUE(IsAligned(block));
    EXPECT_TRUE(IsAligned(neighbour));
}
TEST_F(EHeapPatternTest, SeedPattern_0001_1024)
{
    const size_t size = 1024;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    FillPattern(block, size, 1U);
    EXPECT_TRUE(PatternMatches(block, size, 1U));
    EXPECT_FALSE(PatternMatches(block, size, 1U + 1U));
    void *neighbour = EHeap::Alloc(size + 8U);
    ASSERT_NE(neighbour, nullptr);
    FillPattern(neighbour, size + 8U, 1U + 4096U);
    EXPECT_TRUE(PatternMatches(block, size, 1U));
    EXPECT_TRUE(PatternMatches(neighbour, size + 8U, 1U + 4096U));
    EXPECT_FALSE(RangesOverlap(block, size, neighbour, size + 8U));
    EXPECT_TRUE(IsAligned(block));
    EXPECT_TRUE(IsAligned(neighbour));
}
TEST_F(EHeapPatternTest, SeedPattern_0002_0001)
{
    const size_t size = 1;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    FillPattern(block, size, 2U);
    EXPECT_TRUE(PatternMatches(block, size, 2U));
    EXPECT_FALSE(PatternMatches(block, size, 2U + 1U));
    void *neighbour = EHeap::Alloc(size + 8U);
    ASSERT_NE(neighbour, nullptr);
    FillPattern(neighbour, size + 8U, 2U + 4096U);
    EXPECT_TRUE(PatternMatches(block, size, 2U));
    EXPECT_TRUE(PatternMatches(neighbour, size + 8U, 2U + 4096U));
    EXPECT_FALSE(RangesOverlap(block, size, neighbour, size + 8U));
    EXPECT_TRUE(IsAligned(block));
    EXPECT_TRUE(IsAligned(neighbour));
}
TEST_F(EHeapPatternTest, SeedPattern_0002_0003)
{
    const size_t size = 3;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    FillPattern(block, size, 2U);
    EXPECT_TRUE(PatternMatches(block, size, 2U));
    EXPECT_FALSE(PatternMatches(block, size, 2U + 1U));
    void *neighbour = EHeap::Alloc(size + 8U);
    ASSERT_NE(neighbour, nullptr);
    FillPattern(neighbour, size + 8U, 2U + 4096U);
    EXPECT_TRUE(PatternMatches(block, size, 2U));
    EXPECT_TRUE(PatternMatches(neighbour, size + 8U, 2U + 4096U));
    EXPECT_FALSE(RangesOverlap(block, size, neighbour, size + 8U));
    EXPECT_TRUE(IsAligned(block));
    EXPECT_TRUE(IsAligned(neighbour));
}
TEST_F(EHeapPatternTest, SeedPattern_0002_0008)
{
    const size_t size = 8;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    FillPattern(block, size, 2U);
    EXPECT_TRUE(PatternMatches(block, size, 2U));
    EXPECT_FALSE(PatternMatches(block, size, 2U + 1U));
    void *neighbour = EHeap::Alloc(size + 8U);
    ASSERT_NE(neighbour, nullptr);
    FillPattern(neighbour, size + 8U, 2U + 4096U);
    EXPECT_TRUE(PatternMatches(block, size, 2U));
    EXPECT_TRUE(PatternMatches(neighbour, size + 8U, 2U + 4096U));
    EXPECT_FALSE(RangesOverlap(block, size, neighbour, size + 8U));
    EXPECT_TRUE(IsAligned(block));
    EXPECT_TRUE(IsAligned(neighbour));
}
TEST_F(EHeapPatternTest, SeedPattern_0002_0017)
{
    const size_t size = 17;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    FillPattern(block, size, 2U);
    EXPECT_TRUE(PatternMatches(block, size, 2U));
    EXPECT_FALSE(PatternMatches(block, size, 2U + 1U));
    void *neighbour = EHeap::Alloc(size + 8U);
    ASSERT_NE(neighbour, nullptr);
    FillPattern(neighbour, size + 8U, 2U + 4096U);
    EXPECT_TRUE(PatternMatches(block, size, 2U));
    EXPECT_TRUE(PatternMatches(neighbour, size + 8U, 2U + 4096U));
    EXPECT_FALSE(RangesOverlap(block, size, neighbour, size + 8U));
    EXPECT_TRUE(IsAligned(block));
    EXPECT_TRUE(IsAligned(neighbour));
}
TEST_F(EHeapPatternTest, SeedPattern_0002_0064)
{
    const size_t size = 64;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    FillPattern(block, size, 2U);
    EXPECT_TRUE(PatternMatches(block, size, 2U));
    EXPECT_FALSE(PatternMatches(block, size, 2U + 1U));
    void *neighbour = EHeap::Alloc(size + 8U);
    ASSERT_NE(neighbour, nullptr);
    FillPattern(neighbour, size + 8U, 2U + 4096U);
    EXPECT_TRUE(PatternMatches(block, size, 2U));
    EXPECT_TRUE(PatternMatches(neighbour, size + 8U, 2U + 4096U));
    EXPECT_FALSE(RangesOverlap(block, size, neighbour, size + 8U));
    EXPECT_TRUE(IsAligned(block));
    EXPECT_TRUE(IsAligned(neighbour));
}
TEST_F(EHeapPatternTest, SeedPattern_0002_0257)
{
    const size_t size = 257;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    FillPattern(block, size, 2U);
    EXPECT_TRUE(PatternMatches(block, size, 2U));
    EXPECT_FALSE(PatternMatches(block, size, 2U + 1U));
    void *neighbour = EHeap::Alloc(size + 8U);
    ASSERT_NE(neighbour, nullptr);
    FillPattern(neighbour, size + 8U, 2U + 4096U);
    EXPECT_TRUE(PatternMatches(block, size, 2U));
    EXPECT_TRUE(PatternMatches(neighbour, size + 8U, 2U + 4096U));
    EXPECT_FALSE(RangesOverlap(block, size, neighbour, size + 8U));
    EXPECT_TRUE(IsAligned(block));
    EXPECT_TRUE(IsAligned(neighbour));
}
TEST_F(EHeapPatternTest, SeedPattern_0002_1024)
{
    const size_t size = 1024;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    FillPattern(block, size, 2U);
    EXPECT_TRUE(PatternMatches(block, size, 2U));
    EXPECT_FALSE(PatternMatches(block, size, 2U + 1U));
    void *neighbour = EHeap::Alloc(size + 8U);
    ASSERT_NE(neighbour, nullptr);
    FillPattern(neighbour, size + 8U, 2U + 4096U);
    EXPECT_TRUE(PatternMatches(block, size, 2U));
    EXPECT_TRUE(PatternMatches(neighbour, size + 8U, 2U + 4096U));
    EXPECT_FALSE(RangesOverlap(block, size, neighbour, size + 8U));
    EXPECT_TRUE(IsAligned(block));
    EXPECT_TRUE(IsAligned(neighbour));
}
TEST_F(EHeapPatternTest, SeedPattern_0003_0001)
{
    const size_t size = 1;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    FillPattern(block, size, 3U);
    EXPECT_TRUE(PatternMatches(block, size, 3U));
    EXPECT_FALSE(PatternMatches(block, size, 3U + 1U));
    void *neighbour = EHeap::Alloc(size + 8U);
    ASSERT_NE(neighbour, nullptr);
    FillPattern(neighbour, size + 8U, 3U + 4096U);
    EXPECT_TRUE(PatternMatches(block, size, 3U));
    EXPECT_TRUE(PatternMatches(neighbour, size + 8U, 3U + 4096U));
    EXPECT_FALSE(RangesOverlap(block, size, neighbour, size + 8U));
    EXPECT_TRUE(IsAligned(block));
    EXPECT_TRUE(IsAligned(neighbour));
}
TEST_F(EHeapPatternTest, SeedPattern_0003_0003)
{
    const size_t size = 3;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    FillPattern(block, size, 3U);
    EXPECT_TRUE(PatternMatches(block, size, 3U));
    EXPECT_FALSE(PatternMatches(block, size, 3U + 1U));
    void *neighbour = EHeap::Alloc(size + 8U);
    ASSERT_NE(neighbour, nullptr);
    FillPattern(neighbour, size + 8U, 3U + 4096U);
    EXPECT_TRUE(PatternMatches(block, size, 3U));
    EXPECT_TRUE(PatternMatches(neighbour, size + 8U, 3U + 4096U));
    EXPECT_FALSE(RangesOverlap(block, size, neighbour, size + 8U));
    EXPECT_TRUE(IsAligned(block));
    EXPECT_TRUE(IsAligned(neighbour));
}
TEST_F(EHeapPatternTest, SeedPattern_0003_0008)
{
    const size_t size = 8;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    FillPattern(block, size, 3U);
    EXPECT_TRUE(PatternMatches(block, size, 3U));
    EXPECT_FALSE(PatternMatches(block, size, 3U + 1U));
    void *neighbour = EHeap::Alloc(size + 8U);
    ASSERT_NE(neighbour, nullptr);
    FillPattern(neighbour, size + 8U, 3U + 4096U);
    EXPECT_TRUE(PatternMatches(block, size, 3U));
    EXPECT_TRUE(PatternMatches(neighbour, size + 8U, 3U + 4096U));
    EXPECT_FALSE(RangesOverlap(block, size, neighbour, size + 8U));
    EXPECT_TRUE(IsAligned(block));
    EXPECT_TRUE(IsAligned(neighbour));
}
TEST_F(EHeapPatternTest, SeedPattern_0003_0017)
{
    const size_t size = 17;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    FillPattern(block, size, 3U);
    EXPECT_TRUE(PatternMatches(block, size, 3U));
    EXPECT_FALSE(PatternMatches(block, size, 3U + 1U));
    void *neighbour = EHeap::Alloc(size + 8U);
    ASSERT_NE(neighbour, nullptr);
    FillPattern(neighbour, size + 8U, 3U + 4096U);
    EXPECT_TRUE(PatternMatches(block, size, 3U));
    EXPECT_TRUE(PatternMatches(neighbour, size + 8U, 3U + 4096U));
    EXPECT_FALSE(RangesOverlap(block, size, neighbour, size + 8U));
    EXPECT_TRUE(IsAligned(block));
    EXPECT_TRUE(IsAligned(neighbour));
}
TEST_F(EHeapPatternTest, SeedPattern_0003_0064)
{
    const size_t size = 64;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    FillPattern(block, size, 3U);
    EXPECT_TRUE(PatternMatches(block, size, 3U));
    EXPECT_FALSE(PatternMatches(block, size, 3U + 1U));
    void *neighbour = EHeap::Alloc(size + 8U);
    ASSERT_NE(neighbour, nullptr);
    FillPattern(neighbour, size + 8U, 3U + 4096U);
    EXPECT_TRUE(PatternMatches(block, size, 3U));
    EXPECT_TRUE(PatternMatches(neighbour, size + 8U, 3U + 4096U));
    EXPECT_FALSE(RangesOverlap(block, size, neighbour, size + 8U));
    EXPECT_TRUE(IsAligned(block));
    EXPECT_TRUE(IsAligned(neighbour));
}
TEST_F(EHeapPatternTest, SeedPattern_0003_0257)
{
    const size_t size = 257;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    FillPattern(block, size, 3U);
    EXPECT_TRUE(PatternMatches(block, size, 3U));
    EXPECT_FALSE(PatternMatches(block, size, 3U + 1U));
    void *neighbour = EHeap::Alloc(size + 8U);
    ASSERT_NE(neighbour, nullptr);
    FillPattern(neighbour, size + 8U, 3U + 4096U);
    EXPECT_TRUE(PatternMatches(block, size, 3U));
    EXPECT_TRUE(PatternMatches(neighbour, size + 8U, 3U + 4096U));
    EXPECT_FALSE(RangesOverlap(block, size, neighbour, size + 8U));
    EXPECT_TRUE(IsAligned(block));
    EXPECT_TRUE(IsAligned(neighbour));
}
TEST_F(EHeapPatternTest, SeedPattern_0003_1024)
{
    const size_t size = 1024;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    FillPattern(block, size, 3U);
    EXPECT_TRUE(PatternMatches(block, size, 3U));
    EXPECT_FALSE(PatternMatches(block, size, 3U + 1U));
    void *neighbour = EHeap::Alloc(size + 8U);
    ASSERT_NE(neighbour, nullptr);
    FillPattern(neighbour, size + 8U, 3U + 4096U);
    EXPECT_TRUE(PatternMatches(block, size, 3U));
    EXPECT_TRUE(PatternMatches(neighbour, size + 8U, 3U + 4096U));
    EXPECT_FALSE(RangesOverlap(block, size, neighbour, size + 8U));
    EXPECT_TRUE(IsAligned(block));
    EXPECT_TRUE(IsAligned(neighbour));
}
TEST_F(EHeapPatternTest, SeedPattern_0007_0001)
{
    const size_t size = 1;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    FillPattern(block, size, 7U);
    EXPECT_TRUE(PatternMatches(block, size, 7U));
    EXPECT_FALSE(PatternMatches(block, size, 7U + 1U));
    void *neighbour = EHeap::Alloc(size + 8U);
    ASSERT_NE(neighbour, nullptr);
    FillPattern(neighbour, size + 8U, 7U + 4096U);
    EXPECT_TRUE(PatternMatches(block, size, 7U));
    EXPECT_TRUE(PatternMatches(neighbour, size + 8U, 7U + 4096U));
    EXPECT_FALSE(RangesOverlap(block, size, neighbour, size + 8U));
    EXPECT_TRUE(IsAligned(block));
    EXPECT_TRUE(IsAligned(neighbour));
}
TEST_F(EHeapPatternTest, SeedPattern_0007_0003)
{
    const size_t size = 3;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    FillPattern(block, size, 7U);
    EXPECT_TRUE(PatternMatches(block, size, 7U));
    EXPECT_FALSE(PatternMatches(block, size, 7U + 1U));
    void *neighbour = EHeap::Alloc(size + 8U);
    ASSERT_NE(neighbour, nullptr);
    FillPattern(neighbour, size + 8U, 7U + 4096U);
    EXPECT_TRUE(PatternMatches(block, size, 7U));
    EXPECT_TRUE(PatternMatches(neighbour, size + 8U, 7U + 4096U));
    EXPECT_FALSE(RangesOverlap(block, size, neighbour, size + 8U));
    EXPECT_TRUE(IsAligned(block));
    EXPECT_TRUE(IsAligned(neighbour));
}
TEST_F(EHeapPatternTest, SeedPattern_0007_0008)
{
    const size_t size = 8;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    FillPattern(block, size, 7U);
    EXPECT_TRUE(PatternMatches(block, size, 7U));
    EXPECT_FALSE(PatternMatches(block, size, 7U + 1U));
    void *neighbour = EHeap::Alloc(size + 8U);
    ASSERT_NE(neighbour, nullptr);
    FillPattern(neighbour, size + 8U, 7U + 4096U);
    EXPECT_TRUE(PatternMatches(block, size, 7U));
    EXPECT_TRUE(PatternMatches(neighbour, size + 8U, 7U + 4096U));
    EXPECT_FALSE(RangesOverlap(block, size, neighbour, size + 8U));
    EXPECT_TRUE(IsAligned(block));
    EXPECT_TRUE(IsAligned(neighbour));
}
TEST_F(EHeapPatternTest, SeedPattern_0007_0017)
{
    const size_t size = 17;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    FillPattern(block, size, 7U);
    EXPECT_TRUE(PatternMatches(block, size, 7U));
    EXPECT_FALSE(PatternMatches(block, size, 7U + 1U));
    void *neighbour = EHeap::Alloc(size + 8U);
    ASSERT_NE(neighbour, nullptr);
    FillPattern(neighbour, size + 8U, 7U + 4096U);
    EXPECT_TRUE(PatternMatches(block, size, 7U));
    EXPECT_TRUE(PatternMatches(neighbour, size + 8U, 7U + 4096U));
    EXPECT_FALSE(RangesOverlap(block, size, neighbour, size + 8U));
    EXPECT_TRUE(IsAligned(block));
    EXPECT_TRUE(IsAligned(neighbour));
}
TEST_F(EHeapPatternTest, SeedPattern_0007_0064)
{
    const size_t size = 64;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    FillPattern(block, size, 7U);
    EXPECT_TRUE(PatternMatches(block, size, 7U));
    EXPECT_FALSE(PatternMatches(block, size, 7U + 1U));
    void *neighbour = EHeap::Alloc(size + 8U);
    ASSERT_NE(neighbour, nullptr);
    FillPattern(neighbour, size + 8U, 7U + 4096U);
    EXPECT_TRUE(PatternMatches(block, size, 7U));
    EXPECT_TRUE(PatternMatches(neighbour, size + 8U, 7U + 4096U));
    EXPECT_FALSE(RangesOverlap(block, size, neighbour, size + 8U));
    EXPECT_TRUE(IsAligned(block));
    EXPECT_TRUE(IsAligned(neighbour));
}
TEST_F(EHeapPatternTest, SeedPattern_0007_0257)
{
    const size_t size = 257;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    FillPattern(block, size, 7U);
    EXPECT_TRUE(PatternMatches(block, size, 7U));
    EXPECT_FALSE(PatternMatches(block, size, 7U + 1U));
    void *neighbour = EHeap::Alloc(size + 8U);
    ASSERT_NE(neighbour, nullptr);
    FillPattern(neighbour, size + 8U, 7U + 4096U);
    EXPECT_TRUE(PatternMatches(block, size, 7U));
    EXPECT_TRUE(PatternMatches(neighbour, size + 8U, 7U + 4096U));
    EXPECT_FALSE(RangesOverlap(block, size, neighbour, size + 8U));
    EXPECT_TRUE(IsAligned(block));
    EXPECT_TRUE(IsAligned(neighbour));
}
TEST_F(EHeapPatternTest, SeedPattern_0007_1024)
{
    const size_t size = 1024;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    FillPattern(block, size, 7U);
    EXPECT_TRUE(PatternMatches(block, size, 7U));
    EXPECT_FALSE(PatternMatches(block, size, 7U + 1U));
    void *neighbour = EHeap::Alloc(size + 8U);
    ASSERT_NE(neighbour, nullptr);
    FillPattern(neighbour, size + 8U, 7U + 4096U);
    EXPECT_TRUE(PatternMatches(block, size, 7U));
    EXPECT_TRUE(PatternMatches(neighbour, size + 8U, 7U + 4096U));
    EXPECT_FALSE(RangesOverlap(block, size, neighbour, size + 8U));
    EXPECT_TRUE(IsAligned(block));
    EXPECT_TRUE(IsAligned(neighbour));
}
TEST_F(EHeapPatternTest, SeedPattern_dead_0001)
{
    const size_t size = 1;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    FillPattern(block, size, 57005U);
    EXPECT_TRUE(PatternMatches(block, size, 57005U));
    EXPECT_FALSE(PatternMatches(block, size, 57005U + 1U));
    void *neighbour = EHeap::Alloc(size + 8U);
    ASSERT_NE(neighbour, nullptr);
    FillPattern(neighbour, size + 8U, 57005U + 4096U);
    EXPECT_TRUE(PatternMatches(block, size, 57005U));
    EXPECT_TRUE(PatternMatches(neighbour, size + 8U, 57005U + 4096U));
    EXPECT_FALSE(RangesOverlap(block, size, neighbour, size + 8U));
    EXPECT_TRUE(IsAligned(block));
    EXPECT_TRUE(IsAligned(neighbour));
}
TEST_F(EHeapPatternTest, SeedPattern_dead_0003)
{
    const size_t size = 3;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    FillPattern(block, size, 57005U);
    EXPECT_TRUE(PatternMatches(block, size, 57005U));
    EXPECT_FALSE(PatternMatches(block, size, 57005U + 1U));
    void *neighbour = EHeap::Alloc(size + 8U);
    ASSERT_NE(neighbour, nullptr);
    FillPattern(neighbour, size + 8U, 57005U + 4096U);
    EXPECT_TRUE(PatternMatches(block, size, 57005U));
    EXPECT_TRUE(PatternMatches(neighbour, size + 8U, 57005U + 4096U));
    EXPECT_FALSE(RangesOverlap(block, size, neighbour, size + 8U));
    EXPECT_TRUE(IsAligned(block));
    EXPECT_TRUE(IsAligned(neighbour));
}
TEST_F(EHeapPatternTest, SeedPattern_dead_0008)
{
    const size_t size = 8;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    FillPattern(block, size, 57005U);
    EXPECT_TRUE(PatternMatches(block, size, 57005U));
    EXPECT_FALSE(PatternMatches(block, size, 57005U + 1U));
    void *neighbour = EHeap::Alloc(size + 8U);
    ASSERT_NE(neighbour, nullptr);
    FillPattern(neighbour, size + 8U, 57005U + 4096U);
    EXPECT_TRUE(PatternMatches(block, size, 57005U));
    EXPECT_TRUE(PatternMatches(neighbour, size + 8U, 57005U + 4096U));
    EXPECT_FALSE(RangesOverlap(block, size, neighbour, size + 8U));
    EXPECT_TRUE(IsAligned(block));
    EXPECT_TRUE(IsAligned(neighbour));
}
TEST_F(EHeapPatternTest, SeedPattern_dead_0017)
{
    const size_t size = 17;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    FillPattern(block, size, 57005U);
    EXPECT_TRUE(PatternMatches(block, size, 57005U));
    EXPECT_FALSE(PatternMatches(block, size, 57005U + 1U));
    void *neighbour = EHeap::Alloc(size + 8U);
    ASSERT_NE(neighbour, nullptr);
    FillPattern(neighbour, size + 8U, 57005U + 4096U);
    EXPECT_TRUE(PatternMatches(block, size, 57005U));
    EXPECT_TRUE(PatternMatches(neighbour, size + 8U, 57005U + 4096U));
    EXPECT_FALSE(RangesOverlap(block, size, neighbour, size + 8U));
    EXPECT_TRUE(IsAligned(block));
    EXPECT_TRUE(IsAligned(neighbour));
}
TEST_F(EHeapPatternTest, SeedPattern_dead_0064)
{
    const size_t size = 64;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    FillPattern(block, size, 57005U);
    EXPECT_TRUE(PatternMatches(block, size, 57005U));
    EXPECT_FALSE(PatternMatches(block, size, 57005U + 1U));
    void *neighbour = EHeap::Alloc(size + 8U);
    ASSERT_NE(neighbour, nullptr);
    FillPattern(neighbour, size + 8U, 57005U + 4096U);
    EXPECT_TRUE(PatternMatches(block, size, 57005U));
    EXPECT_TRUE(PatternMatches(neighbour, size + 8U, 57005U + 4096U));
    EXPECT_FALSE(RangesOverlap(block, size, neighbour, size + 8U));
    EXPECT_TRUE(IsAligned(block));
    EXPECT_TRUE(IsAligned(neighbour));
}
TEST_F(EHeapPatternTest, SeedPattern_dead_0257)
{
    const size_t size = 257;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    FillPattern(block, size, 57005U);
    EXPECT_TRUE(PatternMatches(block, size, 57005U));
    EXPECT_FALSE(PatternMatches(block, size, 57005U + 1U));
    void *neighbour = EHeap::Alloc(size + 8U);
    ASSERT_NE(neighbour, nullptr);
    FillPattern(neighbour, size + 8U, 57005U + 4096U);
    EXPECT_TRUE(PatternMatches(block, size, 57005U));
    EXPECT_TRUE(PatternMatches(neighbour, size + 8U, 57005U + 4096U));
    EXPECT_FALSE(RangesOverlap(block, size, neighbour, size + 8U));
    EXPECT_TRUE(IsAligned(block));
    EXPECT_TRUE(IsAligned(neighbour));
}
TEST_F(EHeapPatternTest, SeedPattern_dead_1024)
{
    const size_t size = 1024;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    FillPattern(block, size, 57005U);
    EXPECT_TRUE(PatternMatches(block, size, 57005U));
    EXPECT_FALSE(PatternMatches(block, size, 57005U + 1U));
    void *neighbour = EHeap::Alloc(size + 8U);
    ASSERT_NE(neighbour, nullptr);
    FillPattern(neighbour, size + 8U, 57005U + 4096U);
    EXPECT_TRUE(PatternMatches(block, size, 57005U));
    EXPECT_TRUE(PatternMatches(neighbour, size + 8U, 57005U + 4096U));
    EXPECT_FALSE(RangesOverlap(block, size, neighbour, size + 8U));
    EXPECT_TRUE(IsAligned(block));
    EXPECT_TRUE(IsAligned(neighbour));
}
TEST_F(EHeapPatternTest, SeedPattern_beef_0001)
{
    const size_t size = 1;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    FillPattern(block, size, 48879U);
    EXPECT_TRUE(PatternMatches(block, size, 48879U));
    EXPECT_FALSE(PatternMatches(block, size, 48879U + 1U));
    void *neighbour = EHeap::Alloc(size + 8U);
    ASSERT_NE(neighbour, nullptr);
    FillPattern(neighbour, size + 8U, 48879U + 4096U);
    EXPECT_TRUE(PatternMatches(block, size, 48879U));
    EXPECT_TRUE(PatternMatches(neighbour, size + 8U, 48879U + 4096U));
    EXPECT_FALSE(RangesOverlap(block, size, neighbour, size + 8U));
    EXPECT_TRUE(IsAligned(block));
    EXPECT_TRUE(IsAligned(neighbour));
}
TEST_F(EHeapPatternTest, SeedPattern_beef_0003)
{
    const size_t size = 3;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    FillPattern(block, size, 48879U);
    EXPECT_TRUE(PatternMatches(block, size, 48879U));
    EXPECT_FALSE(PatternMatches(block, size, 48879U + 1U));
    void *neighbour = EHeap::Alloc(size + 8U);
    ASSERT_NE(neighbour, nullptr);
    FillPattern(neighbour, size + 8U, 48879U + 4096U);
    EXPECT_TRUE(PatternMatches(block, size, 48879U));
    EXPECT_TRUE(PatternMatches(neighbour, size + 8U, 48879U + 4096U));
    EXPECT_FALSE(RangesOverlap(block, size, neighbour, size + 8U));
    EXPECT_TRUE(IsAligned(block));
    EXPECT_TRUE(IsAligned(neighbour));
}
TEST_F(EHeapPatternTest, SeedPattern_beef_0008)
{
    const size_t size = 8;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    FillPattern(block, size, 48879U);
    EXPECT_TRUE(PatternMatches(block, size, 48879U));
    EXPECT_FALSE(PatternMatches(block, size, 48879U + 1U));
    void *neighbour = EHeap::Alloc(size + 8U);
    ASSERT_NE(neighbour, nullptr);
    FillPattern(neighbour, size + 8U, 48879U + 4096U);
    EXPECT_TRUE(PatternMatches(block, size, 48879U));
    EXPECT_TRUE(PatternMatches(neighbour, size + 8U, 48879U + 4096U));
    EXPECT_FALSE(RangesOverlap(block, size, neighbour, size + 8U));
    EXPECT_TRUE(IsAligned(block));
    EXPECT_TRUE(IsAligned(neighbour));
}
TEST_F(EHeapPatternTest, SeedPattern_beef_0017)
{
    const size_t size = 17;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    FillPattern(block, size, 48879U);
    EXPECT_TRUE(PatternMatches(block, size, 48879U));
    EXPECT_FALSE(PatternMatches(block, size, 48879U + 1U));
    void *neighbour = EHeap::Alloc(size + 8U);
    ASSERT_NE(neighbour, nullptr);
    FillPattern(neighbour, size + 8U, 48879U + 4096U);
    EXPECT_TRUE(PatternMatches(block, size, 48879U));
    EXPECT_TRUE(PatternMatches(neighbour, size + 8U, 48879U + 4096U));
    EXPECT_FALSE(RangesOverlap(block, size, neighbour, size + 8U));
    EXPECT_TRUE(IsAligned(block));
    EXPECT_TRUE(IsAligned(neighbour));
}
TEST_F(EHeapPatternTest, SeedPattern_beef_0064)
{
    const size_t size = 64;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    FillPattern(block, size, 48879U);
    EXPECT_TRUE(PatternMatches(block, size, 48879U));
    EXPECT_FALSE(PatternMatches(block, size, 48879U + 1U));
    void *neighbour = EHeap::Alloc(size + 8U);
    ASSERT_NE(neighbour, nullptr);
    FillPattern(neighbour, size + 8U, 48879U + 4096U);
    EXPECT_TRUE(PatternMatches(block, size, 48879U));
    EXPECT_TRUE(PatternMatches(neighbour, size + 8U, 48879U + 4096U));
    EXPECT_FALSE(RangesOverlap(block, size, neighbour, size + 8U));
    EXPECT_TRUE(IsAligned(block));
    EXPECT_TRUE(IsAligned(neighbour));
}
TEST_F(EHeapPatternTest, SeedPattern_beef_0257)
{
    const size_t size = 257;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    FillPattern(block, size, 48879U);
    EXPECT_TRUE(PatternMatches(block, size, 48879U));
    EXPECT_FALSE(PatternMatches(block, size, 48879U + 1U));
    void *neighbour = EHeap::Alloc(size + 8U);
    ASSERT_NE(neighbour, nullptr);
    FillPattern(neighbour, size + 8U, 48879U + 4096U);
    EXPECT_TRUE(PatternMatches(block, size, 48879U));
    EXPECT_TRUE(PatternMatches(neighbour, size + 8U, 48879U + 4096U));
    EXPECT_FALSE(RangesOverlap(block, size, neighbour, size + 8U));
    EXPECT_TRUE(IsAligned(block));
    EXPECT_TRUE(IsAligned(neighbour));
}
TEST_F(EHeapPatternTest, SeedPattern_beef_1024)
{
    const size_t size = 1024;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    FillPattern(block, size, 48879U);
    EXPECT_TRUE(PatternMatches(block, size, 48879U));
    EXPECT_FALSE(PatternMatches(block, size, 48879U + 1U));
    void *neighbour = EHeap::Alloc(size + 8U);
    ASSERT_NE(neighbour, nullptr);
    FillPattern(neighbour, size + 8U, 48879U + 4096U);
    EXPECT_TRUE(PatternMatches(block, size, 48879U));
    EXPECT_TRUE(PatternMatches(neighbour, size + 8U, 48879U + 4096U));
    EXPECT_FALSE(RangesOverlap(block, size, neighbour, size + 8U));
    EXPECT_TRUE(IsAligned(block));
    EXPECT_TRUE(IsAligned(neighbour));
}
// ---------------------------------------------------------------------------
// Sequential allocation chains
// ---------------------------------------------------------------------------
class EHeapChainTest : public EHeapTest {};
TEST_F(EHeapChainTest, ChainOf_001_Blocks)
{
    constexpr size_t COUNT = 1U;
    std::array<BlockRecord, COUNT> blocks {};
    const size_t before = EHeap::AllocatedSize();
    size_t expected = 0U;
    for (size_t index = 0; index < COUNT; ++index) {
        const size_t size = 1U + ((index * 37U) % 300U);
        void *block = EHeap::Alloc(size);
        ASSERT_NE(block, nullptr);
        blocks[index] = BlockRecord {block, size, index + 1U};
        expected += AlignUpBytes(size);
        FillPattern(block, size, blocks[index].seed);
    }
    EXPECT_EQ(EHeap::AllocatedSize() - before, expected);
    for (size_t index = 0; index < COUNT; ++index) {
        EXPECT_TRUE(PatternMatches(blocks[index].ptr, blocks[index].size, blocks[index].seed));
        EXPECT_TRUE(IsAligned(blocks[index].ptr));
        if (index > 0U) {
            EXPECT_GE(Addr(blocks[index].ptr), Addr(blocks[index - 1U].ptr) + AlignUpBytes(blocks[index - 1U].size));
            EXPECT_FALSE(
                RangesOverlap(blocks[index].ptr, blocks[index].size, blocks[index - 1U].ptr, blocks[index - 1U].size));
        }
    }
}
TEST_F(EHeapChainTest, ChainOf_002_Blocks)
{
    constexpr size_t COUNT = 2U;
    std::array<BlockRecord, COUNT> blocks {};
    const size_t before = EHeap::AllocatedSize();
    size_t expected = 0U;
    for (size_t index = 0; index < COUNT; ++index) {
        const size_t size = 1U + ((index * 37U) % 300U);
        void *block = EHeap::Alloc(size);
        ASSERT_NE(block, nullptr);
        blocks[index] = BlockRecord {block, size, index + 1U};
        expected += AlignUpBytes(size);
        FillPattern(block, size, blocks[index].seed);
    }
    EXPECT_EQ(EHeap::AllocatedSize() - before, expected);
    for (size_t index = 0; index < COUNT; ++index) {
        EXPECT_TRUE(PatternMatches(blocks[index].ptr, blocks[index].size, blocks[index].seed));
        EXPECT_TRUE(IsAligned(blocks[index].ptr));
        if (index > 0U) {
            EXPECT_GE(Addr(blocks[index].ptr), Addr(blocks[index - 1U].ptr) + AlignUpBytes(blocks[index - 1U].size));
            EXPECT_FALSE(
                RangesOverlap(blocks[index].ptr, blocks[index].size, blocks[index - 1U].ptr, blocks[index - 1U].size));
        }
    }
}
TEST_F(EHeapChainTest, ChainOf_003_Blocks)
{
    constexpr size_t COUNT = 3U;
    std::array<BlockRecord, COUNT> blocks {};
    const size_t before = EHeap::AllocatedSize();
    size_t expected = 0U;
    for (size_t index = 0; index < COUNT; ++index) {
        const size_t size = 1U + ((index * 37U) % 300U);
        void *block = EHeap::Alloc(size);
        ASSERT_NE(block, nullptr);
        blocks[index] = BlockRecord {block, size, index + 1U};
        expected += AlignUpBytes(size);
        FillPattern(block, size, blocks[index].seed);
    }
    EXPECT_EQ(EHeap::AllocatedSize() - before, expected);
    for (size_t index = 0; index < COUNT; ++index) {
        EXPECT_TRUE(PatternMatches(blocks[index].ptr, blocks[index].size, blocks[index].seed));
        EXPECT_TRUE(IsAligned(blocks[index].ptr));
        if (index > 0U) {
            EXPECT_GE(Addr(blocks[index].ptr), Addr(blocks[index - 1U].ptr) + AlignUpBytes(blocks[index - 1U].size));
            EXPECT_FALSE(
                RangesOverlap(blocks[index].ptr, blocks[index].size, blocks[index - 1U].ptr, blocks[index - 1U].size));
        }
    }
}
TEST_F(EHeapChainTest, ChainOf_004_Blocks)
{
    constexpr size_t COUNT = 4U;
    std::array<BlockRecord, COUNT> blocks {};
    const size_t before = EHeap::AllocatedSize();
    size_t expected = 0U;
    for (size_t index = 0; index < COUNT; ++index) {
        const size_t size = 1U + ((index * 37U) % 300U);
        void *block = EHeap::Alloc(size);
        ASSERT_NE(block, nullptr);
        blocks[index] = BlockRecord {block, size, index + 1U};
        expected += AlignUpBytes(size);
        FillPattern(block, size, blocks[index].seed);
    }
    EXPECT_EQ(EHeap::AllocatedSize() - before, expected);
    for (size_t index = 0; index < COUNT; ++index) {
        EXPECT_TRUE(PatternMatches(blocks[index].ptr, blocks[index].size, blocks[index].seed));
        EXPECT_TRUE(IsAligned(blocks[index].ptr));
        if (index > 0U) {
            EXPECT_GE(Addr(blocks[index].ptr), Addr(blocks[index - 1U].ptr) + AlignUpBytes(blocks[index - 1U].size));
            EXPECT_FALSE(
                RangesOverlap(blocks[index].ptr, blocks[index].size, blocks[index - 1U].ptr, blocks[index - 1U].size));
        }
    }
}
TEST_F(EHeapChainTest, ChainOf_005_Blocks)
{
    constexpr size_t COUNT = 5U;
    std::array<BlockRecord, COUNT> blocks {};
    const size_t before = EHeap::AllocatedSize();
    size_t expected = 0U;
    for (size_t index = 0; index < COUNT; ++index) {
        const size_t size = 1U + ((index * 37U) % 300U);
        void *block = EHeap::Alloc(size);
        ASSERT_NE(block, nullptr);
        blocks[index] = BlockRecord {block, size, index + 1U};
        expected += AlignUpBytes(size);
        FillPattern(block, size, blocks[index].seed);
    }
    EXPECT_EQ(EHeap::AllocatedSize() - before, expected);
    for (size_t index = 0; index < COUNT; ++index) {
        EXPECT_TRUE(PatternMatches(blocks[index].ptr, blocks[index].size, blocks[index].seed));
        EXPECT_TRUE(IsAligned(blocks[index].ptr));
        if (index > 0U) {
            EXPECT_GE(Addr(blocks[index].ptr), Addr(blocks[index - 1U].ptr) + AlignUpBytes(blocks[index - 1U].size));
            EXPECT_FALSE(
                RangesOverlap(blocks[index].ptr, blocks[index].size, blocks[index - 1U].ptr, blocks[index - 1U].size));
        }
    }
}
TEST_F(EHeapChainTest, ChainOf_006_Blocks)
{
    constexpr size_t COUNT = 6U;
    std::array<BlockRecord, COUNT> blocks {};
    const size_t before = EHeap::AllocatedSize();
    size_t expected = 0U;
    for (size_t index = 0; index < COUNT; ++index) {
        const size_t size = 1U + ((index * 37U) % 300U);
        void *block = EHeap::Alloc(size);
        ASSERT_NE(block, nullptr);
        blocks[index] = BlockRecord {block, size, index + 1U};
        expected += AlignUpBytes(size);
        FillPattern(block, size, blocks[index].seed);
    }
    EXPECT_EQ(EHeap::AllocatedSize() - before, expected);
    for (size_t index = 0; index < COUNT; ++index) {
        EXPECT_TRUE(PatternMatches(blocks[index].ptr, blocks[index].size, blocks[index].seed));
        EXPECT_TRUE(IsAligned(blocks[index].ptr));
        if (index > 0U) {
            EXPECT_GE(Addr(blocks[index].ptr), Addr(blocks[index - 1U].ptr) + AlignUpBytes(blocks[index - 1U].size));
            EXPECT_FALSE(
                RangesOverlap(blocks[index].ptr, blocks[index].size, blocks[index - 1U].ptr, blocks[index - 1U].size));
        }
    }
}
TEST_F(EHeapChainTest, ChainOf_007_Blocks)
{
    constexpr size_t COUNT = 7U;
    std::array<BlockRecord, COUNT> blocks {};
    const size_t before = EHeap::AllocatedSize();
    size_t expected = 0U;
    for (size_t index = 0; index < COUNT; ++index) {
        const size_t size = 1U + ((index * 37U) % 300U);
        void *block = EHeap::Alloc(size);
        ASSERT_NE(block, nullptr);
        blocks[index] = BlockRecord {block, size, index + 1U};
        expected += AlignUpBytes(size);
        FillPattern(block, size, blocks[index].seed);
    }
    EXPECT_EQ(EHeap::AllocatedSize() - before, expected);
    for (size_t index = 0; index < COUNT; ++index) {
        EXPECT_TRUE(PatternMatches(blocks[index].ptr, blocks[index].size, blocks[index].seed));
        EXPECT_TRUE(IsAligned(blocks[index].ptr));
        if (index > 0U) {
            EXPECT_GE(Addr(blocks[index].ptr), Addr(blocks[index - 1U].ptr) + AlignUpBytes(blocks[index - 1U].size));
            EXPECT_FALSE(
                RangesOverlap(blocks[index].ptr, blocks[index].size, blocks[index - 1U].ptr, blocks[index - 1U].size));
        }
    }
}
TEST_F(EHeapChainTest, ChainOf_008_Blocks)
{
    constexpr size_t COUNT = 8U;
    std::array<BlockRecord, COUNT> blocks {};
    const size_t before = EHeap::AllocatedSize();
    size_t expected = 0U;
    for (size_t index = 0; index < COUNT; ++index) {
        const size_t size = 1U + ((index * 37U) % 300U);
        void *block = EHeap::Alloc(size);
        ASSERT_NE(block, nullptr);
        blocks[index] = BlockRecord {block, size, index + 1U};
        expected += AlignUpBytes(size);
        FillPattern(block, size, blocks[index].seed);
    }
    EXPECT_EQ(EHeap::AllocatedSize() - before, expected);
    for (size_t index = 0; index < COUNT; ++index) {
        EXPECT_TRUE(PatternMatches(blocks[index].ptr, blocks[index].size, blocks[index].seed));
        EXPECT_TRUE(IsAligned(blocks[index].ptr));
        if (index > 0U) {
            EXPECT_GE(Addr(blocks[index].ptr), Addr(blocks[index - 1U].ptr) + AlignUpBytes(blocks[index - 1U].size));
            EXPECT_FALSE(
                RangesOverlap(blocks[index].ptr, blocks[index].size, blocks[index - 1U].ptr, blocks[index - 1U].size));
        }
    }
}
TEST_F(EHeapChainTest, ChainOf_009_Blocks)
{
    constexpr size_t COUNT = 9U;
    std::array<BlockRecord, COUNT> blocks {};
    const size_t before = EHeap::AllocatedSize();
    size_t expected = 0U;
    for (size_t index = 0; index < COUNT; ++index) {
        const size_t size = 1U + ((index * 37U) % 300U);
        void *block = EHeap::Alloc(size);
        ASSERT_NE(block, nullptr);
        blocks[index] = BlockRecord {block, size, index + 1U};
        expected += AlignUpBytes(size);
        FillPattern(block, size, blocks[index].seed);
    }
    EXPECT_EQ(EHeap::AllocatedSize() - before, expected);
    for (size_t index = 0; index < COUNT; ++index) {
        EXPECT_TRUE(PatternMatches(blocks[index].ptr, blocks[index].size, blocks[index].seed));
        EXPECT_TRUE(IsAligned(blocks[index].ptr));
        if (index > 0U) {
            EXPECT_GE(Addr(blocks[index].ptr), Addr(blocks[index - 1U].ptr) + AlignUpBytes(blocks[index - 1U].size));
            EXPECT_FALSE(
                RangesOverlap(blocks[index].ptr, blocks[index].size, blocks[index - 1U].ptr, blocks[index - 1U].size));
        }
    }
}
TEST_F(EHeapChainTest, ChainOf_010_Blocks)
{
    constexpr size_t COUNT = 10U;
    std::array<BlockRecord, COUNT> blocks {};
    const size_t before = EHeap::AllocatedSize();
    size_t expected = 0U;
    for (size_t index = 0; index < COUNT; ++index) {
        const size_t size = 1U + ((index * 37U) % 300U);
        void *block = EHeap::Alloc(size);
        ASSERT_NE(block, nullptr);
        blocks[index] = BlockRecord {block, size, index + 1U};
        expected += AlignUpBytes(size);
        FillPattern(block, size, blocks[index].seed);
    }
    EXPECT_EQ(EHeap::AllocatedSize() - before, expected);
    for (size_t index = 0; index < COUNT; ++index) {
        EXPECT_TRUE(PatternMatches(blocks[index].ptr, blocks[index].size, blocks[index].seed));
        EXPECT_TRUE(IsAligned(blocks[index].ptr));
        if (index > 0U) {
            EXPECT_GE(Addr(blocks[index].ptr), Addr(blocks[index - 1U].ptr) + AlignUpBytes(blocks[index - 1U].size));
            EXPECT_FALSE(
                RangesOverlap(blocks[index].ptr, blocks[index].size, blocks[index - 1U].ptr, blocks[index - 1U].size));
        }
    }
}
TEST_F(EHeapChainTest, ChainOf_012_Blocks)
{
    constexpr size_t COUNT = 12U;
    std::array<BlockRecord, COUNT> blocks {};
    const size_t before = EHeap::AllocatedSize();
    size_t expected = 0U;
    for (size_t index = 0; index < COUNT; ++index) {
        const size_t size = 1U + ((index * 37U) % 300U);
        void *block = EHeap::Alloc(size);
        ASSERT_NE(block, nullptr);
        blocks[index] = BlockRecord {block, size, index + 1U};
        expected += AlignUpBytes(size);
        FillPattern(block, size, blocks[index].seed);
    }
    EXPECT_EQ(EHeap::AllocatedSize() - before, expected);
    for (size_t index = 0; index < COUNT; ++index) {
        EXPECT_TRUE(PatternMatches(blocks[index].ptr, blocks[index].size, blocks[index].seed));
        EXPECT_TRUE(IsAligned(blocks[index].ptr));
        if (index > 0U) {
            EXPECT_GE(Addr(blocks[index].ptr), Addr(blocks[index - 1U].ptr) + AlignUpBytes(blocks[index - 1U].size));
            EXPECT_FALSE(
                RangesOverlap(blocks[index].ptr, blocks[index].size, blocks[index - 1U].ptr, blocks[index - 1U].size));
        }
    }
}
TEST_F(EHeapChainTest, ChainOf_014_Blocks)
{
    constexpr size_t COUNT = 14U;
    std::array<BlockRecord, COUNT> blocks {};
    const size_t before = EHeap::AllocatedSize();
    size_t expected = 0U;
    for (size_t index = 0; index < COUNT; ++index) {
        const size_t size = 1U + ((index * 37U) % 300U);
        void *block = EHeap::Alloc(size);
        ASSERT_NE(block, nullptr);
        blocks[index] = BlockRecord {block, size, index + 1U};
        expected += AlignUpBytes(size);
        FillPattern(block, size, blocks[index].seed);
    }
    EXPECT_EQ(EHeap::AllocatedSize() - before, expected);
    for (size_t index = 0; index < COUNT; ++index) {
        EXPECT_TRUE(PatternMatches(blocks[index].ptr, blocks[index].size, blocks[index].seed));
        EXPECT_TRUE(IsAligned(blocks[index].ptr));
        if (index > 0U) {
            EXPECT_GE(Addr(blocks[index].ptr), Addr(blocks[index - 1U].ptr) + AlignUpBytes(blocks[index - 1U].size));
            EXPECT_FALSE(
                RangesOverlap(blocks[index].ptr, blocks[index].size, blocks[index - 1U].ptr, blocks[index - 1U].size));
        }
    }
}
TEST_F(EHeapChainTest, ChainOf_016_Blocks)
{
    constexpr size_t COUNT = 16U;
    std::array<BlockRecord, COUNT> blocks {};
    const size_t before = EHeap::AllocatedSize();
    size_t expected = 0U;
    for (size_t index = 0; index < COUNT; ++index) {
        const size_t size = 1U + ((index * 37U) % 300U);
        void *block = EHeap::Alloc(size);
        ASSERT_NE(block, nullptr);
        blocks[index] = BlockRecord {block, size, index + 1U};
        expected += AlignUpBytes(size);
        FillPattern(block, size, blocks[index].seed);
    }
    EXPECT_EQ(EHeap::AllocatedSize() - before, expected);
    for (size_t index = 0; index < COUNT; ++index) {
        EXPECT_TRUE(PatternMatches(blocks[index].ptr, blocks[index].size, blocks[index].seed));
        EXPECT_TRUE(IsAligned(blocks[index].ptr));
        if (index > 0U) {
            EXPECT_GE(Addr(blocks[index].ptr), Addr(blocks[index - 1U].ptr) + AlignUpBytes(blocks[index - 1U].size));
            EXPECT_FALSE(
                RangesOverlap(blocks[index].ptr, blocks[index].size, blocks[index - 1U].ptr, blocks[index - 1U].size));
        }
    }
}
TEST_F(EHeapChainTest, ChainOf_020_Blocks)
{
    constexpr size_t COUNT = 20U;
    std::array<BlockRecord, COUNT> blocks {};
    const size_t before = EHeap::AllocatedSize();
    size_t expected = 0U;
    for (size_t index = 0; index < COUNT; ++index) {
        const size_t size = 1U + ((index * 37U) % 300U);
        void *block = EHeap::Alloc(size);
        ASSERT_NE(block, nullptr);
        blocks[index] = BlockRecord {block, size, index + 1U};
        expected += AlignUpBytes(size);
        FillPattern(block, size, blocks[index].seed);
    }
    EXPECT_EQ(EHeap::AllocatedSize() - before, expected);
    for (size_t index = 0; index < COUNT; ++index) {
        EXPECT_TRUE(PatternMatches(blocks[index].ptr, blocks[index].size, blocks[index].seed));
        EXPECT_TRUE(IsAligned(blocks[index].ptr));
        if (index > 0U) {
            EXPECT_GE(Addr(blocks[index].ptr), Addr(blocks[index - 1U].ptr) + AlignUpBytes(blocks[index - 1U].size));
            EXPECT_FALSE(
                RangesOverlap(blocks[index].ptr, blocks[index].size, blocks[index - 1U].ptr, blocks[index - 1U].size));
        }
    }
}
TEST_F(EHeapChainTest, ChainOf_024_Blocks)
{
    constexpr size_t COUNT = 24U;
    std::array<BlockRecord, COUNT> blocks {};
    const size_t before = EHeap::AllocatedSize();
    size_t expected = 0U;
    for (size_t index = 0; index < COUNT; ++index) {
        const size_t size = 1U + ((index * 37U) % 300U);
        void *block = EHeap::Alloc(size);
        ASSERT_NE(block, nullptr);
        blocks[index] = BlockRecord {block, size, index + 1U};
        expected += AlignUpBytes(size);
        FillPattern(block, size, blocks[index].seed);
    }
    EXPECT_EQ(EHeap::AllocatedSize() - before, expected);
    for (size_t index = 0; index < COUNT; ++index) {
        EXPECT_TRUE(PatternMatches(blocks[index].ptr, blocks[index].size, blocks[index].seed));
        EXPECT_TRUE(IsAligned(blocks[index].ptr));
        if (index > 0U) {
            EXPECT_GE(Addr(blocks[index].ptr), Addr(blocks[index - 1U].ptr) + AlignUpBytes(blocks[index - 1U].size));
            EXPECT_FALSE(
                RangesOverlap(blocks[index].ptr, blocks[index].size, blocks[index - 1U].ptr, blocks[index - 1U].size));
        }
    }
}
TEST_F(EHeapChainTest, ChainOf_028_Blocks)
{
    constexpr size_t COUNT = 28U;
    std::array<BlockRecord, COUNT> blocks {};
    const size_t before = EHeap::AllocatedSize();
    size_t expected = 0U;
    for (size_t index = 0; index < COUNT; ++index) {
        const size_t size = 1U + ((index * 37U) % 300U);
        void *block = EHeap::Alloc(size);
        ASSERT_NE(block, nullptr);
        blocks[index] = BlockRecord {block, size, index + 1U};
        expected += AlignUpBytes(size);
        FillPattern(block, size, blocks[index].seed);
    }
    EXPECT_EQ(EHeap::AllocatedSize() - before, expected);
    for (size_t index = 0; index < COUNT; ++index) {
        EXPECT_TRUE(PatternMatches(blocks[index].ptr, blocks[index].size, blocks[index].seed));
        EXPECT_TRUE(IsAligned(blocks[index].ptr));
        if (index > 0U) {
            EXPECT_GE(Addr(blocks[index].ptr), Addr(blocks[index - 1U].ptr) + AlignUpBytes(blocks[index - 1U].size));
            EXPECT_FALSE(
                RangesOverlap(blocks[index].ptr, blocks[index].size, blocks[index - 1U].ptr, blocks[index - 1U].size));
        }
    }
}
TEST_F(EHeapChainTest, ChainOf_032_Blocks)
{
    constexpr size_t COUNT = 32U;
    std::array<BlockRecord, COUNT> blocks {};
    const size_t before = EHeap::AllocatedSize();
    size_t expected = 0U;
    for (size_t index = 0; index < COUNT; ++index) {
        const size_t size = 1U + ((index * 37U) % 300U);
        void *block = EHeap::Alloc(size);
        ASSERT_NE(block, nullptr);
        blocks[index] = BlockRecord {block, size, index + 1U};
        expected += AlignUpBytes(size);
        FillPattern(block, size, blocks[index].seed);
    }
    EXPECT_EQ(EHeap::AllocatedSize() - before, expected);
    for (size_t index = 0; index < COUNT; ++index) {
        EXPECT_TRUE(PatternMatches(blocks[index].ptr, blocks[index].size, blocks[index].seed));
        EXPECT_TRUE(IsAligned(blocks[index].ptr));
        if (index > 0U) {
            EXPECT_GE(Addr(blocks[index].ptr), Addr(blocks[index - 1U].ptr) + AlignUpBytes(blocks[index - 1U].size));
            EXPECT_FALSE(
                RangesOverlap(blocks[index].ptr, blocks[index].size, blocks[index - 1U].ptr, blocks[index - 1U].size));
        }
    }
}
TEST_F(EHeapChainTest, ChainOf_040_Blocks)
{
    constexpr size_t COUNT = 40U;
    std::array<BlockRecord, COUNT> blocks {};
    const size_t before = EHeap::AllocatedSize();
    size_t expected = 0U;
    for (size_t index = 0; index < COUNT; ++index) {
        const size_t size = 1U + ((index * 37U) % 300U);
        void *block = EHeap::Alloc(size);
        ASSERT_NE(block, nullptr);
        blocks[index] = BlockRecord {block, size, index + 1U};
        expected += AlignUpBytes(size);
        FillPattern(block, size, blocks[index].seed);
    }
    EXPECT_EQ(EHeap::AllocatedSize() - before, expected);
    for (size_t index = 0; index < COUNT; ++index) {
        EXPECT_TRUE(PatternMatches(blocks[index].ptr, blocks[index].size, blocks[index].seed));
        EXPECT_TRUE(IsAligned(blocks[index].ptr));
        if (index > 0U) {
            EXPECT_GE(Addr(blocks[index].ptr), Addr(blocks[index - 1U].ptr) + AlignUpBytes(blocks[index - 1U].size));
            EXPECT_FALSE(
                RangesOverlap(blocks[index].ptr, blocks[index].size, blocks[index - 1U].ptr, blocks[index - 1U].size));
        }
    }
}
TEST_F(EHeapChainTest, ChainOf_048_Blocks)
{
    constexpr size_t COUNT = 48U;
    std::array<BlockRecord, COUNT> blocks {};
    const size_t before = EHeap::AllocatedSize();
    size_t expected = 0U;
    for (size_t index = 0; index < COUNT; ++index) {
        const size_t size = 1U + ((index * 37U) % 300U);
        void *block = EHeap::Alloc(size);
        ASSERT_NE(block, nullptr);
        blocks[index] = BlockRecord {block, size, index + 1U};
        expected += AlignUpBytes(size);
        FillPattern(block, size, blocks[index].seed);
    }
    EXPECT_EQ(EHeap::AllocatedSize() - before, expected);
    for (size_t index = 0; index < COUNT; ++index) {
        EXPECT_TRUE(PatternMatches(blocks[index].ptr, blocks[index].size, blocks[index].seed));
        EXPECT_TRUE(IsAligned(blocks[index].ptr));
        if (index > 0U) {
            EXPECT_GE(Addr(blocks[index].ptr), Addr(blocks[index - 1U].ptr) + AlignUpBytes(blocks[index - 1U].size));
            EXPECT_FALSE(
                RangesOverlap(blocks[index].ptr, blocks[index].size, blocks[index - 1U].ptr, blocks[index - 1U].size));
        }
    }
}
TEST_F(EHeapChainTest, ChainOf_056_Blocks)
{
    constexpr size_t COUNT = 56U;
    std::array<BlockRecord, COUNT> blocks {};
    const size_t before = EHeap::AllocatedSize();
    size_t expected = 0U;
    for (size_t index = 0; index < COUNT; ++index) {
        const size_t size = 1U + ((index * 37U) % 300U);
        void *block = EHeap::Alloc(size);
        ASSERT_NE(block, nullptr);
        blocks[index] = BlockRecord {block, size, index + 1U};
        expected += AlignUpBytes(size);
        FillPattern(block, size, blocks[index].seed);
    }
    EXPECT_EQ(EHeap::AllocatedSize() - before, expected);
    for (size_t index = 0; index < COUNT; ++index) {
        EXPECT_TRUE(PatternMatches(blocks[index].ptr, blocks[index].size, blocks[index].seed));
        EXPECT_TRUE(IsAligned(blocks[index].ptr));
        if (index > 0U) {
            EXPECT_GE(Addr(blocks[index].ptr), Addr(blocks[index - 1U].ptr) + AlignUpBytes(blocks[index - 1U].size));
            EXPECT_FALSE(
                RangesOverlap(blocks[index].ptr, blocks[index].size, blocks[index - 1U].ptr, blocks[index - 1U].size));
        }
    }
}
TEST_F(EHeapChainTest, ChainOf_064_Blocks)
{
    constexpr size_t COUNT = 64U;
    std::array<BlockRecord, COUNT> blocks {};
    const size_t before = EHeap::AllocatedSize();
    size_t expected = 0U;
    for (size_t index = 0; index < COUNT; ++index) {
        const size_t size = 1U + ((index * 37U) % 300U);
        void *block = EHeap::Alloc(size);
        ASSERT_NE(block, nullptr);
        blocks[index] = BlockRecord {block, size, index + 1U};
        expected += AlignUpBytes(size);
        FillPattern(block, size, blocks[index].seed);
    }
    EXPECT_EQ(EHeap::AllocatedSize() - before, expected);
    for (size_t index = 0; index < COUNT; ++index) {
        EXPECT_TRUE(PatternMatches(blocks[index].ptr, blocks[index].size, blocks[index].seed));
        EXPECT_TRUE(IsAligned(blocks[index].ptr));
        if (index > 0U) {
            EXPECT_GE(Addr(blocks[index].ptr), Addr(blocks[index - 1U].ptr) + AlignUpBytes(blocks[index - 1U].size));
            EXPECT_FALSE(
                RangesOverlap(blocks[index].ptr, blocks[index].size, blocks[index - 1U].ptr, blocks[index - 1U].size));
        }
    }
}
// ---------------------------------------------------------------------------
// EHeap::Free size accounting
// ---------------------------------------------------------------------------
class EHeapFreeTest : public EHeapTest {};
TEST_F(EHeapFreeTest, FreeSize_000000)
{
    const size_t size = 0U;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    const size_t allocatedBefore = EHeap::AllocatedSize();
    const size_t freedBefore = EHeap::FreedSize();
    EHeap::Free(block, size);
    EXPECT_EQ(EHeap::FreedSize() - freedBefore, ExpectedFreedSize(size));
    EXPECT_EQ(EHeap::AllocatedSize(), allocatedBefore);
    EXPECT_EQ(EHeap::AllocatedSize() - allocatedBefore, 0U);
}
TEST_F(EHeapFreeTest, FreeSize_000001)
{
    const size_t size = 1U;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    const size_t allocatedBefore = EHeap::AllocatedSize();
    const size_t freedBefore = EHeap::FreedSize();
    EHeap::Free(block, size);
    EXPECT_EQ(EHeap::FreedSize() - freedBefore, ExpectedFreedSize(size));
    EXPECT_EQ(EHeap::AllocatedSize(), allocatedBefore);
    EXPECT_EQ(EHeap::AllocatedSize() - allocatedBefore, 0U);
}
TEST_F(EHeapFreeTest, FreeSize_000007)
{
    const size_t size = 7U;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    const size_t allocatedBefore = EHeap::AllocatedSize();
    const size_t freedBefore = EHeap::FreedSize();
    EHeap::Free(block, size);
    EXPECT_EQ(EHeap::FreedSize() - freedBefore, ExpectedFreedSize(size));
    EXPECT_EQ(EHeap::AllocatedSize(), allocatedBefore);
    EXPECT_EQ(EHeap::AllocatedSize() - allocatedBefore, 0U);
}
TEST_F(EHeapFreeTest, FreeSize_000008)
{
    const size_t size = 8U;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    const size_t allocatedBefore = EHeap::AllocatedSize();
    const size_t freedBefore = EHeap::FreedSize();
    EHeap::Free(block, size);
    EXPECT_EQ(EHeap::FreedSize() - freedBefore, ExpectedFreedSize(size));
    EXPECT_EQ(EHeap::AllocatedSize(), allocatedBefore);
    EXPECT_EQ(EHeap::AllocatedSize() - allocatedBefore, 0U);
}
TEST_F(EHeapFreeTest, FreeSize_000009)
{
    const size_t size = 9U;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    const size_t allocatedBefore = EHeap::AllocatedSize();
    const size_t freedBefore = EHeap::FreedSize();
    EHeap::Free(block, size);
    EXPECT_EQ(EHeap::FreedSize() - freedBefore, ExpectedFreedSize(size));
    EXPECT_EQ(EHeap::AllocatedSize(), allocatedBefore);
    EXPECT_EQ(EHeap::AllocatedSize() - allocatedBefore, 0U);
}
TEST_F(EHeapFreeTest, FreeSize_000016)
{
    const size_t size = 16U;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    const size_t allocatedBefore = EHeap::AllocatedSize();
    const size_t freedBefore = EHeap::FreedSize();
    EHeap::Free(block, size);
    EXPECT_EQ(EHeap::FreedSize() - freedBefore, ExpectedFreedSize(size));
    EXPECT_EQ(EHeap::AllocatedSize(), allocatedBefore);
    EXPECT_EQ(EHeap::AllocatedSize() - allocatedBefore, 0U);
}
TEST_F(EHeapFreeTest, FreeSize_000017)
{
    const size_t size = 17U;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    const size_t allocatedBefore = EHeap::AllocatedSize();
    const size_t freedBefore = EHeap::FreedSize();
    EHeap::Free(block, size);
    EXPECT_EQ(EHeap::FreedSize() - freedBefore, ExpectedFreedSize(size));
    EXPECT_EQ(EHeap::AllocatedSize(), allocatedBefore);
    EXPECT_EQ(EHeap::AllocatedSize() - allocatedBefore, 0U);
}
TEST_F(EHeapFreeTest, FreeSize_000032)
{
    const size_t size = 32U;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    const size_t allocatedBefore = EHeap::AllocatedSize();
    const size_t freedBefore = EHeap::FreedSize();
    EHeap::Free(block, size);
    EXPECT_EQ(EHeap::FreedSize() - freedBefore, ExpectedFreedSize(size));
    EXPECT_EQ(EHeap::AllocatedSize(), allocatedBefore);
    EXPECT_EQ(EHeap::AllocatedSize() - allocatedBefore, 0U);
}
TEST_F(EHeapFreeTest, FreeSize_000064)
{
    const size_t size = 64U;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    const size_t allocatedBefore = EHeap::AllocatedSize();
    const size_t freedBefore = EHeap::FreedSize();
    EHeap::Free(block, size);
    EXPECT_EQ(EHeap::FreedSize() - freedBefore, ExpectedFreedSize(size));
    EXPECT_EQ(EHeap::AllocatedSize(), allocatedBefore);
    EXPECT_EQ(EHeap::AllocatedSize() - allocatedBefore, 0U);
}
TEST_F(EHeapFreeTest, FreeSize_000100)
{
    const size_t size = 100U;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    const size_t allocatedBefore = EHeap::AllocatedSize();
    const size_t freedBefore = EHeap::FreedSize();
    EHeap::Free(block, size);
    EXPECT_EQ(EHeap::FreedSize() - freedBefore, ExpectedFreedSize(size));
    EXPECT_EQ(EHeap::AllocatedSize(), allocatedBefore);
    EXPECT_EQ(EHeap::AllocatedSize() - allocatedBefore, 0U);
}
TEST_F(EHeapFreeTest, FreeSize_000128)
{
    const size_t size = 128U;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    const size_t allocatedBefore = EHeap::AllocatedSize();
    const size_t freedBefore = EHeap::FreedSize();
    EHeap::Free(block, size);
    EXPECT_EQ(EHeap::FreedSize() - freedBefore, ExpectedFreedSize(size));
    EXPECT_EQ(EHeap::AllocatedSize(), allocatedBefore);
    EXPECT_EQ(EHeap::AllocatedSize() - allocatedBefore, 0U);
}
TEST_F(EHeapFreeTest, FreeSize_000255)
{
    const size_t size = 255U;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    const size_t allocatedBefore = EHeap::AllocatedSize();
    const size_t freedBefore = EHeap::FreedSize();
    EHeap::Free(block, size);
    EXPECT_EQ(EHeap::FreedSize() - freedBefore, ExpectedFreedSize(size));
    EXPECT_EQ(EHeap::AllocatedSize(), allocatedBefore);
    EXPECT_EQ(EHeap::AllocatedSize() - allocatedBefore, 0U);
}
TEST_F(EHeapFreeTest, FreeSize_000256)
{
    const size_t size = 256U;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    const size_t allocatedBefore = EHeap::AllocatedSize();
    const size_t freedBefore = EHeap::FreedSize();
    EHeap::Free(block, size);
    EXPECT_EQ(EHeap::FreedSize() - freedBefore, ExpectedFreedSize(size));
    EXPECT_EQ(EHeap::AllocatedSize(), allocatedBefore);
    EXPECT_EQ(EHeap::AllocatedSize() - allocatedBefore, 0U);
}
TEST_F(EHeapFreeTest, FreeSize_000512)
{
    const size_t size = 512U;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    const size_t allocatedBefore = EHeap::AllocatedSize();
    const size_t freedBefore = EHeap::FreedSize();
    EHeap::Free(block, size);
    EXPECT_EQ(EHeap::FreedSize() - freedBefore, ExpectedFreedSize(size));
    EXPECT_EQ(EHeap::AllocatedSize(), allocatedBefore);
    EXPECT_EQ(EHeap::AllocatedSize() - allocatedBefore, 0U);
}
TEST_F(EHeapFreeTest, FreeSize_001024)
{
    const size_t size = 1024U;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    const size_t allocatedBefore = EHeap::AllocatedSize();
    const size_t freedBefore = EHeap::FreedSize();
    EHeap::Free(block, size);
    EXPECT_EQ(EHeap::FreedSize() - freedBefore, ExpectedFreedSize(size));
    EXPECT_EQ(EHeap::AllocatedSize(), allocatedBefore);
    EXPECT_EQ(EHeap::AllocatedSize() - allocatedBefore, 0U);
}
TEST_F(EHeapFreeTest, FreeSize_002048)
{
    const size_t size = 2048U;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    const size_t allocatedBefore = EHeap::AllocatedSize();
    const size_t freedBefore = EHeap::FreedSize();
    EHeap::Free(block, size);
    EXPECT_EQ(EHeap::FreedSize() - freedBefore, ExpectedFreedSize(size));
    EXPECT_EQ(EHeap::AllocatedSize(), allocatedBefore);
    EXPECT_EQ(EHeap::AllocatedSize() - allocatedBefore, 0U);
}
TEST_F(EHeapFreeTest, FreeSize_004096)
{
    const size_t size = 4096U;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    const size_t allocatedBefore = EHeap::AllocatedSize();
    const size_t freedBefore = EHeap::FreedSize();
    EHeap::Free(block, size);
    EXPECT_EQ(EHeap::FreedSize() - freedBefore, ExpectedFreedSize(size));
    EXPECT_EQ(EHeap::AllocatedSize(), allocatedBefore);
    EXPECT_EQ(EHeap::AllocatedSize() - allocatedBefore, 0U);
}
TEST_F(EHeapFreeTest, FreeSize_008192)
{
    const size_t size = 8192U;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    const size_t allocatedBefore = EHeap::AllocatedSize();
    const size_t freedBefore = EHeap::FreedSize();
    EHeap::Free(block, size);
    EXPECT_EQ(EHeap::FreedSize() - freedBefore, ExpectedFreedSize(size));
    EXPECT_EQ(EHeap::AllocatedSize(), allocatedBefore);
    EXPECT_EQ(EHeap::AllocatedSize() - allocatedBefore, 0U);
}
TEST_F(EHeapFreeTest, FreeSize_016384)
{
    const size_t size = 16384U;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    const size_t allocatedBefore = EHeap::AllocatedSize();
    const size_t freedBefore = EHeap::FreedSize();
    EHeap::Free(block, size);
    EXPECT_EQ(EHeap::FreedSize() - freedBefore, ExpectedFreedSize(size));
    EXPECT_EQ(EHeap::AllocatedSize(), allocatedBefore);
    EXPECT_EQ(EHeap::AllocatedSize() - allocatedBefore, 0U);
}
TEST_F(EHeapFreeTest, FreeSize_065536)
{
    const size_t size = 65536U;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    const size_t allocatedBefore = EHeap::AllocatedSize();
    const size_t freedBefore = EHeap::FreedSize();
    EHeap::Free(block, size);
    EXPECT_EQ(EHeap::FreedSize() - freedBefore, ExpectedFreedSize(size));
    EXPECT_EQ(EHeap::AllocatedSize(), allocatedBefore);
    EXPECT_EQ(EHeap::AllocatedSize() - allocatedBefore, 0U);
}
TEST_F(EHeapFreeTest, RepeatedFree_0008_x2)
{
    const size_t size = 8U;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    const size_t freedBefore = EHeap::FreedSize();
    for (size_t repeat = 0; repeat < 2U; ++repeat) {
        EHeap::Free(block, size);
    }
    EXPECT_EQ(EHeap::FreedSize() - freedBefore, ExpectedFreedSize(size) * 2U);
    EXPECT_EQ(EHeap::AllocatedSize() % ALLOC_ALIGNMENT, 0U);
}
TEST_F(EHeapFreeTest, RepeatedFree_0008_x3)
{
    const size_t size = 8U;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    const size_t freedBefore = EHeap::FreedSize();
    for (size_t repeat = 0; repeat < 3U; ++repeat) {
        EHeap::Free(block, size);
    }
    EXPECT_EQ(EHeap::FreedSize() - freedBefore, ExpectedFreedSize(size) * 3U);
    EXPECT_EQ(EHeap::AllocatedSize() % ALLOC_ALIGNMENT, 0U);
}
TEST_F(EHeapFreeTest, RepeatedFree_0008_x5)
{
    const size_t size = 8U;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    const size_t freedBefore = EHeap::FreedSize();
    for (size_t repeat = 0; repeat < 5U; ++repeat) {
        EHeap::Free(block, size);
    }
    EXPECT_EQ(EHeap::FreedSize() - freedBefore, ExpectedFreedSize(size) * 5U);
    EXPECT_EQ(EHeap::AllocatedSize() % ALLOC_ALIGNMENT, 0U);
}
TEST_F(EHeapFreeTest, RepeatedFree_0064_x2)
{
    const size_t size = 64U;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    const size_t freedBefore = EHeap::FreedSize();
    for (size_t repeat = 0; repeat < 2U; ++repeat) {
        EHeap::Free(block, size);
    }
    EXPECT_EQ(EHeap::FreedSize() - freedBefore, ExpectedFreedSize(size) * 2U);
    EXPECT_EQ(EHeap::AllocatedSize() % ALLOC_ALIGNMENT, 0U);
}
TEST_F(EHeapFreeTest, RepeatedFree_0064_x3)
{
    const size_t size = 64U;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    const size_t freedBefore = EHeap::FreedSize();
    for (size_t repeat = 0; repeat < 3U; ++repeat) {
        EHeap::Free(block, size);
    }
    EXPECT_EQ(EHeap::FreedSize() - freedBefore, ExpectedFreedSize(size) * 3U);
    EXPECT_EQ(EHeap::AllocatedSize() % ALLOC_ALIGNMENT, 0U);
}
TEST_F(EHeapFreeTest, RepeatedFree_0064_x5)
{
    const size_t size = 64U;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    const size_t freedBefore = EHeap::FreedSize();
    for (size_t repeat = 0; repeat < 5U; ++repeat) {
        EHeap::Free(block, size);
    }
    EXPECT_EQ(EHeap::FreedSize() - freedBefore, ExpectedFreedSize(size) * 5U);
    EXPECT_EQ(EHeap::AllocatedSize() % ALLOC_ALIGNMENT, 0U);
}
TEST_F(EHeapFreeTest, RepeatedFree_0256_x2)
{
    const size_t size = 256U;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    const size_t freedBefore = EHeap::FreedSize();
    for (size_t repeat = 0; repeat < 2U; ++repeat) {
        EHeap::Free(block, size);
    }
    EXPECT_EQ(EHeap::FreedSize() - freedBefore, ExpectedFreedSize(size) * 2U);
    EXPECT_EQ(EHeap::AllocatedSize() % ALLOC_ALIGNMENT, 0U);
}
TEST_F(EHeapFreeTest, RepeatedFree_0256_x3)
{
    const size_t size = 256U;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    const size_t freedBefore = EHeap::FreedSize();
    for (size_t repeat = 0; repeat < 3U; ++repeat) {
        EHeap::Free(block, size);
    }
    EXPECT_EQ(EHeap::FreedSize() - freedBefore, ExpectedFreedSize(size) * 3U);
    EXPECT_EQ(EHeap::AllocatedSize() % ALLOC_ALIGNMENT, 0U);
}
TEST_F(EHeapFreeTest, RepeatedFree_0256_x5)
{
    const size_t size = 256U;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    const size_t freedBefore = EHeap::FreedSize();
    for (size_t repeat = 0; repeat < 5U; ++repeat) {
        EHeap::Free(block, size);
    }
    EXPECT_EQ(EHeap::FreedSize() - freedBefore, ExpectedFreedSize(size) * 5U);
    EXPECT_EQ(EHeap::AllocatedSize() % ALLOC_ALIGNMENT, 0U);
}
TEST_F(EHeapFreeTest, RepeatedFree_1024_x2)
{
    const size_t size = 1024U;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    const size_t freedBefore = EHeap::FreedSize();
    for (size_t repeat = 0; repeat < 2U; ++repeat) {
        EHeap::Free(block, size);
    }
    EXPECT_EQ(EHeap::FreedSize() - freedBefore, ExpectedFreedSize(size) * 2U);
    EXPECT_EQ(EHeap::AllocatedSize() % ALLOC_ALIGNMENT, 0U);
}
TEST_F(EHeapFreeTest, RepeatedFree_1024_x3)
{
    const size_t size = 1024U;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    const size_t freedBefore = EHeap::FreedSize();
    for (size_t repeat = 0; repeat < 3U; ++repeat) {
        EHeap::Free(block, size);
    }
    EXPECT_EQ(EHeap::FreedSize() - freedBefore, ExpectedFreedSize(size) * 3U);
    EXPECT_EQ(EHeap::AllocatedSize() % ALLOC_ALIGNMENT, 0U);
}
TEST_F(EHeapFreeTest, RepeatedFree_1024_x5)
{
    const size_t size = 1024U;
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    const size_t freedBefore = EHeap::FreedSize();
    for (size_t repeat = 0; repeat < 5U; ++repeat) {
        EHeap::Free(block, size);
    }
    EXPECT_EQ(EHeap::FreedSize() - freedBefore, ExpectedFreedSize(size) * 5U);
    EXPECT_EQ(EHeap::AllocatedSize() % ALLOC_ALIGNMENT, 0U);
}
TEST_F(EHeapFreeTest, FreeOddSize_0000)
{
    void *block = EHeap::Alloc(0U);
    ASSERT_NE(block, nullptr);
    const size_t freedBefore = EHeap::FreedSize();
    EHeap::Free(block, 0U);
    EXPECT_EQ(EHeap::FreedSize() - freedBefore, ExpectedFreedSize(0U));
    EXPECT_LE(EHeap::FreedSize() - freedBefore, 0U);
}
TEST_F(EHeapFreeTest, FreeOddSize_0003)
{
    void *block = EHeap::Alloc(3U);
    ASSERT_NE(block, nullptr);
    const size_t freedBefore = EHeap::FreedSize();
    EHeap::Free(block, 3U);
    EXPECT_EQ(EHeap::FreedSize() - freedBefore, ExpectedFreedSize(3U));
    EXPECT_LE(EHeap::FreedSize() - freedBefore, 3U);
}
TEST_F(EHeapFreeTest, FreeOddSize_0033)
{
    void *block = EHeap::Alloc(33U);
    ASSERT_NE(block, nullptr);
    const size_t freedBefore = EHeap::FreedSize();
    EHeap::Free(block, 33U);
    EXPECT_EQ(EHeap::FreedSize() - freedBefore, ExpectedFreedSize(33U));
    EXPECT_LE(EHeap::FreedSize() - freedBefore, 33U);
}
TEST_F(EHeapFreeTest, FreeOddSize_0333)
{
    void *block = EHeap::Alloc(333U);
    ASSERT_NE(block, nullptr);
    const size_t freedBefore = EHeap::FreedSize();
    EHeap::Free(block, 333U);
    EXPECT_EQ(EHeap::FreedSize() - freedBefore, ExpectedFreedSize(333U));
    EXPECT_LE(EHeap::FreedSize() - freedBefore, 333U);
}
TEST_F(EHeapFreeTest, FreeOddSize_3333)
{
    void *block = EHeap::Alloc(3333U);
    ASSERT_NE(block, nullptr);
    const size_t freedBefore = EHeap::FreedSize();
    EHeap::Free(block, 3333U);
    EXPECT_EQ(EHeap::FreedSize() - freedBefore, ExpectedFreedSize(3333U));
    EXPECT_LE(EHeap::FreedSize() - freedBefore, 3333U);
}
// ---------------------------------------------------------------------------
// Allocated/freed size accounting
// ---------------------------------------------------------------------------
class EHeapAccountingTest : public EHeapTest {};
TEST_F(EHeapAccountingTest, SumOf_001_Allocations)
{
    constexpr size_t ALLOCS = 1U;
    const size_t before = EHeap::AllocatedSize();
    size_t expected = 0U;
    for (size_t index = 0; index < ALLOCS; ++index) {
        const size_t size = index * 13U;
        ASSERT_NE(EHeap::Alloc(size), nullptr);
        expected += AlignUpBytes(size);
    }
    EXPECT_EQ(EHeap::AllocatedSize() - before, expected);
    EXPECT_EQ((EHeap::AllocatedSize() - before) % ALLOC_ALIGNMENT, 0U);
    EXPECT_GE(EHeap::AllocatedSize(), before);
}
TEST_F(EHeapAccountingTest, SumOf_002_Allocations)
{
    constexpr size_t ALLOCS = 2U;
    const size_t before = EHeap::AllocatedSize();
    size_t expected = 0U;
    for (size_t index = 0; index < ALLOCS; ++index) {
        const size_t size = index * 13U;
        ASSERT_NE(EHeap::Alloc(size), nullptr);
        expected += AlignUpBytes(size);
    }
    EXPECT_EQ(EHeap::AllocatedSize() - before, expected);
    EXPECT_EQ((EHeap::AllocatedSize() - before) % ALLOC_ALIGNMENT, 0U);
    EXPECT_GE(EHeap::AllocatedSize(), before);
}
TEST_F(EHeapAccountingTest, SumOf_003_Allocations)
{
    constexpr size_t ALLOCS = 3U;
    const size_t before = EHeap::AllocatedSize();
    size_t expected = 0U;
    for (size_t index = 0; index < ALLOCS; ++index) {
        const size_t size = index * 13U;
        ASSERT_NE(EHeap::Alloc(size), nullptr);
        expected += AlignUpBytes(size);
    }
    EXPECT_EQ(EHeap::AllocatedSize() - before, expected);
    EXPECT_EQ((EHeap::AllocatedSize() - before) % ALLOC_ALIGNMENT, 0U);
    EXPECT_GE(EHeap::AllocatedSize(), before);
}
TEST_F(EHeapAccountingTest, SumOf_004_Allocations)
{
    constexpr size_t ALLOCS = 4U;
    const size_t before = EHeap::AllocatedSize();
    size_t expected = 0U;
    for (size_t index = 0; index < ALLOCS; ++index) {
        const size_t size = index * 13U;
        ASSERT_NE(EHeap::Alloc(size), nullptr);
        expected += AlignUpBytes(size);
    }
    EXPECT_EQ(EHeap::AllocatedSize() - before, expected);
    EXPECT_EQ((EHeap::AllocatedSize() - before) % ALLOC_ALIGNMENT, 0U);
    EXPECT_GE(EHeap::AllocatedSize(), before);
}
TEST_F(EHeapAccountingTest, SumOf_006_Allocations)
{
    constexpr size_t ALLOCS = 6U;
    const size_t before = EHeap::AllocatedSize();
    size_t expected = 0U;
    for (size_t index = 0; index < ALLOCS; ++index) {
        const size_t size = index * 13U;
        ASSERT_NE(EHeap::Alloc(size), nullptr);
        expected += AlignUpBytes(size);
    }
    EXPECT_EQ(EHeap::AllocatedSize() - before, expected);
    EXPECT_EQ((EHeap::AllocatedSize() - before) % ALLOC_ALIGNMENT, 0U);
    EXPECT_GE(EHeap::AllocatedSize(), before);
}
TEST_F(EHeapAccountingTest, SumOf_008_Allocations)
{
    constexpr size_t ALLOCS = 8U;
    const size_t before = EHeap::AllocatedSize();
    size_t expected = 0U;
    for (size_t index = 0; index < ALLOCS; ++index) {
        const size_t size = index * 13U;
        ASSERT_NE(EHeap::Alloc(size), nullptr);
        expected += AlignUpBytes(size);
    }
    EXPECT_EQ(EHeap::AllocatedSize() - before, expected);
    EXPECT_EQ((EHeap::AllocatedSize() - before) % ALLOC_ALIGNMENT, 0U);
    EXPECT_GE(EHeap::AllocatedSize(), before);
}
TEST_F(EHeapAccountingTest, SumOf_010_Allocations)
{
    constexpr size_t ALLOCS = 10U;
    const size_t before = EHeap::AllocatedSize();
    size_t expected = 0U;
    for (size_t index = 0; index < ALLOCS; ++index) {
        const size_t size = index * 13U;
        ASSERT_NE(EHeap::Alloc(size), nullptr);
        expected += AlignUpBytes(size);
    }
    EXPECT_EQ(EHeap::AllocatedSize() - before, expected);
    EXPECT_EQ((EHeap::AllocatedSize() - before) % ALLOC_ALIGNMENT, 0U);
    EXPECT_GE(EHeap::AllocatedSize(), before);
}
TEST_F(EHeapAccountingTest, SumOf_012_Allocations)
{
    constexpr size_t ALLOCS = 12U;
    const size_t before = EHeap::AllocatedSize();
    size_t expected = 0U;
    for (size_t index = 0; index < ALLOCS; ++index) {
        const size_t size = index * 13U;
        ASSERT_NE(EHeap::Alloc(size), nullptr);
        expected += AlignUpBytes(size);
    }
    EXPECT_EQ(EHeap::AllocatedSize() - before, expected);
    EXPECT_EQ((EHeap::AllocatedSize() - before) % ALLOC_ALIGNMENT, 0U);
    EXPECT_GE(EHeap::AllocatedSize(), before);
}
TEST_F(EHeapAccountingTest, SumOf_016_Allocations)
{
    constexpr size_t ALLOCS = 16U;
    const size_t before = EHeap::AllocatedSize();
    size_t expected = 0U;
    for (size_t index = 0; index < ALLOCS; ++index) {
        const size_t size = index * 13U;
        ASSERT_NE(EHeap::Alloc(size), nullptr);
        expected += AlignUpBytes(size);
    }
    EXPECT_EQ(EHeap::AllocatedSize() - before, expected);
    EXPECT_EQ((EHeap::AllocatedSize() - before) % ALLOC_ALIGNMENT, 0U);
    EXPECT_GE(EHeap::AllocatedSize(), before);
}
TEST_F(EHeapAccountingTest, SumOf_020_Allocations)
{
    constexpr size_t ALLOCS = 20U;
    const size_t before = EHeap::AllocatedSize();
    size_t expected = 0U;
    for (size_t index = 0; index < ALLOCS; ++index) {
        const size_t size = index * 13U;
        ASSERT_NE(EHeap::Alloc(size), nullptr);
        expected += AlignUpBytes(size);
    }
    EXPECT_EQ(EHeap::AllocatedSize() - before, expected);
    EXPECT_EQ((EHeap::AllocatedSize() - before) % ALLOC_ALIGNMENT, 0U);
    EXPECT_GE(EHeap::AllocatedSize(), before);
}
TEST_F(EHeapAccountingTest, SumOf_024_Allocations)
{
    constexpr size_t ALLOCS = 24U;
    const size_t before = EHeap::AllocatedSize();
    size_t expected = 0U;
    for (size_t index = 0; index < ALLOCS; ++index) {
        const size_t size = index * 13U;
        ASSERT_NE(EHeap::Alloc(size), nullptr);
        expected += AlignUpBytes(size);
    }
    EXPECT_EQ(EHeap::AllocatedSize() - before, expected);
    EXPECT_EQ((EHeap::AllocatedSize() - before) % ALLOC_ALIGNMENT, 0U);
    EXPECT_GE(EHeap::AllocatedSize(), before);
}
TEST_F(EHeapAccountingTest, SumOf_032_Allocations)
{
    constexpr size_t ALLOCS = 32U;
    const size_t before = EHeap::AllocatedSize();
    size_t expected = 0U;
    for (size_t index = 0; index < ALLOCS; ++index) {
        const size_t size = index * 13U;
        ASSERT_NE(EHeap::Alloc(size), nullptr);
        expected += AlignUpBytes(size);
    }
    EXPECT_EQ(EHeap::AllocatedSize() - before, expected);
    EXPECT_EQ((EHeap::AllocatedSize() - before) % ALLOC_ALIGNMENT, 0U);
    EXPECT_GE(EHeap::AllocatedSize(), before);
}
TEST_F(EHeapAccountingTest, SumOf_040_Allocations)
{
    constexpr size_t ALLOCS = 40U;
    const size_t before = EHeap::AllocatedSize();
    size_t expected = 0U;
    for (size_t index = 0; index < ALLOCS; ++index) {
        const size_t size = index * 13U;
        ASSERT_NE(EHeap::Alloc(size), nullptr);
        expected += AlignUpBytes(size);
    }
    EXPECT_EQ(EHeap::AllocatedSize() - before, expected);
    EXPECT_EQ((EHeap::AllocatedSize() - before) % ALLOC_ALIGNMENT, 0U);
    EXPECT_GE(EHeap::AllocatedSize(), before);
}
TEST_F(EHeapAccountingTest, SumOf_048_Allocations)
{
    constexpr size_t ALLOCS = 48U;
    const size_t before = EHeap::AllocatedSize();
    size_t expected = 0U;
    for (size_t index = 0; index < ALLOCS; ++index) {
        const size_t size = index * 13U;
        ASSERT_NE(EHeap::Alloc(size), nullptr);
        expected += AlignUpBytes(size);
    }
    EXPECT_EQ(EHeap::AllocatedSize() - before, expected);
    EXPECT_EQ((EHeap::AllocatedSize() - before) % ALLOC_ALIGNMENT, 0U);
    EXPECT_GE(EHeap::AllocatedSize(), before);
}
TEST_F(EHeapAccountingTest, FreeEveryBlock_004)
{
    constexpr size_t ALLOCS = 4U;
    const size_t before = EHeap::AllocatedSize();
    size_t expected = 0U;
    std::array<void *, ALLOCS> blocks {};
    for (size_t index = 0; index < ALLOCS; ++index) {
        const size_t size = 5U + index;
        blocks[index] = EHeap::Alloc(size);
        ASSERT_NE(blocks[index], nullptr);
        expected += AlignUpBytes(size);
    }
    EXPECT_EQ(EHeap::AllocatedSize() - before, expected);
    size_t freedBefore = EHeap::FreedSize();
    size_t expectedFreed = 0U;
    for (size_t index = 0; index < ALLOCS; ++index) {
        EHeap::Free(blocks[index], 5U + index);
        expectedFreed += ExpectedFreedSize(5U + index);
    }
    EXPECT_EQ(EHeap::FreedSize() - freedBefore, expectedFreed);
    EXPECT_EQ(EHeap::AllocatedSize() - before, expected);
}
TEST_F(EHeapAccountingTest, FreeEveryBlock_008)
{
    constexpr size_t ALLOCS = 8U;
    const size_t before = EHeap::AllocatedSize();
    size_t expected = 0U;
    std::array<void *, ALLOCS> blocks {};
    for (size_t index = 0; index < ALLOCS; ++index) {
        const size_t size = 5U + index;
        blocks[index] = EHeap::Alloc(size);
        ASSERT_NE(blocks[index], nullptr);
        expected += AlignUpBytes(size);
    }
    EXPECT_EQ(EHeap::AllocatedSize() - before, expected);
    size_t freedBefore = EHeap::FreedSize();
    size_t expectedFreed = 0U;
    for (size_t index = 0; index < ALLOCS; ++index) {
        EHeap::Free(blocks[index], 5U + index);
        expectedFreed += ExpectedFreedSize(5U + index);
    }
    EXPECT_EQ(EHeap::FreedSize() - freedBefore, expectedFreed);
    EXPECT_EQ(EHeap::AllocatedSize() - before, expected);
}
TEST_F(EHeapAccountingTest, FreeEveryBlock_016)
{
    constexpr size_t ALLOCS = 16U;
    const size_t before = EHeap::AllocatedSize();
    size_t expected = 0U;
    std::array<void *, ALLOCS> blocks {};
    for (size_t index = 0; index < ALLOCS; ++index) {
        const size_t size = 5U + index;
        blocks[index] = EHeap::Alloc(size);
        ASSERT_NE(blocks[index], nullptr);
        expected += AlignUpBytes(size);
    }
    EXPECT_EQ(EHeap::AllocatedSize() - before, expected);
    size_t freedBefore = EHeap::FreedSize();
    size_t expectedFreed = 0U;
    for (size_t index = 0; index < ALLOCS; ++index) {
        EHeap::Free(blocks[index], 5U + index);
        expectedFreed += ExpectedFreedSize(5U + index);
    }
    EXPECT_EQ(EHeap::FreedSize() - freedBefore, expectedFreed);
    EXPECT_EQ(EHeap::AllocatedSize() - before, expected);
}
TEST_F(EHeapAccountingTest, FreeEveryBlock_032)
{
    constexpr size_t ALLOCS = 32U;
    const size_t before = EHeap::AllocatedSize();
    size_t expected = 0U;
    std::array<void *, ALLOCS> blocks {};
    for (size_t index = 0; index < ALLOCS; ++index) {
        const size_t size = 5U + index;
        blocks[index] = EHeap::Alloc(size);
        ASSERT_NE(blocks[index], nullptr);
        expected += AlignUpBytes(size);
    }
    EXPECT_EQ(EHeap::AllocatedSize() - before, expected);
    size_t freedBefore = EHeap::FreedSize();
    size_t expectedFreed = 0U;
    for (size_t index = 0; index < ALLOCS; ++index) {
        EHeap::Free(blocks[index], 5U + index);
        expectedFreed += ExpectedFreedSize(5U + index);
    }
    EXPECT_EQ(EHeap::FreedSize() - freedBefore, expectedFreed);
    EXPECT_EQ(EHeap::AllocatedSize() - before, expected);
}
TEST_F(EHeapAccountingTest, MonotonicGrowth_005_Steps)
{
    constexpr size_t STEPS = 5U;
    size_t previous = EHeap::AllocatedSize();
    size_t allocated = 0U;
    for (size_t step = 0; step < STEPS; ++step) {
        const size_t size = 1U + ((step * 7U) % 97U);
        void *block = EHeap::Alloc(size);
        ASSERT_NE(block, nullptr);
        allocated += AlignUpBytes(size);
        const size_t current = EHeap::AllocatedSize();
        EXPECT_GE(current, previous);
        EXPECT_EQ(current - previous, AlignUpBytes(size));
        previous = current;
    }
    EXPECT_GT(allocated, 0U);
}
TEST_F(EHeapAccountingTest, MonotonicGrowth_010_Steps)
{
    constexpr size_t STEPS = 10U;
    size_t previous = EHeap::AllocatedSize();
    size_t allocated = 0U;
    for (size_t step = 0; step < STEPS; ++step) {
        const size_t size = 1U + ((step * 7U) % 97U);
        void *block = EHeap::Alloc(size);
        ASSERT_NE(block, nullptr);
        allocated += AlignUpBytes(size);
        const size_t current = EHeap::AllocatedSize();
        EXPECT_GE(current, previous);
        EXPECT_EQ(current - previous, AlignUpBytes(size));
        previous = current;
    }
    EXPECT_GT(allocated, 0U);
}
TEST_F(EHeapAccountingTest, MonotonicGrowth_020_Steps)
{
    constexpr size_t STEPS = 20U;
    size_t previous = EHeap::AllocatedSize();
    size_t allocated = 0U;
    for (size_t step = 0; step < STEPS; ++step) {
        const size_t size = 1U + ((step * 7U) % 97U);
        void *block = EHeap::Alloc(size);
        ASSERT_NE(block, nullptr);
        allocated += AlignUpBytes(size);
        const size_t current = EHeap::AllocatedSize();
        EXPECT_GE(current, previous);
        EXPECT_EQ(current - previous, AlignUpBytes(size));
        previous = current;
    }
    EXPECT_GT(allocated, 0U);
}
TEST_F(EHeapAccountingTest, MonotonicGrowth_050_Steps)
{
    constexpr size_t STEPS = 50U;
    size_t previous = EHeap::AllocatedSize();
    size_t allocated = 0U;
    for (size_t step = 0; step < STEPS; ++step) {
        const size_t size = 1U + ((step * 7U) % 97U);
        void *block = EHeap::Alloc(size);
        ASSERT_NE(block, nullptr);
        allocated += AlignUpBytes(size);
        const size_t current = EHeap::AllocatedSize();
        EXPECT_GE(current, previous);
        EXPECT_EQ(current - previous, AlignUpBytes(size));
        previous = current;
    }
    EXPECT_GT(allocated, 0U);
}
// ---------------------------------------------------------------------------
// EHeap::Scope lifetime: initialization, nesting, finalization and re-creation
// ---------------------------------------------------------------------------
TEST(EHeapLifecycleTest, InitializeFinalizeTogglesState)
{
    ScopedAllocatorsManagerSession session;
    EXPECT_TRUE(ScopedAllocatorsManager::IsInitialized());
    EXPECT_EQ(EHeap::AllocatedSize(), 0U);
    EXPECT_EQ(EHeap::FreedSize(), 0U);
    ScopedAllocatorsManager::Finalize();
    EXPECT_FALSE(ScopedAllocatorsManager::IsInitialized());
    ScopedAllocatorsManager::Initialize();
    EXPECT_TRUE(ScopedAllocatorsManager::IsInitialized());
}
TEST(EHeapLifecycleTest, FreshScopeReservesTheNullSlot)
{
    ScopedAllocatorsManagerSession session;
    EXPECT_EQ(EHeap::AllocatedSize(), 0U);
    {
        EHeap::Scope scope;
        // EHeap::InitializeEHeapSpace() consumes one aligned slot so that a compressed
        // nullptr is never produced by a valid allocation.
        EXPECT_EQ(EHeap::AllocatedSize(), ALLOC_ALIGNMENT);
        void *first = EHeap::Alloc(0U);
        ASSERT_NE(first, nullptr);
        EXPECT_EQ(EHeap::AllocatedSize(), ALLOC_ALIGNMENT);
    }
    EXPECT_EQ(EHeap::AllocatedSize(), 0U);
    EXPECT_EQ(EHeap::FreedSize(), 0U);
}
TEST(EHeapLifecycleTest, ScopeDestructionReleasesTheHeap)
{
    ScopedAllocatorsManagerSession session;
    {
        EHeap::Scope scope;
        for (size_t index = 0; index < 32U; ++index) {
            ASSERT_NE(EHeap::Alloc(index), nullptr);
        }
        EXPECT_GT(EHeap::AllocatedSize(), 0U);
        EHeap::Free(EHeap::Alloc(64U), 64U);
        EXPECT_GE(EHeap::FreedSize(), ExpectedFreedSize(64U));
    }
    EXPECT_EQ(EHeap::AllocatedSize(), 0U);
    EXPECT_EQ(EHeap::FreedSize(), 0U);
}
TEST(EHeapLifecycleTest, SizeGettersAreZeroWithoutHeap)
{
    ScopedAllocatorsManagerSession session;
    EXPECT_EQ(EHeap::AllocatedSize(), 0U);
    EXPECT_EQ(EHeap::FreedSize(), 0U);
    EHeap::ResetEHeapSpace();
    EXPECT_EQ(EHeap::AllocatedSize(), 0U);
    EXPECT_EQ(EHeap::FreedSize(), 0U);
}
TEST(EHeapLifecycleTest, NestedScopesKeepTheHeapAlive)
{
    ScopedAllocatorsManagerSession session;
    EHeap::Scope outer;
    void *block = EHeap::Alloc(256U);
    ASSERT_NE(block, nullptr);
    FillPattern(block, 256U, 0x11U);
    const size_t allocatedOuter = EHeap::AllocatedSize();
    {
        EHeap::Scope inner;
        EXPECT_EQ(EHeap::AllocatedSize(), allocatedOuter);
        void *innerBlock = EHeap::Alloc(32U);
        ASSERT_NE(innerBlock, nullptr);
        FillByte(innerBlock, 32U, 0xA5U);
        EXPECT_EQ(EHeap::AllocatedSize(), allocatedOuter + AlignUpBytes(32U));
    }
    EXPECT_EQ(EHeap::AllocatedSize(), allocatedOuter + AlignUpBytes(32U));
    EXPECT_TRUE(PatternMatches(block, 256U, 0x11U));
}
TEST(EHeapLifecycleTest, ThreeLevelNestingKeepsAllocations)
{
    ScopedAllocatorsManagerSession session;
    EHeap::Scope level1;
    void *block1 = EHeap::Alloc(64U);
    ASSERT_NE(block1, nullptr);
    FillByte(block1, 64U, 0x01U);
    {
        EHeap::Scope level2;
        void *block2 = EHeap::Alloc(128U);
        ASSERT_NE(block2, nullptr);
        FillByte(block2, 128U, 0x02U);
        {
            EHeap::Scope level3;
            void *block3 = EHeap::Alloc(256U);
            ASSERT_NE(block3, nullptr);
            FillByte(block3, 256U, 0x03U);
            EXPECT_TRUE(AllBytes(block1, 64U, 0x01U));
            EXPECT_TRUE(AllBytes(block2, 128U, 0x02U));
            EXPECT_TRUE(AllBytes(block3, 256U, 0x03U));
        }
        EXPECT_TRUE(AllBytes(block1, 64U, 0x01U));
        EXPECT_TRUE(AllBytes(block2, 128U, 0x02U));
    }
    EXPECT_TRUE(AllBytes(block1, 64U, 0x01U));
    EXPECT_GT(EHeap::AllocatedSize(), AlignUpBytes(64U + 128U + 256U));
}
TEST(EHeapLifecycleTest, ManyNestedScopesShareOneHeap)
{
    ScopedAllocatorsManagerSession session;
    constexpr size_t DEPTH = 128U;
    std::vector<std::unique_ptr<EHeap::Scope>> scopes;
    scopes.reserve(DEPTH);
    scopes.push_back(std::make_unique<EHeap::Scope>());
    const size_t base = EHeap::AllocatedSize();
    for (size_t index = 1; index < DEPTH; ++index) {
        scopes.push_back(std::make_unique<EHeap::Scope>());
        ASSERT_NE(EHeap::Alloc(8U), nullptr);
    }
    EXPECT_EQ(EHeap::AllocatedSize(), base + (DEPTH - 1U) * AlignUpBytes(8U));
    while (!scopes.empty()) {
        scopes.pop_back();
    }
    EXPECT_EQ(EHeap::AllocatedSize(), 0U);
}
TEST(EHeapLifecycleTest, AllocationSurvivesNestedScopeDestruction)
{
    ScopedAllocatorsManagerSession session;
    EHeap::Scope outer;
    auto *value = EAllocator::New<int>(1234);
    ASSERT_NE(value, nullptr);
    {
        EHeap::Scope inner;
        EXPECT_EQ(*value, 1234);
        *value = 4321;
    }
    EXPECT_EQ(*value, 4321);
}
TEST(EHeapLifecycleTest, ReinitializationResetsAccounting)
{
    ScopedAllocatorsManagerSession session;
    {
        EHeap::Scope scope;
        EXPECT_EQ(EHeap::AllocatedSize(), ALLOC_ALIGNMENT);
        ASSERT_NE(EHeap::Alloc(4096U), nullptr);
        EXPECT_EQ(EHeap::AllocatedSize(), ALLOC_ALIGNMENT + AlignUpBytes(4096U));
        EHeap::Free(EHeap::Alloc(64U), 64U);
        EXPECT_GE(EHeap::FreedSize(), ExpectedFreedSize(64U));
    }
    {
        EHeap::Scope scope;
        EXPECT_EQ(EHeap::AllocatedSize(), ALLOC_ALIGNMENT);
        EXPECT_EQ(EHeap::FreedSize(), 0U);
    }
}
TEST(EHeapLifecycleTest, AllocationWorksAgainAfterReinitialization)
{
    ScopedAllocatorsManagerSession session;
    {
        EHeap::Scope scope;
        void *block = EHeap::Alloc(512U);
        ASSERT_NE(block, nullptr);
        FillPattern(block, 512U, 0x31U);
    }
    {
        EHeap::Scope scope;
        void *block = EHeap::Alloc(512U);
        ASSERT_NE(block, nullptr);
        EXPECT_TRUE(IsAligned(block));
        FillPattern(block, 512U, 0x32U);
        EXPECT_TRUE(PatternMatches(block, 512U, 0x32U));
    }
}
TEST(EHeapLifecycleTest, WorkerThreadScopeKeepsMainHeapAlive)
{
    ScopedAllocatorsManagerSession session;
    EHeap::Scope mainScope;
    void *block = EHeap::Alloc(128U);
    ASSERT_NE(block, nullptr);
    FillPattern(block, 128U, 0xABCU);
    const size_t allocatedBefore = EHeap::AllocatedSize();
    std::atomic<bool> workerDone {false};
    std::thread worker {[&workerDone]() {
        EHeap::Scope workerScope;
        HeapMutationGuard guard;
        void *inner = EHeap::Alloc(64U);
        if (inner != nullptr) {
            FillByte(inner, 64U, 0x7EU);
        }
        // Atomic with release order reason: publish the worker result before join
        workerDone.store(true, std::memory_order_release);
    }};
    worker.join();
    // Atomic with acquire order reason: flag is observed after join
    EXPECT_TRUE(workerDone.load(std::memory_order_acquire));
    EXPECT_EQ(EHeap::AllocatedSize() - allocatedBefore, 64U);
    EXPECT_TRUE(PatternMatches(block, 128U, 0xABCU));
}
TEST(EHeapLifecycleTest, WorkerThreadAllocationIsVisibleToMain)
{
    ScopedAllocatorsManagerSession session;
    EHeap::Scope mainScope;
    std::atomic<void *> workerBlock {nullptr};
    std::thread worker {[&workerBlock]() {
        EHeap::Scope workerScope;
        HeapMutationGuard guard;
        void *block = EHeap::Alloc(96U);
        if (block != nullptr) {
            FillPattern(block, 96U, 0x55U);
        }
        // Atomic with release order reason: publish the allocated block before join
        workerBlock.store(block, std::memory_order_release);
    }};
    worker.join();
    // Atomic with acquire order reason: block is observed after join
    void *block = workerBlock.load(std::memory_order_acquire);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(PatternMatches(block, 96U, 0x55U));
    EXPECT_TRUE(IsAligned(block));
}
TEST(EHeapLifecycleTest, FreeWithoutHeapIsANoOp)
{
    ScopedAllocatorsManagerSession session;
    int dummy = 0;
    EAllocator::Free(&dummy, sizeof(dummy));
    EHeap::Free(&dummy, sizeof(dummy));
    EXPECT_EQ(EHeap::FreedSize(), 0U);
    EXPECT_EQ(EHeap::AllocatedSize(), 0U);
}
TEST(EHeapLifecycleTest, ReinitializationCycles_002)
{
    ScopedAllocatorsManagerSession session;
    size_t previousId = EHeap::DeprecatedId();
    for (size_t cycle = 0; cycle < 2U; ++cycle) {
        EHeap::Scope scope;
        EXPECT_EQ(EHeap::AllocatedSize(), ALLOC_ALIGNMENT);
        ASSERT_NE(EHeap::Alloc(16U + cycle), nullptr);
        const size_t currentId = EHeap::DeprecatedId();
        EXPECT_GT(currentId, previousId);
        previousId = currentId;
    }
    EXPECT_EQ(EHeap::DeprecatedId(), previousId);
    EXPECT_EQ(EHeap::AllocatedSize(), 0U);
}
TEST(EHeapLifecycleTest, ReinitializationCycles_003)
{
    ScopedAllocatorsManagerSession session;
    size_t previousId = EHeap::DeprecatedId();
    for (size_t cycle = 0; cycle < 3U; ++cycle) {
        EHeap::Scope scope;
        EXPECT_EQ(EHeap::AllocatedSize(), ALLOC_ALIGNMENT);
        ASSERT_NE(EHeap::Alloc(16U + cycle), nullptr);
        const size_t currentId = EHeap::DeprecatedId();
        EXPECT_GT(currentId, previousId);
        previousId = currentId;
    }
    EXPECT_EQ(EHeap::DeprecatedId(), previousId);
    EXPECT_EQ(EHeap::AllocatedSize(), 0U);
}
TEST(EHeapLifecycleTest, ReinitializationCycles_004)
{
    ScopedAllocatorsManagerSession session;
    size_t previousId = EHeap::DeprecatedId();
    for (size_t cycle = 0; cycle < 4U; ++cycle) {
        EHeap::Scope scope;
        EXPECT_EQ(EHeap::AllocatedSize(), ALLOC_ALIGNMENT);
        ASSERT_NE(EHeap::Alloc(16U + cycle), nullptr);
        const size_t currentId = EHeap::DeprecatedId();
        EXPECT_GT(currentId, previousId);
        previousId = currentId;
    }
    EXPECT_EQ(EHeap::DeprecatedId(), previousId);
    EXPECT_EQ(EHeap::AllocatedSize(), 0U);
}
TEST(EHeapLifecycleTest, ReinitializationCycles_008)
{
    ScopedAllocatorsManagerSession session;
    size_t previousId = EHeap::DeprecatedId();
    for (size_t cycle = 0; cycle < 8U; ++cycle) {
        EHeap::Scope scope;
        EXPECT_EQ(EHeap::AllocatedSize(), ALLOC_ALIGNMENT);
        ASSERT_NE(EHeap::Alloc(16U + cycle), nullptr);
        const size_t currentId = EHeap::DeprecatedId();
        EXPECT_GT(currentId, previousId);
        previousId = currentId;
    }
    EXPECT_EQ(EHeap::DeprecatedId(), previousId);
    EXPECT_EQ(EHeap::AllocatedSize(), 0U);
}
TEST(EHeapLifecycleTest, ReinitializationCycles_016)
{
    ScopedAllocatorsManagerSession session;
    size_t previousId = EHeap::DeprecatedId();
    for (size_t cycle = 0; cycle < 16U; ++cycle) {
        EHeap::Scope scope;
        EXPECT_EQ(EHeap::AllocatedSize(), ALLOC_ALIGNMENT);
        ASSERT_NE(EHeap::Alloc(16U + cycle), nullptr);
        const size_t currentId = EHeap::DeprecatedId();
        EXPECT_GT(currentId, previousId);
        previousId = currentId;
    }
    EXPECT_EQ(EHeap::DeprecatedId(), previousId);
    EXPECT_EQ(EHeap::AllocatedSize(), 0U);
}
TEST(EHeapLifecycleTest, ReinitializationCycles_032)
{
    ScopedAllocatorsManagerSession session;
    size_t previousId = EHeap::DeprecatedId();
    for (size_t cycle = 0; cycle < 32U; ++cycle) {
        EHeap::Scope scope;
        EXPECT_EQ(EHeap::AllocatedSize(), ALLOC_ALIGNMENT);
        ASSERT_NE(EHeap::Alloc(16U + cycle), nullptr);
        const size_t currentId = EHeap::DeprecatedId();
        EXPECT_GT(currentId, previousId);
        previousId = currentId;
    }
    EXPECT_EQ(EHeap::DeprecatedId(), previousId);
    EXPECT_EQ(EHeap::AllocatedSize(), 0U);
}
TEST(EHeapLifecycleTest, ReinitializationCycles_064)
{
    ScopedAllocatorsManagerSession session;
    size_t previousId = EHeap::DeprecatedId();
    for (size_t cycle = 0; cycle < 64U; ++cycle) {
        EHeap::Scope scope;
        EXPECT_EQ(EHeap::AllocatedSize(), ALLOC_ALIGNMENT);
        ASSERT_NE(EHeap::Alloc(16U + cycle), nullptr);
        const size_t currentId = EHeap::DeprecatedId();
        EXPECT_GT(currentId, previousId);
        previousId = currentId;
    }
    EXPECT_EQ(EHeap::DeprecatedId(), previousId);
    EXPECT_EQ(EHeap::AllocatedSize(), 0U);
}
// ---------------------------------------------------------------------------
// EHeap::ResetEHeapSpace: the current contract is a no-op while scopes are alive
// ---------------------------------------------------------------------------
class EHeapResetTest : public EHeapTest {};
TEST_F(EHeapResetTest, ResetWithActiveScopeKeepsAllocatedSize)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(1024U);
    ASSERT_NE(block, nullptr);
    const size_t allocated = EHeap::AllocatedSize();
    EHeap::ResetEHeapSpace();
    EXPECT_EQ(EHeap::AllocatedSize(), allocated);
    EXPECT_GT(EHeap::AllocatedSize(), before);
}
TEST_F(EHeapResetTest, ResetWithActiveScopePreservesContent)
{
    void *block = EHeap::Alloc(2048U);
    ASSERT_NE(block, nullptr);
    FillPattern(block, 2048U, 0xC0FFEEU);
    EHeap::ResetEHeapSpace();
    EXPECT_TRUE(PatternMatches(block, 2048U, 0xC0FFEEU));
}
TEST_F(EHeapResetTest, ResetWithActiveScopeKeepsCursor)
{
    void *first = EHeap::Alloc(128U);
    ASSERT_NE(first, nullptr);
    EHeap::ResetEHeapSpace();
    void *second = EHeap::Alloc(128U);
    ASSERT_NE(second, nullptr);
    EXPECT_GT(Addr(second), Addr(first));
    EXPECT_FALSE(RangesOverlap(first, 128U, second, 128U));
}
TEST_F(EHeapResetTest, ResetIsIdempotent)
{
    void *block = EHeap::Alloc(64U);
    ASSERT_NE(block, nullptr);
    FillByte(block, 64U, 0x42U);
    const size_t allocated = EHeap::AllocatedSize();
    const size_t heapId = EHeap::DeprecatedId();
    EHeap::ResetEHeapSpace();
    EHeap::ResetEHeapSpace();
    EHeap::ResetEHeapSpace();
    EXPECT_EQ(EHeap::AllocatedSize(), allocated);
    EXPECT_EQ(EHeap::DeprecatedId(), heapId);
    EXPECT_TRUE(AllBytes(block, 64U, 0x42U));
}
TEST_F(EHeapResetTest, ResetKeepsFreedAccounting)
{
    void *block = EHeap::Alloc(256U);
    ASSERT_NE(block, nullptr);
    EHeap::Free(block, 256U);
    const size_t freed = EHeap::FreedSize();
    EHeap::ResetEHeapSpace();
    EXPECT_EQ(EHeap::FreedSize(), freed);
}
TEST_F(EHeapResetTest, ResetWithNestedScopeIsNoOp)
{
    void *block = EHeap::Alloc(512U);
    ASSERT_NE(block, nullptr);
    FillPattern(block, 512U, 0x99U);
    EHeap::Scope nested;
    const size_t allocated = EHeap::AllocatedSize();
    EHeap::ResetEHeapSpace();
    EXPECT_EQ(EHeap::AllocatedSize(), allocated);
    EXPECT_TRUE(PatternMatches(block, 512U, 0x99U));
}
TEST_F(EHeapResetTest, ResetFromIdleWorkerThreadIsNoOp)
{
    void *block = EHeap::Alloc(384U);
    ASSERT_NE(block, nullptr);
    FillPattern(block, 384U, 0x77U);
    const size_t allocated = EHeap::AllocatedSize();
    std::atomic<bool> resetDone {false};
    std::thread worker {[&resetDone]() {
        EHeap::ResetEHeapSpace();
        // Atomic with release order reason: publish reset completion before join
        resetDone.store(true, std::memory_order_release);
    }};
    worker.join();
    // Atomic with acquire order reason: flag is observed after join
    EXPECT_TRUE(resetDone.load(std::memory_order_acquire));
    EXPECT_EQ(EHeap::AllocatedSize(), allocated);
    EXPECT_TRUE(PatternMatches(block, 384U, 0x77U));
}
TEST_F(EHeapResetTest, ResetAfterAllocations_0000)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(0U);
    ASSERT_NE(block, nullptr);
    const size_t allocated = EHeap::AllocatedSize();
    EHeap::ResetEHeapSpace();
    EXPECT_EQ(EHeap::AllocatedSize(), allocated);
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(0U));
    EXPECT_TRUE(IsAligned(block));
}
TEST_F(EHeapResetTest, ResetAfterAllocations_0001)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(1U);
    ASSERT_NE(block, nullptr);
    const size_t allocated = EHeap::AllocatedSize();
    EHeap::ResetEHeapSpace();
    EXPECT_EQ(EHeap::AllocatedSize(), allocated);
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(1U));
    EXPECT_TRUE(IsAligned(block));
}
TEST_F(EHeapResetTest, ResetAfterAllocations_0007)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(7U);
    ASSERT_NE(block, nullptr);
    const size_t allocated = EHeap::AllocatedSize();
    EHeap::ResetEHeapSpace();
    EXPECT_EQ(EHeap::AllocatedSize(), allocated);
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(7U));
    EXPECT_TRUE(IsAligned(block));
}
TEST_F(EHeapResetTest, ResetAfterAllocations_0008)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(8U);
    ASSERT_NE(block, nullptr);
    const size_t allocated = EHeap::AllocatedSize();
    EHeap::ResetEHeapSpace();
    EXPECT_EQ(EHeap::AllocatedSize(), allocated);
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(8U));
    EXPECT_TRUE(IsAligned(block));
}
TEST_F(EHeapResetTest, ResetAfterAllocations_0009)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(9U);
    ASSERT_NE(block, nullptr);
    const size_t allocated = EHeap::AllocatedSize();
    EHeap::ResetEHeapSpace();
    EXPECT_EQ(EHeap::AllocatedSize(), allocated);
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(9U));
    EXPECT_TRUE(IsAligned(block));
}
TEST_F(EHeapResetTest, ResetAfterAllocations_0064)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(64U);
    ASSERT_NE(block, nullptr);
    const size_t allocated = EHeap::AllocatedSize();
    EHeap::ResetEHeapSpace();
    EXPECT_EQ(EHeap::AllocatedSize(), allocated);
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(64U));
    EXPECT_TRUE(IsAligned(block));
}
TEST_F(EHeapResetTest, ResetAfterAllocations_0100)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(100U);
    ASSERT_NE(block, nullptr);
    const size_t allocated = EHeap::AllocatedSize();
    EHeap::ResetEHeapSpace();
    EXPECT_EQ(EHeap::AllocatedSize(), allocated);
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(100U));
    EXPECT_TRUE(IsAligned(block));
}
TEST_F(EHeapResetTest, ResetAfterAllocations_0256)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(256U);
    ASSERT_NE(block, nullptr);
    const size_t allocated = EHeap::AllocatedSize();
    EHeap::ResetEHeapSpace();
    EXPECT_EQ(EHeap::AllocatedSize(), allocated);
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(256U));
    EXPECT_TRUE(IsAligned(block));
}
TEST_F(EHeapResetTest, ResetAfterAllocations_1024)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(1024U);
    ASSERT_NE(block, nullptr);
    const size_t allocated = EHeap::AllocatedSize();
    EHeap::ResetEHeapSpace();
    EXPECT_EQ(EHeap::AllocatedSize(), allocated);
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(1024U));
    EXPECT_TRUE(IsAligned(block));
}
TEST_F(EHeapResetTest, ResetAfterAllocations_4096)
{
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(4096U);
    ASSERT_NE(block, nullptr);
    const size_t allocated = EHeap::AllocatedSize();
    EHeap::ResetEHeapSpace();
    EXPECT_EQ(EHeap::AllocatedSize(), allocated);
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(4096U));
    EXPECT_TRUE(IsAligned(block));
}
TEST_F(EHeapResetTest, ResetInterleavedWith_02_Allocations)
{
    std::array<BlockRecord, 2U> blocks {};
    for (size_t index = 0; index < 2U; ++index) {
        const size_t size = 16U * (index + 1U);
        void *block = EHeap::Alloc(size);
        ASSERT_NE(block, nullptr);
        blocks[index] = BlockRecord {block, size, index};
        FillPattern(block, size, index);
        EHeap::ResetEHeapSpace();
    }
    for (size_t index = 0; index < 2U; ++index) {
        EXPECT_TRUE(PatternMatches(blocks[index].ptr, blocks[index].size, blocks[index].seed));
    }
    for (size_t lhs = 0; lhs < 2U; ++lhs) {
        for (size_t rhs = lhs + 1U; rhs < 2U; ++rhs) {
            EXPECT_FALSE(RangesOverlap(blocks[lhs].ptr, blocks[lhs].size, blocks[rhs].ptr, blocks[rhs].size));
        }
    }
}
TEST_F(EHeapResetTest, ResetInterleavedWith_04_Allocations)
{
    std::array<BlockRecord, 4U> blocks {};
    for (size_t index = 0; index < 4U; ++index) {
        const size_t size = 16U * (index + 1U);
        void *block = EHeap::Alloc(size);
        ASSERT_NE(block, nullptr);
        blocks[index] = BlockRecord {block, size, index};
        FillPattern(block, size, index);
        EHeap::ResetEHeapSpace();
    }
    for (size_t index = 0; index < 4U; ++index) {
        EXPECT_TRUE(PatternMatches(blocks[index].ptr, blocks[index].size, blocks[index].seed));
    }
    for (size_t lhs = 0; lhs < 4U; ++lhs) {
        for (size_t rhs = lhs + 1U; rhs < 4U; ++rhs) {
            EXPECT_FALSE(RangesOverlap(blocks[lhs].ptr, blocks[lhs].size, blocks[rhs].ptr, blocks[rhs].size));
        }
    }
}
TEST_F(EHeapResetTest, ResetInterleavedWith_08_Allocations)
{
    std::array<BlockRecord, 8U> blocks {};
    for (size_t index = 0; index < 8U; ++index) {
        const size_t size = 16U * (index + 1U);
        void *block = EHeap::Alloc(size);
        ASSERT_NE(block, nullptr);
        blocks[index] = BlockRecord {block, size, index};
        FillPattern(block, size, index);
        EHeap::ResetEHeapSpace();
    }
    for (size_t index = 0; index < 8U; ++index) {
        EXPECT_TRUE(PatternMatches(blocks[index].ptr, blocks[index].size, blocks[index].seed));
    }
    for (size_t lhs = 0; lhs < 8U; ++lhs) {
        for (size_t rhs = lhs + 1U; rhs < 8U; ++rhs) {
            EXPECT_FALSE(RangesOverlap(blocks[lhs].ptr, blocks[lhs].size, blocks[rhs].ptr, blocks[rhs].size));
        }
    }
}
TEST_F(EHeapResetTest, ResetInterleavedWith_16_Allocations)
{
    std::array<BlockRecord, 16U> blocks {};
    for (size_t index = 0; index < 16U; ++index) {
        const size_t size = 16U * (index + 1U);
        void *block = EHeap::Alloc(size);
        ASSERT_NE(block, nullptr);
        blocks[index] = BlockRecord {block, size, index};
        FillPattern(block, size, index);
        EHeap::ResetEHeapSpace();
    }
    for (size_t index = 0; index < 16U; ++index) {
        EXPECT_TRUE(PatternMatches(blocks[index].ptr, blocks[index].size, blocks[index].seed));
    }
    for (size_t lhs = 0; lhs < 16U; ++lhs) {
        for (size_t rhs = lhs + 1U; rhs < 16U; ++rhs) {
            EXPECT_FALSE(RangesOverlap(blocks[lhs].ptr, blocks[lhs].size, blocks[rhs].ptr, blocks[rhs].size));
        }
    }
}
TEST_F(EHeapResetTest, ResetDoesNotInvalidateAllocatorObjects)
{
    auto *value = EAllocator::New<ValueProbe>(11, 22);
    ASSERT_NE(value, nullptr);
    EHeap::ResetEHeapSpace();
    EXPECT_EQ(value->first, 11);
    EXPECT_EQ(value->second, 22);
}
TEST(EHeapResetWithoutHeapTest, ResetWithoutHeapDoesNotCrash)
{
    ScopedAllocatorsManagerSession session;
    EHeap::ResetEHeapSpace();
    EHeap::ResetEHeapSpace();
    EXPECT_EQ(EHeap::AllocatedSize(), 0U);
    EXPECT_EQ(EHeap::FreedSize(), 0U);
}
TEST(EHeapResetWithoutHeapTest, ResetAfterScopeDestructionDoesNotCrash)
{
    ScopedAllocatorsManagerSession session;
    {
        EHeap::Scope scope;
        ASSERT_NE(EHeap::Alloc(32U), nullptr);
    }
    EHeap::ResetEHeapSpace();
    EXPECT_EQ(EHeap::AllocatedSize(), 0U);
}
// ---------------------------------------------------------------------------
// EHeap::DeprecatedId: monotonic heap generation counter
// ---------------------------------------------------------------------------
class EHeapDeprecatedIdTest : public EHeapTest {};
TEST_F(EHeapDeprecatedIdTest, PositiveAfterInitialization)
{
    EXPECT_GT(EHeap::DeprecatedId(), 0U);
}
TEST_F(EHeapDeprecatedIdTest, StableAcross_001_Allocations)
{
    const size_t heapId = EHeap::DeprecatedId();
    for (size_t index = 0; index < 1U; ++index) {
        ASSERT_NE(EHeap::Alloc(index), nullptr);
        EXPECT_EQ(EHeap::DeprecatedId(), heapId);
    }
}
TEST_F(EHeapDeprecatedIdTest, StableAcross_002_Allocations)
{
    const size_t heapId = EHeap::DeprecatedId();
    for (size_t index = 0; index < 2U; ++index) {
        ASSERT_NE(EHeap::Alloc(index), nullptr);
        EXPECT_EQ(EHeap::DeprecatedId(), heapId);
    }
}
TEST_F(EHeapDeprecatedIdTest, StableAcross_005_Allocations)
{
    const size_t heapId = EHeap::DeprecatedId();
    for (size_t index = 0; index < 5U; ++index) {
        ASSERT_NE(EHeap::Alloc(index), nullptr);
        EXPECT_EQ(EHeap::DeprecatedId(), heapId);
    }
}
TEST_F(EHeapDeprecatedIdTest, StableAcross_016_Allocations)
{
    const size_t heapId = EHeap::DeprecatedId();
    for (size_t index = 0; index < 16U; ++index) {
        ASSERT_NE(EHeap::Alloc(index), nullptr);
        EXPECT_EQ(EHeap::DeprecatedId(), heapId);
    }
}
TEST_F(EHeapDeprecatedIdTest, StableAcross_064_Allocations)
{
    const size_t heapId = EHeap::DeprecatedId();
    for (size_t index = 0; index < 64U; ++index) {
        ASSERT_NE(EHeap::Alloc(index), nullptr);
        EXPECT_EQ(EHeap::DeprecatedId(), heapId);
    }
}
TEST_F(EHeapDeprecatedIdTest, StableAcross_256_Allocations)
{
    const size_t heapId = EHeap::DeprecatedId();
    for (size_t index = 0; index < 256U; ++index) {
        ASSERT_NE(EHeap::Alloc(index), nullptr);
        EXPECT_EQ(EHeap::DeprecatedId(), heapId);
    }
}
TEST_F(EHeapDeprecatedIdTest, StableAcrossFree)
{
    void *block = EHeap::Alloc(64U);
    ASSERT_NE(block, nullptr);
    const size_t heapId = EHeap::DeprecatedId();
    EHeap::Free(block, 64U);
    EXPECT_EQ(EHeap::DeprecatedId(), heapId);
}
TEST_F(EHeapDeprecatedIdTest, StableAcrossNestedScope)
{
    const size_t heapId = EHeap::DeprecatedId();
    {
        EHeap::Scope nested;
        ASSERT_NE(EHeap::Alloc(32U), nullptr);
        EXPECT_EQ(EHeap::DeprecatedId(), heapId);
    }
    EXPECT_EQ(EHeap::DeprecatedId(), heapId);
}
TEST_F(EHeapDeprecatedIdTest, StableAcrossNoOpReset)
{
    const size_t heapId = EHeap::DeprecatedId();
    EHeap::ResetEHeapSpace();
    EXPECT_EQ(EHeap::DeprecatedId(), heapId);
}
TEST_F(EHeapDeprecatedIdTest, NonDecreasingAcrossAllocations)
{
    size_t previous = EHeap::DeprecatedId();
    for (size_t index = 0; index < 128U; ++index) {
        ASSERT_NE(EHeap::Alloc(1U + index), nullptr);
        const size_t current = EHeap::DeprecatedId();
        EXPECT_GE(current, previous);
        previous = current;
    }
}
TEST(EHeapDeprecatedIdLifecycleTest, IncreasesBy_01_PerReinitCycle)
{
    ScopedAllocatorsManagerSession session;
    const size_t initialId = EHeap::DeprecatedId();
    for (size_t cycle = 0; cycle < 1U; ++cycle) {
        EHeap::Scope scope;
        EXPECT_EQ(EHeap::DeprecatedId(), initialId + cycle + 1U);
        ASSERT_NE(EHeap::Alloc(8U), nullptr);
    }
    EXPECT_EQ(EHeap::DeprecatedId(), initialId + 1U);
}
TEST(EHeapDeprecatedIdLifecycleTest, IncreasesBy_02_PerReinitCycle)
{
    ScopedAllocatorsManagerSession session;
    const size_t initialId = EHeap::DeprecatedId();
    for (size_t cycle = 0; cycle < 2U; ++cycle) {
        EHeap::Scope scope;
        EXPECT_EQ(EHeap::DeprecatedId(), initialId + cycle + 1U);
        ASSERT_NE(EHeap::Alloc(8U), nullptr);
    }
    EXPECT_EQ(EHeap::DeprecatedId(), initialId + 2U);
}
TEST(EHeapDeprecatedIdLifecycleTest, IncreasesBy_03_PerReinitCycle)
{
    ScopedAllocatorsManagerSession session;
    const size_t initialId = EHeap::DeprecatedId();
    for (size_t cycle = 0; cycle < 3U; ++cycle) {
        EHeap::Scope scope;
        EXPECT_EQ(EHeap::DeprecatedId(), initialId + cycle + 1U);
        ASSERT_NE(EHeap::Alloc(8U), nullptr);
    }
    EXPECT_EQ(EHeap::DeprecatedId(), initialId + 3U);
}
TEST(EHeapDeprecatedIdLifecycleTest, IncreasesBy_04_PerReinitCycle)
{
    ScopedAllocatorsManagerSession session;
    const size_t initialId = EHeap::DeprecatedId();
    for (size_t cycle = 0; cycle < 4U; ++cycle) {
        EHeap::Scope scope;
        EXPECT_EQ(EHeap::DeprecatedId(), initialId + cycle + 1U);
        ASSERT_NE(EHeap::Alloc(8U), nullptr);
    }
    EXPECT_EQ(EHeap::DeprecatedId(), initialId + 4U);
}
TEST(EHeapDeprecatedIdLifecycleTest, IncreasesBy_08_PerReinitCycle)
{
    ScopedAllocatorsManagerSession session;
    const size_t initialId = EHeap::DeprecatedId();
    for (size_t cycle = 0; cycle < 8U; ++cycle) {
        EHeap::Scope scope;
        EXPECT_EQ(EHeap::DeprecatedId(), initialId + cycle + 1U);
        ASSERT_NE(EHeap::Alloc(8U), nullptr);
    }
    EXPECT_EQ(EHeap::DeprecatedId(), initialId + 8U);
}
TEST(EHeapDeprecatedIdLifecycleTest, IncreasesBy_16_PerReinitCycle)
{
    ScopedAllocatorsManagerSession session;
    const size_t initialId = EHeap::DeprecatedId();
    for (size_t cycle = 0; cycle < 16U; ++cycle) {
        EHeap::Scope scope;
        EXPECT_EQ(EHeap::DeprecatedId(), initialId + cycle + 1U);
        ASSERT_NE(EHeap::Alloc(8U), nullptr);
    }
    EXPECT_EQ(EHeap::DeprecatedId(), initialId + 16U);
}
TEST(EHeapDeprecatedIdLifecycleTest, UnchangedAfterFinalize)
{
    ScopedAllocatorsManagerSession session;
    size_t heapId = 0U;
    {
        EHeap::Scope scope;
        heapId = EHeap::DeprecatedId();
    }
    ScopedAllocatorsManager::Finalize();
    EXPECT_EQ(EHeap::DeprecatedId(), heapId);
    ScopedAllocatorsManager::Initialize();
}
TEST(EHeapDeprecatedIdLifecycleTest, StrictlyIncreasingOverCycles)
{
    ScopedAllocatorsManagerSession session;
    size_t previous = EHeap::DeprecatedId();
    for (size_t cycle = 0; cycle < 24U; ++cycle) {
        EHeap::Scope scope;
        const size_t current = EHeap::DeprecatedId();
        EXPECT_GT(current, previous);
        previous = current;
    }
}
// ---------------------------------------------------------------------------
// EAllocator: raw allocation, object construction, array construction
// ---------------------------------------------------------------------------
class EAllocatorTest : public EHeapTest {};
TEST_F(EAllocatorTest, NewInt__2147483648)
{
    auto *value = EAllocator::New<int>(-2147483648);
    ASSERT_NE(value, nullptr);
    EXPECT_EQ(*value, -2147483648);
    EXPECT_TRUE(IsAligned(value));
    *value = *value + 1;
    EXPECT_EQ(*value, -2147483648 + 1);
}
TEST_F(EAllocatorTest, NewInt__1)
{
    auto *value = EAllocator::New<int>(-1);
    ASSERT_NE(value, nullptr);
    EXPECT_EQ(*value, -1);
    EXPECT_TRUE(IsAligned(value));
    *value = *value + 1;
    EXPECT_EQ(*value, -1 + 1);
}
TEST_F(EAllocatorTest, NewInt_0)
{
    auto *value = EAllocator::New<int>(0);
    ASSERT_NE(value, nullptr);
    EXPECT_EQ(*value, 0);
    EXPECT_TRUE(IsAligned(value));
    *value = *value + 1;
    EXPECT_EQ(*value, 0 + 1);
}
TEST_F(EAllocatorTest, NewInt_1)
{
    auto *value = EAllocator::New<int>(1);
    ASSERT_NE(value, nullptr);
    EXPECT_EQ(*value, 1);
    EXPECT_TRUE(IsAligned(value));
    *value = *value + 1;
    EXPECT_EQ(*value, 1 + 1);
}
TEST_F(EAllocatorTest, NewInt_42)
{
    auto *value = EAllocator::New<int>(42);
    ASSERT_NE(value, nullptr);
    EXPECT_EQ(*value, 42);
    EXPECT_TRUE(IsAligned(value));
    *value = *value + 1;
    EXPECT_EQ(*value, 42 + 1);
}
TEST_F(EAllocatorTest, NewInt_255)
{
    auto *value = EAllocator::New<int>(255);
    ASSERT_NE(value, nullptr);
    EXPECT_EQ(*value, 255);
    EXPECT_TRUE(IsAligned(value));
    *value = *value + 1;
    EXPECT_EQ(*value, 255 + 1);
}
TEST_F(EAllocatorTest, NewInt_65535)
{
    auto *value = EAllocator::New<int>(65535);
    ASSERT_NE(value, nullptr);
    EXPECT_EQ(*value, 65535);
    EXPECT_TRUE(IsAligned(value));
    *value = *value + 1;
    EXPECT_EQ(*value, 65535 + 1);
}
TEST_F(EAllocatorTest, NewInt_2147483647)
{
    auto *value = EAllocator::New<int>(2147483647);
    ASSERT_NE(value, nullptr);
    EXPECT_EQ(*value, 2147483647);
    EXPECT_TRUE(IsAligned(value));
    *value = *value + 1;
    EXPECT_EQ(*value, 2147483647 + 1);
}
TEST_F(EAllocatorTest, NewDouble_0_0)
{
    auto *value = EAllocator::New<double>(0.0);
    ASSERT_NE(value, nullptr);
    EXPECT_DOUBLE_EQ(*value, 0.0);
    EXPECT_TRUE(IsAligned(value));
}
TEST_F(EAllocatorTest, NewDouble_neg0_5)
{
    auto *value = EAllocator::New<double>(-0.5);
    ASSERT_NE(value, nullptr);
    EXPECT_DOUBLE_EQ(*value, -0.5);
    EXPECT_TRUE(IsAligned(value));
}
TEST_F(EAllocatorTest, NewDouble_1_25)
{
    auto *value = EAllocator::New<double>(1.25);
    ASSERT_NE(value, nullptr);
    EXPECT_DOUBLE_EQ(*value, 1.25);
    EXPECT_TRUE(IsAligned(value));
}
TEST_F(EAllocatorTest, NewDouble_35000000000_0)
{
    auto *value = EAllocator::New<double>(35000000000.0);
    ASSERT_NE(value, nullptr);
    EXPECT_DOUBLE_EQ(*value, 35000000000.0);
    EXPECT_TRUE(IsAligned(value));
}
TEST_F(EAllocatorTest, NewValueProbe_0_0)
{
    auto *probe = EAllocator::New<ValueProbe>(0, 0);
    ASSERT_NE(probe, nullptr);
    EXPECT_EQ(probe->first, 0);
    EXPECT_EQ(probe->second, 0);
    EXPECT_TRUE(IsAligned(probe));
    probe->second = probe->first;
    EXPECT_EQ(probe->second, 0);
}
TEST_F(EAllocatorTest, NewValueProbe_1_2)
{
    auto *probe = EAllocator::New<ValueProbe>(1, 2);
    ASSERT_NE(probe, nullptr);
    EXPECT_EQ(probe->first, 1);
    EXPECT_EQ(probe->second, 2);
    EXPECT_TRUE(IsAligned(probe));
    probe->second = probe->first;
    EXPECT_EQ(probe->second, 1);
}
TEST_F(EAllocatorTest, NewValueProbe_neg1_1)
{
    auto *probe = EAllocator::New<ValueProbe>(-1, 1);
    ASSERT_NE(probe, nullptr);
    EXPECT_EQ(probe->first, -1);
    EXPECT_EQ(probe->second, 1);
    EXPECT_TRUE(IsAligned(probe));
    probe->second = probe->first;
    EXPECT_EQ(probe->second, -1);
}
TEST_F(EAllocatorTest, NewValueProbe_7_neg7)
{
    auto *probe = EAllocator::New<ValueProbe>(7, -7);
    ASSERT_NE(probe, nullptr);
    EXPECT_EQ(probe->first, 7);
    EXPECT_EQ(probe->second, -7);
    EXPECT_TRUE(IsAligned(probe));
    probe->second = probe->first;
    EXPECT_EQ(probe->second, 7);
}
TEST_F(EAllocatorTest, NewValueProbe_100_200)
{
    auto *probe = EAllocator::New<ValueProbe>(100, 200);
    ASSERT_NE(probe, nullptr);
    EXPECT_EQ(probe->first, 100);
    EXPECT_EQ(probe->second, 200);
    EXPECT_TRUE(IsAligned(probe));
    probe->second = probe->first;
    EXPECT_EQ(probe->second, 100);
}
TEST_F(EAllocatorTest, NewValueProbe_999_1000)
{
    auto *probe = EAllocator::New<ValueProbe>(999, 1000);
    ASSERT_NE(probe, nullptr);
    EXPECT_EQ(probe->first, 999);
    EXPECT_EQ(probe->second, 1000);
    EXPECT_TRUE(IsAligned(probe));
    probe->second = probe->first;
    EXPECT_EQ(probe->second, 999);
}
TEST_F(EAllocatorTest, NewIntArrayZeroInit_0000)
{
    constexpr size_t COUNT = 0U;
    int *array = EAllocator::New<int[]>(COUNT);
    ASSERT_NE(array, nullptr);
    EXPECT_TRUE(IsAligned(array));
    for (size_t index = 0; index < COUNT; ++index) {
        EXPECT_EQ(array[index], 0);
        array[index] = static_cast<int>(index * 3U);
    }
    for (size_t index = 0; index < COUNT; ++index) {
        EXPECT_EQ(array[index], static_cast<int>(index * 3U));
    }
}
TEST_F(EAllocatorTest, NewIntArrayZeroInit_0001)
{
    constexpr size_t COUNT = 1U;
    int *array = EAllocator::New<int[]>(COUNT);
    ASSERT_NE(array, nullptr);
    EXPECT_TRUE(IsAligned(array));
    for (size_t index = 0; index < COUNT; ++index) {
        EXPECT_EQ(array[index], 0);
        array[index] = static_cast<int>(index * 3U);
    }
    for (size_t index = 0; index < COUNT; ++index) {
        EXPECT_EQ(array[index], static_cast<int>(index * 3U));
    }
}
TEST_F(EAllocatorTest, NewIntArrayZeroInit_0002)
{
    constexpr size_t COUNT = 2U;
    int *array = EAllocator::New<int[]>(COUNT);
    ASSERT_NE(array, nullptr);
    EXPECT_TRUE(IsAligned(array));
    for (size_t index = 0; index < COUNT; ++index) {
        EXPECT_EQ(array[index], 0);
        array[index] = static_cast<int>(index * 3U);
    }
    for (size_t index = 0; index < COUNT; ++index) {
        EXPECT_EQ(array[index], static_cast<int>(index * 3U));
    }
}
TEST_F(EAllocatorTest, NewIntArrayZeroInit_0003)
{
    constexpr size_t COUNT = 3U;
    int *array = EAllocator::New<int[]>(COUNT);
    ASSERT_NE(array, nullptr);
    EXPECT_TRUE(IsAligned(array));
    for (size_t index = 0; index < COUNT; ++index) {
        EXPECT_EQ(array[index], 0);
        array[index] = static_cast<int>(index * 3U);
    }
    for (size_t index = 0; index < COUNT; ++index) {
        EXPECT_EQ(array[index], static_cast<int>(index * 3U));
    }
}
TEST_F(EAllocatorTest, NewIntArrayZeroInit_0008)
{
    constexpr size_t COUNT = 8U;
    int *array = EAllocator::New<int[]>(COUNT);
    ASSERT_NE(array, nullptr);
    EXPECT_TRUE(IsAligned(array));
    for (size_t index = 0; index < COUNT; ++index) {
        EXPECT_EQ(array[index], 0);
        array[index] = static_cast<int>(index * 3U);
    }
    for (size_t index = 0; index < COUNT; ++index) {
        EXPECT_EQ(array[index], static_cast<int>(index * 3U));
    }
}
TEST_F(EAllocatorTest, NewIntArrayZeroInit_0016)
{
    constexpr size_t COUNT = 16U;
    int *array = EAllocator::New<int[]>(COUNT);
    ASSERT_NE(array, nullptr);
    EXPECT_TRUE(IsAligned(array));
    for (size_t index = 0; index < COUNT; ++index) {
        EXPECT_EQ(array[index], 0);
        array[index] = static_cast<int>(index * 3U);
    }
    for (size_t index = 0; index < COUNT; ++index) {
        EXPECT_EQ(array[index], static_cast<int>(index * 3U));
    }
}
TEST_F(EAllocatorTest, NewIntArrayZeroInit_0064)
{
    constexpr size_t COUNT = 64U;
    int *array = EAllocator::New<int[]>(COUNT);
    ASSERT_NE(array, nullptr);
    EXPECT_TRUE(IsAligned(array));
    for (size_t index = 0; index < COUNT; ++index) {
        EXPECT_EQ(array[index], 0);
        array[index] = static_cast<int>(index * 3U);
    }
    for (size_t index = 0; index < COUNT; ++index) {
        EXPECT_EQ(array[index], static_cast<int>(index * 3U));
    }
}
TEST_F(EAllocatorTest, NewIntArrayZeroInit_0100)
{
    constexpr size_t COUNT = 100U;
    int *array = EAllocator::New<int[]>(COUNT);
    ASSERT_NE(array, nullptr);
    EXPECT_TRUE(IsAligned(array));
    for (size_t index = 0; index < COUNT; ++index) {
        EXPECT_EQ(array[index], 0);
        array[index] = static_cast<int>(index * 3U);
    }
    for (size_t index = 0; index < COUNT; ++index) {
        EXPECT_EQ(array[index], static_cast<int>(index * 3U));
    }
}
TEST_F(EAllocatorTest, NewArrayConstructsEveryElement)
{
    constexpr size_t COUNT = 32U;
    // Atomic with relaxed order reason: single-threaded probe counter
    DefaultConstructibleProbe::constructed.store(0U, std::memory_order_relaxed);
    // Atomic with relaxed order reason: single-threaded probe counter
    DefaultConstructibleProbe::destroyed.store(0U, std::memory_order_relaxed);
    auto *array = EAllocator::New<DefaultConstructibleProbe[]>(COUNT);
    ASSERT_NE(array, nullptr);
    // Atomic with relaxed order reason: single-threaded probe counter
    EXPECT_EQ(DefaultConstructibleProbe::constructed.load(std::memory_order_relaxed), COUNT);
    for (size_t index = 0; index < COUNT; ++index) {
        EXPECT_EQ(array[index].value, 42);
        EXPECT_TRUE(array[index].defaultConstructed);
        array[index].~DefaultConstructibleProbe();
    }
    // Atomic with relaxed order reason: single-threaded probe counter
    EXPECT_EQ(DefaultConstructibleProbe::destroyed.load(std::memory_order_relaxed), COUNT);
}
TEST_F(EAllocatorTest, NewInvokesValueConstructor)
{
    // Atomic with relaxed order reason: single-threaded probe counter
    DefaultConstructibleProbe::constructed.store(0U, std::memory_order_relaxed);
    // Atomic with relaxed order reason: single-threaded probe counter
    DefaultConstructibleProbe::destroyed.store(0U, std::memory_order_relaxed);
    auto *probe = EAllocator::New<DefaultConstructibleProbe>(7);
    ASSERT_NE(probe, nullptr);
    // Atomic with relaxed order reason: single-threaded probe counter
    EXPECT_EQ(DefaultConstructibleProbe::constructed.load(std::memory_order_relaxed), 1U);
    EXPECT_EQ(probe->value, 7);
    EXPECT_FALSE(probe->defaultConstructed);
    probe->~DefaultConstructibleProbe();
    // Atomic with relaxed order reason: single-threaded probe counter
    EXPECT_EQ(DefaultConstructibleProbe::destroyed.load(std::memory_order_relaxed), 1U);
}
TEST_F(EAllocatorTest, NewLargeProbe_00)
{
    auto *probe = EAllocator::New<LargeProbe>(0x00U);
    ASSERT_NE(probe, nullptr);
    EXPECT_TRUE(IsAligned(probe));
    EXPECT_EQ(probe->payload.front(), 0x00U);
    EXPECT_EQ(probe->payload.back(), 0x00U);
    EXPECT_EQ(static_cast<size_t>(std::count(probe->payload.begin(), probe->payload.end(), 0x00U)),
              probe->payload.size());
    probe->payload[0] = 0x11U;
    EXPECT_EQ(probe->payload[0], 0x11U);
}
TEST_F(EAllocatorTest, NewLargeProbe_5a)
{
    auto *probe = EAllocator::New<LargeProbe>(0x5AU);
    ASSERT_NE(probe, nullptr);
    EXPECT_TRUE(IsAligned(probe));
    EXPECT_EQ(probe->payload.front(), 0x5AU);
    EXPECT_EQ(probe->payload.back(), 0x5AU);
    EXPECT_EQ(static_cast<size_t>(std::count(probe->payload.begin(), probe->payload.end(), 0x5AU)),
              probe->payload.size());
    probe->payload[0] = 0x11U;
    EXPECT_EQ(probe->payload[0], 0x11U);
}
TEST_F(EAllocatorTest, NewLargeProbe_a5)
{
    auto *probe = EAllocator::New<LargeProbe>(0xA5U);
    ASSERT_NE(probe, nullptr);
    EXPECT_TRUE(IsAligned(probe));
    EXPECT_EQ(probe->payload.front(), 0xA5U);
    EXPECT_EQ(probe->payload.back(), 0xA5U);
    EXPECT_EQ(static_cast<size_t>(std::count(probe->payload.begin(), probe->payload.end(), 0xA5U)),
              probe->payload.size());
    probe->payload[0] = 0x11U;
    EXPECT_EQ(probe->payload[0], 0x11U);
}
TEST_F(EAllocatorTest, NewLargeProbe_ff)
{
    auto *probe = EAllocator::New<LargeProbe>(0xFFU);
    ASSERT_NE(probe, nullptr);
    EXPECT_TRUE(IsAligned(probe));
    EXPECT_EQ(probe->payload.front(), 0xFFU);
    EXPECT_EQ(probe->payload.back(), 0xFFU);
    EXPECT_EQ(static_cast<size_t>(std::count(probe->payload.begin(), probe->payload.end(), 0xFFU)),
              probe->payload.size());
    probe->payload[0] = 0x11U;
    EXPECT_EQ(probe->payload[0], 0x11U);
}
TEST_F(EAllocatorTest, NewManyObjects_0001)
{
    constexpr int COUNT = 1;
    std::array<int *, COUNT> pointers {};
    for (int index = 0; index < COUNT; ++index) {
        pointers[static_cast<size_t>(index)] = EAllocator::New<int>(index);
        ASSERT_NE(pointers[static_cast<size_t>(index)], nullptr);
    }
    for (int index = 0; index < COUNT; ++index) {
        EXPECT_EQ(*pointers[static_cast<size_t>(index)], index);
        EXPECT_TRUE(IsAligned(pointers[static_cast<size_t>(index)]));
    }
    for (int index = 1; index < COUNT; ++index) {
        EXPECT_GT(Addr(pointers[static_cast<size_t>(index)]), Addr(pointers[static_cast<size_t>(index - 1)]));
    }
}
TEST_F(EAllocatorTest, NewManyObjects_0002)
{
    constexpr int COUNT = 2;
    std::array<int *, COUNT> pointers {};
    for (int index = 0; index < COUNT; ++index) {
        pointers[static_cast<size_t>(index)] = EAllocator::New<int>(index);
        ASSERT_NE(pointers[static_cast<size_t>(index)], nullptr);
    }
    for (int index = 0; index < COUNT; ++index) {
        EXPECT_EQ(*pointers[static_cast<size_t>(index)], index);
        EXPECT_TRUE(IsAligned(pointers[static_cast<size_t>(index)]));
    }
    for (int index = 1; index < COUNT; ++index) {
        EXPECT_GT(Addr(pointers[static_cast<size_t>(index)]), Addr(pointers[static_cast<size_t>(index - 1)]));
    }
}
TEST_F(EAllocatorTest, NewManyObjects_0005)
{
    constexpr int COUNT = 5;
    std::array<int *, COUNT> pointers {};
    for (int index = 0; index < COUNT; ++index) {
        pointers[static_cast<size_t>(index)] = EAllocator::New<int>(index);
        ASSERT_NE(pointers[static_cast<size_t>(index)], nullptr);
    }
    for (int index = 0; index < COUNT; ++index) {
        EXPECT_EQ(*pointers[static_cast<size_t>(index)], index);
        EXPECT_TRUE(IsAligned(pointers[static_cast<size_t>(index)]));
    }
    for (int index = 1; index < COUNT; ++index) {
        EXPECT_GT(Addr(pointers[static_cast<size_t>(index)]), Addr(pointers[static_cast<size_t>(index - 1)]));
    }
}
TEST_F(EAllocatorTest, NewManyObjects_0016)
{
    constexpr int COUNT = 16;
    std::array<int *, COUNT> pointers {};
    for (int index = 0; index < COUNT; ++index) {
        pointers[static_cast<size_t>(index)] = EAllocator::New<int>(index);
        ASSERT_NE(pointers[static_cast<size_t>(index)], nullptr);
    }
    for (int index = 0; index < COUNT; ++index) {
        EXPECT_EQ(*pointers[static_cast<size_t>(index)], index);
        EXPECT_TRUE(IsAligned(pointers[static_cast<size_t>(index)]));
    }
    for (int index = 1; index < COUNT; ++index) {
        EXPECT_GT(Addr(pointers[static_cast<size_t>(index)]), Addr(pointers[static_cast<size_t>(index - 1)]));
    }
}
TEST_F(EAllocatorTest, NewManyObjects_0064)
{
    constexpr int COUNT = 64;
    std::array<int *, COUNT> pointers {};
    for (int index = 0; index < COUNT; ++index) {
        pointers[static_cast<size_t>(index)] = EAllocator::New<int>(index);
        ASSERT_NE(pointers[static_cast<size_t>(index)], nullptr);
    }
    for (int index = 0; index < COUNT; ++index) {
        EXPECT_EQ(*pointers[static_cast<size_t>(index)], index);
        EXPECT_TRUE(IsAligned(pointers[static_cast<size_t>(index)]));
    }
    for (int index = 1; index < COUNT; ++index) {
        EXPECT_GT(Addr(pointers[static_cast<size_t>(index)]), Addr(pointers[static_cast<size_t>(index - 1)]));
    }
}
TEST_F(EAllocatorTest, NewManyObjects_0128)
{
    constexpr int COUNT = 128;
    std::array<int *, COUNT> pointers {};
    for (int index = 0; index < COUNT; ++index) {
        pointers[static_cast<size_t>(index)] = EAllocator::New<int>(index);
        ASSERT_NE(pointers[static_cast<size_t>(index)], nullptr);
    }
    for (int index = 0; index < COUNT; ++index) {
        EXPECT_EQ(*pointers[static_cast<size_t>(index)], index);
        EXPECT_TRUE(IsAligned(pointers[static_cast<size_t>(index)]));
    }
    for (int index = 1; index < COUNT; ++index) {
        EXPECT_GT(Addr(pointers[static_cast<size_t>(index)]), Addr(pointers[static_cast<size_t>(index - 1)]));
    }
}
TEST_F(EAllocatorTest, AllocAndFree_00000)
{
    const size_t size = 0U;
    void *block = EAllocator::Alloc(size);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    FillPattern(block, size, 0x1FU);
    EXPECT_TRUE(PatternMatches(block, size, 0x1FU));
    const size_t freedBefore = EHeap::FreedSize();
    EAllocator::Free(block, size);
    EXPECT_EQ(EHeap::FreedSize() - freedBefore, ExpectedFreedSize(size));
}
TEST_F(EAllocatorTest, AllocAndFree_00001)
{
    const size_t size = 1U;
    void *block = EAllocator::Alloc(size);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    FillPattern(block, size, 0x1FU);
    EXPECT_TRUE(PatternMatches(block, size, 0x1FU));
    const size_t freedBefore = EHeap::FreedSize();
    EAllocator::Free(block, size);
    EXPECT_EQ(EHeap::FreedSize() - freedBefore, ExpectedFreedSize(size));
}
TEST_F(EAllocatorTest, AllocAndFree_00007)
{
    const size_t size = 7U;
    void *block = EAllocator::Alloc(size);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    FillPattern(block, size, 0x1FU);
    EXPECT_TRUE(PatternMatches(block, size, 0x1FU));
    const size_t freedBefore = EHeap::FreedSize();
    EAllocator::Free(block, size);
    EXPECT_EQ(EHeap::FreedSize() - freedBefore, ExpectedFreedSize(size));
}
TEST_F(EAllocatorTest, AllocAndFree_00008)
{
    const size_t size = 8U;
    void *block = EAllocator::Alloc(size);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    FillPattern(block, size, 0x1FU);
    EXPECT_TRUE(PatternMatches(block, size, 0x1FU));
    const size_t freedBefore = EHeap::FreedSize();
    EAllocator::Free(block, size);
    EXPECT_EQ(EHeap::FreedSize() - freedBefore, ExpectedFreedSize(size));
}
TEST_F(EAllocatorTest, AllocAndFree_00009)
{
    const size_t size = 9U;
    void *block = EAllocator::Alloc(size);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    FillPattern(block, size, 0x1FU);
    EXPECT_TRUE(PatternMatches(block, size, 0x1FU));
    const size_t freedBefore = EHeap::FreedSize();
    EAllocator::Free(block, size);
    EXPECT_EQ(EHeap::FreedSize() - freedBefore, ExpectedFreedSize(size));
}
TEST_F(EAllocatorTest, AllocAndFree_00032)
{
    const size_t size = 32U;
    void *block = EAllocator::Alloc(size);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    FillPattern(block, size, 0x1FU);
    EXPECT_TRUE(PatternMatches(block, size, 0x1FU));
    const size_t freedBefore = EHeap::FreedSize();
    EAllocator::Free(block, size);
    EXPECT_EQ(EHeap::FreedSize() - freedBefore, ExpectedFreedSize(size));
}
TEST_F(EAllocatorTest, AllocAndFree_00128)
{
    const size_t size = 128U;
    void *block = EAllocator::Alloc(size);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    FillPattern(block, size, 0x1FU);
    EXPECT_TRUE(PatternMatches(block, size, 0x1FU));
    const size_t freedBefore = EHeap::FreedSize();
    EAllocator::Free(block, size);
    EXPECT_EQ(EHeap::FreedSize() - freedBefore, ExpectedFreedSize(size));
}
TEST_F(EAllocatorTest, AllocAndFree_01024)
{
    const size_t size = 1024U;
    void *block = EAllocator::Alloc(size);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    FillPattern(block, size, 0x1FU);
    EXPECT_TRUE(PatternMatches(block, size, 0x1FU));
    const size_t freedBefore = EHeap::FreedSize();
    EAllocator::Free(block, size);
    EXPECT_EQ(EHeap::FreedSize() - freedBefore, ExpectedFreedSize(size));
}
TEST_F(EAllocatorTest, AllocAndFree_08192)
{
    const size_t size = 8192U;
    void *block = EAllocator::Alloc(size);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    FillPattern(block, size, 0x1FU);
    EXPECT_TRUE(PatternMatches(block, size, 0x1FU));
    const size_t freedBefore = EHeap::FreedSize();
    EAllocator::Free(block, size);
    EXPECT_EQ(EHeap::FreedSize() - freedBefore, ExpectedFreedSize(size));
}
TEST_F(EAllocatorTest, AdapterIsAvailableAndComparable)
{
    EAllocator allocator;
    EAllocatorAdapter<void> first = allocator.Adapter();
    EAllocatorAdapter<void> second = allocator.Adapter();
    EXPECT_TRUE(first == second);
    EXPECT_FALSE(first != second);
    EAllocatorAdapter<int> intAdapter {&allocator};
    EXPECT_TRUE(intAdapter == EAllocatorAdapter<int> {&allocator});
}
TEST_F(EAllocatorTest, CreateAllocatorAndNewAllocatorAgree)
{
    EAllocator inlineAllocator = EHeap::CreateAllocator();
    std::unique_ptr<EAllocator> heapAllocator = EHeap::NewAllocator();
    ASSERT_NE(heapAllocator, nullptr);
    EXPECT_TRUE(inlineAllocator.Adapter() == heapAllocator->Adapter());
}
TEST_F(EAllocatorTest, NewPairKeepsBothFields)
{
    auto *pair = EAllocator::New<std::pair<int, int>>(3, 4);
    ASSERT_NE(pair, nullptr);
    EXPECT_EQ(pair->first, 3);
    EXPECT_EQ(pair->second, 4);
    auto *moved = EAllocator::New<std::pair<int, int>>(std::move(*pair));
    EXPECT_EQ(moved->first, 3);
    EXPECT_EQ(moved->second, 4);
}
// ---------------------------------------------------------------------------
// EAllocatorAdapter: the std-compatible allocator facade
// ---------------------------------------------------------------------------
class EAllocatorAdapterTest : public EHeapTest {};
TEST_F(EAllocatorAdapterTest, MaxSizeFor_char)
{
    using Adapter = EAllocatorAdapter<char>;
    EXPECT_EQ(Adapter {}.max_size(), static_cast<size_t>(-1) / sizeof(char));
    EXPECT_EQ(std::allocator_traits<Adapter>::max_size(Adapter {}), static_cast<size_t>(-1) / sizeof(char));
}
TEST_F(EAllocatorAdapterTest, MaxSizeFor_short)
{
    using Adapter = EAllocatorAdapter<short>;
    EXPECT_EQ(Adapter {}.max_size(), static_cast<size_t>(-1) / sizeof(short));
    EXPECT_EQ(std::allocator_traits<Adapter>::max_size(Adapter {}), static_cast<size_t>(-1) / sizeof(short));
}
TEST_F(EAllocatorAdapterTest, MaxSizeFor_int)
{
    using Adapter = EAllocatorAdapter<int>;
    EXPECT_EQ(Adapter {}.max_size(), static_cast<size_t>(-1) / sizeof(int));
    EXPECT_EQ(std::allocator_traits<Adapter>::max_size(Adapter {}), static_cast<size_t>(-1) / sizeof(int));
}
TEST_F(EAllocatorAdapterTest, MaxSizeFor_long)
{
    using Adapter = EAllocatorAdapter<long>;
    EXPECT_EQ(Adapter {}.max_size(), static_cast<size_t>(-1) / sizeof(long));
    EXPECT_EQ(std::allocator_traits<Adapter>::max_size(Adapter {}), static_cast<size_t>(-1) / sizeof(long));
}
TEST_F(EAllocatorAdapterTest, MaxSizeFor_double)
{
    using Adapter = EAllocatorAdapter<double>;
    EXPECT_EQ(Adapter {}.max_size(), static_cast<size_t>(-1) / sizeof(double));
    EXPECT_EQ(std::allocator_traits<Adapter>::max_size(Adapter {}), static_cast<size_t>(-1) / sizeof(double));
}
TEST_F(EAllocatorAdapterTest, MaxSizeFor_voidPtr)
{
    using Adapter = EAllocatorAdapter<void *>;
    EXPECT_EQ(Adapter {}.max_size(), static_cast<size_t>(-1) / sizeof(void *));
    EXPECT_EQ(std::allocator_traits<Adapter>::max_size(Adapter {}), static_cast<size_t>(-1) / sizeof(void *));
}
TEST_F(EAllocatorAdapterTest, MaxSizeFor_LargeProbe)
{
    using Adapter = EAllocatorAdapter<LargeProbe>;
    EXPECT_EQ(Adapter {}.max_size(), static_cast<size_t>(-1) / sizeof(LargeProbe));
    EXPECT_EQ(std::allocator_traits<Adapter>::max_size(Adapter {}), static_cast<size_t>(-1) / sizeof(LargeProbe));
}
TEST_F(EAllocatorAdapterTest, AllocateElements_0000)
{
    EAllocator allocator;
    EAllocatorAdapter<int> adapter {&allocator};
    constexpr size_t COUNT = 0U;
    int *block = adapter.allocate(COUNT);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    for (size_t index = 0; index < COUNT; ++index) {
        block[index] = static_cast<int>(index);
    }
    for (size_t index = 0; index < COUNT; ++index) {
        EXPECT_EQ(block[index], static_cast<int>(index));
    }
    adapter.deallocate(block, COUNT);
}
TEST_F(EAllocatorAdapterTest, AllocateElements_0001)
{
    EAllocator allocator;
    EAllocatorAdapter<int> adapter {&allocator};
    constexpr size_t COUNT = 1U;
    int *block = adapter.allocate(COUNT);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    for (size_t index = 0; index < COUNT; ++index) {
        block[index] = static_cast<int>(index);
    }
    for (size_t index = 0; index < COUNT; ++index) {
        EXPECT_EQ(block[index], static_cast<int>(index));
    }
    adapter.deallocate(block, COUNT);
}
TEST_F(EAllocatorAdapterTest, AllocateElements_0002)
{
    EAllocator allocator;
    EAllocatorAdapter<int> adapter {&allocator};
    constexpr size_t COUNT = 2U;
    int *block = adapter.allocate(COUNT);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    for (size_t index = 0; index < COUNT; ++index) {
        block[index] = static_cast<int>(index);
    }
    for (size_t index = 0; index < COUNT; ++index) {
        EXPECT_EQ(block[index], static_cast<int>(index));
    }
    adapter.deallocate(block, COUNT);
}
TEST_F(EAllocatorAdapterTest, AllocateElements_0003)
{
    EAllocator allocator;
    EAllocatorAdapter<int> adapter {&allocator};
    constexpr size_t COUNT = 3U;
    int *block = adapter.allocate(COUNT);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    for (size_t index = 0; index < COUNT; ++index) {
        block[index] = static_cast<int>(index);
    }
    for (size_t index = 0; index < COUNT; ++index) {
        EXPECT_EQ(block[index], static_cast<int>(index));
    }
    adapter.deallocate(block, COUNT);
}
TEST_F(EAllocatorAdapterTest, AllocateElements_0008)
{
    EAllocator allocator;
    EAllocatorAdapter<int> adapter {&allocator};
    constexpr size_t COUNT = 8U;
    int *block = adapter.allocate(COUNT);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    for (size_t index = 0; index < COUNT; ++index) {
        block[index] = static_cast<int>(index);
    }
    for (size_t index = 0; index < COUNT; ++index) {
        EXPECT_EQ(block[index], static_cast<int>(index));
    }
    adapter.deallocate(block, COUNT);
}
TEST_F(EAllocatorAdapterTest, AllocateElements_0017)
{
    EAllocator allocator;
    EAllocatorAdapter<int> adapter {&allocator};
    constexpr size_t COUNT = 17U;
    int *block = adapter.allocate(COUNT);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    for (size_t index = 0; index < COUNT; ++index) {
        block[index] = static_cast<int>(index);
    }
    for (size_t index = 0; index < COUNT; ++index) {
        EXPECT_EQ(block[index], static_cast<int>(index));
    }
    adapter.deallocate(block, COUNT);
}
TEST_F(EAllocatorAdapterTest, AllocateElements_0064)
{
    EAllocator allocator;
    EAllocatorAdapter<int> adapter {&allocator};
    constexpr size_t COUNT = 64U;
    int *block = adapter.allocate(COUNT);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    for (size_t index = 0; index < COUNT; ++index) {
        block[index] = static_cast<int>(index);
    }
    for (size_t index = 0; index < COUNT; ++index) {
        EXPECT_EQ(block[index], static_cast<int>(index));
    }
    adapter.deallocate(block, COUNT);
}
TEST_F(EAllocatorAdapterTest, AllocateElements_0128)
{
    EAllocator allocator;
    EAllocatorAdapter<int> adapter {&allocator};
    constexpr size_t COUNT = 128U;
    int *block = adapter.allocate(COUNT);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    for (size_t index = 0; index < COUNT; ++index) {
        block[index] = static_cast<int>(index);
    }
    for (size_t index = 0; index < COUNT; ++index) {
        EXPECT_EQ(block[index], static_cast<int>(index));
    }
    adapter.deallocate(block, COUNT);
}
TEST_F(EAllocatorAdapterTest, TraitsConstructDestroy_001)
{
    EAllocator allocator;
    using Adapter = EAllocatorAdapter<ValueProbe>;
    using Traits = std::allocator_traits<Adapter>;
    Adapter adapter {&allocator};
    constexpr size_t COUNT = 1U;
    ValueProbe *block = adapter.allocate(COUNT);
    ASSERT_NE(block, nullptr);
    for (size_t index = 0; index < COUNT; ++index) {
        Traits::construct(adapter, block + index, static_cast<int>(index), static_cast<int>(index * 2U));
    }
    for (size_t index = 0; index < COUNT; ++index) {
        EXPECT_EQ(block[index].first, static_cast<int>(index));
        EXPECT_EQ(block[index].second, static_cast<int>(index * 2U));
    }
    for (size_t index = 0; index < COUNT; ++index) {
        Traits::destroy(adapter, block + index);
    }
    adapter.deallocate(block, COUNT);
}
TEST_F(EAllocatorAdapterTest, TraitsConstructDestroy_002)
{
    EAllocator allocator;
    using Adapter = EAllocatorAdapter<ValueProbe>;
    using Traits = std::allocator_traits<Adapter>;
    Adapter adapter {&allocator};
    constexpr size_t COUNT = 2U;
    ValueProbe *block = adapter.allocate(COUNT);
    ASSERT_NE(block, nullptr);
    for (size_t index = 0; index < COUNT; ++index) {
        Traits::construct(adapter, block + index, static_cast<int>(index), static_cast<int>(index * 2U));
    }
    for (size_t index = 0; index < COUNT; ++index) {
        EXPECT_EQ(block[index].first, static_cast<int>(index));
        EXPECT_EQ(block[index].second, static_cast<int>(index * 2U));
    }
    for (size_t index = 0; index < COUNT; ++index) {
        Traits::destroy(adapter, block + index);
    }
    adapter.deallocate(block, COUNT);
}
TEST_F(EAllocatorAdapterTest, TraitsConstructDestroy_004)
{
    EAllocator allocator;
    using Adapter = EAllocatorAdapter<ValueProbe>;
    using Traits = std::allocator_traits<Adapter>;
    Adapter adapter {&allocator};
    constexpr size_t COUNT = 4U;
    ValueProbe *block = adapter.allocate(COUNT);
    ASSERT_NE(block, nullptr);
    for (size_t index = 0; index < COUNT; ++index) {
        Traits::construct(adapter, block + index, static_cast<int>(index), static_cast<int>(index * 2U));
    }
    for (size_t index = 0; index < COUNT; ++index) {
        EXPECT_EQ(block[index].first, static_cast<int>(index));
        EXPECT_EQ(block[index].second, static_cast<int>(index * 2U));
    }
    for (size_t index = 0; index < COUNT; ++index) {
        Traits::destroy(adapter, block + index);
    }
    adapter.deallocate(block, COUNT);
}
TEST_F(EAllocatorAdapterTest, TraitsConstructDestroy_008)
{
    EAllocator allocator;
    using Adapter = EAllocatorAdapter<ValueProbe>;
    using Traits = std::allocator_traits<Adapter>;
    Adapter adapter {&allocator};
    constexpr size_t COUNT = 8U;
    ValueProbe *block = adapter.allocate(COUNT);
    ASSERT_NE(block, nullptr);
    for (size_t index = 0; index < COUNT; ++index) {
        Traits::construct(adapter, block + index, static_cast<int>(index), static_cast<int>(index * 2U));
    }
    for (size_t index = 0; index < COUNT; ++index) {
        EXPECT_EQ(block[index].first, static_cast<int>(index));
        EXPECT_EQ(block[index].second, static_cast<int>(index * 2U));
    }
    for (size_t index = 0; index < COUNT; ++index) {
        Traits::destroy(adapter, block + index);
    }
    adapter.deallocate(block, COUNT);
}
TEST_F(EAllocatorAdapterTest, TraitsConstructDestroy_016)
{
    EAllocator allocator;
    using Adapter = EAllocatorAdapter<ValueProbe>;
    using Traits = std::allocator_traits<Adapter>;
    Adapter adapter {&allocator};
    constexpr size_t COUNT = 16U;
    ValueProbe *block = adapter.allocate(COUNT);
    ASSERT_NE(block, nullptr);
    for (size_t index = 0; index < COUNT; ++index) {
        Traits::construct(adapter, block + index, static_cast<int>(index), static_cast<int>(index * 2U));
    }
    for (size_t index = 0; index < COUNT; ++index) {
        EXPECT_EQ(block[index].first, static_cast<int>(index));
        EXPECT_EQ(block[index].second, static_cast<int>(index * 2U));
    }
    for (size_t index = 0; index < COUNT; ++index) {
        Traits::destroy(adapter, block + index);
    }
    adapter.deallocate(block, COUNT);
}
TEST_F(EAllocatorAdapterTest, TraitsConstructDestroy_032)
{
    EAllocator allocator;
    using Adapter = EAllocatorAdapter<ValueProbe>;
    using Traits = std::allocator_traits<Adapter>;
    Adapter adapter {&allocator};
    constexpr size_t COUNT = 32U;
    ValueProbe *block = adapter.allocate(COUNT);
    ASSERT_NE(block, nullptr);
    for (size_t index = 0; index < COUNT; ++index) {
        Traits::construct(adapter, block + index, static_cast<int>(index), static_cast<int>(index * 2U));
    }
    for (size_t index = 0; index < COUNT; ++index) {
        EXPECT_EQ(block[index].first, static_cast<int>(index));
        EXPECT_EQ(block[index].second, static_cast<int>(index * 2U));
    }
    for (size_t index = 0; index < COUNT; ++index) {
        Traits::destroy(adapter, block + index);
    }
    adapter.deallocate(block, COUNT);
}
TEST_F(EAllocatorAdapterTest, AddressOfLvalue)
{
    EAllocatorAdapter<int> adapter;
    int value = 5;
    EXPECT_EQ(adapter.address(value), &value);
    *adapter.address(value) = 6;
    EXPECT_EQ(value, 6);
}
TEST_F(EAllocatorAdapterTest, AllocateWithHint)
{
    EAllocator allocator;
    EAllocatorAdapter<int> adapter {&allocator};
    void *hint = EHeap::Alloc(64U);
    int *block = adapter.allocate(4U, hint);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(IsAligned(block));
    block[0] = 1;
    block[1] = 2;
    block[2] = 3;
    block[3] = 4;
    EXPECT_EQ(block[0] + block[3], 5);
    adapter.deallocate(block, 4U);
}
TEST_F(EAllocatorAdapterTest, EqualityIsAlwaysTrue)
{
    EAllocator first;
    EAllocator second;
    EAllocatorAdapter<int> lhs {&first};
    EAllocatorAdapter<int> rhs {&second};
    EXPECT_TRUE(lhs == rhs);
    EXPECT_FALSE(lhs != rhs);
    EXPECT_TRUE(EAllocatorAdapter<int> {} == rhs);
}
TEST_F(EAllocatorAdapterTest, CrossTypeConversion)
{
    EAllocator allocator;
    EAllocatorAdapter<void> voidAdapter {&allocator};
    EAllocatorAdapter<int> intAdapter {voidAdapter};
    EAllocatorAdapter<double> doubleAdapter {intAdapter};
    EXPECT_TRUE(intAdapter == EAllocatorAdapter<int> {&allocator});
    EXPECT_TRUE(doubleAdapter == EAllocatorAdapter<double> {&allocator});
    EXPECT_EQ(intAdapter.max_size(), static_cast<size_t>(-1) / sizeof(int));
}
TEST_F(EAllocatorAdapterTest, CopyAndMoveSemantics)
{
    EAllocator allocator;
    EAllocatorAdapter<int> original {&allocator};
    EAllocatorAdapter<int> copied {original};
    EAllocatorAdapter<int> assigned;
    assigned = original;
    EAllocatorAdapter<int> moved {std::move(copied)};
    EAllocatorAdapter<int> moveAssigned;
    moveAssigned = std::move(assigned);
    EXPECT_TRUE(moved == original);
    EXPECT_TRUE(moveAssigned == original);
    EXPECT_TRUE(moved == EAllocatorAdapter<int> {});
}
TEST_F(EAllocatorAdapterTest, RebindAllocatesTheRequestedType)
{
    EAllocator allocator;
    EAllocatorAdapter<int>::rebind<double>::other adapter {&allocator};
    double *value = adapter.allocate(1U);
    ASSERT_NE(value, nullptr);
    *value = 2.5;
    EXPECT_DOUBLE_EQ(*value, 2.5);
    adapter.deallocate(value, 1U);
}
TEST_F(EAllocatorAdapterTest, UninitializedFillThroughTraits)
{
    EAllocator allocator;
    using Adapter = EAllocatorAdapter<int>;
    using Traits = std::allocator_traits<Adapter>;
    Adapter adapter {&allocator};
    constexpr size_t COUNT = 32U;
    int *block = Traits::allocate(adapter, COUNT);
    ASSERT_NE(block, nullptr);
    std::uninitialized_fill_n(block, COUNT, 9);
    EXPECT_EQ(std::count(block, block + COUNT, 9), static_cast<ptrdiff_t>(COUNT));
    for (size_t index = 0; index < COUNT; ++index) {
        Traits::destroy(adapter, block + index);
    }
    Traits::deallocate(adapter, block, COUNT);
}
TEST_F(EAllocatorAdapterTest, VoidAdapterSupportsDefaultConstruction)
{
    EAllocatorAdapter<void> first;
    EAllocatorAdapter<void> second {first};
    EAllocatorAdapter<void> third {std::move(second)};
    EAllocatorAdapter<void> fourth;
    fourth = third;
    EXPECT_TRUE(fourth == first);
}
TEST_F(EAllocatorAdapterTest, SelectOnContainerCopyConstructionKeepsEquality)
{
    EAllocator allocator;
    EAllocatorAdapter<int> adapter {&allocator};
    EAllocatorAdapter<int> selected =
        std::allocator_traits<EAllocatorAdapter<int>>::select_on_container_copy_construction(adapter);
    EXPECT_TRUE(selected == adapter);
    EXPECT_EQ(selected.max_size(), adapter.max_size());
}
// ---------------------------------------------------------------------------
// EHeap::EPtr: raw pointer in debug builds, compressed 32-bit handle otherwise
// ---------------------------------------------------------------------------
class EHeapPointerTest : public EHeapTest {};
TEST_F(EHeapPointerTest, DefaultConstructionYieldsNull)
{
    EPtr<int> pointer {};
    EXPECT_EQ(static_cast<int *>(pointer), nullptr);
    EXPECT_TRUE(pointer == nullptr);
}
TEST_F(EHeapPointerTest, NullConstructionYieldsNull)
{
    EPtr<int> pointer {nullptr};
    EXPECT_EQ(static_cast<int *>(pointer), nullptr);
    EXPECT_TRUE(pointer == nullptr);
}
TEST_F(EHeapPointerTest, HeapIntRoundTrip_0)
{
    auto *value = EAllocator::New<int>(0);
    ASSERT_NE(value, nullptr);
    EPtr<int> pointer {value};
    EXPECT_EQ(static_cast<int *>(pointer), value);
    EXPECT_EQ(*pointer, 0);
    *pointer = *pointer + 1;
    EXPECT_EQ(*value, 0 + 1);
}
TEST_F(EHeapPointerTest, HeapIntRoundTrip_1)
{
    auto *value = EAllocator::New<int>(1);
    ASSERT_NE(value, nullptr);
    EPtr<int> pointer {value};
    EXPECT_EQ(static_cast<int *>(pointer), value);
    EXPECT_EQ(*pointer, 1);
    *pointer = *pointer + 1;
    EXPECT_EQ(*value, 1 + 1);
}
TEST_F(EHeapPointerTest, HeapIntRoundTrip_neg1)
{
    auto *value = EAllocator::New<int>(-1);
    ASSERT_NE(value, nullptr);
    EPtr<int> pointer {value};
    EXPECT_EQ(static_cast<int *>(pointer), value);
    EXPECT_EQ(*pointer, -1);
    *pointer = *pointer + 1;
    EXPECT_EQ(*value, -1 + 1);
}
TEST_F(EHeapPointerTest, HeapIntRoundTrip_42)
{
    auto *value = EAllocator::New<int>(42);
    ASSERT_NE(value, nullptr);
    EPtr<int> pointer {value};
    EXPECT_EQ(static_cast<int *>(pointer), value);
    EXPECT_EQ(*pointer, 42);
    *pointer = *pointer + 1;
    EXPECT_EQ(*value, 42 + 1);
}
TEST_F(EHeapPointerTest, HeapIntRoundTrip_1024)
{
    auto *value = EAllocator::New<int>(1024);
    ASSERT_NE(value, nullptr);
    EPtr<int> pointer {value};
    EXPECT_EQ(static_cast<int *>(pointer), value);
    EXPECT_EQ(*pointer, 1024);
    *pointer = *pointer + 1;
    EXPECT_EQ(*value, 1024 + 1);
}
TEST_F(EHeapPointerTest, HeapIntRoundTrip_neg1024)
{
    auto *value = EAllocator::New<int>(-1024);
    ASSERT_NE(value, nullptr);
    EPtr<int> pointer {value};
    EXPECT_EQ(static_cast<int *>(pointer), value);
    EXPECT_EQ(*pointer, -1024);
    *pointer = *pointer + 1;
    EXPECT_EQ(*value, -1024 + 1);
}
TEST_F(EHeapPointerTest, HeapIntRoundTrip_1000000)
{
    auto *value = EAllocator::New<int>(1000000);
    ASSERT_NE(value, nullptr);
    EPtr<int> pointer {value};
    EXPECT_EQ(static_cast<int *>(pointer), value);
    EXPECT_EQ(*pointer, 1000000);
    *pointer = *pointer + 1;
    EXPECT_EQ(*value, 1000000 + 1);
}
TEST_F(EHeapPointerTest, ArrowOperator_1_2)
{
    auto *probe = EAllocator::New<ValueProbe>(1, 2);
    ASSERT_NE(probe, nullptr);
    EPtr<ValueProbe> pointer {probe};
    EXPECT_EQ(pointer->first, 1);
    EXPECT_EQ(pointer->second, 2);
    pointer->second = pointer->first + pointer->second;
    EXPECT_EQ(pointer->second, 1 + 2);
}
TEST_F(EHeapPointerTest, ArrowOperator_10_20)
{
    auto *probe = EAllocator::New<ValueProbe>(10, 20);
    ASSERT_NE(probe, nullptr);
    EPtr<ValueProbe> pointer {probe};
    EXPECT_EQ(pointer->first, 10);
    EXPECT_EQ(pointer->second, 20);
    pointer->second = pointer->first + pointer->second;
    EXPECT_EQ(pointer->second, 10 + 20);
}
TEST_F(EHeapPointerTest, ArrowOperator_neg5_5)
{
    auto *probe = EAllocator::New<ValueProbe>(-5, 5);
    ASSERT_NE(probe, nullptr);
    EPtr<ValueProbe> pointer {probe};
    EXPECT_EQ(pointer->first, -5);
    EXPECT_EQ(pointer->second, 5);
    pointer->second = pointer->first + pointer->second;
    EXPECT_EQ(pointer->second, -5 + 5);
}
TEST_F(EHeapPointerTest, ArrowOperator_100_200)
{
    auto *probe = EAllocator::New<ValueProbe>(100, 200);
    ASSERT_NE(probe, nullptr);
    EPtr<ValueProbe> pointer {probe};
    EXPECT_EQ(pointer->first, 100);
    EXPECT_EQ(pointer->second, 200);
    pointer->second = pointer->first + pointer->second;
    EXPECT_EQ(pointer->second, 100 + 200);
}
TEST_F(EHeapPointerTest, CopyConstructionSharesTheTarget)
{
    auto *value = EAllocator::New<int>(7);
    ASSERT_NE(value, nullptr);
    EPtr<int> first {value};
    EPtr<int> second {first};
    EXPECT_EQ(static_cast<int *>(first), static_cast<int *>(second));
    *second = 8;
    EXPECT_EQ(*first, 8);
}
TEST_F(EHeapPointerTest, CopyAssignmentSharesTheTarget)
{
    auto *lhs = EAllocator::New<int>(1);
    auto *rhs = EAllocator::New<int>(2);
    ASSERT_NE(lhs, nullptr);
    ASSERT_NE(rhs, nullptr);
    EPtr<int> first {lhs};
    EPtr<int> second {rhs};
    first = second;
    EXPECT_EQ(static_cast<int *>(first), rhs);
    *first = 3;
    EXPECT_EQ(*rhs, 3);
    EXPECT_EQ(*lhs, 1);
}
TEST_F(EHeapPointerTest, MoveConstructionKeepsTheTarget)
{
    auto *value = EAllocator::New<int>(5);
    ASSERT_NE(value, nullptr);
    EPtr<int> first {value};
    EPtr<int> second {std::move(first)};
    EXPECT_EQ(static_cast<int *>(second), value);
    EXPECT_EQ(*second, 5);
}
TEST_F(EHeapPointerTest, MoveAssignmentKeepsTheTarget)
{
    auto *value = EAllocator::New<int>(6);
    ASSERT_NE(value, nullptr);
    EPtr<int> first {value};
    EPtr<int> second {};
    second = std::move(first);
    EXPECT_EQ(static_cast<int *>(second), value);
    EXPECT_EQ(*second, 6);
}
TEST_F(EHeapPointerTest, PointerVector_001)
{
    constexpr int COUNT = 1;
    std::vector<EPtr<int>> pointers;
    pointers.reserve(COUNT);
    for (int index = 0; index < COUNT; ++index) {
        auto *value = EAllocator::New<int>(index * index);
        ASSERT_NE(value, nullptr);
        pointers.emplace_back(value);
    }
    for (int index = 0; index < COUNT; ++index) {
        EXPECT_EQ(*pointers[static_cast<size_t>(index)], index * index);
    }
    for (int index = 0; index < COUNT; ++index) {
        *pointers[static_cast<size_t>(index)] = index;
    }
    for (int index = 0; index < COUNT; ++index) {
        EXPECT_EQ(*pointers[static_cast<size_t>(index)], index);
    }
}
TEST_F(EHeapPointerTest, PointerVector_002)
{
    constexpr int COUNT = 2;
    std::vector<EPtr<int>> pointers;
    pointers.reserve(COUNT);
    for (int index = 0; index < COUNT; ++index) {
        auto *value = EAllocator::New<int>(index * index);
        ASSERT_NE(value, nullptr);
        pointers.emplace_back(value);
    }
    for (int index = 0; index < COUNT; ++index) {
        EXPECT_EQ(*pointers[static_cast<size_t>(index)], index * index);
    }
    for (int index = 0; index < COUNT; ++index) {
        *pointers[static_cast<size_t>(index)] = index;
    }
    for (int index = 0; index < COUNT; ++index) {
        EXPECT_EQ(*pointers[static_cast<size_t>(index)], index);
    }
}
TEST_F(EHeapPointerTest, PointerVector_004)
{
    constexpr int COUNT = 4;
    std::vector<EPtr<int>> pointers;
    pointers.reserve(COUNT);
    for (int index = 0; index < COUNT; ++index) {
        auto *value = EAllocator::New<int>(index * index);
        ASSERT_NE(value, nullptr);
        pointers.emplace_back(value);
    }
    for (int index = 0; index < COUNT; ++index) {
        EXPECT_EQ(*pointers[static_cast<size_t>(index)], index * index);
    }
    for (int index = 0; index < COUNT; ++index) {
        *pointers[static_cast<size_t>(index)] = index;
    }
    for (int index = 0; index < COUNT; ++index) {
        EXPECT_EQ(*pointers[static_cast<size_t>(index)], index);
    }
}
TEST_F(EHeapPointerTest, PointerVector_008)
{
    constexpr int COUNT = 8;
    std::vector<EPtr<int>> pointers;
    pointers.reserve(COUNT);
    for (int index = 0; index < COUNT; ++index) {
        auto *value = EAllocator::New<int>(index * index);
        ASSERT_NE(value, nullptr);
        pointers.emplace_back(value);
    }
    for (int index = 0; index < COUNT; ++index) {
        EXPECT_EQ(*pointers[static_cast<size_t>(index)], index * index);
    }
    for (int index = 0; index < COUNT; ++index) {
        *pointers[static_cast<size_t>(index)] = index;
    }
    for (int index = 0; index < COUNT; ++index) {
        EXPECT_EQ(*pointers[static_cast<size_t>(index)], index);
    }
}
TEST_F(EHeapPointerTest, PointerVector_016)
{
    constexpr int COUNT = 16;
    std::vector<EPtr<int>> pointers;
    pointers.reserve(COUNT);
    for (int index = 0; index < COUNT; ++index) {
        auto *value = EAllocator::New<int>(index * index);
        ASSERT_NE(value, nullptr);
        pointers.emplace_back(value);
    }
    for (int index = 0; index < COUNT; ++index) {
        EXPECT_EQ(*pointers[static_cast<size_t>(index)], index * index);
    }
    for (int index = 0; index < COUNT; ++index) {
        *pointers[static_cast<size_t>(index)] = index;
    }
    for (int index = 0; index < COUNT; ++index) {
        EXPECT_EQ(*pointers[static_cast<size_t>(index)], index);
    }
}
TEST_F(EHeapPointerTest, PointerVector_032)
{
    constexpr int COUNT = 32;
    std::vector<EPtr<int>> pointers;
    pointers.reserve(COUNT);
    for (int index = 0; index < COUNT; ++index) {
        auto *value = EAllocator::New<int>(index * index);
        ASSERT_NE(value, nullptr);
        pointers.emplace_back(value);
    }
    for (int index = 0; index < COUNT; ++index) {
        EXPECT_EQ(*pointers[static_cast<size_t>(index)], index * index);
    }
    for (int index = 0; index < COUNT; ++index) {
        *pointers[static_cast<size_t>(index)] = index;
    }
    for (int index = 0; index < COUNT; ++index) {
        EXPECT_EQ(*pointers[static_cast<size_t>(index)], index);
    }
}
TEST_F(EHeapPointerTest, ArenaVectorOfPointers)
{
    ArenaVector<EPtr<int>> pointers;
    for (int index = 0; index < 16; ++index) {
        auto *value = EAllocator::New<int>(index + 1);
        ASSERT_NE(value, nullptr);
        pointers.push_back(EPtr<int> {value});
    }
    int sum = 0;
    for (const auto &pointer : pointers) {
        sum += *pointer;
    }
    EXPECT_EQ(sum, 136);
}
TEST_F(EHeapPointerTest, LargeObjectPointer)
{
    auto *probe = EAllocator::New<LargeProbe>(0x33U);
    ASSERT_NE(probe, nullptr);
    EPtr<LargeProbe> pointer {probe};
    EXPECT_EQ(pointer->payload.front(), 0x33U);
    EXPECT_EQ(pointer->payload.size(), 4096U);
    pointer->payload[100] = 0x44U;
    EXPECT_EQ(probe->payload[100], 0x44U);
}
TEST_F(EHeapPointerTest, EmptyPointerDoesNotCompareEqualToHeapPointer)
{
    auto *value = EAllocator::New<int>(0);
    ASSERT_NE(value, nullptr);
    EPtr<int> pointer {value};
    EPtr<int> empty {};
    EXPECT_FALSE(pointer == nullptr);
    EXPECT_TRUE(empty == nullptr);
    EXPECT_NE(static_cast<int *>(pointer), static_cast<int *>(empty));
}
TEST_F(EHeapPointerTest, PointerSizeMatchesBuildFlavour)
{
#if defined(NDEBUG) || defined(PANDA_FAST_VERIFY)
    EXPECT_EQ(sizeof(EPtr<int>), sizeof(uint32_t));
    EXPECT_EQ(sizeof(EPtr<LargeProbe>), sizeof(uint32_t));
#else
    EXPECT_EQ(sizeof(EPtr<int>), sizeof(int *));
    EXPECT_EQ(sizeof(EPtr<LargeProbe>), sizeof(LargeProbe *));
#endif
}
TEST_F(EHeapPointerTest, PointerUniquePerAllocation)
{
    auto *first = EAllocator::New<int>(1);
    auto *second = EAllocator::New<int>(2);
    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);
    EPtr<int> firstPointer {first};
    EPtr<int> secondPointer {second};
    EXPECT_NE(static_cast<int *>(firstPointer), static_cast<int *>(secondPointer));
    EXPECT_NE(Addr(static_cast<int *>(firstPointer)), Addr(static_cast<int *>(secondPointer)));
    EXPECT_EQ(*firstPointer, 1);
    EXPECT_EQ(*secondPointer, 2);
}
// ---------------------------------------------------------------------------
// Sequence containers: ArenaVector / ArenaDeque / ArenaList
// ---------------------------------------------------------------------------
class EHeapSequenceContainerTest : public EHeapTest {};
TEST_F(EHeapSequenceContainerTest, ArenaVector_PushAndIterate)
{
    ArenaVector<int> container;
    for (int value = 0; value < 32; ++value) {
        container.push_back(value * 2);
    }
    EXPECT_EQ(static_cast<int>(container.size()), 32);
    EXPECT_EQ(container.front(), 0);
    EXPECT_EQ(container.back(), 62);
    int sum = 0;
    int seen = 0;
    for (const int value : container) {
        EXPECT_EQ(value % 2, 0);
        sum += value;
        ++seen;
    }
    EXPECT_EQ(seen, 32);
    EXPECT_EQ(sum, 32 * 31);
}
TEST_F(EHeapSequenceContainerTest, ArenaVector_ResizeAndReuse)
{
    ArenaVector<int> container;
    for (int value = 0; value < 64; ++value) {
        container.push_back(value * 2);
    }
    EXPECT_EQ(static_cast<int>(container.size()), 64);
    container.resize(50U);
    EXPECT_EQ(static_cast<int>(container.size()), 50);
    EXPECT_EQ(container.front(), 0);
    container.push_back(1000);
    EXPECT_EQ(container.back(), 1000);
    container.clear();
    EXPECT_TRUE(container.empty());
    EXPECT_EQ(static_cast<int>(container.size()), 0);
}
TEST_F(EHeapSequenceContainerTest, ArenaVector_CopyRoundTrip)
{
    ArenaVector<int> container;
    for (int value = 0; value < 16; ++value) {
        container.push_back(value * 2);
    }
    ArenaVector<int> copyOfContainer(container.begin(), container.end());
    EXPECT_EQ(copyOfContainer.size(), container.size());
    EXPECT_TRUE(std::equal(container.begin(), container.end(), copyOfContainer.begin()));
    copyOfContainer.push_back(1000);
    EXPECT_EQ(copyOfContainer.size(), container.size() + 1U);
    EXPECT_EQ(static_cast<int>(container.size()), 16);
}
TEST_F(EHeapSequenceContainerTest, ArenaVector_MoveRoundTrip)
{
    ArenaVector<int> container;
    for (int value = 0; value < 8; ++value) {
        container.push_back(value * 2);
    }
    ArenaVector<int> moved(container.begin(), container.end());
    moved.push_back(77);
    EXPECT_EQ(moved.size(), 9U);
    EXPECT_EQ(moved.back(), 77);
    EXPECT_EQ(moved.front(), 0);
    EXPECT_TRUE(IsAligned(moved.data()));
}
TEST_F(EHeapSequenceContainerTest, ArenaVector_InsertFrontAndErase)
{
    ArenaVector<int> container;
    for (int value = 1; value <= 8; ++value) {
        container.push_back(value * 2);
    }
    container.insert(container.begin(), -1);
    EXPECT_EQ(static_cast<int>(container.size()), 9);
    EXPECT_EQ(container.front(), -1);
    container.erase(container.begin());
    EXPECT_EQ(static_cast<int>(container.size()), 8);
    EXPECT_EQ(container.front(), 2);
}
TEST_F(EHeapSequenceContainerTest, ArenaVector_ClearThenReuse)
{
    ArenaVector<int> container;
    for (int value = 0; value < 24; ++value) {
        container.push_back(value * 2);
    }
    const size_t allocatedDuringFill = EHeap::AllocatedSize();
    container.clear();
    EXPECT_TRUE(container.empty());
    for (int value = 0; value < 4; ++value) {
        container.push_back(value * 2);
    }
    EXPECT_EQ(static_cast<int>(container.size()), 4);
    EXPECT_EQ(container.front(), 0);
    EXPECT_GE(EHeap::AllocatedSize(), allocatedDuringFill);
}
TEST_F(EHeapSequenceContainerTest, ArenaDeque_PushAndIterate)
{
    ArenaDeque<int> container;
    for (int value = 0; value < 32; ++value) {
        container.push_back(value * 2);
    }
    EXPECT_EQ(static_cast<int>(container.size()), 32);
    EXPECT_EQ(container.front(), 0);
    EXPECT_EQ(container.back(), 62);
    int sum = 0;
    int seen = 0;
    for (const int value : container) {
        EXPECT_EQ(value % 2, 0);
        sum += value;
        ++seen;
    }
    EXPECT_EQ(seen, 32);
    EXPECT_EQ(sum, 32 * 31);
}
TEST_F(EHeapSequenceContainerTest, ArenaDeque_ResizeAndReuse)
{
    ArenaDeque<int> container;
    for (int value = 0; value < 64; ++value) {
        container.push_back(value * 2);
    }
    EXPECT_EQ(static_cast<int>(container.size()), 64);
    container.resize(50U);
    EXPECT_EQ(static_cast<int>(container.size()), 50);
    EXPECT_EQ(container.front(), 0);
    container.push_back(1000);
    EXPECT_EQ(container.back(), 1000);
    container.clear();
    EXPECT_TRUE(container.empty());
    EXPECT_EQ(static_cast<int>(container.size()), 0);
}
TEST_F(EHeapSequenceContainerTest, ArenaDeque_CopyRoundTrip)
{
    ArenaDeque<int> container;
    for (int value = 0; value < 16; ++value) {
        container.push_back(value * 2);
    }
    ArenaVector<int> copyOfContainer(container.begin(), container.end());
    EXPECT_EQ(copyOfContainer.size(), container.size());
    EXPECT_TRUE(std::equal(container.begin(), container.end(), copyOfContainer.begin()));
    copyOfContainer.push_back(1000);
    EXPECT_EQ(copyOfContainer.size(), container.size() + 1U);
    EXPECT_EQ(static_cast<int>(container.size()), 16);
}
TEST_F(EHeapSequenceContainerTest, ArenaDeque_MoveRoundTrip)
{
    ArenaDeque<int> container;
    for (int value = 0; value < 8; ++value) {
        container.push_back(value * 2);
    }
    ArenaVector<int> moved(container.begin(), container.end());
    moved.push_back(77);
    EXPECT_EQ(moved.size(), 9U);
    EXPECT_EQ(moved.back(), 77);
    EXPECT_EQ(moved.front(), 0);
    EXPECT_TRUE(IsAligned(moved.data()));
}
TEST_F(EHeapSequenceContainerTest, ArenaDeque_InsertFrontAndErase)
{
    ArenaDeque<int> container;
    for (int value = 1; value <= 8; ++value) {
        container.push_back(value * 2);
    }
    container.insert(container.begin(), -1);
    EXPECT_EQ(static_cast<int>(container.size()), 9);
    EXPECT_EQ(container.front(), -1);
    container.erase(container.begin());
    EXPECT_EQ(static_cast<int>(container.size()), 8);
    EXPECT_EQ(container.front(), 2);
}
TEST_F(EHeapSequenceContainerTest, ArenaDeque_ClearThenReuse)
{
    ArenaDeque<int> container;
    for (int value = 0; value < 24; ++value) {
        container.push_back(value * 2);
    }
    const size_t allocatedDuringFill = EHeap::AllocatedSize();
    container.clear();
    EXPECT_TRUE(container.empty());
    for (int value = 0; value < 4; ++value) {
        container.push_back(value * 2);
    }
    EXPECT_EQ(static_cast<int>(container.size()), 4);
    EXPECT_EQ(container.front(), 0);
    EXPECT_GE(EHeap::AllocatedSize(), allocatedDuringFill);
}
TEST_F(EHeapSequenceContainerTest, ArenaList_PushAndIterate)
{
    ArenaList<int> container;
    for (int value = 0; value < 32; ++value) {
        container.push_back(value * 2);
    }
    EXPECT_EQ(static_cast<int>(container.size()), 32);
    EXPECT_EQ(container.front(), 0);
    EXPECT_EQ(container.back(), 62);
    int sum = 0;
    int seen = 0;
    for (const int value : container) {
        EXPECT_EQ(value % 2, 0);
        sum += value;
        ++seen;
    }
    EXPECT_EQ(seen, 32);
    EXPECT_EQ(sum, 32 * 31);
}
TEST_F(EHeapSequenceContainerTest, ArenaList_ResizeAndReuse)
{
    ArenaList<int> container;
    for (int value = 0; value < 64; ++value) {
        container.push_back(value * 2);
    }
    EXPECT_EQ(static_cast<int>(container.size()), 64);
    container.resize(50U);
    EXPECT_EQ(static_cast<int>(container.size()), 50);
    EXPECT_EQ(container.front(), 0);
    container.push_back(1000);
    EXPECT_EQ(container.back(), 1000);
    container.clear();
    EXPECT_TRUE(container.empty());
    EXPECT_EQ(static_cast<int>(container.size()), 0);
}
TEST_F(EHeapSequenceContainerTest, ArenaList_CopyRoundTrip)
{
    ArenaList<int> container;
    for (int value = 0; value < 16; ++value) {
        container.push_back(value * 2);
    }
    ArenaVector<int> copyOfContainer(container.begin(), container.end());
    EXPECT_EQ(copyOfContainer.size(), container.size());
    EXPECT_TRUE(std::equal(container.begin(), container.end(), copyOfContainer.begin()));
    copyOfContainer.push_back(1000);
    EXPECT_EQ(copyOfContainer.size(), container.size() + 1U);
    EXPECT_EQ(static_cast<int>(container.size()), 16);
}
TEST_F(EHeapSequenceContainerTest, ArenaList_MoveRoundTrip)
{
    ArenaList<int> container;
    for (int value = 0; value < 8; ++value) {
        container.push_back(value * 2);
    }
    ArenaVector<int> moved(container.begin(), container.end());
    moved.push_back(77);
    EXPECT_EQ(moved.size(), 9U);
    EXPECT_EQ(moved.back(), 77);
    EXPECT_EQ(moved.front(), 0);
    EXPECT_TRUE(IsAligned(moved.data()));
}
TEST_F(EHeapSequenceContainerTest, ArenaList_InsertFrontAndErase)
{
    ArenaList<int> container;
    for (int value = 1; value <= 8; ++value) {
        container.push_back(value * 2);
    }
    container.insert(container.begin(), -1);
    EXPECT_EQ(static_cast<int>(container.size()), 9);
    EXPECT_EQ(container.front(), -1);
    container.erase(container.begin());
    EXPECT_EQ(static_cast<int>(container.size()), 8);
    EXPECT_EQ(container.front(), 2);
}
TEST_F(EHeapSequenceContainerTest, ArenaList_ClearThenReuse)
{
    ArenaList<int> container;
    for (int value = 0; value < 24; ++value) {
        container.push_back(value * 2);
    }
    const size_t allocatedDuringFill = EHeap::AllocatedSize();
    container.clear();
    EXPECT_TRUE(container.empty());
    for (int value = 0; value < 4; ++value) {
        container.push_back(value * 2);
    }
    EXPECT_EQ(static_cast<int>(container.size()), 4);
    EXPECT_EQ(container.front(), 0);
    EXPECT_GE(EHeap::AllocatedSize(), allocatedDuringFill);
}
// ---------------------------------------------------------------------------
// ArenaForwardList
// ---------------------------------------------------------------------------
class EHeapForwardListTest : public EHeapTest {};
TEST_F(EHeapForwardListTest, PushFrontAndIterate)
{
    ArenaForwardList<int> container;
    for (int value = 0; value < 16; ++value) {
        container.push_front(value);
    }
    EXPECT_EQ(container.front(), 15);
    EXPECT_EQ(static_cast<size_t>(std::distance(container.begin(), container.end())), 16U);
    ArenaVector<int> collected(container.begin(), container.end());
    EXPECT_EQ(collected.size(), 16U);
    EXPECT_EQ(collected.front(), 15);
    EXPECT_EQ(collected.back(), 0);
}
TEST_F(EHeapForwardListTest, InsertAfterAndEraseAfter)
{
    ArenaForwardList<int> container;
    container.push_front(1);
    container.insert_after(container.before_begin(), 0);
    EXPECT_EQ(container.front(), 0);
    EXPECT_EQ(static_cast<size_t>(std::distance(container.begin(), container.end())), 2U);
    container.erase_after(container.before_begin());
    EXPECT_EQ(container.front(), 1);
    EXPECT_EQ(static_cast<size_t>(std::distance(container.begin(), container.end())), 1U);
}
TEST_F(EHeapForwardListTest, CopyRoundTrip)
{
    ArenaForwardList<int> container;
    for (int value = 0; value < 8; ++value) {
        container.push_front(value);
    }
    ArenaForwardList<int> copy {container};
    EXPECT_EQ(static_cast<size_t>(std::distance(copy.begin(), copy.end())), 8U);
    EXPECT_TRUE(std::equal(container.begin(), container.end(), copy.begin()));
    copy.push_front(99);
    EXPECT_EQ(copy.front(), 99);
    EXPECT_EQ(container.front(), 7);
}
TEST_F(EHeapForwardListTest, ClearThenReuse)
{
    ArenaForwardList<int> container;
    for (int value = 0; value < 12; ++value) {
        container.push_front(value);
    }
    container.clear();
    EXPECT_TRUE(container.empty());
    container.push_front(5);
    EXPECT_EQ(container.front(), 5);
    EXPECT_EQ(static_cast<size_t>(std::distance(container.begin(), container.end())), 1U);
}
// ---------------------------------------------------------------------------
// ArenaStack / ArenaQueue
// ---------------------------------------------------------------------------
class EHeapAdapterContainerTest : public EHeapTest {};
TEST_F(EHeapAdapterContainerTest, StackPushPop)
{
    ArenaStack<int> container;
    for (int value = 0; value < 16; ++value) {
        container.push(value);
    }
    EXPECT_EQ(container.size(), 16U);
    EXPECT_FALSE(container.empty());
    EXPECT_EQ(container.top(), 15);
    container.pop();
    EXPECT_EQ(container.top(), 14);
    while (!container.empty()) {
        container.pop();
    }
    EXPECT_EQ(container.size(), 0U);
}
TEST_F(EHeapAdapterContainerTest, StackCopyAndReuse)
{
    ArenaStack<int> container;
    for (int value = 0; value < 8; ++value) {
        container.push(value);
    }
    ArenaStack<int> copy {container};
    EXPECT_EQ(copy.size(), container.size());
    EXPECT_EQ(copy.top(), container.top());
    copy.pop();
    EXPECT_EQ(copy.top(), 6);
    EXPECT_EQ(container.top(), 7);
}
TEST_F(EHeapAdapterContainerTest, StackDrainSum)
{
    ArenaStack<int> container;
    int expected = 0;
    for (int value = 1; value <= 20; ++value) {
        container.push(value);
        expected += value;
    }
    int sum = 0;
    while (!container.empty()) {
        sum += container.top();
        container.pop();
    }
    EXPECT_EQ(sum, expected);
    EXPECT_EQ(sum, 210);
}
TEST_F(EHeapAdapterContainerTest, QueuePushPop)
{
    ArenaQueue<int> container;
    for (int value = 0; value < 16; ++value) {
        container.push(value);
    }
    EXPECT_EQ(container.size(), 16U);
    EXPECT_EQ(container.front(), 0);
    EXPECT_EQ(container.back(), 15);
    container.pop();
    EXPECT_EQ(container.front(), 1);
    EXPECT_EQ(container.back(), 15);
}
TEST_F(EHeapAdapterContainerTest, QueueFifoOrder)
{
    ArenaQueue<int> container;
    for (int value = 0; value < 32; ++value) {
        container.push(value);
    }
    int expected = 0;
    while (!container.empty()) {
        EXPECT_EQ(container.front(), expected);
        container.pop();
        ++expected;
    }
    EXPECT_EQ(expected, 32);
}
TEST_F(EHeapAdapterContainerTest, QueueCopyAndMove)
{
    ArenaQueue<int> container;
    for (int value = 0; value < 8; ++value) {
        container.push(value);
    }
    ArenaQueue<int> copy {container};
    EXPECT_EQ(copy.size(), container.size());
    EXPECT_EQ(copy.front(), 0);
    copy.pop();
    EXPECT_EQ(container.front(), 0);
    EXPECT_EQ(copy.front(), 1);
}
// ---------------------------------------------------------------------------
// ArenaSet / ArenaMap / ArenaMultiMap / unordered variants
// ---------------------------------------------------------------------------
class EHeapAssociativeContainerTest : public EHeapTest {};
TEST_F(EHeapAssociativeContainerTest, ArenaSet_InsertAndFind)
{
    ArenaSet<int> container;
    for (int value = 0; value < 32; ++value) {
        container.insert(value);
    }
    EXPECT_EQ(container.size(), 32U);
    for (int value = 0; value < 32; ++value) {
        EXPECT_EQ(container.count(value), 1U);
        EXPECT_TRUE(container.find(value) != container.end());
    }
    EXPECT_EQ(container.count(1000), 0U);
}
TEST_F(EHeapAssociativeContainerTest, ArenaSet_DuplicateInsertKeepsSingleEntry)
{
    ArenaSet<int> container;
    int value = 5;
    container.insert(value);
    container.insert(value);
    container.insert(value);
    EXPECT_EQ(container.size(), 1U);
    EXPECT_EQ(container.count(value), 1U);
    container.erase(value);
    EXPECT_EQ(container.size(), 0U);
    EXPECT_EQ(container.count(value), 0U);
}
TEST_F(EHeapAssociativeContainerTest, ArenaSet_IterateAll)
{
    ArenaSet<int> container;
    for (int value = 0; value < 40; ++value) {
        container.insert(value * 3);
    }
    int sum = 0;
    int previous = -1;
    for (const int element : container) {
        sum += element;
        EXPECT_GT(element, previous);
        previous = element;
    }
    EXPECT_EQ(sum, 3 * (40 * 39 / 2));
    EXPECT_EQ(container.size(), 40U);
}
TEST_F(EHeapAssociativeContainerTest, ArenaSet_IterateAndCollect)
{
    ArenaSet<int> container;
    for (int value = 0; value < 12; ++value) {
        container.insert(value);
    }
    ArenaMap<int, int> mirror;
    for (const auto &entry : container) {
        (void)entry;
        ++mirror[0];
    }
    EXPECT_EQ(container.size(), 12U);
    EXPECT_EQ(mirror[0], 12);
    ArenaVector<int> keys;
    for (const auto &entry : container) {
        keys.push_back(entry);
    }
    EXPECT_EQ(keys.size(), 12U);
    EXPECT_EQ(*std::min_element(keys.begin(), keys.end()), 0);
    EXPECT_EQ(*std::max_element(keys.begin(), keys.end()), 11);
}
TEST_F(EHeapAssociativeContainerTest, ArenaSet_EraseAndClear)
{
    ArenaSet<int> container;
    for (int value = 0; value < 24; ++value) {
        container.insert(value);
    }
    container.erase(0);
    EXPECT_EQ(container.count(0), 0U);
    EXPECT_EQ(container.size(), 23U);
    container.clear();
    EXPECT_TRUE(container.empty());
    EXPECT_EQ(container.size(), 0U);
}
TEST_F(EHeapAssociativeContainerTest, ArenaSet_BulkInsert)
{
    ArenaSet<int> container;
    const size_t allocatedBefore = EHeap::AllocatedSize();
    for (int value = 0; value < 128; ++value) {
        container.insert(value);
    }
    EXPECT_EQ(container.size(), 128U);
    EXPECT_GE(EHeap::AllocatedSize(), allocatedBefore);
    EXPECT_EQ(container.count(127), 1U);
}
TEST_F(EHeapAssociativeContainerTest, ArenaUnorderedSet_InsertAndFind)
{
    ArenaUnorderedSet<int> container;
    for (int value = 0; value < 32; ++value) {
        container.insert(value);
    }
    EXPECT_EQ(container.size(), 32U);
    for (int value = 0; value < 32; ++value) {
        EXPECT_EQ(container.count(value), 1U);
        EXPECT_TRUE(container.find(value) != container.end());
    }
    EXPECT_EQ(container.count(1000), 0U);
}
TEST_F(EHeapAssociativeContainerTest, ArenaUnorderedSet_DuplicateInsertKeepsSingleEntry)
{
    ArenaUnorderedSet<int> container;
    int value = 5;
    container.insert(value);
    container.insert(value);
    container.insert(value);
    EXPECT_EQ(container.size(), 1U);
    EXPECT_EQ(container.count(value), 1U);
    container.erase(value);
    EXPECT_EQ(container.size(), 0U);
    EXPECT_EQ(container.count(value), 0U);
}
TEST_F(EHeapAssociativeContainerTest, ArenaUnorderedSet_IterateAll)
{
    ArenaUnorderedSet<int> container;
    for (int value = 0; value < 40; ++value) {
        container.insert(value * 3);
    }
    int sum = 0;
    int previous = -1;
    for (const int element : container) {
        sum += element;
        EXPECT_GE(element, 0);
        (void)previous;
        previous = element;
    }
    EXPECT_EQ(sum, 3 * (40 * 39 / 2));
    EXPECT_EQ(container.size(), 40U);
}
TEST_F(EHeapAssociativeContainerTest, ArenaUnorderedSet_IterateAndCollect)
{
    ArenaUnorderedSet<int> container;
    for (int value = 0; value < 12; ++value) {
        container.insert(value);
    }
    ArenaMap<int, int> mirror;
    for (const auto &entry : container) {
        (void)entry;
        ++mirror[0];
    }
    EXPECT_EQ(container.size(), 12U);
    EXPECT_EQ(mirror[0], 12);
    ArenaVector<int> keys;
    for (const auto &entry : container) {
        keys.push_back(entry);
    }
    EXPECT_EQ(keys.size(), 12U);
    EXPECT_EQ(*std::min_element(keys.begin(), keys.end()), 0);
    EXPECT_EQ(*std::max_element(keys.begin(), keys.end()), 11);
}
TEST_F(EHeapAssociativeContainerTest, ArenaUnorderedSet_EraseAndClear)
{
    ArenaUnorderedSet<int> container;
    for (int value = 0; value < 24; ++value) {
        container.insert(value);
    }
    container.erase(0);
    EXPECT_EQ(container.count(0), 0U);
    EXPECT_EQ(container.size(), 23U);
    container.clear();
    EXPECT_TRUE(container.empty());
    EXPECT_EQ(container.size(), 0U);
}
TEST_F(EHeapAssociativeContainerTest, ArenaUnorderedSet_BulkInsert)
{
    ArenaUnorderedSet<int> container;
    const size_t allocatedBefore = EHeap::AllocatedSize();
    for (int value = 0; value < 128; ++value) {
        container.insert(value);
    }
    EXPECT_EQ(container.size(), 128U);
    EXPECT_GE(EHeap::AllocatedSize(), allocatedBefore);
    EXPECT_EQ(container.count(127), 1U);
}
TEST_F(EHeapAssociativeContainerTest, ArenaMap_InsertAndFind)
{
    ArenaMap<int, int> container;
    for (int value = 0; value < 32; ++value) {
        container[value] = value * 10;
    }
    EXPECT_EQ(container.size(), 32U);
    for (int value = 0; value < 32; ++value) {
        EXPECT_EQ(container.count(value), 1U);
        EXPECT_TRUE(container.find(value) != container.end());
    }
    EXPECT_EQ(container.count(1000), 0U);
}
TEST_F(EHeapAssociativeContainerTest, ArenaMap_DuplicateInsertKeepsSingleEntry)
{
    ArenaMap<int, int> container;
    int value = 5;
    container[value] = value * 10;
    container[value] = value * 10;
    container[value] = value * 10;
    EXPECT_EQ(container.size(), 1U);
    EXPECT_EQ(container.count(value), 1U);
    container.erase(value);
    EXPECT_EQ(container.size(), 0U);
    EXPECT_EQ(container.count(value), 0U);
}
TEST_F(EHeapAssociativeContainerTest, ArenaMap_IterateAll)
{
    ArenaMap<int, int> container;
    for (int value = 0; value < 40; ++value) {
        container[value] = value * value;
    }
    int sum = 0;
    for (const auto &entry : container) {
        sum += entry.second;
    }
    EXPECT_EQ(sum, 40 * 39 * 79 / 6);
    for (int value = 0; value < 40; ++value) {
        EXPECT_EQ(container[value], value * value);
    }
    EXPECT_EQ(container.size(), 40U);
}
TEST_F(EHeapAssociativeContainerTest, ArenaMap_IterateAndCollect)
{
    ArenaMap<int, int> container;
    for (int value = 0; value < 12; ++value) {
        container[value] = value * 10;
    }
    ArenaMap<int, int> mirror;
    for (const auto &entry : container) {
        (void)entry;
        ++mirror[0];
    }
    EXPECT_EQ(container.size(), 12U);
    EXPECT_EQ(mirror[0], 12);
    ArenaVector<int> keys;
    for (const auto &entry : container) {
        keys.push_back(entry.first);
    }
    EXPECT_EQ(keys.size(), 12U);
    EXPECT_EQ(*std::min_element(keys.begin(), keys.end()), 0);
    EXPECT_EQ(*std::max_element(keys.begin(), keys.end()), 11);
}
TEST_F(EHeapAssociativeContainerTest, ArenaMap_EraseAndClear)
{
    ArenaMap<int, int> container;
    for (int value = 0; value < 24; ++value) {
        container[value] = value * 10;
    }
    container.erase(0);
    EXPECT_EQ(container.count(0), 0U);
    EXPECT_EQ(container.size(), 23U);
    container.clear();
    EXPECT_TRUE(container.empty());
    EXPECT_EQ(container.size(), 0U);
}
TEST_F(EHeapAssociativeContainerTest, ArenaMap_BulkInsert)
{
    ArenaMap<int, int> container;
    const size_t allocatedBefore = EHeap::AllocatedSize();
    for (int value = 0; value < 128; ++value) {
        container[value] = value * 10;
    }
    EXPECT_EQ(container.size(), 128U);
    EXPECT_GE(EHeap::AllocatedSize(), allocatedBefore);
    EXPECT_EQ(container.count(127), 1U);
}
TEST_F(EHeapAssociativeContainerTest, ArenaUnorderedMap_InsertAndFind)
{
    ArenaUnorderedMap<int, int> container;
    for (int value = 0; value < 32; ++value) {
        container[value] = value * 10;
    }
    EXPECT_EQ(container.size(), 32U);
    for (int value = 0; value < 32; ++value) {
        EXPECT_EQ(container.count(value), 1U);
        EXPECT_TRUE(container.find(value) != container.end());
    }
    EXPECT_EQ(container.count(1000), 0U);
}
TEST_F(EHeapAssociativeContainerTest, ArenaUnorderedMap_DuplicateInsertKeepsSingleEntry)
{
    ArenaUnorderedMap<int, int> container;
    int value = 5;
    container[value] = value * 10;
    container[value] = value * 10;
    container[value] = value * 10;
    EXPECT_EQ(container.size(), 1U);
    EXPECT_EQ(container.count(value), 1U);
    container.erase(value);
    EXPECT_EQ(container.size(), 0U);
    EXPECT_EQ(container.count(value), 0U);
}
TEST_F(EHeapAssociativeContainerTest, ArenaUnorderedMap_IterateAll)
{
    ArenaUnorderedMap<int, int> container;
    for (int value = 0; value < 40; ++value) {
        container[value] = value * value;
    }
    int sum = 0;
    for (const auto &entry : container) {
        sum += entry.second;
    }
    EXPECT_EQ(sum, 40 * 39 * 79 / 6);
    for (int value = 0; value < 40; ++value) {
        EXPECT_EQ(container[value], value * value);
    }
    EXPECT_EQ(container.size(), 40U);
}
TEST_F(EHeapAssociativeContainerTest, ArenaUnorderedMap_IterateAndCollect)
{
    ArenaUnorderedMap<int, int> container;
    for (int value = 0; value < 12; ++value) {
        container[value] = value * 10;
    }
    ArenaMap<int, int> mirror;
    for (const auto &entry : container) {
        (void)entry;
        ++mirror[0];
    }
    EXPECT_EQ(container.size(), 12U);
    EXPECT_EQ(mirror[0], 12);
    ArenaVector<int> keys;
    for (const auto &entry : container) {
        keys.push_back(entry.first);
    }
    EXPECT_EQ(keys.size(), 12U);
    EXPECT_EQ(*std::min_element(keys.begin(), keys.end()), 0);
    EXPECT_EQ(*std::max_element(keys.begin(), keys.end()), 11);
}
TEST_F(EHeapAssociativeContainerTest, ArenaUnorderedMap_EraseAndClear)
{
    ArenaUnorderedMap<int, int> container;
    for (int value = 0; value < 24; ++value) {
        container[value] = value * 10;
    }
    container.erase(0);
    EXPECT_EQ(container.count(0), 0U);
    EXPECT_EQ(container.size(), 23U);
    container.clear();
    EXPECT_TRUE(container.empty());
    EXPECT_EQ(container.size(), 0U);
}
TEST_F(EHeapAssociativeContainerTest, ArenaUnorderedMap_BulkInsert)
{
    ArenaUnorderedMap<int, int> container;
    const size_t allocatedBefore = EHeap::AllocatedSize();
    for (int value = 0; value < 128; ++value) {
        container[value] = value * 10;
    }
    EXPECT_EQ(container.size(), 128U);
    EXPECT_GE(EHeap::AllocatedSize(), allocatedBefore);
    EXPECT_EQ(container.count(127), 1U);
}
TEST_F(EHeapAssociativeContainerTest, ArenaMultiMap_InsertAndCount)
{
    ArenaMultiMap<int, int> container;
    for (int value = 0; value < 40; ++value) {
        container.insert({value % 4, value});
    }
    EXPECT_EQ(container.size(), 40U);
    for (int key = 0; key < 4; ++key) {
        EXPECT_EQ(container.count(key), 10U);
        const auto range = container.equal_range(key);
        size_t elements = 0U;
        for (auto iterator = range.first; iterator != range.second; ++iterator) {
            EXPECT_EQ(iterator->first, key);
            ++elements;
        }
        EXPECT_EQ(elements, 10U);
    }
}
TEST_F(EHeapAssociativeContainerTest, ArenaMultiMap_EraseAllForKey)
{
    ArenaMultiMap<int, int> container;
    for (int value = 0; value < 20; ++value) {
        container.insert({value % 4, value});
    }
    EXPECT_EQ(container.erase(1), 5U);
    EXPECT_EQ(container.count(1), 0U);
    EXPECT_EQ(container.size(), 15U);
    container.clear();
    EXPECT_TRUE(container.empty());
}
TEST_F(EHeapAssociativeContainerTest, ArenaUnorderedMultiMap_InsertAndCount)
{
    ArenaUnorderedMultiMap<int, int> container;
    for (int value = 0; value < 40; ++value) {
        container.insert({value % 4, value});
    }
    EXPECT_EQ(container.size(), 40U);
    for (int key = 0; key < 4; ++key) {
        EXPECT_EQ(container.count(key), 10U);
        const auto range = container.equal_range(key);
        size_t elements = 0U;
        for (auto iterator = range.first; iterator != range.second; ++iterator) {
            EXPECT_EQ(iterator->first, key);
            ++elements;
        }
        EXPECT_EQ(elements, 10U);
    }
}
TEST_F(EHeapAssociativeContainerTest, ArenaUnorderedMultiMap_EraseAllForKey)
{
    ArenaUnorderedMultiMap<int, int> container;
    for (int value = 0; value < 20; ++value) {
        container.insert({value % 4, value});
    }
    EXPECT_EQ(container.erase(1), 5U);
    EXPECT_EQ(container.count(1), 0U);
    EXPECT_EQ(container.size(), 15U);
    container.clear();
    EXPECT_TRUE(container.empty());
}
// ---------------------------------------------------------------------------
// ArenaString / ArenaDoubleUnorderedMap
// ---------------------------------------------------------------------------
class EHeapStringTest : public EHeapTest {};
TEST_F(EHeapStringTest, AppendAndCompare)
{
    ArenaString text;
    text.append("es2panda");
    text += " eheap";
    EXPECT_EQ(text.size(), 14U);
    EXPECT_EQ(text, "es2panda eheap");
    EXPECT_EQ(text.find("eheap"), 9U);
    EXPECT_EQ(text.substr(0, 8), "es2panda");
}
TEST_F(EHeapStringTest, CopyAndMove)
{
    ArenaString text {"arena allocated string"};
    ArenaString copy {text};
    EXPECT_EQ(copy, text);
    EXPECT_EQ(copy.size(), text.size());
    ArenaString moved {std::move(copy)};
    EXPECT_EQ(moved, text);
    EXPECT_EQ(moved.size(), 22U);
}
TEST_F(EHeapStringTest, LargeStringGrowth)
{
    ArenaString text;
    for (size_t index = 0; index < 512U; ++index) {
        text.push_back(static_cast<char>('a' + (index % 26U)));
    }
    EXPECT_EQ(text.size(), 512U);
    EXPECT_EQ(text.front(), 'a');
    EXPECT_EQ(text[26], 'a');
    EXPECT_EQ(text.back(), static_cast<char>('a' + (511U % 26U)));
}
TEST_F(EHeapStringTest, ClearAndReuse)
{
    ArenaString text {"temporary"};
    text.clear();
    EXPECT_TRUE(text.empty());
    EXPECT_EQ(text.size(), 0U);
    text.assign("again");
    EXPECT_EQ(text, "again");
    EXPECT_EQ(text.size(), 5U);
}
TEST_F(EHeapStringTest, HashIsAvailableForUnorderedSets)
{
    std::unordered_set<ArenaString> names;
    names.insert(ArenaString {"alpha"});
    names.insert(ArenaString {"beta"});
    names.insert(ArenaString {"alpha"});
    EXPECT_EQ(names.size(), 2U);
    EXPECT_EQ(names.count(ArenaString {"beta"}), 1U);
    EXPECT_EQ(names.count(ArenaString {"gamma"}), 0U);
}
TEST_F(EHeapStringTest, RelationalOperators)
{
    const ArenaString first {"apple"};
    const ArenaString second {"banana"};
    EXPECT_TRUE(first < second);
    EXPECT_TRUE(second > first);
    EXPECT_TRUE(first != second);
    EXPECT_FALSE(first == second);
    EXPECT_TRUE(ArenaString {"same"} == ArenaString {"same"});
}
class EHeapDoubleMapTest : public EHeapTest {};
TEST_F(EHeapDoubleMapTest, NestedAssignment)
{
    ArenaDoubleUnorderedMap<int, int, int> container;
    for (int outer = 0; outer < 8; ++outer) {
        for (int inner = 0; inner < 4; ++inner) {
            container[outer][inner] = outer * 100 + inner;
        }
    }
    EXPECT_EQ(container.size(), 8U);
    EXPECT_EQ(container[3][2], 302);
    EXPECT_EQ(container[7][3], 703);
}
TEST_F(EHeapDoubleMapTest, OuterLookup)
{
    ArenaDoubleUnorderedMap<int, int, int> container;
    container[1][1] = 11;
    container[2][2] = 22;
    auto outer = container.find(2);
    ASSERT_NE(outer, container.end());
    EXPECT_EQ(outer->second.at(2), 22);
    EXPECT_EQ(container.find(3), container.end());
}
TEST_F(EHeapDoubleMapTest, EraseInnerAndOuter)
{
    ArenaDoubleUnorderedMap<int, int, int> container;
    container[1][1] = 1;
    container[1][2] = 2;
    container[2][1] = 3;
    EXPECT_EQ(container[1].erase(1), 1U);
    EXPECT_EQ(container[1].size(), 1U);
    EXPECT_EQ(container.erase(2), 1U);
    EXPECT_EQ(container.size(), 1U);
}
TEST_F(EHeapDoubleMapTest, CopyRoundTrip)
{
    ArenaDoubleUnorderedMap<int, int, int> container;
    for (int outer = 0; outer < 4; ++outer) {
        for (int inner = 0; inner < 4; ++inner) {
            container[outer][inner] = outer + inner;
        }
    }
    ArenaDoubleUnorderedMap<int, int, int> copy {container};
    EXPECT_EQ(copy.size(), container.size());
    EXPECT_EQ(copy[2][3], 5);
    copy[2][3] = 100;
    EXPECT_EQ(container[2][3], 5);
}
TEST_F(EHeapDoubleMapTest, IterateAllPairs)
{
    ArenaDoubleUnorderedMap<int, int, int> container;
    int expected = 0;
    for (int outer = 0; outer < 6; ++outer) {
        for (int inner = 0; inner < 3; ++inner) {
            container[outer][inner] = outer + inner;
            expected += outer + inner;
        }
    }
    int sum = 0;
    for (const auto &outer : container) {
        for (const auto &inner : outer.second) {
            sum += inner.second;
        }
    }
    EXPECT_EQ(sum, expected);
    EXPECT_EQ(container.size(), 6U);
}
// ---------------------------------------------------------------------------
// SArena* aliases backed by ark::ArenaAllocator
// ---------------------------------------------------------------------------
class EHeapSArenaContainerTest : public EHeapSArenaTest {};
TEST_F(EHeapSArenaContainerTest, VectorPushAndSort)
{
    SArenaVector<int> values {ark::ArenaAllocatorAdapter<int, false>(Allocator())};
    for (int value = 0; value < 64; ++value) {
        values.push_back((value * 37) % 64);
    }
    EXPECT_EQ(values.size(), 64U);
    std::sort(values.begin(), values.end());
    for (size_t index = 1; index < values.size(); ++index) {
        EXPECT_LE(values[index - 1U], values[index]);
    }
}
TEST_F(EHeapSArenaContainerTest, VectorReserve)
{
    SArenaVector<int> values {ark::ArenaAllocatorAdapter<int, false>(Allocator())};
    values.reserve(256U);
    EXPECT_GE(values.capacity(), 256U);
    for (int value = 0; value < 128; ++value) {
        values.push_back(value);
    }
    EXPECT_EQ(values.size(), 128U);
    EXPECT_EQ(values.front(), 0);
    EXPECT_EQ(values.back(), 127);
}
TEST_F(EHeapSArenaContainerTest, MapInsertAndFind)
{
    SArenaMap<int, int> values {std::less<int>(),
                                ark::ArenaAllocatorAdapter<std::pair<const int, int>, false>(Allocator())};
    for (int key = 0; key < 32; ++key) {
        values[key] = key * 2;
    }
    EXPECT_EQ(values.size(), 32U);
    EXPECT_EQ(values[7], 14);
    EXPECT_EQ(values.count(100), 0U);
}
TEST_F(EHeapSArenaContainerTest, UnorderedMapInsertAndFind)
{
    SArenaUnorderedMap<int, int> values {0U, std::hash<int>(), std::equal_to<int>(),
                                         ark::ArenaAllocatorAdapter<std::pair<const int, int>, false>(Allocator())};
    for (int key = 0; key < 32; ++key) {
        values[key] = key * 3;
    }
    EXPECT_EQ(values.size(), 32U);
    EXPECT_EQ(values[9], 27);
    EXPECT_EQ(values.count(100), 0U);
}
TEST_F(EHeapSArenaContainerTest, ListPushAndIterate)
{
    SArenaList<int> values {ark::ArenaAllocatorAdapter<int, false>(Allocator())};
    for (int value = 0; value < 16; ++value) {
        values.push_back(value);
        values.push_front(-value);
    }
    EXPECT_EQ(values.size(), 32U);
    EXPECT_EQ(values.front(), -15);
    EXPECT_EQ(values.back(), 15);
}
TEST_F(EHeapSArenaContainerTest, StringAppend)
{
    SArenaString text {ark::ArenaAllocatorAdapter<char, false>(Allocator())};
    text.append("sarena");
    text.append(" string");
    EXPECT_EQ(text, "sarena string");
    EXPECT_EQ(text.size(), 13U);
}
TEST_F(EHeapSArenaContainerTest, VectorClearAndReuse)
{
    SArenaVector<int> values {ark::ArenaAllocatorAdapter<int, false>(Allocator())};
    for (int value = 0; value < 100; ++value) {
        values.push_back(value);
    }
    values.clear();
    EXPECT_TRUE(values.empty());
    values.push_back(7);
    EXPECT_EQ(values.size(), 1U);
    EXPECT_EQ(values.front(), 7);
}
// ---------------------------------------------------------------------------
// Allocator factories
// ---------------------------------------------------------------------------
class EHeapAllocatorFactoryTest : public EHeapSArenaTest {};
TEST_F(EHeapAllocatorFactoryTest, ScopedAllocatorStartsEmpty)
{
    auto allocator = ScopedAllocatorsManager::NewAllocator();
    ASSERT_NE(allocator, nullptr);
    void *first = allocator->Alloc(16U);
    ASSERT_NE(first, nullptr);
    EXPECT_EQ(static_cast<size_t>(Addr(first) % alignof(uint64_t)), 0U);
    SArenaAllocator inlineAllocator = ScopedAllocatorsManager::CreateAllocator();
    void *second = inlineAllocator.Alloc(8U);
    ASSERT_NE(second, nullptr);
    EXPECT_NE(first, second);
    EXPECT_EQ(inlineAllocator.Alloc(0U), inlineAllocator.Alloc(0U));
}
TEST_F(EHeapAllocatorFactoryTest, ScopedAllocatorServesContainers)
{
    auto allocator = ScopedAllocatorsManager::NewAllocator();
    SArenaVector<int> values {ark::ArenaAllocatorAdapter<int, false>(allocator.get())};
    for (int value = 0; value < 32; ++value) {
        values.push_back(value);
    }
    EXPECT_EQ(values.size(), 32U);
    EXPECT_EQ(values.back(), 31);
}
TEST_F(EHeapAllocatorFactoryTest, CreateAllocatorValueIsUsable)
{
    SArenaAllocator allocator = ScopedAllocatorsManager::CreateAllocator();
    void *block = allocator.Alloc(64U);
    ASSERT_NE(block, nullptr);
    FillByte(block, 64U, 0x5AU);
    EXPECT_TRUE(AllBytes(block, 64U, 0x5AU));
    SArenaString text {ark::ArenaAllocatorAdapter<char, false>(&allocator)};
    text.append("value");
    EXPECT_EQ(text, "value");
}
// ---------------------------------------------------------------------------
// Multi-threaded stress tests
//
// Every mutation of the heap is serialized by `HeapMutationGuard`; the assertions
// below check cross-thread invariants rather than lock-free progress.
// ---------------------------------------------------------------------------
class EHeapThreadingTest : public EHeapTest {};
TEST_F(EHeapThreadingTest, SerializedAlloc_threads01_blocks16)
{
    constexpr size_t THREADS = 1U;
    constexpr size_t BLOCKS_PER_THREAD = 16U;
    std::vector<std::vector<BlockRecord>> perThread(THREADS);
    std::vector<size_t> perThreadBytes(THREADS, 0U);
    const size_t allocatedBefore = EHeap::AllocatedSize();
    RunThreads(THREADS, [&perThread, &perThreadBytes](size_t threadIndex) {
        auto &local = perThread[threadIndex];
        local.reserve(BLOCKS_PER_THREAD);
        size_t bytes = 0U;
        for (size_t iteration = 0; iteration < BLOCKS_PER_THREAD; ++iteration) {
            const size_t size = 1U + ((threadIndex * 131U + iteration * 37U) % 512U);
            const uint64_t seed = threadIndex * 1000U + iteration;
            HeapMutationGuard guard;
            void *block = EHeap::Alloc(size);
            if (block == nullptr) {
                return;
            }
            FillPattern(block, size, seed);
            local.push_back(BlockRecord {block, size, seed});
            bytes += AlignUpBytes(size);
        }
        perThreadBytes[threadIndex] = bytes;
    });
    size_t expectedBytes = 0U;
    for (const size_t bytes : perThreadBytes) {
        expectedBytes += bytes;
    }
    EXPECT_EQ(EHeap::AllocatedSize() - allocatedBefore, expectedBytes);
    std::vector<BlockRecord> all;
    all.reserve(THREADS * BLOCKS_PER_THREAD);
    for (const auto &local : perThread) {
        all.insert(all.end(), local.begin(), local.end());
    }
    EXPECT_EQ(all.size(), THREADS * BLOCKS_PER_THREAD);
    for (const auto &record : all) {
        EXPECT_TRUE(IsAligned(record.ptr));
        EXPECT_TRUE(PatternMatches(record.ptr, record.size, record.seed));
    }
    std::sort(all.begin(), all.end(),
              [](const BlockRecord &lhs, const BlockRecord &rhs) { return Addr(lhs.ptr) < Addr(rhs.ptr); });
    for (size_t index = 1; index < all.size(); ++index) {
        EXPECT_LE(Addr(all[index - 1U].ptr) + all[index - 1U].size, Addr(all[index].ptr));
    }
}
TEST_F(EHeapThreadingTest, SerializedAlloc_threads01_blocks48)
{
    constexpr size_t THREADS = 1U;
    constexpr size_t BLOCKS_PER_THREAD = 48U;
    std::vector<std::vector<BlockRecord>> perThread(THREADS);
    std::vector<size_t> perThreadBytes(THREADS, 0U);
    const size_t allocatedBefore = EHeap::AllocatedSize();
    RunThreads(THREADS, [&perThread, &perThreadBytes](size_t threadIndex) {
        auto &local = perThread[threadIndex];
        local.reserve(BLOCKS_PER_THREAD);
        size_t bytes = 0U;
        for (size_t iteration = 0; iteration < BLOCKS_PER_THREAD; ++iteration) {
            const size_t size = 1U + ((threadIndex * 131U + iteration * 37U) % 512U);
            const uint64_t seed = threadIndex * 1000U + iteration;
            HeapMutationGuard guard;
            void *block = EHeap::Alloc(size);
            if (block == nullptr) {
                return;
            }
            FillPattern(block, size, seed);
            local.push_back(BlockRecord {block, size, seed});
            bytes += AlignUpBytes(size);
        }
        perThreadBytes[threadIndex] = bytes;
    });
    size_t expectedBytes = 0U;
    for (const size_t bytes : perThreadBytes) {
        expectedBytes += bytes;
    }
    EXPECT_EQ(EHeap::AllocatedSize() - allocatedBefore, expectedBytes);
    std::vector<BlockRecord> all;
    all.reserve(THREADS * BLOCKS_PER_THREAD);
    for (const auto &local : perThread) {
        all.insert(all.end(), local.begin(), local.end());
    }
    EXPECT_EQ(all.size(), THREADS * BLOCKS_PER_THREAD);
    for (const auto &record : all) {
        EXPECT_TRUE(IsAligned(record.ptr));
        EXPECT_TRUE(PatternMatches(record.ptr, record.size, record.seed));
    }
    std::sort(all.begin(), all.end(),
              [](const BlockRecord &lhs, const BlockRecord &rhs) { return Addr(lhs.ptr) < Addr(rhs.ptr); });
    for (size_t index = 1; index < all.size(); ++index) {
        EXPECT_LE(Addr(all[index - 1U].ptr) + all[index - 1U].size, Addr(all[index].ptr));
    }
}
TEST_F(EHeapThreadingTest, SerializedAlloc_threads02_blocks16)
{
    constexpr size_t THREADS = 2U;
    constexpr size_t BLOCKS_PER_THREAD = 16U;
    std::vector<std::vector<BlockRecord>> perThread(THREADS);
    std::vector<size_t> perThreadBytes(THREADS, 0U);
    const size_t allocatedBefore = EHeap::AllocatedSize();
    RunThreads(THREADS, [&perThread, &perThreadBytes](size_t threadIndex) {
        auto &local = perThread[threadIndex];
        local.reserve(BLOCKS_PER_THREAD);
        size_t bytes = 0U;
        for (size_t iteration = 0; iteration < BLOCKS_PER_THREAD; ++iteration) {
            const size_t size = 1U + ((threadIndex * 131U + iteration * 37U) % 512U);
            const uint64_t seed = threadIndex * 1000U + iteration;
            HeapMutationGuard guard;
            void *block = EHeap::Alloc(size);
            if (block == nullptr) {
                return;
            }
            FillPattern(block, size, seed);
            local.push_back(BlockRecord {block, size, seed});
            bytes += AlignUpBytes(size);
        }
        perThreadBytes[threadIndex] = bytes;
    });
    size_t expectedBytes = 0U;
    for (const size_t bytes : perThreadBytes) {
        expectedBytes += bytes;
    }
    EXPECT_EQ(EHeap::AllocatedSize() - allocatedBefore, expectedBytes);
    std::vector<BlockRecord> all;
    all.reserve(THREADS * BLOCKS_PER_THREAD);
    for (const auto &local : perThread) {
        all.insert(all.end(), local.begin(), local.end());
    }
    EXPECT_EQ(all.size(), THREADS * BLOCKS_PER_THREAD);
    for (const auto &record : all) {
        EXPECT_TRUE(IsAligned(record.ptr));
        EXPECT_TRUE(PatternMatches(record.ptr, record.size, record.seed));
    }
    std::sort(all.begin(), all.end(),
              [](const BlockRecord &lhs, const BlockRecord &rhs) { return Addr(lhs.ptr) < Addr(rhs.ptr); });
    for (size_t index = 1; index < all.size(); ++index) {
        EXPECT_LE(Addr(all[index - 1U].ptr) + all[index - 1U].size, Addr(all[index].ptr));
    }
}
TEST_F(EHeapThreadingTest, SerializedAlloc_threads02_blocks48)
{
    constexpr size_t THREADS = 2U;
    constexpr size_t BLOCKS_PER_THREAD = 48U;
    std::vector<std::vector<BlockRecord>> perThread(THREADS);
    std::vector<size_t> perThreadBytes(THREADS, 0U);
    const size_t allocatedBefore = EHeap::AllocatedSize();
    RunThreads(THREADS, [&perThread, &perThreadBytes](size_t threadIndex) {
        auto &local = perThread[threadIndex];
        local.reserve(BLOCKS_PER_THREAD);
        size_t bytes = 0U;
        for (size_t iteration = 0; iteration < BLOCKS_PER_THREAD; ++iteration) {
            const size_t size = 1U + ((threadIndex * 131U + iteration * 37U) % 512U);
            const uint64_t seed = threadIndex * 1000U + iteration;
            HeapMutationGuard guard;
            void *block = EHeap::Alloc(size);
            if (block == nullptr) {
                return;
            }
            FillPattern(block, size, seed);
            local.push_back(BlockRecord {block, size, seed});
            bytes += AlignUpBytes(size);
        }
        perThreadBytes[threadIndex] = bytes;
    });
    size_t expectedBytes = 0U;
    for (const size_t bytes : perThreadBytes) {
        expectedBytes += bytes;
    }
    EXPECT_EQ(EHeap::AllocatedSize() - allocatedBefore, expectedBytes);
    std::vector<BlockRecord> all;
    all.reserve(THREADS * BLOCKS_PER_THREAD);
    for (const auto &local : perThread) {
        all.insert(all.end(), local.begin(), local.end());
    }
    EXPECT_EQ(all.size(), THREADS * BLOCKS_PER_THREAD);
    for (const auto &record : all) {
        EXPECT_TRUE(IsAligned(record.ptr));
        EXPECT_TRUE(PatternMatches(record.ptr, record.size, record.seed));
    }
    std::sort(all.begin(), all.end(),
              [](const BlockRecord &lhs, const BlockRecord &rhs) { return Addr(lhs.ptr) < Addr(rhs.ptr); });
    for (size_t index = 1; index < all.size(); ++index) {
        EXPECT_LE(Addr(all[index - 1U].ptr) + all[index - 1U].size, Addr(all[index].ptr));
    }
}
TEST_F(EHeapThreadingTest, SerializedAlloc_threads03_blocks16)
{
    constexpr size_t THREADS = 3U;
    constexpr size_t BLOCKS_PER_THREAD = 16U;
    std::vector<std::vector<BlockRecord>> perThread(THREADS);
    std::vector<size_t> perThreadBytes(THREADS, 0U);
    const size_t allocatedBefore = EHeap::AllocatedSize();
    RunThreads(THREADS, [&perThread, &perThreadBytes](size_t threadIndex) {
        auto &local = perThread[threadIndex];
        local.reserve(BLOCKS_PER_THREAD);
        size_t bytes = 0U;
        for (size_t iteration = 0; iteration < BLOCKS_PER_THREAD; ++iteration) {
            const size_t size = 1U + ((threadIndex * 131U + iteration * 37U) % 512U);
            const uint64_t seed = threadIndex * 1000U + iteration;
            HeapMutationGuard guard;
            void *block = EHeap::Alloc(size);
            if (block == nullptr) {
                return;
            }
            FillPattern(block, size, seed);
            local.push_back(BlockRecord {block, size, seed});
            bytes += AlignUpBytes(size);
        }
        perThreadBytes[threadIndex] = bytes;
    });
    size_t expectedBytes = 0U;
    for (const size_t bytes : perThreadBytes) {
        expectedBytes += bytes;
    }
    EXPECT_EQ(EHeap::AllocatedSize() - allocatedBefore, expectedBytes);
    std::vector<BlockRecord> all;
    all.reserve(THREADS * BLOCKS_PER_THREAD);
    for (const auto &local : perThread) {
        all.insert(all.end(), local.begin(), local.end());
    }
    EXPECT_EQ(all.size(), THREADS * BLOCKS_PER_THREAD);
    for (const auto &record : all) {
        EXPECT_TRUE(IsAligned(record.ptr));
        EXPECT_TRUE(PatternMatches(record.ptr, record.size, record.seed));
    }
    std::sort(all.begin(), all.end(),
              [](const BlockRecord &lhs, const BlockRecord &rhs) { return Addr(lhs.ptr) < Addr(rhs.ptr); });
    for (size_t index = 1; index < all.size(); ++index) {
        EXPECT_LE(Addr(all[index - 1U].ptr) + all[index - 1U].size, Addr(all[index].ptr));
    }
}
TEST_F(EHeapThreadingTest, SerializedAlloc_threads03_blocks48)
{
    constexpr size_t THREADS = 3U;
    constexpr size_t BLOCKS_PER_THREAD = 48U;
    std::vector<std::vector<BlockRecord>> perThread(THREADS);
    std::vector<size_t> perThreadBytes(THREADS, 0U);
    const size_t allocatedBefore = EHeap::AllocatedSize();
    RunThreads(THREADS, [&perThread, &perThreadBytes](size_t threadIndex) {
        auto &local = perThread[threadIndex];
        local.reserve(BLOCKS_PER_THREAD);
        size_t bytes = 0U;
        for (size_t iteration = 0; iteration < BLOCKS_PER_THREAD; ++iteration) {
            const size_t size = 1U + ((threadIndex * 131U + iteration * 37U) % 512U);
            const uint64_t seed = threadIndex * 1000U + iteration;
            HeapMutationGuard guard;
            void *block = EHeap::Alloc(size);
            if (block == nullptr) {
                return;
            }
            FillPattern(block, size, seed);
            local.push_back(BlockRecord {block, size, seed});
            bytes += AlignUpBytes(size);
        }
        perThreadBytes[threadIndex] = bytes;
    });
    size_t expectedBytes = 0U;
    for (const size_t bytes : perThreadBytes) {
        expectedBytes += bytes;
    }
    EXPECT_EQ(EHeap::AllocatedSize() - allocatedBefore, expectedBytes);
    std::vector<BlockRecord> all;
    all.reserve(THREADS * BLOCKS_PER_THREAD);
    for (const auto &local : perThread) {
        all.insert(all.end(), local.begin(), local.end());
    }
    EXPECT_EQ(all.size(), THREADS * BLOCKS_PER_THREAD);
    for (const auto &record : all) {
        EXPECT_TRUE(IsAligned(record.ptr));
        EXPECT_TRUE(PatternMatches(record.ptr, record.size, record.seed));
    }
    std::sort(all.begin(), all.end(),
              [](const BlockRecord &lhs, const BlockRecord &rhs) { return Addr(lhs.ptr) < Addr(rhs.ptr); });
    for (size_t index = 1; index < all.size(); ++index) {
        EXPECT_LE(Addr(all[index - 1U].ptr) + all[index - 1U].size, Addr(all[index].ptr));
    }
}
TEST_F(EHeapThreadingTest, SerializedAlloc_threads04_blocks16)
{
    constexpr size_t THREADS = 4U;
    constexpr size_t BLOCKS_PER_THREAD = 16U;
    std::vector<std::vector<BlockRecord>> perThread(THREADS);
    std::vector<size_t> perThreadBytes(THREADS, 0U);
    const size_t allocatedBefore = EHeap::AllocatedSize();
    RunThreads(THREADS, [&perThread, &perThreadBytes](size_t threadIndex) {
        auto &local = perThread[threadIndex];
        local.reserve(BLOCKS_PER_THREAD);
        size_t bytes = 0U;
        for (size_t iteration = 0; iteration < BLOCKS_PER_THREAD; ++iteration) {
            const size_t size = 1U + ((threadIndex * 131U + iteration * 37U) % 512U);
            const uint64_t seed = threadIndex * 1000U + iteration;
            HeapMutationGuard guard;
            void *block = EHeap::Alloc(size);
            if (block == nullptr) {
                return;
            }
            FillPattern(block, size, seed);
            local.push_back(BlockRecord {block, size, seed});
            bytes += AlignUpBytes(size);
        }
        perThreadBytes[threadIndex] = bytes;
    });
    size_t expectedBytes = 0U;
    for (const size_t bytes : perThreadBytes) {
        expectedBytes += bytes;
    }
    EXPECT_EQ(EHeap::AllocatedSize() - allocatedBefore, expectedBytes);
    std::vector<BlockRecord> all;
    all.reserve(THREADS * BLOCKS_PER_THREAD);
    for (const auto &local : perThread) {
        all.insert(all.end(), local.begin(), local.end());
    }
    EXPECT_EQ(all.size(), THREADS * BLOCKS_PER_THREAD);
    for (const auto &record : all) {
        EXPECT_TRUE(IsAligned(record.ptr));
        EXPECT_TRUE(PatternMatches(record.ptr, record.size, record.seed));
    }
    std::sort(all.begin(), all.end(),
              [](const BlockRecord &lhs, const BlockRecord &rhs) { return Addr(lhs.ptr) < Addr(rhs.ptr); });
    for (size_t index = 1; index < all.size(); ++index) {
        EXPECT_LE(Addr(all[index - 1U].ptr) + all[index - 1U].size, Addr(all[index].ptr));
    }
}
TEST_F(EHeapThreadingTest, SerializedAlloc_threads04_blocks48)
{
    constexpr size_t THREADS = 4U;
    constexpr size_t BLOCKS_PER_THREAD = 48U;
    std::vector<std::vector<BlockRecord>> perThread(THREADS);
    std::vector<size_t> perThreadBytes(THREADS, 0U);
    const size_t allocatedBefore = EHeap::AllocatedSize();
    RunThreads(THREADS, [&perThread, &perThreadBytes](size_t threadIndex) {
        auto &local = perThread[threadIndex];
        local.reserve(BLOCKS_PER_THREAD);
        size_t bytes = 0U;
        for (size_t iteration = 0; iteration < BLOCKS_PER_THREAD; ++iteration) {
            const size_t size = 1U + ((threadIndex * 131U + iteration * 37U) % 512U);
            const uint64_t seed = threadIndex * 1000U + iteration;
            HeapMutationGuard guard;
            void *block = EHeap::Alloc(size);
            if (block == nullptr) {
                return;
            }
            FillPattern(block, size, seed);
            local.push_back(BlockRecord {block, size, seed});
            bytes += AlignUpBytes(size);
        }
        perThreadBytes[threadIndex] = bytes;
    });
    size_t expectedBytes = 0U;
    for (const size_t bytes : perThreadBytes) {
        expectedBytes += bytes;
    }
    EXPECT_EQ(EHeap::AllocatedSize() - allocatedBefore, expectedBytes);
    std::vector<BlockRecord> all;
    all.reserve(THREADS * BLOCKS_PER_THREAD);
    for (const auto &local : perThread) {
        all.insert(all.end(), local.begin(), local.end());
    }
    EXPECT_EQ(all.size(), THREADS * BLOCKS_PER_THREAD);
    for (const auto &record : all) {
        EXPECT_TRUE(IsAligned(record.ptr));
        EXPECT_TRUE(PatternMatches(record.ptr, record.size, record.seed));
    }
    std::sort(all.begin(), all.end(),
              [](const BlockRecord &lhs, const BlockRecord &rhs) { return Addr(lhs.ptr) < Addr(rhs.ptr); });
    for (size_t index = 1; index < all.size(); ++index) {
        EXPECT_LE(Addr(all[index - 1U].ptr) + all[index - 1U].size, Addr(all[index].ptr));
    }
}
TEST_F(EHeapThreadingTest, SerializedAlloc_threads06_blocks16)
{
    constexpr size_t THREADS = 6U;
    constexpr size_t BLOCKS_PER_THREAD = 16U;
    std::vector<std::vector<BlockRecord>> perThread(THREADS);
    std::vector<size_t> perThreadBytes(THREADS, 0U);
    const size_t allocatedBefore = EHeap::AllocatedSize();
    RunThreads(THREADS, [&perThread, &perThreadBytes](size_t threadIndex) {
        auto &local = perThread[threadIndex];
        local.reserve(BLOCKS_PER_THREAD);
        size_t bytes = 0U;
        for (size_t iteration = 0; iteration < BLOCKS_PER_THREAD; ++iteration) {
            const size_t size = 1U + ((threadIndex * 131U + iteration * 37U) % 512U);
            const uint64_t seed = threadIndex * 1000U + iteration;
            HeapMutationGuard guard;
            void *block = EHeap::Alloc(size);
            if (block == nullptr) {
                return;
            }
            FillPattern(block, size, seed);
            local.push_back(BlockRecord {block, size, seed});
            bytes += AlignUpBytes(size);
        }
        perThreadBytes[threadIndex] = bytes;
    });
    size_t expectedBytes = 0U;
    for (const size_t bytes : perThreadBytes) {
        expectedBytes += bytes;
    }
    EXPECT_EQ(EHeap::AllocatedSize() - allocatedBefore, expectedBytes);
    std::vector<BlockRecord> all;
    all.reserve(THREADS * BLOCKS_PER_THREAD);
    for (const auto &local : perThread) {
        all.insert(all.end(), local.begin(), local.end());
    }
    EXPECT_EQ(all.size(), THREADS * BLOCKS_PER_THREAD);
    for (const auto &record : all) {
        EXPECT_TRUE(IsAligned(record.ptr));
        EXPECT_TRUE(PatternMatches(record.ptr, record.size, record.seed));
    }
    std::sort(all.begin(), all.end(),
              [](const BlockRecord &lhs, const BlockRecord &rhs) { return Addr(lhs.ptr) < Addr(rhs.ptr); });
    for (size_t index = 1; index < all.size(); ++index) {
        EXPECT_LE(Addr(all[index - 1U].ptr) + all[index - 1U].size, Addr(all[index].ptr));
    }
}
TEST_F(EHeapThreadingTest, SerializedAlloc_threads06_blocks48)
{
    constexpr size_t THREADS = 6U;
    constexpr size_t BLOCKS_PER_THREAD = 48U;
    std::vector<std::vector<BlockRecord>> perThread(THREADS);
    std::vector<size_t> perThreadBytes(THREADS, 0U);
    const size_t allocatedBefore = EHeap::AllocatedSize();
    RunThreads(THREADS, [&perThread, &perThreadBytes](size_t threadIndex) {
        auto &local = perThread[threadIndex];
        local.reserve(BLOCKS_PER_THREAD);
        size_t bytes = 0U;
        for (size_t iteration = 0; iteration < BLOCKS_PER_THREAD; ++iteration) {
            const size_t size = 1U + ((threadIndex * 131U + iteration * 37U) % 512U);
            const uint64_t seed = threadIndex * 1000U + iteration;
            HeapMutationGuard guard;
            void *block = EHeap::Alloc(size);
            if (block == nullptr) {
                return;
            }
            FillPattern(block, size, seed);
            local.push_back(BlockRecord {block, size, seed});
            bytes += AlignUpBytes(size);
        }
        perThreadBytes[threadIndex] = bytes;
    });
    size_t expectedBytes = 0U;
    for (const size_t bytes : perThreadBytes) {
        expectedBytes += bytes;
    }
    EXPECT_EQ(EHeap::AllocatedSize() - allocatedBefore, expectedBytes);
    std::vector<BlockRecord> all;
    all.reserve(THREADS * BLOCKS_PER_THREAD);
    for (const auto &local : perThread) {
        all.insert(all.end(), local.begin(), local.end());
    }
    EXPECT_EQ(all.size(), THREADS * BLOCKS_PER_THREAD);
    for (const auto &record : all) {
        EXPECT_TRUE(IsAligned(record.ptr));
        EXPECT_TRUE(PatternMatches(record.ptr, record.size, record.seed));
    }
    std::sort(all.begin(), all.end(),
              [](const BlockRecord &lhs, const BlockRecord &rhs) { return Addr(lhs.ptr) < Addr(rhs.ptr); });
    for (size_t index = 1; index < all.size(); ++index) {
        EXPECT_LE(Addr(all[index - 1U].ptr) + all[index - 1U].size, Addr(all[index].ptr));
    }
}
TEST_F(EHeapThreadingTest, SerializedAlloc_threads08_blocks16)
{
    constexpr size_t THREADS = 8U;
    constexpr size_t BLOCKS_PER_THREAD = 16U;
    std::vector<std::vector<BlockRecord>> perThread(THREADS);
    std::vector<size_t> perThreadBytes(THREADS, 0U);
    const size_t allocatedBefore = EHeap::AllocatedSize();
    RunThreads(THREADS, [&perThread, &perThreadBytes](size_t threadIndex) {
        auto &local = perThread[threadIndex];
        local.reserve(BLOCKS_PER_THREAD);
        size_t bytes = 0U;
        for (size_t iteration = 0; iteration < BLOCKS_PER_THREAD; ++iteration) {
            const size_t size = 1U + ((threadIndex * 131U + iteration * 37U) % 512U);
            const uint64_t seed = threadIndex * 1000U + iteration;
            HeapMutationGuard guard;
            void *block = EHeap::Alloc(size);
            if (block == nullptr) {
                return;
            }
            FillPattern(block, size, seed);
            local.push_back(BlockRecord {block, size, seed});
            bytes += AlignUpBytes(size);
        }
        perThreadBytes[threadIndex] = bytes;
    });
    size_t expectedBytes = 0U;
    for (const size_t bytes : perThreadBytes) {
        expectedBytes += bytes;
    }
    EXPECT_EQ(EHeap::AllocatedSize() - allocatedBefore, expectedBytes);
    std::vector<BlockRecord> all;
    all.reserve(THREADS * BLOCKS_PER_THREAD);
    for (const auto &local : perThread) {
        all.insert(all.end(), local.begin(), local.end());
    }
    EXPECT_EQ(all.size(), THREADS * BLOCKS_PER_THREAD);
    for (const auto &record : all) {
        EXPECT_TRUE(IsAligned(record.ptr));
        EXPECT_TRUE(PatternMatches(record.ptr, record.size, record.seed));
    }
    std::sort(all.begin(), all.end(),
              [](const BlockRecord &lhs, const BlockRecord &rhs) { return Addr(lhs.ptr) < Addr(rhs.ptr); });
    for (size_t index = 1; index < all.size(); ++index) {
        EXPECT_LE(Addr(all[index - 1U].ptr) + all[index - 1U].size, Addr(all[index].ptr));
    }
}
TEST_F(EHeapThreadingTest, SerializedAlloc_threads08_blocks48)
{
    constexpr size_t THREADS = 8U;
    constexpr size_t BLOCKS_PER_THREAD = 48U;
    std::vector<std::vector<BlockRecord>> perThread(THREADS);
    std::vector<size_t> perThreadBytes(THREADS, 0U);
    const size_t allocatedBefore = EHeap::AllocatedSize();
    RunThreads(THREADS, [&perThread, &perThreadBytes](size_t threadIndex) {
        auto &local = perThread[threadIndex];
        local.reserve(BLOCKS_PER_THREAD);
        size_t bytes = 0U;
        for (size_t iteration = 0; iteration < BLOCKS_PER_THREAD; ++iteration) {
            const size_t size = 1U + ((threadIndex * 131U + iteration * 37U) % 512U);
            const uint64_t seed = threadIndex * 1000U + iteration;
            HeapMutationGuard guard;
            void *block = EHeap::Alloc(size);
            if (block == nullptr) {
                return;
            }
            FillPattern(block, size, seed);
            local.push_back(BlockRecord {block, size, seed});
            bytes += AlignUpBytes(size);
        }
        perThreadBytes[threadIndex] = bytes;
    });
    size_t expectedBytes = 0U;
    for (const size_t bytes : perThreadBytes) {
        expectedBytes += bytes;
    }
    EXPECT_EQ(EHeap::AllocatedSize() - allocatedBefore, expectedBytes);
    std::vector<BlockRecord> all;
    all.reserve(THREADS * BLOCKS_PER_THREAD);
    for (const auto &local : perThread) {
        all.insert(all.end(), local.begin(), local.end());
    }
    EXPECT_EQ(all.size(), THREADS * BLOCKS_PER_THREAD);
    for (const auto &record : all) {
        EXPECT_TRUE(IsAligned(record.ptr));
        EXPECT_TRUE(PatternMatches(record.ptr, record.size, record.seed));
    }
    std::sort(all.begin(), all.end(),
              [](const BlockRecord &lhs, const BlockRecord &rhs) { return Addr(lhs.ptr) < Addr(rhs.ptr); });
    for (size_t index = 1; index < all.size(); ++index) {
        EXPECT_LE(Addr(all[index - 1U].ptr) + all[index - 1U].size, Addr(all[index].ptr));
    }
}
TEST_F(EHeapThreadingTest, SerializedAlloc_threads12_blocks16)
{
    constexpr size_t THREADS = 12U;
    constexpr size_t BLOCKS_PER_THREAD = 16U;
    std::vector<std::vector<BlockRecord>> perThread(THREADS);
    std::vector<size_t> perThreadBytes(THREADS, 0U);
    const size_t allocatedBefore = EHeap::AllocatedSize();
    RunThreads(THREADS, [&perThread, &perThreadBytes](size_t threadIndex) {
        auto &local = perThread[threadIndex];
        local.reserve(BLOCKS_PER_THREAD);
        size_t bytes = 0U;
        for (size_t iteration = 0; iteration < BLOCKS_PER_THREAD; ++iteration) {
            const size_t size = 1U + ((threadIndex * 131U + iteration * 37U) % 512U);
            const uint64_t seed = threadIndex * 1000U + iteration;
            HeapMutationGuard guard;
            void *block = EHeap::Alloc(size);
            if (block == nullptr) {
                return;
            }
            FillPattern(block, size, seed);
            local.push_back(BlockRecord {block, size, seed});
            bytes += AlignUpBytes(size);
        }
        perThreadBytes[threadIndex] = bytes;
    });
    size_t expectedBytes = 0U;
    for (const size_t bytes : perThreadBytes) {
        expectedBytes += bytes;
    }
    EXPECT_EQ(EHeap::AllocatedSize() - allocatedBefore, expectedBytes);
    std::vector<BlockRecord> all;
    all.reserve(THREADS * BLOCKS_PER_THREAD);
    for (const auto &local : perThread) {
        all.insert(all.end(), local.begin(), local.end());
    }
    EXPECT_EQ(all.size(), THREADS * BLOCKS_PER_THREAD);
    for (const auto &record : all) {
        EXPECT_TRUE(IsAligned(record.ptr));
        EXPECT_TRUE(PatternMatches(record.ptr, record.size, record.seed));
    }
    std::sort(all.begin(), all.end(),
              [](const BlockRecord &lhs, const BlockRecord &rhs) { return Addr(lhs.ptr) < Addr(rhs.ptr); });
    for (size_t index = 1; index < all.size(); ++index) {
        EXPECT_LE(Addr(all[index - 1U].ptr) + all[index - 1U].size, Addr(all[index].ptr));
    }
}
TEST_F(EHeapThreadingTest, SerializedAlloc_threads12_blocks48)
{
    constexpr size_t THREADS = 12U;
    constexpr size_t BLOCKS_PER_THREAD = 48U;
    std::vector<std::vector<BlockRecord>> perThread(THREADS);
    std::vector<size_t> perThreadBytes(THREADS, 0U);
    const size_t allocatedBefore = EHeap::AllocatedSize();
    RunThreads(THREADS, [&perThread, &perThreadBytes](size_t threadIndex) {
        auto &local = perThread[threadIndex];
        local.reserve(BLOCKS_PER_THREAD);
        size_t bytes = 0U;
        for (size_t iteration = 0; iteration < BLOCKS_PER_THREAD; ++iteration) {
            const size_t size = 1U + ((threadIndex * 131U + iteration * 37U) % 512U);
            const uint64_t seed = threadIndex * 1000U + iteration;
            HeapMutationGuard guard;
            void *block = EHeap::Alloc(size);
            if (block == nullptr) {
                return;
            }
            FillPattern(block, size, seed);
            local.push_back(BlockRecord {block, size, seed});
            bytes += AlignUpBytes(size);
        }
        perThreadBytes[threadIndex] = bytes;
    });
    size_t expectedBytes = 0U;
    for (const size_t bytes : perThreadBytes) {
        expectedBytes += bytes;
    }
    EXPECT_EQ(EHeap::AllocatedSize() - allocatedBefore, expectedBytes);
    std::vector<BlockRecord> all;
    all.reserve(THREADS * BLOCKS_PER_THREAD);
    for (const auto &local : perThread) {
        all.insert(all.end(), local.begin(), local.end());
    }
    EXPECT_EQ(all.size(), THREADS * BLOCKS_PER_THREAD);
    for (const auto &record : all) {
        EXPECT_TRUE(IsAligned(record.ptr));
        EXPECT_TRUE(PatternMatches(record.ptr, record.size, record.seed));
    }
    std::sort(all.begin(), all.end(),
              [](const BlockRecord &lhs, const BlockRecord &rhs) { return Addr(lhs.ptr) < Addr(rhs.ptr); });
    for (size_t index = 1; index < all.size(); ++index) {
        EXPECT_LE(Addr(all[index - 1U].ptr) + all[index - 1U].size, Addr(all[index].ptr));
    }
}
TEST_F(EHeapThreadingTest, SerializedAlloc_threads16_blocks16)
{
    constexpr size_t THREADS = 16U;
    constexpr size_t BLOCKS_PER_THREAD = 16U;
    std::vector<std::vector<BlockRecord>> perThread(THREADS);
    std::vector<size_t> perThreadBytes(THREADS, 0U);
    const size_t allocatedBefore = EHeap::AllocatedSize();
    RunThreads(THREADS, [&perThread, &perThreadBytes](size_t threadIndex) {
        auto &local = perThread[threadIndex];
        local.reserve(BLOCKS_PER_THREAD);
        size_t bytes = 0U;
        for (size_t iteration = 0; iteration < BLOCKS_PER_THREAD; ++iteration) {
            const size_t size = 1U + ((threadIndex * 131U + iteration * 37U) % 512U);
            const uint64_t seed = threadIndex * 1000U + iteration;
            HeapMutationGuard guard;
            void *block = EHeap::Alloc(size);
            if (block == nullptr) {
                return;
            }
            FillPattern(block, size, seed);
            local.push_back(BlockRecord {block, size, seed});
            bytes += AlignUpBytes(size);
        }
        perThreadBytes[threadIndex] = bytes;
    });
    size_t expectedBytes = 0U;
    for (const size_t bytes : perThreadBytes) {
        expectedBytes += bytes;
    }
    EXPECT_EQ(EHeap::AllocatedSize() - allocatedBefore, expectedBytes);
    std::vector<BlockRecord> all;
    all.reserve(THREADS * BLOCKS_PER_THREAD);
    for (const auto &local : perThread) {
        all.insert(all.end(), local.begin(), local.end());
    }
    EXPECT_EQ(all.size(), THREADS * BLOCKS_PER_THREAD);
    for (const auto &record : all) {
        EXPECT_TRUE(IsAligned(record.ptr));
        EXPECT_TRUE(PatternMatches(record.ptr, record.size, record.seed));
    }
    std::sort(all.begin(), all.end(),
              [](const BlockRecord &lhs, const BlockRecord &rhs) { return Addr(lhs.ptr) < Addr(rhs.ptr); });
    for (size_t index = 1; index < all.size(); ++index) {
        EXPECT_LE(Addr(all[index - 1U].ptr) + all[index - 1U].size, Addr(all[index].ptr));
    }
}
TEST_F(EHeapThreadingTest, SerializedAlloc_threads16_blocks48)
{
    constexpr size_t THREADS = 16U;
    constexpr size_t BLOCKS_PER_THREAD = 48U;
    std::vector<std::vector<BlockRecord>> perThread(THREADS);
    std::vector<size_t> perThreadBytes(THREADS, 0U);
    const size_t allocatedBefore = EHeap::AllocatedSize();
    RunThreads(THREADS, [&perThread, &perThreadBytes](size_t threadIndex) {
        auto &local = perThread[threadIndex];
        local.reserve(BLOCKS_PER_THREAD);
        size_t bytes = 0U;
        for (size_t iteration = 0; iteration < BLOCKS_PER_THREAD; ++iteration) {
            const size_t size = 1U + ((threadIndex * 131U + iteration * 37U) % 512U);
            const uint64_t seed = threadIndex * 1000U + iteration;
            HeapMutationGuard guard;
            void *block = EHeap::Alloc(size);
            if (block == nullptr) {
                return;
            }
            FillPattern(block, size, seed);
            local.push_back(BlockRecord {block, size, seed});
            bytes += AlignUpBytes(size);
        }
        perThreadBytes[threadIndex] = bytes;
    });
    size_t expectedBytes = 0U;
    for (const size_t bytes : perThreadBytes) {
        expectedBytes += bytes;
    }
    EXPECT_EQ(EHeap::AllocatedSize() - allocatedBefore, expectedBytes);
    std::vector<BlockRecord> all;
    all.reserve(THREADS * BLOCKS_PER_THREAD);
    for (const auto &local : perThread) {
        all.insert(all.end(), local.begin(), local.end());
    }
    EXPECT_EQ(all.size(), THREADS * BLOCKS_PER_THREAD);
    for (const auto &record : all) {
        EXPECT_TRUE(IsAligned(record.ptr));
        EXPECT_TRUE(PatternMatches(record.ptr, record.size, record.seed));
    }
    std::sort(all.begin(), all.end(),
              [](const BlockRecord &lhs, const BlockRecord &rhs) { return Addr(lhs.ptr) < Addr(rhs.ptr); });
    for (size_t index = 1; index < all.size(); ++index) {
        EXPECT_LE(Addr(all[index - 1U].ptr) + all[index - 1U].size, Addr(all[index].ptr));
    }
}
TEST_F(EHeapThreadingTest, SerializedFree_threads02)
{
    constexpr size_t THREADS = 2U;
    constexpr size_t BLOCKS_PER_THREAD = 32U;
    std::vector<BlockRecord> all;
    all.reserve(THREADS * BLOCKS_PER_THREAD);
    for (size_t index = 0; index < THREADS * BLOCKS_PER_THREAD; ++index) {
        const size_t size = 8U + (index % 64U);
        void *block = EHeap::Alloc(size);
        ASSERT_NE(block, nullptr);
        all.push_back(BlockRecord {block, size, index});
    }
    const size_t freedBefore = EHeap::FreedSize();
    std::vector<size_t> perThreadFreed(THREADS, 0U);
    RunThreads(THREADS, [&all, &perThreadFreed](size_t threadIndex) {
        size_t freed = 0U;
        for (size_t index = threadIndex; index < all.size(); index += THREADS) {
            HeapMutationGuard guard;
            EHeap::Free(all[index].ptr, all[index].size);
            freed += ExpectedFreedSize(all[index].size);
        }
        perThreadFreed[threadIndex] = freed;
    });
    size_t expectedFreed = 0U;
    for (const size_t freed : perThreadFreed) {
        expectedFreed += freed;
    }
    EXPECT_EQ(EHeap::FreedSize() - freedBefore, expectedFreed);
    EXPECT_EQ(all.size(), THREADS * BLOCKS_PER_THREAD);
}
TEST_F(EHeapThreadingTest, SerializedFree_threads04)
{
    constexpr size_t THREADS = 4U;
    constexpr size_t BLOCKS_PER_THREAD = 32U;
    std::vector<BlockRecord> all;
    all.reserve(THREADS * BLOCKS_PER_THREAD);
    for (size_t index = 0; index < THREADS * BLOCKS_PER_THREAD; ++index) {
        const size_t size = 8U + (index % 64U);
        void *block = EHeap::Alloc(size);
        ASSERT_NE(block, nullptr);
        all.push_back(BlockRecord {block, size, index});
    }
    const size_t freedBefore = EHeap::FreedSize();
    std::vector<size_t> perThreadFreed(THREADS, 0U);
    RunThreads(THREADS, [&all, &perThreadFreed](size_t threadIndex) {
        size_t freed = 0U;
        for (size_t index = threadIndex; index < all.size(); index += THREADS) {
            HeapMutationGuard guard;
            EHeap::Free(all[index].ptr, all[index].size);
            freed += ExpectedFreedSize(all[index].size);
        }
        perThreadFreed[threadIndex] = freed;
    });
    size_t expectedFreed = 0U;
    for (const size_t freed : perThreadFreed) {
        expectedFreed += freed;
    }
    EXPECT_EQ(EHeap::FreedSize() - freedBefore, expectedFreed);
    EXPECT_EQ(all.size(), THREADS * BLOCKS_PER_THREAD);
}
TEST_F(EHeapThreadingTest, SerializedFree_threads08)
{
    constexpr size_t THREADS = 8U;
    constexpr size_t BLOCKS_PER_THREAD = 32U;
    std::vector<BlockRecord> all;
    all.reserve(THREADS * BLOCKS_PER_THREAD);
    for (size_t index = 0; index < THREADS * BLOCKS_PER_THREAD; ++index) {
        const size_t size = 8U + (index % 64U);
        void *block = EHeap::Alloc(size);
        ASSERT_NE(block, nullptr);
        all.push_back(BlockRecord {block, size, index});
    }
    const size_t freedBefore = EHeap::FreedSize();
    std::vector<size_t> perThreadFreed(THREADS, 0U);
    RunThreads(THREADS, [&all, &perThreadFreed](size_t threadIndex) {
        size_t freed = 0U;
        for (size_t index = threadIndex; index < all.size(); index += THREADS) {
            HeapMutationGuard guard;
            EHeap::Free(all[index].ptr, all[index].size);
            freed += ExpectedFreedSize(all[index].size);
        }
        perThreadFreed[threadIndex] = freed;
    });
    size_t expectedFreed = 0U;
    for (const size_t freed : perThreadFreed) {
        expectedFreed += freed;
    }
    EXPECT_EQ(EHeap::FreedSize() - freedBefore, expectedFreed);
    EXPECT_EQ(all.size(), THREADS * BLOCKS_PER_THREAD);
}
TEST_F(EHeapThreadingTest, SerializedFree_threads16)
{
    constexpr size_t THREADS = 16U;
    constexpr size_t BLOCKS_PER_THREAD = 32U;
    std::vector<BlockRecord> all;
    all.reserve(THREADS * BLOCKS_PER_THREAD);
    for (size_t index = 0; index < THREADS * BLOCKS_PER_THREAD; ++index) {
        const size_t size = 8U + (index % 64U);
        void *block = EHeap::Alloc(size);
        ASSERT_NE(block, nullptr);
        all.push_back(BlockRecord {block, size, index});
    }
    const size_t freedBefore = EHeap::FreedSize();
    std::vector<size_t> perThreadFreed(THREADS, 0U);
    RunThreads(THREADS, [&all, &perThreadFreed](size_t threadIndex) {
        size_t freed = 0U;
        for (size_t index = threadIndex; index < all.size(); index += THREADS) {
            HeapMutationGuard guard;
            EHeap::Free(all[index].ptr, all[index].size);
            freed += ExpectedFreedSize(all[index].size);
        }
        perThreadFreed[threadIndex] = freed;
    });
    size_t expectedFreed = 0U;
    for (const size_t freed : perThreadFreed) {
        expectedFreed += freed;
    }
    EXPECT_EQ(EHeap::FreedSize() - freedBefore, expectedFreed);
    EXPECT_EQ(all.size(), THREADS * BLOCKS_PER_THREAD);
}
TEST_F(EHeapThreadingTest, ParallelReadVerify_threads01)
{
    constexpr size_t THREADS = 1U;
    constexpr size_t BLOCKS = 64U;
    std::array<BlockRecord, BLOCKS> records {};
    for (size_t index = 0; index < BLOCKS; ++index) {
        const size_t size = 16U + (index * 13U) % 256U;
        void *block = EHeap::Alloc(size);
        ASSERT_NE(block, nullptr);
        FillPattern(block, size, index + 7U);
        records[index] = BlockRecord {block, size, index + 7U};
    }
    std::atomic<size_t> mismatches {0U};
    RunThreads(THREADS, [&records, &mismatches](size_t threadIndex) {
        const size_t chunk = BLOCKS / THREADS;
        const size_t begin = threadIndex * chunk;
        const size_t end = begin + chunk;
        for (size_t index = begin; index < end; ++index) {
            if (!PatternMatches(records[index].ptr, records[index].size, records[index].seed)) {
                // Atomic with relaxed order reason: worker counter read after every worker joined
                mismatches.fetch_add(1U, std::memory_order_relaxed);
            }
        }
    });
    // Atomic with relaxed order reason: read after every worker joined
    EXPECT_EQ(mismatches.load(std::memory_order_relaxed), 0U);
}
TEST_F(EHeapThreadingTest, ParallelReadVerify_threads02)
{
    constexpr size_t THREADS = 2U;
    constexpr size_t BLOCKS = 64U;
    std::array<BlockRecord, BLOCKS> records {};
    for (size_t index = 0; index < BLOCKS; ++index) {
        const size_t size = 16U + (index * 13U) % 256U;
        void *block = EHeap::Alloc(size);
        ASSERT_NE(block, nullptr);
        FillPattern(block, size, index + 7U);
        records[index] = BlockRecord {block, size, index + 7U};
    }
    std::atomic<size_t> mismatches {0U};
    RunThreads(THREADS, [&records, &mismatches](size_t threadIndex) {
        const size_t chunk = BLOCKS / THREADS;
        const size_t begin = threadIndex * chunk;
        const size_t end = begin + chunk;
        for (size_t index = begin; index < end; ++index) {
            if (!PatternMatches(records[index].ptr, records[index].size, records[index].seed)) {
                // Atomic with relaxed order reason: worker counter read after every worker joined
                mismatches.fetch_add(1U, std::memory_order_relaxed);
            }
        }
    });
    // Atomic with relaxed order reason: read after every worker joined
    EXPECT_EQ(mismatches.load(std::memory_order_relaxed), 0U);
}
TEST_F(EHeapThreadingTest, ParallelReadVerify_threads04)
{
    constexpr size_t THREADS = 4U;
    constexpr size_t BLOCKS = 64U;
    std::array<BlockRecord, BLOCKS> records {};
    for (size_t index = 0; index < BLOCKS; ++index) {
        const size_t size = 16U + (index * 13U) % 256U;
        void *block = EHeap::Alloc(size);
        ASSERT_NE(block, nullptr);
        FillPattern(block, size, index + 7U);
        records[index] = BlockRecord {block, size, index + 7U};
    }
    std::atomic<size_t> mismatches {0U};
    RunThreads(THREADS, [&records, &mismatches](size_t threadIndex) {
        const size_t chunk = BLOCKS / THREADS;
        const size_t begin = threadIndex * chunk;
        const size_t end = begin + chunk;
        for (size_t index = begin; index < end; ++index) {
            if (!PatternMatches(records[index].ptr, records[index].size, records[index].seed)) {
                // Atomic with relaxed order reason: worker counter read after every worker joined
                mismatches.fetch_add(1U, std::memory_order_relaxed);
            }
        }
    });
    // Atomic with relaxed order reason: read after every worker joined
    EXPECT_EQ(mismatches.load(std::memory_order_relaxed), 0U);
}
TEST_F(EHeapThreadingTest, ParallelReadVerify_threads08)
{
    constexpr size_t THREADS = 8U;
    constexpr size_t BLOCKS = 64U;
    std::array<BlockRecord, BLOCKS> records {};
    for (size_t index = 0; index < BLOCKS; ++index) {
        const size_t size = 16U + (index * 13U) % 256U;
        void *block = EHeap::Alloc(size);
        ASSERT_NE(block, nullptr);
        FillPattern(block, size, index + 7U);
        records[index] = BlockRecord {block, size, index + 7U};
    }
    std::atomic<size_t> mismatches {0U};
    RunThreads(THREADS, [&records, &mismatches](size_t threadIndex) {
        const size_t chunk = BLOCKS / THREADS;
        const size_t begin = threadIndex * chunk;
        const size_t end = begin + chunk;
        for (size_t index = begin; index < end; ++index) {
            if (!PatternMatches(records[index].ptr, records[index].size, records[index].seed)) {
                // Atomic with relaxed order reason: worker counter read after every worker joined
                mismatches.fetch_add(1U, std::memory_order_relaxed);
            }
        }
    });
    // Atomic with relaxed order reason: read after every worker joined
    EXPECT_EQ(mismatches.load(std::memory_order_relaxed), 0U);
}
TEST_F(EHeapThreadingTest, ParallelPointerDereference_threads01)
{
    constexpr size_t THREADS = 1U;
    constexpr size_t VALUES = 256U;
    std::vector<EPtr<int>> pointers;
    pointers.reserve(VALUES);
    for (size_t index = 0; index < VALUES; ++index) {
        auto *value = EAllocator::New<int>(static_cast<int>(index));
        ASSERT_NE(value, nullptr);
        pointers.emplace_back(value);
    }
    std::atomic<long long> total {0};
    RunThreads(THREADS, [&pointers, &total](size_t threadIndex) {
        long long local = 0;
        for (size_t index = threadIndex; index < VALUES; index += THREADS) {
            local += *pointers[index];
        }
        // Atomic with relaxed order reason: worker counter read after every worker joined
        total.fetch_add(local, std::memory_order_relaxed);
    });
    // Atomic with relaxed order reason: read after every worker joined
    EXPECT_EQ(total.load(std::memory_order_relaxed), static_cast<long long>(VALUES * (VALUES - 1U) / 2U));
}
TEST_F(EHeapThreadingTest, ParallelPointerDereference_threads02)
{
    constexpr size_t THREADS = 2U;
    constexpr size_t VALUES = 256U;
    std::vector<EPtr<int>> pointers;
    pointers.reserve(VALUES);
    for (size_t index = 0; index < VALUES; ++index) {
        auto *value = EAllocator::New<int>(static_cast<int>(index));
        ASSERT_NE(value, nullptr);
        pointers.emplace_back(value);
    }
    std::atomic<long long> total {0};
    RunThreads(THREADS, [&pointers, &total](size_t threadIndex) {
        long long local = 0;
        for (size_t index = threadIndex; index < VALUES; index += THREADS) {
            local += *pointers[index];
        }
        // Atomic with relaxed order reason: worker counter read after every worker joined
        total.fetch_add(local, std::memory_order_relaxed);
    });
    // Atomic with relaxed order reason: read after every worker joined
    EXPECT_EQ(total.load(std::memory_order_relaxed), static_cast<long long>(VALUES * (VALUES - 1U) / 2U));
}
TEST_F(EHeapThreadingTest, ParallelPointerDereference_threads04)
{
    constexpr size_t THREADS = 4U;
    constexpr size_t VALUES = 256U;
    std::vector<EPtr<int>> pointers;
    pointers.reserve(VALUES);
    for (size_t index = 0; index < VALUES; ++index) {
        auto *value = EAllocator::New<int>(static_cast<int>(index));
        ASSERT_NE(value, nullptr);
        pointers.emplace_back(value);
    }
    std::atomic<long long> total {0};
    RunThreads(THREADS, [&pointers, &total](size_t threadIndex) {
        long long local = 0;
        for (size_t index = threadIndex; index < VALUES; index += THREADS) {
            local += *pointers[index];
        }
        // Atomic with relaxed order reason: worker counter read after every worker joined
        total.fetch_add(local, std::memory_order_relaxed);
    });
    // Atomic with relaxed order reason: read after every worker joined
    EXPECT_EQ(total.load(std::memory_order_relaxed), static_cast<long long>(VALUES * (VALUES - 1U) / 2U));
}
TEST_F(EHeapThreadingTest, ParallelPointerDereference_threads08)
{
    constexpr size_t THREADS = 8U;
    constexpr size_t VALUES = 256U;
    std::vector<EPtr<int>> pointers;
    pointers.reserve(VALUES);
    for (size_t index = 0; index < VALUES; ++index) {
        auto *value = EAllocator::New<int>(static_cast<int>(index));
        ASSERT_NE(value, nullptr);
        pointers.emplace_back(value);
    }
    std::atomic<long long> total {0};
    RunThreads(THREADS, [&pointers, &total](size_t threadIndex) {
        long long local = 0;
        for (size_t index = threadIndex; index < VALUES; index += THREADS) {
            local += *pointers[index];
        }
        // Atomic with relaxed order reason: worker counter read after every worker joined
        total.fetch_add(local, std::memory_order_relaxed);
    });
    // Atomic with relaxed order reason: read after every worker joined
    EXPECT_EQ(total.load(std::memory_order_relaxed), static_cast<long long>(VALUES * (VALUES - 1U) / 2U));
}
TEST_F(EHeapThreadingTest, PerThreadVector_threads02)
{
    constexpr size_t THREADS = 2U;
    constexpr size_t ELEMENTS = 64U;
    std::vector<int> sums(THREADS, 0);
    RunThreads(THREADS, [&sums](size_t threadIndex) {
        HeapMutationGuard guard;
        ArenaVector<int> values;
        values.reserve(ELEMENTS);
        for (size_t index = 0; index < ELEMENTS; ++index) {
            values.push_back(static_cast<int>(threadIndex) + static_cast<int>(index));
        }
        int sum = 0;
        for (const int value : values) {
            sum += value;
        }
        sums[threadIndex] = sum;
    });
    int expectedSumBase = 0;
    for (int index = 0; index < static_cast<int>(ELEMENTS); ++index) {
        expectedSumBase += index;
    }
    for (size_t threadIndex = 0; threadIndex < THREADS; ++threadIndex) {
        EXPECT_EQ(sums[threadIndex], expectedSumBase + static_cast<int>(threadIndex * ELEMENTS));
    }
}
TEST_F(EHeapThreadingTest, PerThreadVector_threads04)
{
    constexpr size_t THREADS = 4U;
    constexpr size_t ELEMENTS = 64U;
    std::vector<int> sums(THREADS, 0);
    RunThreads(THREADS, [&sums](size_t threadIndex) {
        HeapMutationGuard guard;
        ArenaVector<int> values;
        values.reserve(ELEMENTS);
        for (size_t index = 0; index < ELEMENTS; ++index) {
            values.push_back(static_cast<int>(threadIndex) + static_cast<int>(index));
        }
        int sum = 0;
        for (const int value : values) {
            sum += value;
        }
        sums[threadIndex] = sum;
    });
    int expectedSumBase = 0;
    for (int index = 0; index < static_cast<int>(ELEMENTS); ++index) {
        expectedSumBase += index;
    }
    for (size_t threadIndex = 0; threadIndex < THREADS; ++threadIndex) {
        EXPECT_EQ(sums[threadIndex], expectedSumBase + static_cast<int>(threadIndex * ELEMENTS));
    }
}
TEST_F(EHeapThreadingTest, PerThreadVector_threads08)
{
    constexpr size_t THREADS = 8U;
    constexpr size_t ELEMENTS = 64U;
    std::vector<int> sums(THREADS, 0);
    RunThreads(THREADS, [&sums](size_t threadIndex) {
        HeapMutationGuard guard;
        ArenaVector<int> values;
        values.reserve(ELEMENTS);
        for (size_t index = 0; index < ELEMENTS; ++index) {
            values.push_back(static_cast<int>(threadIndex) + static_cast<int>(index));
        }
        int sum = 0;
        for (const int value : values) {
            sum += value;
        }
        sums[threadIndex] = sum;
    });
    int expectedSumBase = 0;
    for (int index = 0; index < static_cast<int>(ELEMENTS); ++index) {
        expectedSumBase += index;
    }
    for (size_t threadIndex = 0; threadIndex < THREADS; ++threadIndex) {
        EXPECT_EQ(sums[threadIndex], expectedSumBase + static_cast<int>(threadIndex * ELEMENTS));
    }
}
TEST_F(EHeapThreadingTest, PerThreadMap_threads02)
{
    constexpr size_t THREADS = 2U;
    constexpr size_t ENTRIES = 48U;
    std::vector<size_t> sizes(THREADS, 0U);
    RunThreads(THREADS, [&sizes](size_t threadIndex) {
        HeapMutationGuard guard;
        ArenaMap<int, int> values;
        for (size_t index = 0; index < ENTRIES; ++index) {
            values[static_cast<int>(index)] = static_cast<int>(threadIndex * 100U + index);
        }
        size_t matched = 0U;
        for (size_t index = 0; index < ENTRIES; ++index) {
            if (values.at(static_cast<int>(index)) == static_cast<int>(threadIndex * 100U + index)) {
                ++matched;
            }
        }
        sizes[threadIndex] = matched;
    });
    for (size_t threadIndex = 0; threadIndex < THREADS; ++threadIndex) {
        EXPECT_EQ(sizes[threadIndex], ENTRIES);
    }
}
TEST_F(EHeapThreadingTest, PerThreadMap_threads04)
{
    constexpr size_t THREADS = 4U;
    constexpr size_t ENTRIES = 48U;
    std::vector<size_t> sizes(THREADS, 0U);
    RunThreads(THREADS, [&sizes](size_t threadIndex) {
        HeapMutationGuard guard;
        ArenaMap<int, int> values;
        for (size_t index = 0; index < ENTRIES; ++index) {
            values[static_cast<int>(index)] = static_cast<int>(threadIndex * 100U + index);
        }
        size_t matched = 0U;
        for (size_t index = 0; index < ENTRIES; ++index) {
            if (values.at(static_cast<int>(index)) == static_cast<int>(threadIndex * 100U + index)) {
                ++matched;
            }
        }
        sizes[threadIndex] = matched;
    });
    for (size_t threadIndex = 0; threadIndex < THREADS; ++threadIndex) {
        EXPECT_EQ(sizes[threadIndex], ENTRIES);
    }
}
TEST_F(EHeapThreadingTest, PerThreadMap_threads08)
{
    constexpr size_t THREADS = 8U;
    constexpr size_t ENTRIES = 48U;
    std::vector<size_t> sizes(THREADS, 0U);
    RunThreads(THREADS, [&sizes](size_t threadIndex) {
        HeapMutationGuard guard;
        ArenaMap<int, int> values;
        for (size_t index = 0; index < ENTRIES; ++index) {
            values[static_cast<int>(index)] = static_cast<int>(threadIndex * 100U + index);
        }
        size_t matched = 0U;
        for (size_t index = 0; index < ENTRIES; ++index) {
            if (values.at(static_cast<int>(index)) == static_cast<int>(threadIndex * 100U + index)) {
                ++matched;
            }
        }
        sizes[threadIndex] = matched;
    });
    for (size_t threadIndex = 0; threadIndex < THREADS; ++threadIndex) {
        EXPECT_EQ(sizes[threadIndex], ENTRIES);
    }
}
TEST_F(EHeapThreadingTest, BarrierPhasedAlloc_threads02)
{
    constexpr size_t THREADS = 2U;
    SpinBarrier barrier {static_cast<uint32_t>(THREADS)};
    std::vector<BlockRecord> records(THREADS);
    RunThreads(THREADS, [&barrier, &records](size_t threadIndex) {
        {
            HeapMutationGuard guard;
            const size_t size = 32U + threadIndex * 8U;
            void *block = EHeap::Alloc(size);
            if (block == nullptr) {
                return;
            }
            FillPattern(block, size, threadIndex + 1U);
            records[threadIndex] = BlockRecord {block, size, threadIndex + 1U};
        }
        barrier.Wait();
        for (size_t other = 0; other < THREADS; ++other) {
            EXPECT_TRUE(PatternMatches(records[other].ptr, records[other].size, records[other].seed));
        }
        barrier.Wait();
        {
            HeapMutationGuard guard;
            EHeap::Free(records[threadIndex].ptr, records[threadIndex].size);
        }
    });
    for (const auto &record : records) {
        EXPECT_TRUE(IsAligned(record.ptr));
    }
}
TEST_F(EHeapThreadingTest, BarrierPhasedAlloc_threads03)
{
    constexpr size_t THREADS = 3U;
    SpinBarrier barrier {static_cast<uint32_t>(THREADS)};
    std::vector<BlockRecord> records(THREADS);
    RunThreads(THREADS, [&barrier, &records](size_t threadIndex) {
        {
            HeapMutationGuard guard;
            const size_t size = 32U + threadIndex * 8U;
            void *block = EHeap::Alloc(size);
            if (block == nullptr) {
                return;
            }
            FillPattern(block, size, threadIndex + 1U);
            records[threadIndex] = BlockRecord {block, size, threadIndex + 1U};
        }
        barrier.Wait();
        for (size_t other = 0; other < THREADS; ++other) {
            EXPECT_TRUE(PatternMatches(records[other].ptr, records[other].size, records[other].seed));
        }
        barrier.Wait();
        {
            HeapMutationGuard guard;
            EHeap::Free(records[threadIndex].ptr, records[threadIndex].size);
        }
    });
    for (const auto &record : records) {
        EXPECT_TRUE(IsAligned(record.ptr));
    }
}
TEST_F(EHeapThreadingTest, BarrierPhasedAlloc_threads04)
{
    constexpr size_t THREADS = 4U;
    SpinBarrier barrier {static_cast<uint32_t>(THREADS)};
    std::vector<BlockRecord> records(THREADS);
    RunThreads(THREADS, [&barrier, &records](size_t threadIndex) {
        {
            HeapMutationGuard guard;
            const size_t size = 32U + threadIndex * 8U;
            void *block = EHeap::Alloc(size);
            if (block == nullptr) {
                return;
            }
            FillPattern(block, size, threadIndex + 1U);
            records[threadIndex] = BlockRecord {block, size, threadIndex + 1U};
        }
        barrier.Wait();
        for (size_t other = 0; other < THREADS; ++other) {
            EXPECT_TRUE(PatternMatches(records[other].ptr, records[other].size, records[other].seed));
        }
        barrier.Wait();
        {
            HeapMutationGuard guard;
            EHeap::Free(records[threadIndex].ptr, records[threadIndex].size);
        }
    });
    for (const auto &record : records) {
        EXPECT_TRUE(IsAligned(record.ptr));
    }
}
TEST_F(EHeapThreadingTest, BarrierPhasedAlloc_threads08)
{
    constexpr size_t THREADS = 8U;
    SpinBarrier barrier {static_cast<uint32_t>(THREADS)};
    std::vector<BlockRecord> records(THREADS);
    RunThreads(THREADS, [&barrier, &records](size_t threadIndex) {
        {
            HeapMutationGuard guard;
            const size_t size = 32U + threadIndex * 8U;
            void *block = EHeap::Alloc(size);
            if (block == nullptr) {
                return;
            }
            FillPattern(block, size, threadIndex + 1U);
            records[threadIndex] = BlockRecord {block, size, threadIndex + 1U};
        }
        barrier.Wait();
        for (size_t other = 0; other < THREADS; ++other) {
            EXPECT_TRUE(PatternMatches(records[other].ptr, records[other].size, records[other].seed));
        }
        barrier.Wait();
        {
            HeapMutationGuard guard;
            EHeap::Free(records[threadIndex].ptr, records[threadIndex].size);
        }
    });
    for (const auto &record : records) {
        EXPECT_TRUE(IsAligned(record.ptr));
    }
}
TEST_F(EHeapThreadingTest, ProducerConsumerBlocks)
{
    constexpr size_t PRODUCED = 128U;
    std::mutex queueMutex;
    std::vector<BlockRecord> queue;
    std::atomic<bool> producerDone {false};
    std::atomic<size_t> verified {0U};
    std::thread producer {[&queue, &queueMutex, &producerDone]() {
        for (size_t index = 0; index < PRODUCED; ++index) {
            BlockRecord record {};
            {
                HeapMutationGuard guard;
                const size_t size = 24U + (index * 11U) % 200U;
                void *block = EHeap::Alloc(size);
                if (block == nullptr) {
                    return;
                }
                FillPattern(block, size, index);
                record = BlockRecord {block, size, index};
            }
            std::lock_guard<std::mutex> lock(queueMutex);
            queue.push_back(record);
        }
        // Atomic with release order reason: publish producer completion to consumers
        producerDone.store(true, std::memory_order_release);
    }};
    std::thread consumer {[&queue, &queueMutex, &producerDone, &verified]() {
        size_t consumed = 0U;
        while (true) {
            BlockRecord record {};
            bool hasRecord = false;
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                if (!queue.empty()) {
                    record = queue.back();
                    queue.pop_back();
                    hasRecord = true;
                }
            }
            if (hasRecord) {
                if (PatternMatches(record.ptr, record.size, record.seed)) {
                    ++consumed;
                }
                continue;
            }
            // Atomic with acquire order reason: producer completion is observed
            if (producerDone.load(std::memory_order_acquire)) {
                break;
            }
            std::this_thread::yield();
        }
        // Atomic with release order reason: publish the verified count before join
        verified.store(consumed, std::memory_order_release);
    }};
    producer.join();
    consumer.join();
    // Atomic with acquire order reason: verified count is observed after join
    EXPECT_EQ(verified.load(std::memory_order_acquire), PRODUCED);
}
TEST_F(EHeapThreadingTest, ProducerConsumerWithManyConsumers)
{
    constexpr size_t PRODUCED = 192U;
    constexpr size_t CONSUMERS = 3U;
    std::mutex queueMutex;
    std::vector<BlockRecord> queue;
    std::atomic<bool> producerDone {false};
    std::atomic<size_t> consumed {0U};
    std::vector<std::thread> consumers;
    consumers.reserve(CONSUMERS);
    for (size_t index = 0; index < CONSUMERS; ++index) {
        consumers.emplace_back([&queue, &queueMutex, &producerDone, &consumed]() {
            while (true) {
                BlockRecord record {};
                bool hasRecord = false;
                {
                    std::lock_guard<std::mutex> lock(queueMutex);
                    if (!queue.empty()) {
                        record = queue.back();
                        queue.pop_back();
                        hasRecord = true;
                    }
                }
                if (hasRecord) {
                    if (PatternMatches(record.ptr, record.size, record.seed)) {
                        // Atomic with relaxed order reason: consumer counter read after join
                        consumed.fetch_add(1U, std::memory_order_relaxed);
                    }
                    continue;
                }
                // Atomic with acquire order reason: producer completion is observed
                if (producerDone.load(std::memory_order_acquire)) {
                    return;
                }
                std::this_thread::yield();
            }
        });
    }
    std::thread producer {[&queue, &queueMutex, &producerDone]() {
        for (size_t index = 0; index < PRODUCED; ++index) {
            BlockRecord record {};
            {
                HeapMutationGuard guard;
                const size_t size = 8U + (index * 7U) % 128U;
                void *block = EHeap::Alloc(size);
                if (block == nullptr) {
                    return;
                }
                FillPattern(block, size, index + 3U);
                record = BlockRecord {block, size, index + 3U};
            }
            std::lock_guard<std::mutex> lock(queueMutex);
            queue.push_back(record);
        }
        // Atomic with release order reason: publish producer completion to consumers
        producerDone.store(true, std::memory_order_release);
    }};
    producer.join();
    for (auto &consumer : consumers) {
        consumer.join();
    }
    // Atomic with acquire order reason: consumed count is observed after join
    EXPECT_EQ(consumed.load(std::memory_order_acquire), PRODUCED);
}
TEST_F(EHeapThreadingTest, NestedScopesInWorkers_threads02)
{
    constexpr size_t THREADS = 2U;
    void *mainBlock = EHeap::Alloc(256U);
    ASSERT_NE(mainBlock, nullptr);
    FillPattern(mainBlock, 256U, 0xDEADU);
    const size_t allocatedBefore = EHeap::AllocatedSize();
    std::atomic<size_t> workerBlocks {0U};
    RunThreads(THREADS, [&workerBlocks](size_t threadIndex) {
        EHeap::Scope workerScope;
        {
            HeapMutationGuard guard;
            const size_t size = 64U + threadIndex * 16U;
            void *block = EHeap::Alloc(size);
            if (block != nullptr) {
                FillByte(block, size, static_cast<uint8_t>(threadIndex));
                // Atomic with relaxed order reason: worker counter read after every worker joined
                workerBlocks.fetch_add(1U, std::memory_order_relaxed);
            }
        }
    });
    // Atomic with relaxed order reason: read after every worker joined
    EXPECT_EQ(workerBlocks.load(std::memory_order_relaxed), THREADS);
    size_t expectedBytes = 0U;
    for (size_t threadIndex = 0; threadIndex < THREADS; ++threadIndex) {
        expectedBytes += AlignUpBytes(64U + threadIndex * 16U);
    }
    EXPECT_EQ(EHeap::AllocatedSize() - allocatedBefore, expectedBytes);
    EXPECT_TRUE(PatternMatches(mainBlock, 256U, 0xDEADU));
}
TEST_F(EHeapThreadingTest, NestedScopesInWorkers_threads04)
{
    constexpr size_t THREADS = 4U;
    void *mainBlock = EHeap::Alloc(256U);
    ASSERT_NE(mainBlock, nullptr);
    FillPattern(mainBlock, 256U, 0xDEADU);
    const size_t allocatedBefore = EHeap::AllocatedSize();
    std::atomic<size_t> workerBlocks {0U};
    RunThreads(THREADS, [&workerBlocks](size_t threadIndex) {
        EHeap::Scope workerScope;
        {
            HeapMutationGuard guard;
            const size_t size = 64U + threadIndex * 16U;
            void *block = EHeap::Alloc(size);
            if (block != nullptr) {
                FillByte(block, size, static_cast<uint8_t>(threadIndex));
                // Atomic with relaxed order reason: worker counter read after every worker joined
                workerBlocks.fetch_add(1U, std::memory_order_relaxed);
            }
        }
    });
    // Atomic with relaxed order reason: read after every worker joined
    EXPECT_EQ(workerBlocks.load(std::memory_order_relaxed), THREADS);
    size_t expectedBytes = 0U;
    for (size_t threadIndex = 0; threadIndex < THREADS; ++threadIndex) {
        expectedBytes += AlignUpBytes(64U + threadIndex * 16U);
    }
    EXPECT_EQ(EHeap::AllocatedSize() - allocatedBefore, expectedBytes);
    EXPECT_TRUE(PatternMatches(mainBlock, 256U, 0xDEADU));
}
TEST_F(EHeapThreadingTest, NestedScopesInWorkers_threads08)
{
    constexpr size_t THREADS = 8U;
    void *mainBlock = EHeap::Alloc(256U);
    ASSERT_NE(mainBlock, nullptr);
    FillPattern(mainBlock, 256U, 0xDEADU);
    const size_t allocatedBefore = EHeap::AllocatedSize();
    std::atomic<size_t> workerBlocks {0U};
    RunThreads(THREADS, [&workerBlocks](size_t threadIndex) {
        EHeap::Scope workerScope;
        {
            HeapMutationGuard guard;
            const size_t size = 64U + threadIndex * 16U;
            void *block = EHeap::Alloc(size);
            if (block != nullptr) {
                FillByte(block, size, static_cast<uint8_t>(threadIndex));
                // Atomic with relaxed order reason: worker counter read after every worker joined
                workerBlocks.fetch_add(1U, std::memory_order_relaxed);
            }
        }
    });
    // Atomic with relaxed order reason: read after every worker joined
    EXPECT_EQ(workerBlocks.load(std::memory_order_relaxed), THREADS);
    size_t expectedBytes = 0U;
    for (size_t threadIndex = 0; threadIndex < THREADS; ++threadIndex) {
        expectedBytes += AlignUpBytes(64U + threadIndex * 16U);
    }
    EXPECT_EQ(EHeap::AllocatedSize() - allocatedBefore, expectedBytes);
    EXPECT_TRUE(PatternMatches(mainBlock, 256U, 0xDEADU));
}
TEST_F(EHeapThreadingTest, InterleavedAllocFree_threads02)
{
    constexpr size_t THREADS = 2U;
    constexpr size_t ROUNDS = 32U;
    const size_t freedBefore = EHeap::FreedSize();
    std::vector<size_t> perThreadFreed(THREADS, 0U);
    RunThreads(THREADS, [&perThreadFreed](size_t threadIndex) {
        size_t freed = 0U;
        for (size_t round = 0; round < ROUNDS; ++round) {
            const size_t size = 16U + ((threadIndex * 17U + round * 5U) % 96U);
            HeapMutationGuard guard;
            void *block = EHeap::Alloc(size);
            if (block == nullptr) {
                return;
            }
            FillPattern(block, size, round);
            EHeap::Free(block, size);
            freed += ExpectedFreedSize(size);
        }
        perThreadFreed[threadIndex] = freed;
    });
    size_t expectedFreed = 0U;
    for (const size_t freed : perThreadFreed) {
        expectedFreed += freed;
    }
    EXPECT_EQ(EHeap::FreedSize() - freedBefore, expectedFreed);
}
TEST_F(EHeapThreadingTest, InterleavedAllocFree_threads04)
{
    constexpr size_t THREADS = 4U;
    constexpr size_t ROUNDS = 32U;
    const size_t freedBefore = EHeap::FreedSize();
    std::vector<size_t> perThreadFreed(THREADS, 0U);
    RunThreads(THREADS, [&perThreadFreed](size_t threadIndex) {
        size_t freed = 0U;
        for (size_t round = 0; round < ROUNDS; ++round) {
            const size_t size = 16U + ((threadIndex * 17U + round * 5U) % 96U);
            HeapMutationGuard guard;
            void *block = EHeap::Alloc(size);
            if (block == nullptr) {
                return;
            }
            FillPattern(block, size, round);
            EHeap::Free(block, size);
            freed += ExpectedFreedSize(size);
        }
        perThreadFreed[threadIndex] = freed;
    });
    size_t expectedFreed = 0U;
    for (const size_t freed : perThreadFreed) {
        expectedFreed += freed;
    }
    EXPECT_EQ(EHeap::FreedSize() - freedBefore, expectedFreed);
}
TEST_F(EHeapThreadingTest, InterleavedAllocFree_threads08)
{
    constexpr size_t THREADS = 8U;
    constexpr size_t ROUNDS = 32U;
    const size_t freedBefore = EHeap::FreedSize();
    std::vector<size_t> perThreadFreed(THREADS, 0U);
    RunThreads(THREADS, [&perThreadFreed](size_t threadIndex) {
        size_t freed = 0U;
        for (size_t round = 0; round < ROUNDS; ++round) {
            const size_t size = 16U + ((threadIndex * 17U + round * 5U) % 96U);
            HeapMutationGuard guard;
            void *block = EHeap::Alloc(size);
            if (block == nullptr) {
                return;
            }
            FillPattern(block, size, round);
            EHeap::Free(block, size);
            freed += ExpectedFreedSize(size);
        }
        perThreadFreed[threadIndex] = freed;
    });
    size_t expectedFreed = 0U;
    for (const size_t freed : perThreadFreed) {
        expectedFreed += freed;
    }
    EXPECT_EQ(EHeap::FreedSize() - freedBefore, expectedFreed);
}
TEST_F(EHeapThreadingTest, ThreadChurn_batches02)
{
    constexpr size_t BATCHES = 2U;
    constexpr size_t THREADS_PER_BATCH = 8U;
    std::atomic<size_t> totalBlocks {0U};
    for (size_t batch = 0; batch < BATCHES; ++batch) {
        RunThreads(THREADS_PER_BATCH, [&totalBlocks](size_t threadIndex) {
            HeapMutationGuard guard;
            for (size_t index = 0; index < 4U; ++index) {
                const size_t size = 32U + threadIndex * 4U + index;
                void *block = EHeap::Alloc(size);
                if (block == nullptr) {
                    return;
                }
                FillPattern(block, size, threadIndex * 10U + index);
                // Atomic with relaxed order reason: worker counter read after every worker joined
                totalBlocks.fetch_add(1U, std::memory_order_relaxed);
            }
        });
    }
    // Atomic with relaxed order reason: read after every worker joined
    EXPECT_EQ(totalBlocks.load(std::memory_order_relaxed), BATCHES * THREADS_PER_BATCH * 4U);
}
TEST_F(EHeapThreadingTest, ThreadChurn_batches04)
{
    constexpr size_t BATCHES = 4U;
    constexpr size_t THREADS_PER_BATCH = 8U;
    std::atomic<size_t> totalBlocks {0U};
    for (size_t batch = 0; batch < BATCHES; ++batch) {
        RunThreads(THREADS_PER_BATCH, [&totalBlocks](size_t threadIndex) {
            HeapMutationGuard guard;
            for (size_t index = 0; index < 4U; ++index) {
                const size_t size = 32U + threadIndex * 4U + index;
                void *block = EHeap::Alloc(size);
                if (block == nullptr) {
                    return;
                }
                FillPattern(block, size, threadIndex * 10U + index);
                // Atomic with relaxed order reason: worker counter read after every worker joined
                totalBlocks.fetch_add(1U, std::memory_order_relaxed);
            }
        });
    }
    // Atomic with relaxed order reason: read after every worker joined
    EXPECT_EQ(totalBlocks.load(std::memory_order_relaxed), BATCHES * THREADS_PER_BATCH * 4U);
}
TEST_F(EHeapThreadingTest, ThreadLocalScratchBuffers_threads02)
{
    constexpr size_t THREADS = 2U;
    constexpr size_t SIZE = 2048U;
    std::vector<void *> buffers(THREADS, nullptr);
    for (size_t index = 0; index < THREADS; ++index) {
        buffers[index] = EHeap::Alloc(SIZE);
        ASSERT_NE(buffers[index], nullptr);
        FillPattern(buffers[index], SIZE, index);
    }
    std::atomic<size_t> failures {0U};
    RunThreads(THREADS, [&buffers, &failures](size_t threadIndex) {
        for (size_t round = 0; round < 256U; ++round) {
            auto *bytes = static_cast<uint8_t *>(buffers[threadIndex]);
            if (round == 0U && !PatternMatches(buffers[threadIndex], SIZE, threadIndex)) {
                // Atomic with relaxed order reason: worker counter read after every worker joined
                failures.fetch_add(1U, std::memory_order_relaxed);
            }
            bytes[round % SIZE] = static_cast<uint8_t>(round);
            if (bytes[round % SIZE] != static_cast<uint8_t>(round)) {
                // Atomic with relaxed order reason: worker counter read after every worker joined
                failures.fetch_add(1U, std::memory_order_relaxed);
            }
        }
    });
    // Atomic with relaxed order reason: read after every worker joined
    EXPECT_EQ(failures.load(std::memory_order_relaxed), 0U);
}
TEST_F(EHeapThreadingTest, ThreadLocalScratchBuffers_threads04)
{
    constexpr size_t THREADS = 4U;
    constexpr size_t SIZE = 2048U;
    std::vector<void *> buffers(THREADS, nullptr);
    for (size_t index = 0; index < THREADS; ++index) {
        buffers[index] = EHeap::Alloc(SIZE);
        ASSERT_NE(buffers[index], nullptr);
        FillPattern(buffers[index], SIZE, index);
    }
    std::atomic<size_t> failures {0U};
    RunThreads(THREADS, [&buffers, &failures](size_t threadIndex) {
        for (size_t round = 0; round < 256U; ++round) {
            auto *bytes = static_cast<uint8_t *>(buffers[threadIndex]);
            if (round == 0U && !PatternMatches(buffers[threadIndex], SIZE, threadIndex)) {
                // Atomic with relaxed order reason: worker counter read after every worker joined
                failures.fetch_add(1U, std::memory_order_relaxed);
            }
            bytes[round % SIZE] = static_cast<uint8_t>(round);
            if (bytes[round % SIZE] != static_cast<uint8_t>(round)) {
                // Atomic with relaxed order reason: worker counter read after every worker joined
                failures.fetch_add(1U, std::memory_order_relaxed);
            }
        }
    });
    // Atomic with relaxed order reason: read after every worker joined
    EXPECT_EQ(failures.load(std::memory_order_relaxed), 0U);
}
TEST_F(EHeapThreadingTest, ThreadLocalScratchBuffers_threads08)
{
    constexpr size_t THREADS = 8U;
    constexpr size_t SIZE = 2048U;
    std::vector<void *> buffers(THREADS, nullptr);
    for (size_t index = 0; index < THREADS; ++index) {
        buffers[index] = EHeap::Alloc(SIZE);
        ASSERT_NE(buffers[index], nullptr);
        FillPattern(buffers[index], SIZE, index);
    }
    std::atomic<size_t> failures {0U};
    RunThreads(THREADS, [&buffers, &failures](size_t threadIndex) {
        for (size_t round = 0; round < 256U; ++round) {
            auto *bytes = static_cast<uint8_t *>(buffers[threadIndex]);
            if (round == 0U && !PatternMatches(buffers[threadIndex], SIZE, threadIndex)) {
                // Atomic with relaxed order reason: worker counter read after every worker joined
                failures.fetch_add(1U, std::memory_order_relaxed);
            }
            bytes[round % SIZE] = static_cast<uint8_t>(round);
            if (bytes[round % SIZE] != static_cast<uint8_t>(round)) {
                // Atomic with relaxed order reason: worker counter read after every worker joined
                failures.fetch_add(1U, std::memory_order_relaxed);
            }
        }
    });
    // Atomic with relaxed order reason: read after every worker joined
    EXPECT_EQ(failures.load(std::memory_order_relaxed), 0U);
}
TEST_F(EHeapThreadingTest, CrossRead_02threads_032blocks)
{
    constexpr size_t THREADS = 2U;
    constexpr size_t BLOCKS = 32U;
    std::array<BlockRecord, BLOCKS> records {};
    for (size_t index = 0; index < BLOCKS; ++index) {
        const size_t size = 8U + (index * 29U) % 512U;
        void *block = EHeap::Alloc(size);
        ASSERT_NE(block, nullptr);
        FillPattern(block, size, index);
        records[index] = BlockRecord {block, size, index};
    }
    std::atomic<size_t> verified {0U};
    RunThreads(THREADS, [&records, &verified](size_t threadIndex) {
        for (size_t index = threadIndex; index < BLOCKS; index += THREADS) {
            if (PatternMatches(records[index].ptr, records[index].size, records[index].seed)) {
                // Atomic with relaxed order reason: worker counter read after every worker joined
                verified.fetch_add(1U, std::memory_order_relaxed);
            }
        }
    });
    // Atomic with relaxed order reason: read after every worker joined
    EXPECT_EQ(verified.load(std::memory_order_relaxed), BLOCKS);
}
TEST_F(EHeapThreadingTest, CrossRead_04threads_032blocks)
{
    constexpr size_t THREADS = 4U;
    constexpr size_t BLOCKS = 32U;
    std::array<BlockRecord, BLOCKS> records {};
    for (size_t index = 0; index < BLOCKS; ++index) {
        const size_t size = 8U + (index * 29U) % 512U;
        void *block = EHeap::Alloc(size);
        ASSERT_NE(block, nullptr);
        FillPattern(block, size, index);
        records[index] = BlockRecord {block, size, index};
    }
    std::atomic<size_t> verified {0U};
    RunThreads(THREADS, [&records, &verified](size_t threadIndex) {
        for (size_t index = threadIndex; index < BLOCKS; index += THREADS) {
            if (PatternMatches(records[index].ptr, records[index].size, records[index].seed)) {
                // Atomic with relaxed order reason: worker counter read after every worker joined
                verified.fetch_add(1U, std::memory_order_relaxed);
            }
        }
    });
    // Atomic with relaxed order reason: read after every worker joined
    EXPECT_EQ(verified.load(std::memory_order_relaxed), BLOCKS);
}
TEST_F(EHeapThreadingTest, CrossRead_08threads_032blocks)
{
    constexpr size_t THREADS = 8U;
    constexpr size_t BLOCKS = 32U;
    std::array<BlockRecord, BLOCKS> records {};
    for (size_t index = 0; index < BLOCKS; ++index) {
        const size_t size = 8U + (index * 29U) % 512U;
        void *block = EHeap::Alloc(size);
        ASSERT_NE(block, nullptr);
        FillPattern(block, size, index);
        records[index] = BlockRecord {block, size, index};
    }
    std::atomic<size_t> verified {0U};
    RunThreads(THREADS, [&records, &verified](size_t threadIndex) {
        for (size_t index = threadIndex; index < BLOCKS; index += THREADS) {
            if (PatternMatches(records[index].ptr, records[index].size, records[index].seed)) {
                // Atomic with relaxed order reason: worker counter read after every worker joined
                verified.fetch_add(1U, std::memory_order_relaxed);
            }
        }
    });
    // Atomic with relaxed order reason: read after every worker joined
    EXPECT_EQ(verified.load(std::memory_order_relaxed), BLOCKS);
}
TEST_F(EHeapThreadingTest, CrossRead_04threads_064blocks)
{
    constexpr size_t THREADS = 4U;
    constexpr size_t BLOCKS = 64U;
    std::array<BlockRecord, BLOCKS> records {};
    for (size_t index = 0; index < BLOCKS; ++index) {
        const size_t size = 8U + (index * 29U) % 512U;
        void *block = EHeap::Alloc(size);
        ASSERT_NE(block, nullptr);
        FillPattern(block, size, index);
        records[index] = BlockRecord {block, size, index};
    }
    std::atomic<size_t> verified {0U};
    RunThreads(THREADS, [&records, &verified](size_t threadIndex) {
        for (size_t index = threadIndex; index < BLOCKS; index += THREADS) {
            if (PatternMatches(records[index].ptr, records[index].size, records[index].seed)) {
                // Atomic with relaxed order reason: worker counter read after every worker joined
                verified.fetch_add(1U, std::memory_order_relaxed);
            }
        }
    });
    // Atomic with relaxed order reason: read after every worker joined
    EXPECT_EQ(verified.load(std::memory_order_relaxed), BLOCKS);
}
TEST_F(EHeapThreadingTest, CrossRead_08threads_064blocks)
{
    constexpr size_t THREADS = 8U;
    constexpr size_t BLOCKS = 64U;
    std::array<BlockRecord, BLOCKS> records {};
    for (size_t index = 0; index < BLOCKS; ++index) {
        const size_t size = 8U + (index * 29U) % 512U;
        void *block = EHeap::Alloc(size);
        ASSERT_NE(block, nullptr);
        FillPattern(block, size, index);
        records[index] = BlockRecord {block, size, index};
    }
    std::atomic<size_t> verified {0U};
    RunThreads(THREADS, [&records, &verified](size_t threadIndex) {
        for (size_t index = threadIndex; index < BLOCKS; index += THREADS) {
            if (PatternMatches(records[index].ptr, records[index].size, records[index].seed)) {
                // Atomic with relaxed order reason: worker counter read after every worker joined
                verified.fetch_add(1U, std::memory_order_relaxed);
            }
        }
    });
    // Atomic with relaxed order reason: read after every worker joined
    EXPECT_EQ(verified.load(std::memory_order_relaxed), BLOCKS);
}
TEST_F(EHeapThreadingTest, CrossRead_08threads_128blocks)
{
    constexpr size_t THREADS = 8U;
    constexpr size_t BLOCKS = 128U;
    std::array<BlockRecord, BLOCKS> records {};
    for (size_t index = 0; index < BLOCKS; ++index) {
        const size_t size = 8U + (index * 29U) % 512U;
        void *block = EHeap::Alloc(size);
        ASSERT_NE(block, nullptr);
        FillPattern(block, size, index);
        records[index] = BlockRecord {block, size, index};
    }
    std::atomic<size_t> verified {0U};
    RunThreads(THREADS, [&records, &verified](size_t threadIndex) {
        for (size_t index = threadIndex; index < BLOCKS; index += THREADS) {
            if (PatternMatches(records[index].ptr, records[index].size, records[index].seed)) {
                // Atomic with relaxed order reason: worker counter read after every worker joined
                verified.fetch_add(1U, std::memory_order_relaxed);
            }
        }
    });
    // Atomic with relaxed order reason: read after every worker joined
    EXPECT_EQ(verified.load(std::memory_order_relaxed), BLOCKS);
}
TEST_F(EHeapThreadingTest, SerializedOrderingAcrossThreads)
{
    constexpr size_t THREADS = 2U;
    constexpr size_t ROUNDS = 32U;
    SpinBarrier barrier {static_cast<uint32_t>(THREADS)};
    std::mutex orderMutex;
    std::vector<uintptr_t> order;
    order.reserve(THREADS * ROUNDS);
    RunThreads(THREADS, [&barrier, &orderMutex, &order](size_t threadIndex) {
        for (size_t round = 0; round < ROUNDS; ++round) {
            barrier.Wait();
            {
                HeapMutationGuard guard;
                void *block = EHeap::Alloc(16U + threadIndex * 8U);
                if (block == nullptr) {
                    return;
                }
                std::lock_guard<std::mutex> lock(orderMutex);
                order.push_back(Addr(block));
            }
            barrier.Wait();
        }
    });
    EXPECT_EQ(order.size(), THREADS * ROUNDS);
    for (size_t index = 1; index < order.size(); ++index) {
        EXPECT_GT(order[index], order[index - 1U]);
    }
}
TEST_F(EHeapThreadingTest, ConcurrentSizeSnapshots)
{
    constexpr size_t THREADS = 8U;
    for (size_t index = 0; index < 128U; ++index) {
        ASSERT_NE(EHeap::Alloc(index), nullptr);
    }
    const size_t snapshot = EHeap::AllocatedSize();
    std::atomic<size_t> differing {0U};
    RunThreads(THREADS, [&differing, snapshot](size_t) {
        for (size_t round = 0; round < 64U; ++round) {
            if (EHeap::AllocatedSize() != snapshot) {
                // Atomic with relaxed order reason: worker counter read after every worker joined
                differing.fetch_add(1U, std::memory_order_relaxed);
            }
            if (EHeap::FreedSize() != EHeap::FreedSize()) {
                // Atomic with relaxed order reason: worker counter read after every worker joined
                differing.fetch_add(1U, std::memory_order_relaxed);
            }
        }
    });
    // Atomic with relaxed order reason: read after every worker joined
    EXPECT_EQ(differing.load(std::memory_order_relaxed), 0U);
}
TEST_F(EHeapThreadingTest, ConcurrentHeapIdSnapshots)
{
    constexpr size_t THREADS = 8U;
    const size_t snapshot = EHeap::DeprecatedId();
    std::atomic<size_t> differing {0U};
    RunThreads(THREADS, [&differing, snapshot](size_t) {
        for (size_t round = 0; round < 64U; ++round) {
            if (EHeap::DeprecatedId() != snapshot) {
                // Atomic with relaxed order reason: worker counter read after every worker joined
                differing.fetch_add(1U, std::memory_order_relaxed);
            }
        }
    });
    // Atomic with relaxed order reason: read after every worker joined
    EXPECT_EQ(differing.load(std::memory_order_relaxed), 0U);
}
// ---------------------------------------------------------------------------
// Object allocation matrix: size, alignment and zero-initialisation
// ---------------------------------------------------------------------------
class EHeapAggregateTest : public EHeapTest {};
TEST_F(EHeapAggregateTest, TrivialType_char)
{
    const size_t before = EHeap::AllocatedSize();
    auto *value = EAllocator::New<char>();
    ASSERT_NE(value, nullptr);
    EXPECT_TRUE(IsAligned(value));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(sizeof(char)));
    EXPECT_EQ(*value, 0);
    *value = static_cast<char>(1);
    EXPECT_NE(*value, static_cast<char>(0));
}
TEST_F(EHeapAggregateTest, TrivialType_short)
{
    const size_t before = EHeap::AllocatedSize();
    auto *value = EAllocator::New<short>();
    ASSERT_NE(value, nullptr);
    EXPECT_TRUE(IsAligned(value));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(sizeof(short)));
    EXPECT_EQ(*value, 0);
    *value = static_cast<short>(1);
    EXPECT_NE(*value, static_cast<short>(0));
}
TEST_F(EHeapAggregateTest, TrivialType_int)
{
    const size_t before = EHeap::AllocatedSize();
    auto *value = EAllocator::New<int>();
    ASSERT_NE(value, nullptr);
    EXPECT_TRUE(IsAligned(value));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(sizeof(int)));
    EXPECT_EQ(*value, 0);
    *value = static_cast<int>(1);
    EXPECT_NE(*value, static_cast<int>(0));
}
TEST_F(EHeapAggregateTest, TrivialType_long)
{
    const size_t before = EHeap::AllocatedSize();
    auto *value = EAllocator::New<long>();
    ASSERT_NE(value, nullptr);
    EXPECT_TRUE(IsAligned(value));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(sizeof(long)));
    EXPECT_EQ(*value, 0L);
    *value = static_cast<long>(1);
    EXPECT_NE(*value, static_cast<long>(0));
}
TEST_F(EHeapAggregateTest, TrivialType_longlong)
{
    const size_t before = EHeap::AllocatedSize();
    auto *value = EAllocator::New<long long>();
    ASSERT_NE(value, nullptr);
    EXPECT_TRUE(IsAligned(value));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(sizeof(long long)));
    EXPECT_EQ(*value, 0LL);
    *value = static_cast<long long>(1);
    EXPECT_NE(*value, static_cast<long long>(0));
}
TEST_F(EHeapAggregateTest, TrivialType_unsigned)
{
    const size_t before = EHeap::AllocatedSize();
    auto *value = EAllocator::New<unsigned>();
    ASSERT_NE(value, nullptr);
    EXPECT_TRUE(IsAligned(value));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(sizeof(unsigned)));
    EXPECT_EQ(*value, 0U);
    *value = static_cast<unsigned>(1);
    EXPECT_NE(*value, static_cast<unsigned>(0));
}
TEST_F(EHeapAggregateTest, TrivialType_float)
{
    const size_t before = EHeap::AllocatedSize();
    auto *value = EAllocator::New<float>();
    ASSERT_NE(value, nullptr);
    EXPECT_TRUE(IsAligned(value));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(sizeof(float)));
    EXPECT_FLOAT_EQ(*value, 0.0F);
    *value = static_cast<float>(1);
    EXPECT_NE(*value, static_cast<float>(0));
}
TEST_F(EHeapAggregateTest, TrivialType_double)
{
    const size_t before = EHeap::AllocatedSize();
    auto *value = EAllocator::New<double>();
    ASSERT_NE(value, nullptr);
    EXPECT_TRUE(IsAligned(value));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(sizeof(double)));
    EXPECT_DOUBLE_EQ(*value, 0.0);
    *value = static_cast<double>(1);
    EXPECT_NE(*value, static_cast<double>(0));
}
TEST_F(EHeapAggregateTest, ValueProbeSize)
{
    const size_t before = EHeap::AllocatedSize();
    auto *probe = EAllocator::New<ValueProbe>(3, 4);
    ASSERT_NE(probe, nullptr);
    EXPECT_TRUE(IsAligned(probe));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(sizeof(ValueProbe)));
    EXPECT_EQ(probe->first, 3);
    EXPECT_EQ(probe->second, 4);
}
TEST_F(EHeapAggregateTest, LargeProbeSize)
{
    const size_t before = EHeap::AllocatedSize();
    auto *probe = EAllocator::New<LargeProbe>(0x21U);
    ASSERT_NE(probe, nullptr);
    EXPECT_TRUE(IsAligned(probe));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(sizeof(LargeProbe)));
    EXPECT_EQ(probe->payload[0], 0x21U);
}
TEST_F(EHeapAggregateTest, PairSize)
{
    const size_t before = EHeap::AllocatedSize();
    auto *pair = EAllocator::New<std::pair<int, int>>(1, 2);
    ASSERT_NE(pair, nullptr);
    EXPECT_TRUE(IsAligned(pair));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(sizeof(std::pair<int, int>)));
    EXPECT_EQ(pair->first, 1);
    EXPECT_EQ(pair->second, 2);
}
TEST_F(EHeapAggregateTest, ArrayProbeSize)
{
    using ArrayProbe = std::array<uint8_t, 24>;
    const size_t before = EHeap::AllocatedSize();
    auto *probe = EAllocator::New<ArrayProbe>();
    ASSERT_NE(probe, nullptr);
    EXPECT_TRUE(IsAligned(probe));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(sizeof(ArrayProbe)));
    EXPECT_EQ(std::count(probe->begin(), probe->end(), 0), 24);
}
TEST_F(EHeapAggregateTest, ZeroSizedStructAllocation)
{
    struct EmptyProbe {};
    const size_t before = EHeap::AllocatedSize();
    auto *probe = EAllocator::New<EmptyProbe>();
    ASSERT_NE(probe, nullptr);
    EXPECT_TRUE(IsAligned(probe));
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(sizeof(EmptyProbe)));
    EXPECT_EQ(sizeof(EmptyProbe), 1U);
}
TEST_F(EHeapAggregateTest, ArrayOfLargeProbes)
{
    constexpr size_t COUNT = 4U;
    auto *probes = EAllocator::New<LargeProbe[]>(COUNT);
    ASSERT_NE(probes, nullptr);
    for (size_t index = 0; index < COUNT; ++index) {
        EXPECT_EQ(probes[index].payload.front(), 0U);
        probes[index].payload[0] = static_cast<uint8_t>(index);
    }
    for (size_t index = 0; index < COUNT; ++index) {
        EXPECT_EQ(probes[index].payload[0], static_cast<uint8_t>(index));
    }
}
TEST_F(EHeapAggregateTest, ArrayOfValueProbes)
{
    struct DefaultProbe {
        int a {0};
        int b {0};
    };
    EAllocator allocator;
    EAllocatorAdapter<DefaultProbe> adapter {&allocator};
    DefaultProbe *probes = adapter.allocate(8U);
    ASSERT_NE(probes, nullptr);
    for (size_t index = 0; index < 8U; ++index) {
        adapter.construct(probes + index);
    }
    for (size_t index = 0; index < 8U; ++index) {
        EXPECT_EQ(probes[index].a, 0);
        EXPECT_EQ(probes[index].b, 0);
        adapter.destroy(probes + index);
    }
    adapter.deallocate(probes, 8U);
}
// ---------------------------------------------------------------------------
// Address progression for repeated allocations
// ---------------------------------------------------------------------------
class EHeapAddressProgressionTest : public EHeapTest {};
TEST_F(EHeapAddressProgressionTest, RepeatedAllocation_0000)
{
    constexpr size_t COUNT = 64U;
    constexpr size_t SIZE = 0U;
    std::array<void *, COUNT> blocks {};
    for (size_t index = 0; index < COUNT; ++index) {
        blocks[index] = EHeap::Alloc(SIZE);
        ASSERT_NE(blocks[index], nullptr);
        EXPECT_TRUE(IsAligned(blocks[index]));
    }
    if (SIZE == 0U) {
        for (size_t index = 1; index < COUNT; ++index) {
            EXPECT_EQ(blocks[index], blocks[0]);
        }
    } else {
        for (size_t index = 1; index < COUNT; ++index) {
            EXPECT_EQ(Addr(blocks[index]), Addr(blocks[index - 1U]) + AlignUpBytes(SIZE));
            EXPECT_FALSE(RangesOverlap(blocks[index - 1U], SIZE, blocks[index], SIZE));
        }
        EXPECT_EQ(Addr(blocks[COUNT - 1U]) - Addr(blocks[0]), (COUNT - 1U) * AlignUpBytes(SIZE));
    }
}
TEST_F(EHeapAddressProgressionTest, RepeatedAllocation_0001)
{
    constexpr size_t COUNT = 64U;
    constexpr size_t SIZE = 1U;
    std::array<void *, COUNT> blocks {};
    for (size_t index = 0; index < COUNT; ++index) {
        blocks[index] = EHeap::Alloc(SIZE);
        ASSERT_NE(blocks[index], nullptr);
        EXPECT_TRUE(IsAligned(blocks[index]));
    }
    if (SIZE == 0U) {
        for (size_t index = 1; index < COUNT; ++index) {
            EXPECT_EQ(blocks[index], blocks[0]);
        }
    } else {
        for (size_t index = 1; index < COUNT; ++index) {
            EXPECT_EQ(Addr(blocks[index]), Addr(blocks[index - 1U]) + AlignUpBytes(SIZE));
            EXPECT_FALSE(RangesOverlap(blocks[index - 1U], SIZE, blocks[index], SIZE));
        }
        EXPECT_EQ(Addr(blocks[COUNT - 1U]) - Addr(blocks[0]), (COUNT - 1U) * AlignUpBytes(SIZE));
    }
}
TEST_F(EHeapAddressProgressionTest, RepeatedAllocation_0002)
{
    constexpr size_t COUNT = 64U;
    constexpr size_t SIZE = 2U;
    std::array<void *, COUNT> blocks {};
    for (size_t index = 0; index < COUNT; ++index) {
        blocks[index] = EHeap::Alloc(SIZE);
        ASSERT_NE(blocks[index], nullptr);
        EXPECT_TRUE(IsAligned(blocks[index]));
    }
    if (SIZE == 0U) {
        for (size_t index = 1; index < COUNT; ++index) {
            EXPECT_EQ(blocks[index], blocks[0]);
        }
    } else {
        for (size_t index = 1; index < COUNT; ++index) {
            EXPECT_EQ(Addr(blocks[index]), Addr(blocks[index - 1U]) + AlignUpBytes(SIZE));
            EXPECT_FALSE(RangesOverlap(blocks[index - 1U], SIZE, blocks[index], SIZE));
        }
        EXPECT_EQ(Addr(blocks[COUNT - 1U]) - Addr(blocks[0]), (COUNT - 1U) * AlignUpBytes(SIZE));
    }
}
TEST_F(EHeapAddressProgressionTest, RepeatedAllocation_0007)
{
    constexpr size_t COUNT = 64U;
    constexpr size_t SIZE = 7U;
    std::array<void *, COUNT> blocks {};
    for (size_t index = 0; index < COUNT; ++index) {
        blocks[index] = EHeap::Alloc(SIZE);
        ASSERT_NE(blocks[index], nullptr);
        EXPECT_TRUE(IsAligned(blocks[index]));
    }
    if (SIZE == 0U) {
        for (size_t index = 1; index < COUNT; ++index) {
            EXPECT_EQ(blocks[index], blocks[0]);
        }
    } else {
        for (size_t index = 1; index < COUNT; ++index) {
            EXPECT_EQ(Addr(blocks[index]), Addr(blocks[index - 1U]) + AlignUpBytes(SIZE));
            EXPECT_FALSE(RangesOverlap(blocks[index - 1U], SIZE, blocks[index], SIZE));
        }
        EXPECT_EQ(Addr(blocks[COUNT - 1U]) - Addr(blocks[0]), (COUNT - 1U) * AlignUpBytes(SIZE));
    }
}
TEST_F(EHeapAddressProgressionTest, RepeatedAllocation_0008)
{
    constexpr size_t COUNT = 64U;
    constexpr size_t SIZE = 8U;
    std::array<void *, COUNT> blocks {};
    for (size_t index = 0; index < COUNT; ++index) {
        blocks[index] = EHeap::Alloc(SIZE);
        ASSERT_NE(blocks[index], nullptr);
        EXPECT_TRUE(IsAligned(blocks[index]));
    }
    if (SIZE == 0U) {
        for (size_t index = 1; index < COUNT; ++index) {
            EXPECT_EQ(blocks[index], blocks[0]);
        }
    } else {
        for (size_t index = 1; index < COUNT; ++index) {
            EXPECT_EQ(Addr(blocks[index]), Addr(blocks[index - 1U]) + AlignUpBytes(SIZE));
            EXPECT_FALSE(RangesOverlap(blocks[index - 1U], SIZE, blocks[index], SIZE));
        }
        EXPECT_EQ(Addr(blocks[COUNT - 1U]) - Addr(blocks[0]), (COUNT - 1U) * AlignUpBytes(SIZE));
    }
}
TEST_F(EHeapAddressProgressionTest, RepeatedAllocation_0009)
{
    constexpr size_t COUNT = 64U;
    constexpr size_t SIZE = 9U;
    std::array<void *, COUNT> blocks {};
    for (size_t index = 0; index < COUNT; ++index) {
        blocks[index] = EHeap::Alloc(SIZE);
        ASSERT_NE(blocks[index], nullptr);
        EXPECT_TRUE(IsAligned(blocks[index]));
    }
    if (SIZE == 0U) {
        for (size_t index = 1; index < COUNT; ++index) {
            EXPECT_EQ(blocks[index], blocks[0]);
        }
    } else {
        for (size_t index = 1; index < COUNT; ++index) {
            EXPECT_EQ(Addr(blocks[index]), Addr(blocks[index - 1U]) + AlignUpBytes(SIZE));
            EXPECT_FALSE(RangesOverlap(blocks[index - 1U], SIZE, blocks[index], SIZE));
        }
        EXPECT_EQ(Addr(blocks[COUNT - 1U]) - Addr(blocks[0]), (COUNT - 1U) * AlignUpBytes(SIZE));
    }
}
TEST_F(EHeapAddressProgressionTest, RepeatedAllocation_0015)
{
    constexpr size_t COUNT = 64U;
    constexpr size_t SIZE = 15U;
    std::array<void *, COUNT> blocks {};
    for (size_t index = 0; index < COUNT; ++index) {
        blocks[index] = EHeap::Alloc(SIZE);
        ASSERT_NE(blocks[index], nullptr);
        EXPECT_TRUE(IsAligned(blocks[index]));
    }
    if (SIZE == 0U) {
        for (size_t index = 1; index < COUNT; ++index) {
            EXPECT_EQ(blocks[index], blocks[0]);
        }
    } else {
        for (size_t index = 1; index < COUNT; ++index) {
            EXPECT_EQ(Addr(blocks[index]), Addr(blocks[index - 1U]) + AlignUpBytes(SIZE));
            EXPECT_FALSE(RangesOverlap(blocks[index - 1U], SIZE, blocks[index], SIZE));
        }
        EXPECT_EQ(Addr(blocks[COUNT - 1U]) - Addr(blocks[0]), (COUNT - 1U) * AlignUpBytes(SIZE));
    }
}
TEST_F(EHeapAddressProgressionTest, RepeatedAllocation_0016)
{
    constexpr size_t COUNT = 64U;
    constexpr size_t SIZE = 16U;
    std::array<void *, COUNT> blocks {};
    for (size_t index = 0; index < COUNT; ++index) {
        blocks[index] = EHeap::Alloc(SIZE);
        ASSERT_NE(blocks[index], nullptr);
        EXPECT_TRUE(IsAligned(blocks[index]));
    }
    if (SIZE == 0U) {
        for (size_t index = 1; index < COUNT; ++index) {
            EXPECT_EQ(blocks[index], blocks[0]);
        }
    } else {
        for (size_t index = 1; index < COUNT; ++index) {
            EXPECT_EQ(Addr(blocks[index]), Addr(blocks[index - 1U]) + AlignUpBytes(SIZE));
            EXPECT_FALSE(RangesOverlap(blocks[index - 1U], SIZE, blocks[index], SIZE));
        }
        EXPECT_EQ(Addr(blocks[COUNT - 1U]) - Addr(blocks[0]), (COUNT - 1U) * AlignUpBytes(SIZE));
    }
}
TEST_F(EHeapAddressProgressionTest, RepeatedAllocation_0017)
{
    constexpr size_t COUNT = 64U;
    constexpr size_t SIZE = 17U;
    std::array<void *, COUNT> blocks {};
    for (size_t index = 0; index < COUNT; ++index) {
        blocks[index] = EHeap::Alloc(SIZE);
        ASSERT_NE(blocks[index], nullptr);
        EXPECT_TRUE(IsAligned(blocks[index]));
    }
    if (SIZE == 0U) {
        for (size_t index = 1; index < COUNT; ++index) {
            EXPECT_EQ(blocks[index], blocks[0]);
        }
    } else {
        for (size_t index = 1; index < COUNT; ++index) {
            EXPECT_EQ(Addr(blocks[index]), Addr(blocks[index - 1U]) + AlignUpBytes(SIZE));
            EXPECT_FALSE(RangesOverlap(blocks[index - 1U], SIZE, blocks[index], SIZE));
        }
        EXPECT_EQ(Addr(blocks[COUNT - 1U]) - Addr(blocks[0]), (COUNT - 1U) * AlignUpBytes(SIZE));
    }
}
TEST_F(EHeapAddressProgressionTest, RepeatedAllocation_0031)
{
    constexpr size_t COUNT = 64U;
    constexpr size_t SIZE = 31U;
    std::array<void *, COUNT> blocks {};
    for (size_t index = 0; index < COUNT; ++index) {
        blocks[index] = EHeap::Alloc(SIZE);
        ASSERT_NE(blocks[index], nullptr);
        EXPECT_TRUE(IsAligned(blocks[index]));
    }
    if (SIZE == 0U) {
        for (size_t index = 1; index < COUNT; ++index) {
            EXPECT_EQ(blocks[index], blocks[0]);
        }
    } else {
        for (size_t index = 1; index < COUNT; ++index) {
            EXPECT_EQ(Addr(blocks[index]), Addr(blocks[index - 1U]) + AlignUpBytes(SIZE));
            EXPECT_FALSE(RangesOverlap(blocks[index - 1U], SIZE, blocks[index], SIZE));
        }
        EXPECT_EQ(Addr(blocks[COUNT - 1U]) - Addr(blocks[0]), (COUNT - 1U) * AlignUpBytes(SIZE));
    }
}
TEST_F(EHeapAddressProgressionTest, RepeatedAllocation_0064)
{
    constexpr size_t COUNT = 64U;
    constexpr size_t SIZE = 64U;
    std::array<void *, COUNT> blocks {};
    for (size_t index = 0; index < COUNT; ++index) {
        blocks[index] = EHeap::Alloc(SIZE);
        ASSERT_NE(blocks[index], nullptr);
        EXPECT_TRUE(IsAligned(blocks[index]));
    }
    if (SIZE == 0U) {
        for (size_t index = 1; index < COUNT; ++index) {
            EXPECT_EQ(blocks[index], blocks[0]);
        }
    } else {
        for (size_t index = 1; index < COUNT; ++index) {
            EXPECT_EQ(Addr(blocks[index]), Addr(blocks[index - 1U]) + AlignUpBytes(SIZE));
            EXPECT_FALSE(RangesOverlap(blocks[index - 1U], SIZE, blocks[index], SIZE));
        }
        EXPECT_EQ(Addr(blocks[COUNT - 1U]) - Addr(blocks[0]), (COUNT - 1U) * AlignUpBytes(SIZE));
    }
}
TEST_F(EHeapAddressProgressionTest, RepeatedAllocation_0255)
{
    constexpr size_t COUNT = 64U;
    constexpr size_t SIZE = 255U;
    std::array<void *, COUNT> blocks {};
    for (size_t index = 0; index < COUNT; ++index) {
        blocks[index] = EHeap::Alloc(SIZE);
        ASSERT_NE(blocks[index], nullptr);
        EXPECT_TRUE(IsAligned(blocks[index]));
    }
    if (SIZE == 0U) {
        for (size_t index = 1; index < COUNT; ++index) {
            EXPECT_EQ(blocks[index], blocks[0]);
        }
    } else {
        for (size_t index = 1; index < COUNT; ++index) {
            EXPECT_EQ(Addr(blocks[index]), Addr(blocks[index - 1U]) + AlignUpBytes(SIZE));
            EXPECT_FALSE(RangesOverlap(blocks[index - 1U], SIZE, blocks[index], SIZE));
        }
        EXPECT_EQ(Addr(blocks[COUNT - 1U]) - Addr(blocks[0]), (COUNT - 1U) * AlignUpBytes(SIZE));
    }
}
TEST_F(EHeapAddressProgressionTest, RepeatedAllocation_0256)
{
    constexpr size_t COUNT = 64U;
    constexpr size_t SIZE = 256U;
    std::array<void *, COUNT> blocks {};
    for (size_t index = 0; index < COUNT; ++index) {
        blocks[index] = EHeap::Alloc(SIZE);
        ASSERT_NE(blocks[index], nullptr);
        EXPECT_TRUE(IsAligned(blocks[index]));
    }
    if (SIZE == 0U) {
        for (size_t index = 1; index < COUNT; ++index) {
            EXPECT_EQ(blocks[index], blocks[0]);
        }
    } else {
        for (size_t index = 1; index < COUNT; ++index) {
            EXPECT_EQ(Addr(blocks[index]), Addr(blocks[index - 1U]) + AlignUpBytes(SIZE));
            EXPECT_FALSE(RangesOverlap(blocks[index - 1U], SIZE, blocks[index], SIZE));
        }
        EXPECT_EQ(Addr(blocks[COUNT - 1U]) - Addr(blocks[0]), (COUNT - 1U) * AlignUpBytes(SIZE));
    }
}
TEST_F(EHeapAddressProgressionTest, RepeatedAllocation_1024)
{
    constexpr size_t COUNT = 64U;
    constexpr size_t SIZE = 1024U;
    std::array<void *, COUNT> blocks {};
    for (size_t index = 0; index < COUNT; ++index) {
        blocks[index] = EHeap::Alloc(SIZE);
        ASSERT_NE(blocks[index], nullptr);
        EXPECT_TRUE(IsAligned(blocks[index]));
    }
    if (SIZE == 0U) {
        for (size_t index = 1; index < COUNT; ++index) {
            EXPECT_EQ(blocks[index], blocks[0]);
        }
    } else {
        for (size_t index = 1; index < COUNT; ++index) {
            EXPECT_EQ(Addr(blocks[index]), Addr(blocks[index - 1U]) + AlignUpBytes(SIZE));
            EXPECT_FALSE(RangesOverlap(blocks[index - 1U], SIZE, blocks[index], SIZE));
        }
        EXPECT_EQ(Addr(blocks[COUNT - 1U]) - Addr(blocks[0]), (COUNT - 1U) * AlignUpBytes(SIZE));
    }
}
// ---------------------------------------------------------------------------
// Composite workloads mixing allocator, containers and pointers
// ---------------------------------------------------------------------------
class EHeapCompositeWorkloadTest : public EHeapTest {};
TEST_F(EHeapCompositeWorkloadTest, VectorOfStrings)
{
    ArenaVector<ArenaString> names;
    for (int index = 0; index < 32; ++index) {
        names.emplace_back("name-" + std::to_string(index));
    }
    EXPECT_EQ(names.size(), 32U);
    EXPECT_EQ(names.front(), "name-0");
    EXPECT_EQ(names.back(), "name-31");
    std::sort(names.begin(), names.end());
    EXPECT_TRUE(std::is_sorted(names.begin(), names.end()));
}
TEST_F(EHeapCompositeWorkloadTest, MapOfVectors)
{
    ArenaMap<int, ArenaVector<int>> groups;
    for (int key = 0; key < 8; ++key) {
        for (int value = 0; value < 16; ++value) {
            groups[key].push_back(key * 100 + value);
        }
    }
    EXPECT_EQ(groups.size(), 8U);
    EXPECT_EQ(groups[3].size(), 16U);
    EXPECT_EQ(groups[3].front(), 300);
    EXPECT_EQ(groups[3].back(), 315);
}
TEST_F(EHeapCompositeWorkloadTest, UnorderedMapOfStrings)
{
    ArenaUnorderedMap<int, ArenaString> labels;
    for (int index = 0; index < 24; ++index) {
        labels[index] = ArenaString {"label"} + std::to_string(index).c_str();
    }
    EXPECT_EQ(labels.size(), 24U);
    EXPECT_EQ(labels[5], "label5");
    EXPECT_EQ(labels.count(100), 0U);
}
TEST_F(EHeapCompositeWorkloadTest, SetOfPointers)
{
    ArenaVector<EPtr<int>> pointers;
    ArenaSet<int> values;
    for (int index = 0; index < 16; ++index) {
        auto *value = EAllocator::New<int>(index);
        ASSERT_NE(value, nullptr);
        pointers.push_back(EPtr<int> {value});
        values.insert(*value);
    }
    EXPECT_EQ(values.size(), 16U);
    EXPECT_TRUE(values.count(7) == 1U);
    EXPECT_EQ(*pointers[7], 7);
}
TEST_F(EHeapCompositeWorkloadTest, NestedVectorMatrix)
{
    ArenaVector<ArenaVector<int>> matrix;
    for (int row = 0; row < 4; ++row) {
        matrix.emplace_back();
        for (int column = 0; column < 4; ++column) {
            matrix.back().push_back(row * 4 + column);
        }
    }
    EXPECT_EQ(matrix.size(), 4U);
    EXPECT_EQ(matrix[2][3], 11);
    EXPECT_EQ(matrix[0][0], 0);
}
TEST_F(EHeapCompositeWorkloadTest, ListOfMaps)
{
    ArenaList<ArenaMap<int, int>> list;
    for (int index = 0; index < 4; ++index) {
        ArenaMap<int, int> entry;
        entry[0] = index;
        entry[1] = index * 2;
        list.push_back(std::move(entry));
    }
    EXPECT_EQ(list.size(), 4U);
    EXPECT_EQ(list.front().at(0), 0);
    EXPECT_EQ(list.back().at(1), 6);
}
TEST_F(EHeapCompositeWorkloadTest, StringKeyedMap)
{
    ArenaMap<ArenaString, int> counters;
    const std::array<const char *, 4> keys {"alpha", "beta", "gamma", "delta"};
    for (const char *key : keys) {
        counters[ArenaString {key}] += 1;
    }
    EXPECT_EQ(counters.size(), 4U);
    EXPECT_EQ(counters.at(ArenaString {"beta"}), 1);
    EXPECT_EQ(counters.count(ArenaString {"omega"}), 0U);
}
TEST_F(EHeapCompositeWorkloadTest, AllocatorOwnedContainer)
{
    auto *container = EAllocator::New<ArenaVector<int>>();
    ASSERT_NE(container, nullptr);
    for (int index = 0; index < 64; ++index) {
        container->push_back(index);
    }
    EXPECT_EQ(container->size(), 64U);
    EXPECT_EQ(container->front(), 0);
    EXPECT_EQ(container->back(), 63);
    container->~ArenaVector<int>();
}
TEST_F(EHeapCompositeWorkloadTest, RepeatedContainerLifetime)
{
    for (int round = 0; round < 8; ++round) {
        ArenaVector<int> values;
        values.reserve(128U);
        for (int index = 0; index < 128; ++index) {
            values.push_back(index + round);
        }
        EXPECT_EQ(values.size(), 128U);
        EXPECT_EQ(values.front(), round);
        EXPECT_EQ(values.back(), 127 + round);
    }
}
TEST_F(EHeapCompositeWorkloadTest, ContainerMoveChain)
{
    ArenaVector<int> first;
    for (int index = 0; index < 32; ++index) {
        first.push_back(index);
    }
    ArenaVector<int> second {std::move(first)};
    ArenaVector<int> third {std::move(second)};
    EXPECT_EQ(third.size(), 32U);
    EXPECT_EQ(third.front(), 0);
    EXPECT_EQ(third.back(), 31);
}
// ---------------------------------------------------------------------------
// Allocations crossing the 256 KiB commit granularity
// ---------------------------------------------------------------------------
class EHeapCommitGranularityTest : public EHeapTest {};
TEST_F(EHeapCommitGranularityTest, CommitBoundary_00262136)
{
    const size_t size = 262136U;
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size));
    EXPECT_TRUE(IsAligned(block));
    auto *bytes = static_cast<uint8_t *>(block);
    for (size_t offset = 0; offset < size; offset += 4096U) {
        bytes[offset] = static_cast<uint8_t>((offset / 4096U) & 0xFFU);
    }
    for (size_t offset = 0; offset < size; offset += 4096U) {
        EXPECT_EQ(bytes[offset], static_cast<uint8_t>((offset / 4096U) & 0xFFU));
    }
    EXPECT_EQ(bytes[0], 0U);
    bytes[size - 1U] = 0x77U;
    EXPECT_EQ(bytes[size - 1U], 0x77U);
}
TEST_F(EHeapCommitGranularityTest, CommitBoundary_00262144)
{
    const size_t size = 262144U;
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size));
    EXPECT_TRUE(IsAligned(block));
    auto *bytes = static_cast<uint8_t *>(block);
    for (size_t offset = 0; offset < size; offset += 4096U) {
        bytes[offset] = static_cast<uint8_t>((offset / 4096U) & 0xFFU);
    }
    for (size_t offset = 0; offset < size; offset += 4096U) {
        EXPECT_EQ(bytes[offset], static_cast<uint8_t>((offset / 4096U) & 0xFFU));
    }
    EXPECT_EQ(bytes[0], 0U);
    bytes[size - 1U] = 0x77U;
    EXPECT_EQ(bytes[size - 1U], 0x77U);
}
TEST_F(EHeapCommitGranularityTest, CommitBoundary_00262152)
{
    const size_t size = 262152U;
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size));
    EXPECT_TRUE(IsAligned(block));
    auto *bytes = static_cast<uint8_t *>(block);
    for (size_t offset = 0; offset < size; offset += 4096U) {
        bytes[offset] = static_cast<uint8_t>((offset / 4096U) & 0xFFU);
    }
    for (size_t offset = 0; offset < size; offset += 4096U) {
        EXPECT_EQ(bytes[offset], static_cast<uint8_t>((offset / 4096U) & 0xFFU));
    }
    EXPECT_EQ(bytes[0], 0U);
    bytes[size - 1U] = 0x77U;
    EXPECT_EQ(bytes[size - 1U], 0x77U);
}
TEST_F(EHeapCommitGranularityTest, CommitBoundary_00524287)
{
    const size_t size = 524287U;
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size));
    EXPECT_TRUE(IsAligned(block));
    auto *bytes = static_cast<uint8_t *>(block);
    for (size_t offset = 0; offset < size; offset += 4096U) {
        bytes[offset] = static_cast<uint8_t>((offset / 4096U) & 0xFFU);
    }
    for (size_t offset = 0; offset < size; offset += 4096U) {
        EXPECT_EQ(bytes[offset], static_cast<uint8_t>((offset / 4096U) & 0xFFU));
    }
    EXPECT_EQ(bytes[0], 0U);
    bytes[size - 1U] = 0x77U;
    EXPECT_EQ(bytes[size - 1U], 0x77U);
}
TEST_F(EHeapCommitGranularityTest, CommitBoundary_00524288)
{
    const size_t size = 524288U;
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size));
    EXPECT_TRUE(IsAligned(block));
    auto *bytes = static_cast<uint8_t *>(block);
    for (size_t offset = 0; offset < size; offset += 4096U) {
        bytes[offset] = static_cast<uint8_t>((offset / 4096U) & 0xFFU);
    }
    for (size_t offset = 0; offset < size; offset += 4096U) {
        EXPECT_EQ(bytes[offset], static_cast<uint8_t>((offset / 4096U) & 0xFFU));
    }
    EXPECT_EQ(bytes[0], 0U);
    bytes[size - 1U] = 0x77U;
    EXPECT_EQ(bytes[size - 1U], 0x77U);
}
TEST_F(EHeapCommitGranularityTest, CommitBoundary_00524289)
{
    const size_t size = 524289U;
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size));
    EXPECT_TRUE(IsAligned(block));
    auto *bytes = static_cast<uint8_t *>(block);
    for (size_t offset = 0; offset < size; offset += 4096U) {
        bytes[offset] = static_cast<uint8_t>((offset / 4096U) & 0xFFU);
    }
    for (size_t offset = 0; offset < size; offset += 4096U) {
        EXPECT_EQ(bytes[offset], static_cast<uint8_t>((offset / 4096U) & 0xFFU));
    }
    EXPECT_EQ(bytes[0], 0U);
    bytes[size - 1U] = 0x77U;
    EXPECT_EQ(bytes[size - 1U], 0x77U);
}
TEST_F(EHeapCommitGranularityTest, CommitBoundary_00300000)
{
    const size_t size = 300000U;
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size));
    EXPECT_TRUE(IsAligned(block));
    auto *bytes = static_cast<uint8_t *>(block);
    for (size_t offset = 0; offset < size; offset += 4096U) {
        bytes[offset] = static_cast<uint8_t>((offset / 4096U) & 0xFFU);
    }
    for (size_t offset = 0; offset < size; offset += 4096U) {
        EXPECT_EQ(bytes[offset], static_cast<uint8_t>((offset / 4096U) & 0xFFU));
    }
    EXPECT_EQ(bytes[0], 0U);
    bytes[size - 1U] = 0x77U;
    EXPECT_EQ(bytes[size - 1U], 0x77U);
}
TEST_F(EHeapCommitGranularityTest, CommitBoundary_01048576)
{
    const size_t size = 1048576U;
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size));
    EXPECT_TRUE(IsAligned(block));
    auto *bytes = static_cast<uint8_t *>(block);
    for (size_t offset = 0; offset < size; offset += 4096U) {
        bytes[offset] = static_cast<uint8_t>((offset / 4096U) & 0xFFU);
    }
    for (size_t offset = 0; offset < size; offset += 4096U) {
        EXPECT_EQ(bytes[offset], static_cast<uint8_t>((offset / 4096U) & 0xFFU));
    }
    EXPECT_EQ(bytes[0], 0U);
    bytes[size - 1U] = 0x77U;
    EXPECT_EQ(bytes[size - 1U], 0x77U);
}
TEST_F(EHeapCommitGranularityTest, CommitBoundary_01048577)
{
    const size_t size = 1048577U;
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size));
    EXPECT_TRUE(IsAligned(block));
    auto *bytes = static_cast<uint8_t *>(block);
    for (size_t offset = 0; offset < size; offset += 4096U) {
        bytes[offset] = static_cast<uint8_t>((offset / 4096U) & 0xFFU);
    }
    for (size_t offset = 0; offset < size; offset += 4096U) {
        EXPECT_EQ(bytes[offset], static_cast<uint8_t>((offset / 4096U) & 0xFFU));
    }
    EXPECT_EQ(bytes[0], 0U);
    bytes[size - 1U] = 0x77U;
    EXPECT_EQ(bytes[size - 1U], 0x77U);
}
TEST_F(EHeapCommitGranularityTest, CommitBoundary_02097152)
{
    const size_t size = 2097152U;
    const size_t before = EHeap::AllocatedSize();
    void *block = EHeap::Alloc(size);
    ASSERT_NE(block, nullptr);
    EXPECT_EQ(EHeap::AllocatedSize() - before, AlignUpBytes(size));
    EXPECT_TRUE(IsAligned(block));
    auto *bytes = static_cast<uint8_t *>(block);
    for (size_t offset = 0; offset < size; offset += 4096U) {
        bytes[offset] = static_cast<uint8_t>((offset / 4096U) & 0xFFU);
    }
    for (size_t offset = 0; offset < size; offset += 4096U) {
        EXPECT_EQ(bytes[offset], static_cast<uint8_t>((offset / 4096U) & 0xFFU));
    }
    EXPECT_EQ(bytes[0], 0U);
    bytes[size - 1U] = 0x77U;
    EXPECT_EQ(bytes[size - 1U], 0x77U);
}
// ---------------------------------------------------------------------------
// Additional container operations
// ---------------------------------------------------------------------------
class EHeapExtraContainerTest : public EHeapTest {};
TEST_F(EHeapExtraContainerTest, VectorInsertAndEraseMiddle)
{
    ArenaVector<int> values;
    for (int index = 0; index < 32; ++index) {
        values.push_back(index);
    }
    values.insert(values.begin() + 16, 1000);
    EXPECT_EQ(values.size(), 33U);
    EXPECT_EQ(values[16], 1000);
    EXPECT_EQ(values[17], 16);
    values.erase(values.begin() + 16);
    EXPECT_EQ(values.size(), 32U);
    EXPECT_EQ(values[16], 16);
}
TEST_F(EHeapExtraContainerTest, VectorAssignAndCapacity)
{
    ArenaVector<int> values;
    values.reserve(64U);
    const size_t capacity = values.capacity();
    values.assign(32U, 7);
    EXPECT_EQ(values.size(), 32U);
    EXPECT_EQ(values.capacity(), capacity);
    EXPECT_EQ(std::count(values.begin(), values.end(), 7), 32);
}
TEST_F(EHeapExtraContainerTest, DequePushFrontAndPopBack)
{
    ArenaDeque<int> values;
    for (int index = 0; index < 16; ++index) {
        values.push_front(index);
        values.push_back(index);
    }
    EXPECT_EQ(values.size(), 32U);
    EXPECT_EQ(values.front(), 15);
    EXPECT_EQ(values.back(), 15);
    values.pop_back();
    values.pop_front();
    EXPECT_EQ(values.size(), 30U);
    EXPECT_EQ(values.front(), 14);
    EXPECT_EQ(values.back(), 14);
}
TEST_F(EHeapExtraContainerTest, ListSpliceBetweenContainers)
{
    ArenaList<int> first {1, 2, 3};
    ArenaList<int> second {4, 5, 6};
    first.splice(first.end(), second);
    EXPECT_EQ(first.size(), 6U);
    EXPECT_TRUE(second.empty());
    EXPECT_EQ(first.back(), 6);
    EXPECT_EQ(first.front(), 1);
}
TEST_F(EHeapExtraContainerTest, ListSortAndUnique)
{
    ArenaList<int> values {4, 2, 4, 1, 3, 2};
    values.sort();
    values.unique();
    EXPECT_EQ(values.size(), 4U);
    EXPECT_EQ(values.front(), 1);
    EXPECT_EQ(values.back(), 4);
    EXPECT_TRUE(std::is_sorted(values.begin(), values.end()));
}
TEST_F(EHeapExtraContainerTest, MapLowerAndUpperBound)
{
    ArenaMap<int, int> values;
    for (int key = 0; key < 16; key += 2) {
        values[key] = key * 10;
    }
    auto lower = values.lower_bound(7);
    ASSERT_NE(lower, values.end());
    EXPECT_EQ(lower->first, 8);
    auto upper = values.upper_bound(8);
    ASSERT_NE(upper, values.end());
    EXPECT_EQ(upper->first, 10);
    EXPECT_EQ(values.lower_bound(100), values.end());
}
TEST_F(EHeapExtraContainerTest, SetEqualRange)
{
    ArenaSet<int> values {1, 3, 5, 7, 9};
    const auto range = values.equal_range(5);
    ASSERT_NE(range.first, values.end());
    EXPECT_EQ(*range.first, 5);
    EXPECT_EQ(*range.second, 7);
    EXPECT_EQ(std::distance(range.first, range.second), 1);
}
TEST_F(EHeapExtraContainerTest, UnorderedMapReserveAndRehash)
{
    ArenaUnorderedMap<int, int> values;
    values.reserve(256U);
    for (int key = 0; key < 128; ++key) {
        values[key] = key;
    }
    EXPECT_GE(values.bucket_count(), 256U);
    values.rehash(512U);
    EXPECT_GE(values.bucket_count(), 512U);
    EXPECT_EQ(values.size(), 128U);
    EXPECT_EQ(values.at(64), 64);
}
TEST_F(EHeapExtraContainerTest, StringInsertEraseReplace)
{
    ArenaString text {"0123456789"};
    text.insert(5U, "-");
    EXPECT_EQ(text, "01234-56789");
    text.erase(5U, 1U);
    EXPECT_EQ(text, "0123456789");
    text.replace(0U, 2U, "ab");
    EXPECT_EQ(text, "ab23456789");
    EXPECT_EQ(text.size(), 10U);
}
TEST_F(EHeapExtraContainerTest, MapExistingKeyKeepsLastValue)
{
    ArenaMap<int, int> values;
    values[1] = 10;
    values[1] = 20;
    EXPECT_EQ(values.size(), 1U);
    EXPECT_EQ(values.at(1), 20);
    values.emplace(2, 30);
    EXPECT_EQ(values.size(), 2U);
    EXPECT_EQ(values.at(2), 30);
}
TEST_F(EHeapExtraContainerTest, VectorOfBoolLikeValues)
{
    ArenaVector<uint8_t> flags;
    flags.reserve(128U);
    for (size_t index = 0; index < 128U; ++index) {
        flags.push_back(static_cast<uint8_t>(index % 2U));
    }
    EXPECT_EQ(std::count(flags.begin(), flags.end(), 1U), 64);
    EXPECT_EQ(flags.size(), 128U);
}
// ---------------------------------------------------------------------------
// Additional threading configurations
// ---------------------------------------------------------------------------
class EHeapExtraThreadingTest : public EHeapTest {};
TEST_F(EHeapExtraThreadingTest, SerializedAlloc_threads24_blocks32)
{
    constexpr size_t THREADS = 24U;
    constexpr size_t BLOCKS_PER_THREAD = 32U;
    std::vector<std::vector<BlockRecord>> perThread(THREADS);
    const size_t allocatedBefore = EHeap::AllocatedSize();
    RunThreads(THREADS, [&perThread](size_t threadIndex) {
        auto &local = perThread[threadIndex];
        local.reserve(BLOCKS_PER_THREAD);
        for (size_t iteration = 0; iteration < BLOCKS_PER_THREAD; ++iteration) {
            const size_t size = 8U + ((threadIndex * 53U + iteration * 19U) % 256U);
            HeapMutationGuard guard;
            void *block = EHeap::Alloc(size);
            if (block == nullptr) {
                return;
            }
            const uint64_t seed = threadIndex * 100U + iteration;
            FillPattern(block, size, seed);
            local.push_back(BlockRecord {block, size, seed});
        }
    });
    size_t expectedBytes = 0U;
    for (const auto &local : perThread) {
        for (const auto &record : local) {
            expectedBytes += AlignUpBytes(record.size);
            EXPECT_TRUE(PatternMatches(record.ptr, record.size, record.seed));
        }
    }
    EXPECT_EQ(EHeap::AllocatedSize() - allocatedBefore, expectedBytes);
    EXPECT_GT(expectedBytes, THREADS * BLOCKS_PER_THREAD * AlignUpBytes(8U) / 2U);
}
TEST_F(EHeapExtraThreadingTest, SerializedAlloc_threads32_blocks24)
{
    constexpr size_t THREADS = 32U;
    constexpr size_t BLOCKS_PER_THREAD = 24U;
    std::vector<std::vector<BlockRecord>> perThread(THREADS);
    const size_t allocatedBefore = EHeap::AllocatedSize();
    RunThreads(THREADS, [&perThread](size_t threadIndex) {
        auto &local = perThread[threadIndex];
        local.reserve(BLOCKS_PER_THREAD);
        for (size_t iteration = 0; iteration < BLOCKS_PER_THREAD; ++iteration) {
            const size_t size = 8U + ((threadIndex * 53U + iteration * 19U) % 256U);
            HeapMutationGuard guard;
            void *block = EHeap::Alloc(size);
            if (block == nullptr) {
                return;
            }
            const uint64_t seed = threadIndex * 100U + iteration;
            FillPattern(block, size, seed);
            local.push_back(BlockRecord {block, size, seed});
        }
    });
    size_t expectedBytes = 0U;
    for (const auto &local : perThread) {
        for (const auto &record : local) {
            expectedBytes += AlignUpBytes(record.size);
            EXPECT_TRUE(PatternMatches(record.ptr, record.size, record.seed));
        }
    }
    EXPECT_EQ(EHeap::AllocatedSize() - allocatedBefore, expectedBytes);
    EXPECT_GT(expectedBytes, THREADS * BLOCKS_PER_THREAD * AlignUpBytes(8U) / 2U);
}
TEST_F(EHeapExtraThreadingTest, BarrierPhasedAlloc_threads12)
{
    constexpr size_t THREADS = 12U;
    SpinBarrier barrier {static_cast<uint32_t>(THREADS)};
    std::vector<BlockRecord> records(THREADS);
    RunThreads(THREADS, [&barrier, &records](size_t threadIndex) {
        {
            HeapMutationGuard guard;
            const size_t size = 16U + threadIndex * 4U;
            void *block = EHeap::Alloc(size);
            if (block == nullptr) {
                return;
            }
            FillPattern(block, size, threadIndex + 9U);
            records[threadIndex] = BlockRecord {block, size, threadIndex + 9U};
        }
        barrier.Wait();
        EXPECT_TRUE(PatternMatches(records[threadIndex].ptr, records[threadIndex].size, records[threadIndex].seed));
        barrier.Wait();
    });
    EXPECT_EQ(records.size(), THREADS);
}
TEST_F(EHeapExtraThreadingTest, BarrierPhasedAlloc_threads16)
{
    constexpr size_t THREADS = 16U;
    SpinBarrier barrier {static_cast<uint32_t>(THREADS)};
    std::vector<BlockRecord> records(THREADS);
    RunThreads(THREADS, [&barrier, &records](size_t threadIndex) {
        {
            HeapMutationGuard guard;
            const size_t size = 16U + threadIndex * 4U;
            void *block = EHeap::Alloc(size);
            if (block == nullptr) {
                return;
            }
            FillPattern(block, size, threadIndex + 9U);
            records[threadIndex] = BlockRecord {block, size, threadIndex + 9U};
        }
        barrier.Wait();
        EXPECT_TRUE(PatternMatches(records[threadIndex].ptr, records[threadIndex].size, records[threadIndex].seed));
        barrier.Wait();
    });
    EXPECT_EQ(records.size(), THREADS);
}
TEST_F(EHeapExtraThreadingTest, CrossRead_16threads_256blocks)
{
    constexpr size_t THREADS = 16U;
    constexpr size_t BLOCKS = 256U;
    std::vector<BlockRecord> records;
    records.reserve(BLOCKS);
    for (size_t index = 0; index < BLOCKS; ++index) {
        const size_t size = 8U + (index * 17U) % 128U;
        void *block = EHeap::Alloc(size);
        ASSERT_NE(block, nullptr);
        FillPattern(block, size, index);
        records.push_back(BlockRecord {block, size, index});
    }
    std::atomic<size_t> verified {0U};
    RunThreads(THREADS, [&records, &verified](size_t threadIndex) {
        for (size_t index = threadIndex; index < records.size(); index += THREADS) {
            if (PatternMatches(records[index].ptr, records[index].size, records[index].seed)) {
                // Atomic with relaxed order reason: worker counter read after every worker joined
                verified.fetch_add(1U, std::memory_order_relaxed);
            }
        }
    });
    // Atomic with relaxed order reason: read after every worker joined
    EXPECT_EQ(verified.load(std::memory_order_relaxed), BLOCKS);
}
TEST_F(EHeapExtraThreadingTest, PerThreadVector_threads16)
{
    constexpr size_t THREADS = 16U;
    constexpr size_t ELEMENTS = 32U;
    std::vector<size_t> sizes(THREADS, 0U);
    RunThreads(THREADS, [&sizes](size_t threadIndex) {
        HeapMutationGuard guard;
        ArenaVector<int> values;
        for (size_t index = 0; index < ELEMENTS; ++index) {
            values.push_back(static_cast<int>(index));
        }
        sizes[threadIndex] = values.size();
    });
    for (size_t threadIndex = 0; threadIndex < THREADS; ++threadIndex) {
        EXPECT_EQ(sizes[threadIndex], ELEMENTS);
    }
}
TEST_F(EHeapExtraThreadingTest, SerializedFree_threads24)
{
    constexpr size_t THREADS = 24U;
    constexpr size_t BLOCKS_PER_THREAD = 16U;
    std::vector<BlockRecord> all;
    all.reserve(THREADS * BLOCKS_PER_THREAD);
    for (size_t index = 0; index < THREADS * BLOCKS_PER_THREAD; ++index) {
        const size_t size = 8U + (index % 40U);
        void *block = EHeap::Alloc(size);
        ASSERT_NE(block, nullptr);
        all.push_back(BlockRecord {block, size, index});
    }
    const size_t freedBefore = EHeap::FreedSize();
    RunThreads(THREADS, [&all](size_t threadIndex) {
        for (size_t index = threadIndex; index < all.size(); index += THREADS) {
            HeapMutationGuard guard;
            EHeap::Free(all[index].ptr, all[index].size);
        }
    });
    size_t expectedFreed = 0U;
    for (const auto &record : all) {
        expectedFreed += ExpectedFreedSize(record.size);
    }
    EXPECT_EQ(EHeap::FreedSize() - freedBefore, expectedFreed);
}
TEST_F(EHeapExtraThreadingTest, InterleavedAllocFree_threads16)
{
    constexpr size_t THREADS = 16U;
    constexpr size_t ROUNDS = 16U;
    const size_t freedBefore = EHeap::FreedSize();
    std::vector<size_t> freedPerThread(THREADS, 0U);
    RunThreads(THREADS, [&freedPerThread](size_t threadIndex) {
        size_t freed = 0U;
        for (size_t round = 0; round < ROUNDS; ++round) {
            const size_t size = 8U + (threadIndex * 3U + round) % 64U;
            HeapMutationGuard guard;
            void *block = EHeap::Alloc(size);
            if (block == nullptr) {
                return;
            }
            FillByte(block, size, static_cast<uint8_t>(round));
            EHeap::Free(block, size);
            freed += ExpectedFreedSize(size);
        }
        freedPerThread[threadIndex] = freed;
    });
    size_t expectedFreed = 0U;
    for (const size_t freed : freedPerThread) {
        expectedFreed += freed;
    }
    EXPECT_EQ(EHeap::FreedSize() - freedBefore, expectedFreed);
}
TEST_F(EHeapExtraThreadingTest, NestedScopesInWorkers_threads16)
{
    constexpr size_t THREADS = 16U;
    void *mainBlock = EHeap::Alloc(512U);
    ASSERT_NE(mainBlock, nullptr);
    FillPattern(mainBlock, 512U, 0x1234U);
    const size_t allocatedBefore = EHeap::AllocatedSize();
    RunThreads(THREADS, [](size_t threadIndex) {
        EHeap::Scope workerScope;
        HeapMutationGuard guard;
        const size_t size = 32U + threadIndex * 8U;
        void *block = EHeap::Alloc(size);
        if (block != nullptr) {
            FillPattern(block, size, threadIndex);
        }
    });
    size_t expectedBytes = 0U;
    for (size_t threadIndex = 0; threadIndex < THREADS; ++threadIndex) {
        expectedBytes += AlignUpBytes(32U + threadIndex * 8U);
    }
    EXPECT_EQ(EHeap::AllocatedSize() - allocatedBefore, expectedBytes);
    EXPECT_TRUE(PatternMatches(mainBlock, 512U, 0x1234U));
}
TEST_F(EHeapExtraThreadingTest, ParallelReadVerify_threads16)
{
    constexpr size_t THREADS = 16U;
    constexpr size_t BLOCKS = 256U;
    std::vector<BlockRecord> records;
    records.reserve(BLOCKS);
    for (size_t index = 0; index < BLOCKS; ++index) {
        const size_t size = 4U + (index % 96U);
        void *block = EHeap::Alloc(size);
        ASSERT_NE(block, nullptr);
        FillByte(block, size, static_cast<uint8_t>(index & 0xFFU));
        records.push_back(BlockRecord {block, size, index & 0xFFU});
    }
    std::atomic<size_t> mismatches {0U};
    RunThreads(THREADS, [&records, &mismatches](size_t threadIndex) {
        for (size_t index = threadIndex; index < BLOCKS; index += THREADS) {
            if (!AllBytes(records[index].ptr, records[index].size, static_cast<uint8_t>(records[index].seed))) {
                // Atomic with relaxed order reason: worker counter read after every worker joined
                mismatches.fetch_add(1U, std::memory_order_relaxed);
            }
        }
    });
    // Atomic with relaxed order reason: read after every worker joined
    EXPECT_EQ(mismatches.load(std::memory_order_relaxed), 0U);
}
// ---------------------------------------------------------------------------
// ArenaStack / ArenaQueue with an explicit underlying container
// ---------------------------------------------------------------------------
class EHeapCustomUnderlyingContainerTest : public EHeapTest {};
TEST_F(EHeapCustomUnderlyingContainerTest, VectorBackedStack)
{
    ArenaStack<int, ArenaVector<int>> stack;
    for (int value = 0; value < 24; ++value) {
        stack.push(value);
    }
    EXPECT_EQ(stack.size(), 24U);
    EXPECT_EQ(stack.top(), 23);
    int sum = 0;
    while (!stack.empty()) {
        sum += stack.top();
        stack.pop();
    }
    EXPECT_EQ(sum, 24 * 23 / 2);
}
TEST_F(EHeapCustomUnderlyingContainerTest, ListBackedStack)
{
    ArenaStack<int, ArenaList<int>> stack;
    for (int value = 0; value < 24; ++value) {
        stack.push(value);
    }
    EXPECT_EQ(stack.size(), 24U);
    EXPECT_EQ(stack.top(), 23);
    stack.pop();
    EXPECT_EQ(stack.top(), 22);
}
TEST_F(EHeapCustomUnderlyingContainerTest, ListBackedQueue)
{
    ArenaQueue<int, ArenaList<int>> queue;
    for (int value = 0; value < 24; ++value) {
        queue.push(value);
    }
    EXPECT_EQ(queue.size(), 24U);
    EXPECT_EQ(queue.front(), 0);
    EXPECT_EQ(queue.back(), 23);
    queue.pop();
    EXPECT_EQ(queue.front(), 1);
}
TEST_F(EHeapCustomUnderlyingContainerTest, DequeBackedQueue)
{
    ArenaQueue<int, ArenaDeque<int>> queue;
    for (int value = 0; value < 24; ++value) {
        queue.push(value);
    }
    int expected = 0;
    while (!queue.empty()) {
        EXPECT_EQ(queue.front(), expected);
        queue.pop();
        ++expected;
    }
    EXPECT_EQ(expected, 24);
}
TEST_F(EHeapCustomUnderlyingContainerTest, PointerValuesInMap)
{
    ArenaMap<int, EPtr<int>> values;
    for (int key = 0; key < 16; ++key) {
        auto *storage = EAllocator::New<int>(key * key);
        ASSERT_NE(storage, nullptr);
        values.emplace(key, EPtr<int> {storage});
    }
    EXPECT_EQ(values.size(), 16U);
    EXPECT_EQ(*values.at(7), 49);
    EXPECT_EQ(*values.at(15), 225);
}
// ---------------------------------------------------------------------------
// Large scale accounting
// ---------------------------------------------------------------------------
class EHeapScaleTest : public EHeapTest {};
TEST_F(EHeapScaleTest, ManySmallAllocations_01024)
{
    constexpr size_t COUNT = 1024U;
    const size_t before = EHeap::AllocatedSize();
    size_t expected = 0U;
    for (size_t index = 0; index < COUNT; ++index) {
        const size_t size = 1U + (index % 64U);
        void *block = EHeap::Alloc(size);
        ASSERT_NE(block, nullptr);
        expected += AlignUpBytes(size);
    }
    EXPECT_EQ(EHeap::AllocatedSize() - before, expected);
    EXPECT_EQ(expected % ALLOC_ALIGNMENT, 0U);
}
TEST_F(EHeapScaleTest, ManySmallAllocations_04096)
{
    constexpr size_t COUNT = 4096U;
    const size_t before = EHeap::AllocatedSize();
    size_t expected = 0U;
    for (size_t index = 0; index < COUNT; ++index) {
        const size_t size = 1U + (index % 64U);
        void *block = EHeap::Alloc(size);
        ASSERT_NE(block, nullptr);
        expected += AlignUpBytes(size);
    }
    EXPECT_EQ(EHeap::AllocatedSize() - before, expected);
    EXPECT_EQ(expected % ALLOC_ALIGNMENT, 0U);
}
TEST_F(EHeapScaleTest, ManyMediumAllocations)
{
    constexpr size_t COUNT = 256U;
    const size_t before = EHeap::AllocatedSize();
    size_t expected = 0U;
    for (size_t index = 0; index < COUNT; ++index) {
        const size_t size = 1024U + index * 8U;
        void *block = EHeap::Alloc(size);
        ASSERT_NE(block, nullptr);
        FillByte(block, size, static_cast<uint8_t>(index));
        expected += AlignUpBytes(size);
    }
    EXPECT_EQ(EHeap::AllocatedSize() - before, expected);
}
TEST_F(EHeapScaleTest, AllocatorNewManyObjects)
{
    constexpr size_t COUNT = 512U;
    std::vector<ValueProbe *> probes;
    probes.reserve(COUNT);
    for (size_t index = 0; index < COUNT; ++index) {
        auto *probe = EAllocator::New<ValueProbe>(static_cast<int>(index), -static_cast<int>(index));
        ASSERT_NE(probe, nullptr);
        probes.push_back(probe);
    }
    for (size_t index = 0; index < COUNT; ++index) {
        EXPECT_EQ(probes[index]->first, static_cast<int>(index));
        EXPECT_EQ(probes[index]->second, -static_cast<int>(index));
    }
}
// ---------------------------------------------------------------------------
// Death tests: exhausting the 32 GiB reservation must abort with a message
// ---------------------------------------------------------------------------
class EHeapDeathTest : public EHeapTest {};
TEST_F(EHeapDeathTest, HugeAllocationAborts)
{
    EXPECT_DEATH(
        {
            EHeap::Scope scope;
            [[maybe_unused]] void *block = EHeap::Alloc(HUGE_ALLOCATION_SIZE);
        },
        "out of memory");
}
TEST_F(EHeapDeathTest, HugeAllocationAfterSmallOnesAborts)
{
    for (size_t index = 0; index < 32U; ++index) {
        ASSERT_NE(EHeap::Alloc(index * 16U), nullptr);
    }
    EXPECT_DEATH(
        {
            EHeap::Scope scope;
            [[maybe_unused]] void *block = EHeap::Alloc(HUGE_ALLOCATION_SIZE);
        },
        "out of memory");
}
TEST_F(EHeapDeathTest, HugeAllocationInNestedScopeAborts)
{
    EXPECT_DEATH(
        {
            EHeap::Scope nested;
            [[maybe_unused]] void *block = EHeap::Alloc(HUGE_ALLOCATION_SIZE);
        },
        "out of memory");
}
TEST_F(EHeapDeathTest, HugeAllocationAfterZeroSizedAllocationAborts)
{
    [[maybe_unused]] void *empty = EHeap::Alloc(0U);
    EXPECT_DEATH({ [[maybe_unused]] void *block = EHeap::Alloc(HUGE_ALLOCATION_SIZE); }, "out of memory");
}
TEST_F(EHeapDeathTest, ParentHeapSurvivesChildAbort)
{
    EXPECT_DEATH(
        {
            EHeap::Scope scope;
            [[maybe_unused]] void *block = EHeap::Alloc(HUGE_ALLOCATION_SIZE);
        },
        "out of memory");
    void *block = EHeap::Alloc(128U);
    ASSERT_NE(block, nullptr);
    FillPattern(block, 128U, 0xBEEFU);
    EXPECT_TRUE(PatternMatches(block, 128U, 0xBEEFU));
    EXPECT_TRUE(IsAligned(block));
}
TEST_F(EHeapDeathTest, HugeAllocationFromAllocatorAborts)
{
    EXPECT_DEATH(
        {
            EHeap::Scope scope;
            [[maybe_unused]] void *block = EAllocator::Alloc(HUGE_ALLOCATION_SIZE);
        },
        "out of memory");
}

}  // namespace
}  // namespace ark::es2panda

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
#include "public/public.h"
#include <gtest/gtest.h>
#include <gtest/gtest-spi.h>
#include <array>
#include <cstring>
#include <cstdlib>
#include <new>
#include <random>
#include <string>
#include <vector>
class AllocationTracker;
static thread_local AllocationTracker *g_activeTracker = nullptr;

struct AllocationState {
    size_t liveCount = 0;
    size_t liveBytes = 0;
    size_t allocations = 0;
    size_t deallocations = 0;
    bool overflow = false;
};
constexpr size_t NON_SSO_STRING_LENGTH = 4096;
constexpr size_t TRACKER_SAMPLE_BYTES = 64;
constexpr size_t ALLOCATION_TRACKER_CAPACITY = 1024;

class AllocationTracker {
    struct Entry {
        void *address;
        size_t bytes;
    };
    std::array<Entry, ALLOCATION_TRACKER_CAPACITY> entries_ {};
    AllocationState state_;
    bool recording_ = false;

public:
    const AllocationState &State() const
    {
        return state_;
    }
    void SetRecording(bool enabled)
    {
        recording_ = enabled;
    }
    AllocationTracker()
    {
        if (g_activeTracker != nullptr) {
            ADD_FAILURE() << "Nested allocation tracking is not supported";
            return;
        }
        g_activeTracker = this;
    }
    ~AllocationTracker()
    {
        if (g_activeTracker == this) {
            g_activeTracker = nullptr;
        }
    }
    AllocationTracker(const AllocationTracker &) = delete;
    AllocationTracker &operator=(const AllocationTracker &) = delete;
    AllocationTracker(AllocationTracker &&) = delete;
    AllocationTracker &operator=(AllocationTracker &&) = delete;

    bool Contains(const void *address) const
    {
        for (size_t i = 0; i < state_.liveCount; ++i) {
            if (entries_[i].address == address) {
                return true;
            }
        }
        return false;
    }
    void OnAllocate(void *address, size_t bytes)
    {
        if (!recording_) {
            return;
        }
        if (state_.liveCount == entries_.size()) {
            state_.overflow = true;
            return;
        }
        entries_[state_.liveCount++] = {address, bytes};
        state_.liveBytes += bytes;
        ++state_.allocations;
    }
    void OnDeallocate(void *address)
    {
        for (size_t i = 0; i < state_.liveCount; ++i) {
            if (entries_[i].address == address) {
                state_.liveBytes -= entries_[i].bytes;
                entries_[i] = entries_[--state_.liveCount];
                ++state_.deallocations;
                return;
            }
        }
    }
};

void *operator new(std::size_t size)
{
    // Allocator replacement must not recurse through a container or smart pointer.
    // NOLINTNEXTLINE(cppcoreguidelines-no-malloc)
    auto *result = std::malloc(size == 0 ? 1 : size);
    if (result == nullptr) {
        // Match the throwing allocation contract used by this gtest target.
        throw std::bad_alloc();
    }
    if (g_activeTracker != nullptr) {
        g_activeTracker->OnAllocate(result, size);
    }
    return result;
}

void *operator new[](std::size_t size)
{
    return ::operator new(size);
}

void operator delete(void *address) noexcept
{
    if (g_activeTracker != nullptr) {
        g_activeTracker->OnDeallocate(address);
    }
    // NOLINTNEXTLINE(cppcoreguidelines-no-malloc)
    std::free(address);
}

void operator delete[](void *address) noexcept
{
    ::operator delete(address);
}

void operator delete(void *address, [[maybe_unused]] std::size_t size) noexcept
{
    ::operator delete(address);
}

void operator delete[](void *address, [[maybe_unused]] std::size_t size) noexcept
{
    ::operator delete(address);
}

using ark::es2panda::public_lib::ApiResultStorage;
using ark::es2panda::public_lib::CopyApiPointerResult;

// Collision-forcing hashes exercise the full-content-equality path; no security use.
static size_t ConstantStringHash(std::string_view)
{
    return 0;
}
static size_t ConstantPointerHash(size_t)
{
    return 0;
}
template <typename T>
static T *CopyPointers(ApiResultStorage &store, size_t count, const T *values)
{
    return CopyApiPointerResult<T>(store, count, [values](size_t index) { return values[index]; });
}

template <typename T>
static void CheckTypedNulls(ApiResultStorage &store, size_t count, std::vector<void *> &buffers)
{
    auto read = [](size_t) -> T { return nullptr; };
    auto *result = CopyApiPointerResult<T>(store, count, read);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(CopyApiPointerResult<T>(store, count, read), result);
    for (auto *previous : buffers) {
        EXPECT_NE(static_cast<void *>(result), previous);
    }
    buffers.push_back(result);
    for (size_t i = 0; i < count; ++i) {
        EXPECT_EQ(result[i], nullptr);
    }
}

TEST(ApiResultProperties, WB01_StringCollisionsCompareCompleteBytes)
{
    ApiResultStorage store(ConstantStringHash, ConstantPointerHash);
    std::vector<std::string> values {"", "a", "ab", "ba", std::string("a\0b", 3), "状态_é", std::string(4096, 'z')};
    std::vector<char *> saved;
    saved.reserve(values.size());
    for (const auto &s : values) {
        saved.push_back(store.String(s));
    }
    for (size_t repeat = 0; repeat < 3; ++repeat) {
        for (size_t i = 0; i < values.size(); ++i) {
            auto *actual = store.String(values[i]);
            EXPECT_EQ(std::string_view(actual, values[i].size()), values[i]);
            EXPECT_EQ(actual[values[i].size()], '\0');
            EXPECT_EQ(std::string_view(saved[i], values[i].size()), values[i]);
        }
    }
}

TEST(ApiResultProperties, WB02_ArrayCollisionsRespectSequenceAndLength)
{
    ApiResultStorage store(ConstantStringHash, ConstantPointerHash);
    char a = 0;
    char b = 0;
    const std::vector<std::vector<char *>> models {{},       {&a},     {&b},     {nullptr},
                                                   {&a, &b}, {&b, &a}, {&a, &a}, {&a, nullptr, &b}};
    std::vector<char **> saved;
    for (const auto &model : models) {
        auto *result = CopyPointers(store, model.size(), model.data());
        ASSERT_NE(result, nullptr);
        saved.push_back(result);
        EXPECT_TRUE(std::equal(model.begin(), model.end(), result));
    }
    EXPECT_NE(saved[0], saved[3]);  // Empty and {nullptr} have different logical lengths.
    for (size_t i = 0; i < models.size(); ++i) {
        auto *result = CopyPointers(store, models[i].size(), models[i].data());
        EXPECT_EQ(result, saved[i]);
        EXPECT_TRUE(std::equal(models[i].begin(), models[i].end(), result));
        EXPECT_TRUE(std::equal(models[i].begin(), models[i].end(), saved[i]));
    }
}

TEST(ApiResultProperties, WB03_ActualCapiTypesUseIndependentTypedBuffers)
{
    ApiResultStorage store(ConstantStringHash, ConstantPointerHash);
    for (size_t count : {0U, 1U, 3U}) {
        std::vector<void *> buffers;
        CheckTypedNulls<char *>(store, count, buffers);
        CheckTypedNulls<es2panda_AstNode *>(store, count, buffers);
        CheckTypedNulls<es2panda_Type *>(store, count, buffers);
        CheckTypedNulls<es2panda_Signature *>(store, count, buffers);
        CheckTypedNulls<es2panda_Variable *>(store, count, buffers);
        CheckTypedNulls<es2panda_Declaration *>(store, count, buffers);
        CheckTypedNulls<es2panda_Scope *>(store, count, buffers);
    }
}

TEST(ApiResultProperties, WB04_TableGrowthPreservesOldBuffers)
{
    ApiResultStorage store;
    std::array<char, 4096> nodes {};
    auto *name = store.String("original");
    std::vector<char *> initial {&nodes[0], &nodes[1], &nodes[2]};
    auto *array = CopyPointers(store, initial.size(), initial.data());
    for (size_t i = 0; i < nodes.size(); ++i) {
        store.String("name_" + std::to_string(i));
        char *single = &nodes[i];
        auto *next = CopyPointers(store, 1, &single);
        EXPECT_EQ(next[0], &nodes[i]);
    }
    EXPECT_STREQ(name, "original");
    for (size_t i = 0; i < initial.size(); ++i) {
        EXPECT_EQ(array[i], &nodes[i]);
    }
}

TEST(ApiResultProperties, WB05_ClearReleasesOwnedCopies)
{
    // Source buffers and assertion allocations are outside the recording window.
    std::string value(NON_SSO_STRING_LENGTH, 'x');
    std::array<char *, 3> values {};
    ApiResultStorage store;
    AllocationTracker tracker;
    tracker.SetRecording(true);
    auto *name = store.String(value);
    auto *array = CopyPointers(store, values.size(), values.data());
    tracker.SetRecording(false);
    ASSERT_TRUE(tracker.Contains(name));
    ASSERT_TRUE(tracker.Contains(array));
    ASSERT_FALSE(tracker.State().overflow);
    ASSERT_GT(tracker.State().liveCount, 0U);
    ASSERT_GT(tracker.State().liveBytes, value.size());
    auto addArray = [&store, &tracker](auto element) {
        using Element = decltype(element);
        std::array<Element, 3> elements {};
        tracker.SetRecording(true);
        auto *copy = CopyPointers(store, elements.size(), elements.data());
        auto *empty = CopyPointers(store, 0, elements.data());
        tracker.SetRecording(false);
        ASSERT_TRUE(tracker.Contains(copy));
        ASSERT_TRUE(tracker.Contains(empty));
    };
    addArray(static_cast<char *>(nullptr));
    addArray(static_cast<es2panda_AstNode *>(nullptr));
    addArray(static_cast<es2panda_Type *>(nullptr));
    addArray(static_cast<es2panda_Signature *>(nullptr));
    addArray(static_cast<es2panda_Variable *>(nullptr));
    addArray(static_cast<es2panda_Declaration *>(nullptr));
    addArray(static_cast<es2panda_Scope *>(nullptr));
    ASSERT_FALSE(tracker.State().overflow);
    store.Clear();
    EXPECT_EQ(tracker.State().liveCount, 0U);
    EXPECT_EQ(tracker.State().liveBytes, 0U);
    EXPECT_EQ(tracker.State().allocations, tracker.State().deallocations);
    auto released = tracker.State().deallocations;
    store.Clear();
    EXPECT_EQ(tracker.State().deallocations, released);
    EXPECT_STREQ(store.String("new"), "new");
}

TEST(ApiResultProperties, DestructorReleasesOwnedCopies)
{
    std::string value(NON_SSO_STRING_LENGTH, 'x');
    AllocationTracker tracker;
    {
        ApiResultStorage store;
        tracker.SetRecording(true);
        auto *name = store.String(value);
        tracker.SetRecording(false);
        ASSERT_TRUE(tracker.Contains(name));
        ASSERT_GT(tracker.State().liveCount, 0U);
    }
    EXPECT_FALSE(tracker.State().overflow);
    EXPECT_EQ(tracker.State().liveCount, 0U);
    EXPECT_EQ(tracker.State().liveBytes, 0U);
    EXPECT_EQ(tracker.State().allocations, tracker.State().deallocations);
}

TEST(ApiResultProperties, AllocationTrackerDetectsOutstandingAllocation)
{
    AllocationTracker tracker;
    EXPECT_NONFATAL_FAILURE(AllocationTracker {}, "Nested allocation tracking is not supported");
    EXPECT_EQ(g_activeTracker, &tracker);
    tracker.SetRecording(true);
    auto *allocation = ::operator new(TRACKER_SAMPLE_BYTES);
    tracker.SetRecording(false);
    EXPECT_EQ(tracker.State().liveCount, 1U);
    EXPECT_EQ(tracker.State().liveBytes, TRACKER_SAMPLE_BYTES);
    ::operator delete(allocation);
    EXPECT_EQ(tracker.State().liveCount, 0U);
    EXPECT_EQ(tracker.State().liveBytes, 0U);
    EXPECT_EQ(tracker.State().deallocations, 1U);
}

TEST(ApiResultProperties, WB06_DeterministicModelAndHistory)
{
    ApiResultStorage store(ConstantStringHash, ConstantPointerHash);
    std::array<char, 8> nodes {};
    // Fixed-seed model data for reproducible tests; no security use.
    std::mt19937 testGenerator(20261013U);
    std::vector<std::string> names;
    std::vector<char *> savedNames;
    std::vector<std::vector<char *>> models;
    std::vector<char **> savedArrays;
    for (size_t step = 0; step < 256; ++step) {
        std::string name;
        for (size_t j = 0, n = testGenerator() % 33; j < n; ++j) {
            name.push_back(static_cast<char>(testGenerator() % 128));
        }
        names.push_back(name);
        savedNames.push_back(store.String(name));
        std::vector<char *> model;
        for (size_t j = 0, n = testGenerator() % 9; j < n; ++j) {
            auto k = testGenerator() % 9;
            model.push_back(k == 8 ? nullptr : &nodes[k]);
        }
        models.push_back(model);
        savedArrays.push_back(CopyPointers(store, model.size(), model.data()));
    }
    for (size_t i = 0; i < names.size(); ++i) {
        EXPECT_EQ(std::string_view(savedNames[i], names[i].size()), names[i]);
        EXPECT_EQ(savedNames[i][names[i].size()], '\0');
        EXPECT_TRUE(std::equal(models[i].begin(), models[i].end(), savedArrays[i]));
    }
}

TEST(ApiResultProperties, WB07_StringArrayReadersMayPopulateStringStorage)
{
    // GetUserPaths converts each StringView through the same context's string storage
    // while reading an array; repeated reads must stay stable across that conversion.
    ApiResultStorage store(ConstantStringHash, ConstantPointerHash);
    std::array<std::string, 3> names {"", "state_value1", "状态"};
    auto read = [&store, &names](size_t index) { return store.String(names[index]); };
    auto *original = CopyApiPointerResult<char *>(store, names.size(), read);
    EXPECT_EQ(CopyApiPointerResult<char *>(store, names.size(), read), original);
    names[1] = "state_value2";
    auto *changed = CopyApiPointerResult<char *>(store, names.size(), read);
    EXPECT_NE(changed, original);
    EXPECT_STREQ(original[0], "");
    EXPECT_STREQ(original[1], "state_value1");
    EXPECT_STREQ(original[2], "状态");
    EXPECT_STREQ(changed[1], "state_value2");
    names[1] = "state_value1";
    EXPECT_EQ(CopyApiPointerResult<char *>(store, names.size(), read), original);
}

TEST(ApiResultProperties, WB08_ClearRecreatesStorageWithOriginalHashOverrides)
{
    static size_t stringHashCalls;
    static size_t pointerHashCalls;
    stringHashCalls = 0;
    pointerHashCalls = 0;
    ApiResultStorage store(
        [](std::string_view) {
            ++stringHashCalls;
            return size_t {0};
        },
        [](size_t) {
            ++pointerHashCalls;
            return size_t {0};
        });
    auto read = [](size_t) -> char * { return nullptr; };
    store.String("before");
    CopyApiPointerResult<char *>(store, 1, read);
    store.Clear();
    store.Clear();
    const auto previousStringCalls = stringHashCalls;
    const auto previousPointerCalls = pointerHashCalls;
    EXPECT_STREQ(store.String("after"), "after");
    EXPECT_EQ(CopyApiPointerResult<char *>(store, 1, read)[0], nullptr);
    EXPECT_GT(stringHashCalls, previousStringCalls);
    EXPECT_GT(pointerHashCalls, previousPointerCalls);
}

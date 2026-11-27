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

#include <algorithm>
#include <cstdint>
#include <tuple>

#include "gtest/gtest.h"
#include "util/stableCodeMap.h"

using ark::es2panda::diagnostic::StableCodeEntry;
using ark::es2panda::diagnostic::StableCodeMap;

namespace {

// Frozen registry anchors: codes are append-only and yaml ids are frozen,
// so these lookups are part of the registry contract. Update them only
// when the corresponding diagnostics are consciously retired.
constexpr uint32_t OPEN_FAILED_ID = 2U;            // fatal
constexpr uint32_t ABSTRACT_CALL_ID = 26U;         // semantic
constexpr uint32_t EXPORTED_ENTITIES_ID = 71336U;  // strict warning + mirror

constexpr auto SEMANTIC = StableCodeMap::Category::SEMANTIC;
constexpr auto WARNING = StableCodeMap::Category::WARNING;
constexpr auto FATAL = StableCodeMap::Category::FATAL;

bool Less(const StableCodeEntry &lhs, const StableCodeEntry &rhs)
{
    return std::tie(lhs.category, lhs.yamlId) < std::tie(rhs.category, rhs.yamlId);
}

const StableCodeEntry *FindEntry(StableCodeMap::Category category, uint32_t yamlId)
{
    for (const auto &entry : ark::es2panda::diagnostic::STABLE_CODE_MAP) {
        if (entry.category == static_cast<uint8_t>(category) && entry.yamlId == yamlId) {
            return &entry;
        }
    }
    return nullptr;
}

}  // namespace

TEST(StableCodeMap, LookupAnchors)
{
    EXPECT_EQ(StableCodeMap::GetCode(FATAL, OPEN_FAILED_ID), 17U);
    EXPECT_EQ(StableCodeMap::GetCode(SEMANTIC, ABSTRACT_CALL_ID), 1U);
}

TEST(StableCodeMap, StrictWarningIsDualRegistered)
{
    // The warning side and the semantic mirror share the yaml id and both
    // carry the strict marker; each side has its own stable code.
    const auto *warningEntry = FindEntry(WARNING, EXPORTED_ENTITIES_ID);
    const auto *mirrorEntry = FindEntry(SEMANTIC, EXPORTED_ENTITIES_ID);
    ASSERT_NE(warningEntry, nullptr);
    ASSERT_NE(mirrorEntry, nullptr);
    EXPECT_TRUE(warningEntry->strict);
    EXPECT_TRUE(mirrorEntry->strict);
    EXPECT_NE(warningEntry->code, mirrorEntry->code);

    EXPECT_EQ(StableCodeMap::GetCode(WARNING, EXPORTED_ENTITIES_ID), warningEntry->code);
    EXPECT_EQ(StableCodeMap::GetCode(SEMANTIC, EXPORTED_ENTITIES_ID), mirrorEntry->code);
    EXPECT_TRUE(StableCodeMap::IsStrict(SEMANTIC, EXPORTED_ENTITIES_ID));
    EXPECT_FALSE(StableCodeMap::IsStrict(SEMANTIC, ABSTRACT_CALL_ID));
}

TEST(StableCodeMap, UnknownPairsReportZero)
{
    EXPECT_EQ(StableCodeMap::GetCode(WARNING, 99999999U), 0U);
    EXPECT_EQ(StableCodeMap::GetCode(FATAL, 99999999U), 0U);
    EXPECT_FALSE(StableCodeMap::Contains(SEMANTIC, 99999999U));
    EXPECT_FALSE(StableCodeMap::IsStrict(WARNING, 99999999U));
}

TEST(StableCodeMap, TableInvariants)
{
    const auto &table = ark::es2panda::diagnostic::STABLE_CODE_MAP;

    EXPECT_EQ(StableCodeMap::Size(), table.size());
    EXPECT_GT(table.size(), 0U);

    // Sorted by (category, yamlId), which the binary search relies on.
    EXPECT_TRUE(std::is_sorted(table.begin(), table.end(), Less));

    for (const auto &entry : table) {
        EXPECT_LT(entry.category, 8U);
        EXPECT_GE(entry.code, 1U);
    }

    // Dual-registration invariant: every strict entry has a counterpart of
    // the other side (warning <-> semantic mirror) with the same yaml id.
    for (const auto &entry : table) {
        if (!entry.strict) {
            continue;
        }
        const auto other = entry.category == static_cast<uint8_t>(WARNING) ? SEMANTIC : WARNING;
        const auto *counterpart = FindEntry(other, entry.yamlId);
        ASSERT_NE(counterpart, nullptr) << "strict entry yamlId " << entry.yamlId << " lacks its counterpart";
        EXPECT_TRUE(counterpart->strict);
        EXPECT_NE(counterpart->code, entry.code);
    }
}

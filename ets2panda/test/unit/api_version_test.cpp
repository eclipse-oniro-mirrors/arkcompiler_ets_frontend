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

#include "gtest/gtest.h"
#include "libarkfile/include/file_format_version.h"
#include "util/apiVersion.h"

using ark::es2panda::api_version::API_24;
using ark::es2panda::api_version::API_26;
using ark::es2panda::api_version::ApiFeature;
using ark::es2panda::api_version::METADATA;
using ark::es2panda::api_version::MIN_SUPPORTED_API_VERSION;

// ─── ApiFeature::AllowedInVersion ───

TEST(ApiFeatureTest, AllowedInVersion_AtMinVersion_ReturnsTrue)
{
    constexpr ApiFeature feature {24, "test feature"};
    ASSERT_TRUE(feature.AllowedInVersion(24));
}

TEST(ApiFeatureTest, AllowedInVersion_AboveMinVersion_ReturnsTrue)
{
    constexpr ApiFeature feature {24, "test feature"};
    ASSERT_TRUE(feature.AllowedInVersion(26));
}

TEST(ApiFeatureTest, AllowedInVersion_BelowMinVersion_ReturnsFalse)
{
    constexpr ApiFeature feature {24, "test feature"};
    ASSERT_FALSE(feature.AllowedInVersion(18));
}

// ─── Metadata feature gating ───

TEST(MetadataFeatureTest, MetadataAllowed_Api26)
{
    ASSERT_TRUE(METADATA.AllowedInVersion(API_26));
}

TEST(MetadataFeatureTest, MetadataDisallowed_Api24)
{
    ASSERT_FALSE(METADATA.AllowedInVersion(API_24));
}

TEST(MetadataFeatureTest, MetadataDisallowed_BelowApi26)
{
    ASSERT_FALSE(METADATA.AllowedInVersion(18));
}

// ─── Version constants ───

TEST(ApiVersionConstantsTest, ApiVersionOrdering)
{
    ASSERT_LT(API_24, API_26);
}

TEST(ApiVersionConstantsTest, MinSupportedIsNotGreaterThanApi24)
{
    ASSERT_LE(MIN_SUPPORTED_API_VERSION, API_24);
}

// ─── Bytecode version mapping (isa.yaml) ───

TEST(BytecodeVersionMappingTest, Api24_MapsTo0006)
{
    auto bcVersion = ark::panda_file::GetVersionByApi(API_24);
    ASSERT_TRUE(bcVersion.has_value());
    const auto &v = bcVersion.value();
    EXPECT_EQ(v[0], 0U);
    EXPECT_EQ(v[1], 0U);
    EXPECT_EQ(v[2], 0U);
    EXPECT_EQ(v[3], 6U);
}

TEST(BytecodeVersionMappingTest, Api26_MapsTo0107)
{
    auto bcVersion = ark::panda_file::GetVersionByApi(API_26);
    ASSERT_TRUE(bcVersion.has_value());
    const auto &v = bcVersion.value();
    EXPECT_EQ(v[0], 0U);
    EXPECT_EQ(v[1], 1U);
    EXPECT_EQ(v[2], 0U);
    EXPECT_EQ(v[3], 7U);
}

TEST(BytecodeVersionMappingTest, UnknownApi_ReturnsLatest)
{
    auto bcVersion = ark::panda_file::GetVersionByApi(99);
    ASSERT_TRUE(bcVersion.has_value());
}

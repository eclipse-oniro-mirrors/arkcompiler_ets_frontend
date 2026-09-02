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

#include "generated/code_fix_register.h"
#include "lsp_api_test.h"
#include "util/diagnostic.h"

#include <gtest/gtest.h>

#include "lsp/include/api.h"
#include "lsp/include/cancellation_token.h"
#include "lsp/include/register_code_fix/fix_spelling_for_type.h"
#include "lsp/include/symbol_reference_index.h"

namespace {

using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::codefixes::FIX_SPELLING_FOR_TYPE;

constexpr std::string_view EXPECTED_FIX_NAME = FIX_SPELLING_FOR_TYPE.GetFixId();
constexpr auto ERROR_CODES = FIX_SPELLING_FOR_TYPE.GetSupportedCodeNumbers();
// TYPE_NOT_FOUND: DiagnosticType::SEMANTIC * DIAGNOSTIC_CODE_MULTIPLIER + 371
constexpr int TYPE_NOT_FOUND_CODE = 2371;
constexpr int DEFAULT_THROTTLE = 20;

class FixSpellingForTypeTests2 : public LSPAPITests {
public:
    static ark::es2panda::lsp::CancellationToken CreateNonCancellationToken()
    {
        return ark::es2panda::lsp::CancellationToken(DEFAULT_THROTTLE, &GetNullHost());
    }

    static std::vector<CodeFixActionInfo> GetFixes(es2panda_Context *context, size_t start, size_t length)
    {
        std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
        CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
        return ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, options);
    }

    static const CodeFixActionInfo *FindSpellingFix(const std::vector<CodeFixActionInfo> &fixes)
    {
        for (const auto &fix : fixes) {
            if (fix.fixName_ == EXPECTED_FIX_NAME) {
                return &fix;
            }
        }
        return nullptr;
    }

    static void ValidateSingleReplacement(const CodeFixActionInfo &fix, const std::string &fileName,
                                          size_t expectedStart, size_t expectedLength, const std::string &expectedText)
    {
        ASSERT_EQ(fix.fixName_, EXPECTED_FIX_NAME);
        ASSERT_EQ(fix.fixId_, EXPECTED_FIX_NAME);
        ASSERT_EQ(fix.description_, "Did you mean '" + expectedText + "'?");
        ASSERT_EQ(fix.changes_.size(), 1U);
        ASSERT_EQ(fix.changes_[0].fileName, fileName);
        ASSERT_EQ(fix.changes_[0].textChanges.size(), 1U);
        EXPECT_EQ(fix.changes_[0].textChanges[0].span.start, expectedStart);
        EXPECT_EQ(fix.changes_[0].textChanges[0].span.length, expectedLength);
        EXPECT_EQ(fix.changes_[0].textChanges[0].newText, expectedText);
    }

    void SetUp() override
    {
        LSPAPITests::SetUp();
        ark::es2panda::lsp::ClearSymbolReferenceIndex();
    }

    void TearDown() override
    {
        ark::es2panda::lsp::ClearSymbolReferenceIndex();
    }

private:
    class NullCancellationToken : public ark::es2panda::lsp::HostCancellationToken {
    public:
        bool IsCancellationRequested() override
        {
            return false;
        }
    };

    static NullCancellationToken &GetNullHost()
    {
        static NullCancellationToken instance;
        return instance;
    }
};

// Plan: type alias misspelling should suggest the alias name
TEST_F(FixSpellingForTypeTests2, SuggestsTypeAliasName)
{
    const std::string source = R"(
type UserIdentifier = string;
let userId: UserIdentfier;
)";
    auto filePaths = CreateTempFile({"suggest_type_alias.ets"}, {source});
    ASSERT_EQ(filePaths.size(), 1U);

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);
    ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(context));
    ASSERT_EQ(ERROR_CODES.size(), 1U);
    ASSERT_EQ(ERROR_CODES[0], TYPE_NOT_FOUND_CODE);

    const size_t start = source.find("UserIdentfier");
    ASSERT_NE(start, std::string::npos);
    auto fixes = GetFixes(context, start, std::string("UserIdentfier").size());
    initializer.DestroyContext(context);

    const auto *fix = FindSpellingFix(fixes);
    ASSERT_NE(fix, nullptr);
    ValidateSingleReplacement(*fix, filePaths[0], start, std::string("UserIdentfier").size(), "UserIdentifier");
}

// Plan: namespace name misspelling should suggest the namespace name
TEST_F(FixSpellingForTypeTests2, SuggestsNamespaceName)
{
    const std::string source = R"(
namespace GeometryUtils {
    export class Point {
        x: number = 0;
    }
}
let point: GeometryUtls.Point;
)";
    auto filePaths = CreateTempFile({"suggest_namespace_name.ets"}, {source});
    ASSERT_EQ(filePaths.size(), 1U);

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);
    ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(context));

    const size_t start = source.find("GeometryUtls");
    ASSERT_NE(start, std::string::npos);
    auto fixes = GetFixes(context, start, std::string("GeometryUtls").size());
    initializer.DestroyContext(context);

    const auto *fix = FindSpellingFix(fixes);
    ASSERT_NE(fix, nullptr);
    ValidateSingleReplacement(*fix, filePaths[0], start, std::string("GeometryUtls").size(), "GeometryUtils");
}

// Plan: generic type misspelling in a type argument position should suggest the generic class name
TEST_F(FixSpellingForTypeTests2, SuggestsGenericTypeName)
{
    const std::string source = R"(
class GenericContainer<T> {
    value: T;
}
let box: GenericContaner<number>;
)";
    auto filePaths = CreateTempFile({"suggest_generic_type.ets"}, {source});
    ASSERT_EQ(filePaths.size(), 1U);

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);
    ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(context));

    const size_t start = source.find("GenericContaner");
    ASSERT_NE(start, std::string::npos);
    auto fixes = GetFixes(context, start, std::string("GenericContaner").size());
    initializer.DestroyContext(context);

    const auto *fix = FindSpellingFix(fixes);
    ASSERT_NE(fix, nullptr);
    ValidateSingleReplacement(*fix, filePaths[0], start, std::string("GenericContaner").size(), "GenericContainer");
}

// Plan: imported type participates in candidates; the change is reported on the importing file path
TEST_F(FixSpellingForTypeTests2, SuggestsImportedTypeWithCorrectFilePath)
{
    std::vector<std::string> fileNames = {"spell_types_export.ets", "spell_types_import.ets"};
    const std::string importSource = R"(
import { PaymentGateway } from './spell_types_export';
let gateway: PaymentGatewy;
)";
    std::vector<std::string> fileContents = {R"(
export class PaymentGateway {
    url: string = "";
}
)",
                                             importSource};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileNames.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);
    ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(context));

    const size_t start = importSource.find("PaymentGatewy");
    ASSERT_NE(start, std::string::npos);
    auto fixes = GetFixes(context, start, std::string("PaymentGatewy").size());
    initializer.DestroyContext(context);

    const auto *fix = FindSpellingFix(fixes);
    ASSERT_NE(fix, nullptr);
    // The imported type name is suggested and the edit targets the importing file, not the exporting one
    ValidateSingleReplacement(*fix, filePaths[1], start, std::string("PaymentGatewy").size(), "PaymentGateway");
}

// Plan: a local type with the closest spelling wins over an imported similar-name candidate
TEST_F(FixSpellingForTypeTests2, PrefersClosestLocalTypeOverImportedCandidate)
{
    std::vector<std::string> fileNames = {"spell_priority_export.ets", "spell_priority_import.ets"};
    const std::string importSource = R"(
import { OrderDetail } from './spell_priority_export';
class OrderDetailsView {
    title: string = "";
}
let view: OrderDetailsVew;
)";
    std::vector<std::string> fileContents = {R"(
export class OrderDetail {
    id: number = 0;
}
)",
                                             importSource};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileNames.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);
    ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(context));

    const size_t start = importSource.find("OrderDetailsVew");
    ASSERT_NE(start, std::string::npos);
    auto fixes = GetFixes(context, start, std::string("OrderDetailsVew").size());
    initializer.DestroyContext(context);

    const auto *fix = FindSpellingFix(fixes);
    ASSERT_NE(fix, nullptr);
    // Local "OrderDetailsView" (distance 1) wins over imported "OrderDetail" (larger distance)
    ValidateSingleReplacement(*fix, filePaths[1], start, std::string("OrderDetailsVew").size(), "OrderDetailsView");
}

// Plan: among multiple local candidates the one with the smallest edit distance is suggested
TEST_F(FixSpellingForTypeTests2, SuggestsClosestAmongMultipleLocalTypes)
{
    const std::string source = R"(
class DataRecord {
    id: number = 0;
}
class DataRecorder {
    active: boolean = false;
}
let record: DataRecrod;
)";
    auto filePaths = CreateTempFile({"suggest_closest_local_type.ets"}, {source});
    ASSERT_EQ(filePaths.size(), 1U);

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);
    ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(context));

    const size_t start = source.find("DataRecrod");
    ASSERT_NE(start, std::string::npos);
    auto fixes = GetFixes(context, start, std::string("DataRecrod").size());
    initializer.DestroyContext(context);

    const auto *fix = FindSpellingFix(fixes);
    ASSERT_NE(fix, nullptr);
    ValidateSingleReplacement(*fix, filePaths[0], start, std::string("DataRecrod").size(), "DataRecord");
}

}  // namespace

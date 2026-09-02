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

#include <gtest/gtest.h>

#include "lsp/include/api.h"
#include "lsp/include/cancellation_token.h"
#include "lsp/include/register_code_fix/fix_spelling_for_property.h"

namespace {

using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::codefixes::FIX_SPELLING_FOR_PROPERTY;

constexpr std::string_view EXPECTED_FIX_NAME = FIX_SPELLING_FOR_PROPERTY.GetFixId();
constexpr auto ERROR_CODES = FIX_SPELLING_FOR_PROPERTY.GetSupportedCodeNumbers();
// PROPERTY_NONEXISTENT: DiagnosticType::SEMANTIC * DIAGNOSTIC_CODE_MULTIPLIER + 87
constexpr int PROPERTY_NONEXISTENT_CODE = 2087;
constexpr int DEFAULT_THROTTLE = 20;

class FixSpellingForPropertyTests2 : public LSPAPITests {
public:
    static ark::es2panda::lsp::CancellationToken CreateNonCancellationToken()
    {
        return ark::es2panda::lsp::CancellationToken(DEFAULT_THROTTLE, &GetNullHost());
    }

    static std::vector<int> PropertyNonexistentCodes()
    {
        return {PROPERTY_NONEXISTENT_CODE};
    }

    static std::vector<int> SupportedErrorCodes()
    {
        return std::vector<int>(ERROR_CODES.begin(), ERROR_CODES.end());
    }

    static std::vector<CodeFixActionInfo> GetFixes(es2panda_Context *context, size_t start, size_t length,
                                                   std::vector<int> errorCodes)
    {
        CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
        return ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, options);
    }

    static std::vector<const CodeFixActionInfo *> CollectSpellingFixes(const std::vector<CodeFixActionInfo> &fixes)
    {
        std::vector<const CodeFixActionInfo *> spellingFixes;
        for (const auto &fix : fixes) {
            if (fix.fixName_ == EXPECTED_FIX_NAME) {
                spellingFixes.push_back(&fix);
            }
        }
        return spellingFixes;
    }

    static bool HasFixNamed(const std::vector<CodeFixActionInfo> &fixes, std::string_view fixName)
    {
        for (const auto &fix : fixes) {
            if (fix.fixName_ == fixName) {
                return true;
            }
        }
        return false;
    }

    // FixSpellingForProperty always replaces the misspelled identifier with the suggestion and
    // reports "Did you mean '<candidate>'?", so the whole action info is checked exactly.
    static void ValidateSpellingFix(const CodeFixActionInfo *info, const std::string &expectedFileName,
                                    const std::string &expectedNewText, size_t expectedStart, size_t expectedLength)
    {
        ASSERT_NE(info, nullptr);
        ASSERT_EQ(info->fixName_, EXPECTED_FIX_NAME);
        ASSERT_EQ(info->fixId_, EXPECTED_FIX_NAME);
        ASSERT_EQ(info->description_, "Did you mean '" + expectedNewText + "'?");
        ASSERT_EQ(info->changes_.size(), 1U);
        ASSERT_EQ(info->changes_[0].fileName, expectedFileName);
        ASSERT_EQ(info->changes_[0].textChanges.size(), 1U);
        ASSERT_EQ(info->changes_[0].textChanges[0].span.start, expectedStart);
        ASSERT_EQ(info->changes_[0].textChanges[0].span.length, expectedLength);
        ASSERT_EQ(info->changes_[0].textChanges[0].newText, expectedNewText);
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

// Member visibility in one context: public/protected/private fields and public/private methods.
// Positions below are the first occurrence offsets of the referenced members in the source.
// Test: a misspelled public field gets the exact spelling suggestion
TEST_F(FixSpellingForPropertyTests2, SuggestPublicField)
{
    const std::string source = R"(
class MyClass {
    public pubField: string = "pub";
    protected protField: string = "prot";
    private secretField: string = "secret";
    public pubMethod(): void {}
    private secretMethod(): void {}
}
function foo(): void {
    let obj = new MyClass();
    obj.pubFild;
    obj.protFild;
    obj.secretFild;
    obj.pubMetho;
    obj.secretMetho;
}
)";
    std::vector<std::string> fileNames = {"FixSpellingAccessKinds.ets"};
    std::vector<std::string> fileContents = {source};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const size_t start = source.find("obj.pubFild") + std::string("obj.").size();
    const size_t length = std::string("pubFild").size();

    auto fixResult = GetFixes(context, start, length, PropertyNonexistentCodes());

    // Only the single spelling fix is expected for this position
    auto spellingFixes = CollectSpellingFixes(fixResult);
    ASSERT_EQ(spellingFixes.size(), 1U);
    ValidateSpellingFix(spellingFixes[0], filePaths[0], "pubField", start, length);

    initializer.DestroyContext(context);
}

// Test: a misspelled protected field referenced from outside the class is still suggested
TEST_F(FixSpellingForPropertyTests2, SuggestProtectedFieldFromOutside)
{
    const std::string source = R"(
class MyClass {
    public pubField: string = "pub";
    protected protField: string = "prot";
    private secretField: string = "secret";
    public pubMethod(): void {}
    private secretMethod(): void {}
}
function foo(): void {
    let obj = new MyClass();
    obj.pubFild;
    obj.protFild;
    obj.secretFild;
    obj.pubMetho;
    obj.secretMetho;
}
)";
    std::vector<std::string> fileNames = {"FixSpellingAccessKinds.ets"};
    std::vector<std::string> fileContents = {source};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const size_t start = source.find("obj.protFild") + std::string("obj.").size();
    const size_t length = std::string("protFild").size();

    auto fixResult = GetFixes(context, start, length, PropertyNonexistentCodes());

    auto spellingFixes = CollectSpellingFixes(fixResult);
    ASSERT_EQ(spellingFixes.size(), 1U);
    ValidateSpellingFix(spellingFixes[0], filePaths[0], "protField", start, length);

    initializer.DestroyContext(context);
}

// Test: a misspelled private field referenced from outside the class is suggested too.
// The suggestion enumerates every property of the object type without filtering by visibility,
// so the produced fix would not compile; this records the current behavior.
TEST_F(FixSpellingForPropertyTests2, SuggestPrivateFieldFromOutside)
{
    const std::string source = R"(
class MyClass {
    public pubField: string = "pub";
    protected protField: string = "prot";
    private secretField: string = "secret";
    public pubMethod(): void {}
    private secretMethod(): void {}
}
function foo(): void {
    let obj = new MyClass();
    obj.pubFild;
    obj.protFild;
    obj.secretFild;
    obj.pubMetho;
    obj.secretMetho;
}
)";
    std::vector<std::string> fileNames = {"FixSpellingAccessKinds.ets"};
    std::vector<std::string> fileContents = {source};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const size_t start = source.find("obj.secretFild") + std::string("obj.").size();
    const size_t length = std::string("secretFild").size();

    auto fixResult = GetFixes(context, start, length, PropertyNonexistentCodes());

    auto spellingFixes = CollectSpellingFixes(fixResult);
    ASSERT_EQ(spellingFixes.size(), 1U);
    ValidateSpellingFix(spellingFixes[0], filePaths[0], "secretField", start, length);

    initializer.DestroyContext(context);
}

// Test: a misspelled public method name gets the exact spelling suggestion
TEST_F(FixSpellingForPropertyTests2, SuggestPublicMethod)
{
    const std::string source = R"(
class MyClass {
    public pubField: string = "pub";
    protected protField: string = "prot";
    private secretField: string = "secret";
    public pubMethod(): void {}
    private secretMethod(): void {}
}
function foo(): void {
    let obj = new MyClass();
    obj.pubFild;
    obj.protFild;
    obj.secretFild;
    obj.pubMetho;
    obj.secretMetho;
}
)";
    std::vector<std::string> fileNames = {"FixSpellingAccessKinds.ets"};
    std::vector<std::string> fileContents = {source};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const size_t start = source.find("obj.pubMetho") + std::string("obj.").size();
    const size_t length = std::string("pubMetho").size();

    auto fixResult = GetFixes(context, start, length, PropertyNonexistentCodes());

    auto spellingFixes = CollectSpellingFixes(fixResult);
    ASSERT_EQ(spellingFixes.size(), 1U);
    ValidateSpellingFix(spellingFixes[0], filePaths[0], "pubMethod", start, length);

    initializer.DestroyContext(context);
}

// Test: a misspelled private method referenced from outside the class is suggested too.
// The suggestion enumerates every property of the object type without filtering by visibility,
// so the produced fix would not compile; this records the current behavior.
TEST_F(FixSpellingForPropertyTests2, SuggestPrivateMethodFromOutside)
{
    const std::string source = R"(
class MyClass {
    public pubField: string = "pub";
    protected protField: string = "prot";
    private secretField: string = "secret";
    public pubMethod(): void {}
    private secretMethod(): void {}
}
function foo(): void {
    let obj = new MyClass();
    obj.pubFild;
    obj.protFild;
    obj.secretFild;
    obj.pubMetho;
    obj.secretMetho;
}
)";
    std::vector<std::string> fileNames = {"FixSpellingAccessKinds.ets"};
    std::vector<std::string> fileContents = {source};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const size_t start = source.find("obj.secretMetho") + std::string("obj.").size();
    const size_t length = std::string("secretMetho").size();

    auto fixResult = GetFixes(context, start, length, SupportedErrorCodes());

    auto spellingFixes = CollectSpellingFixes(fixResult);
    ASSERT_EQ(spellingFixes.size(), 1U);
    ValidateSpellingFix(spellingFixes[0], filePaths[0], "secretMethod", start, length);

    initializer.DestroyContext(context);
}

// Similarity boundary in one context: transposed letters hit, substituted letter misses.
// Test: transposed letters still hit the similarity threshold and suggest the candidate
TEST_F(FixSpellingForPropertyTests2, SimilarityBoundaryTransposedLettersHit)
{
    const std::string source = R"(
class MyClass {
    color: string = "red";
    name: string = "hello";
}
function foo(): void {
    let obj = new MyClass();
    obj.coler;
    obj.namx;
}
)";
    std::vector<std::string> fileNames = {"FixSpellingSimilarityBoundary.ets"};
    std::vector<std::string> fileContents = {source};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const size_t start = source.find("obj.coler") + std::string("obj.").size();
    const size_t length = std::string("coler").size();

    auto fixResult = GetFixes(context, start, length, PropertyNonexistentCodes());

    auto spellingFixes = CollectSpellingFixes(fixResult);
    ASSERT_EQ(spellingFixes.size(), 1U);
    ValidateSpellingFix(spellingFixes[0], filePaths[0], "color", start, length);

    initializer.DestroyContext(context);
}

// Test: a substituted letter falls below the similarity threshold, so no spelling fix is returned
TEST_F(FixSpellingForPropertyTests2, SimilarityBoundarySubstitutedLetterMiss)
{
    const std::string source = R"(
class MyClass {
    color: string = "red";
    name: string = "hello";
}
function foo(): void {
    let obj = new MyClass();
    obj.coler;
    obj.namx;
}
)";
    std::vector<std::string> fileNames = {"FixSpellingSimilarityBoundary.ets"};
    std::vector<std::string> fileContents = {source};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const size_t start = source.find("obj.namx") + std::string("obj.").size();
    const size_t length = std::string("namx").size();

    auto fixResult = GetFixes(context, start, length, SupportedErrorCodes());

    auto spellingFixes = CollectSpellingFixes(fixResult);
    ASSERT_TRUE(spellingFixes.empty());

    initializer.DestroyContext(context);
}

// Test: with several candidates only the single closest one is suggested.
// "nams" is closest to "names"; the action list never contains more than one spelling fix,
// so no ordering between multiple spelling candidates exists.
TEST_F(FixSpellingForPropertyTests2, MultipleCandidatesSuggestsSingleClosest)
{
    const std::string source = R"(
class MyClass {
    name: string = "hello";
    names: string[] = ["a", "b"];
    named: string = "test";
}
function foo(): void {
    let obj = new MyClass();
    obj.nams;
}
)";
    std::vector<std::string> fileNames = {"FixSpellingMultipleCandidates.ets"};
    std::vector<std::string> fileContents = {source};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const size_t start = source.find("obj.nams") + std::string("obj.").size();
    const size_t length = std::string("nams").size();

    auto fixResult = GetFixes(context, start, length, PropertyNonexistentCodes());

    auto spellingFixes = CollectSpellingFixes(fixResult);
    ASSERT_EQ(spellingFixes.size(), 1U);
    ValidateSpellingFix(spellingFixes[0], filePaths[0], "names", start, length);

    initializer.DestroyContext(context);
}

// Test: obj.unknown = 1 produces no spelling fix at all. The only action comes from
// AddLocalVariableForClass, which shares the PROPERTY_NONEXISTENT error code.
TEST_F(FixSpellingForPropertyTests2, AssignToUnknownPropertyReturnsNoSpellingFix)
{
    const std::string source = R"(
class MyClass {
    name: string = "hello";
    age: number = 25;
}
function foo(): void {
    let obj = new MyClass();
    obj.unknown = 1;
}
)";
    std::vector<std::string> fileNames = {"FixSpellingAssignUnknown.ets"};
    std::vector<std::string> fileContents = {source};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const size_t start = source.find("obj.unknown") + std::string("obj.").size();
    const size_t length = std::string("unknown").size();

    auto fixResult = GetFixes(context, start, length, PropertyNonexistentCodes());

    ASSERT_EQ(fixResult.size(), 1U);
    auto spellingFixes = CollectSpellingFixes(fixResult);
    ASSERT_TRUE(spellingFixes.empty());
    ASSERT_EQ(fixResult[0].fixName_, "AddLocalVariableForClass");
    ASSERT_EQ(fixResult[0].description_, "Add class field declaration");

    initializer.DestroyContext(context);
}

// Shared PROPERTY_NONEXISTENT error code in one context: with a good candidate the spelling fix
// is returned together with AddLocalVariableForClass; without a candidate only the latter remains.
// Test: when a spelling candidate exists, FixSpellingForProperty is returned alongside the shared fix
TEST_F(FixSpellingForPropertyTests2, SharedErrorCodeIncludesSpellingFix)
{
    const std::string source = R"(
class MyClass {
    name: string = "hello";
}
let obj = new MyClass();
obj.nam = "x";
obj.unknown = 1;
)";
    std::vector<std::string> fileNames = {"FixSpellingSharedErrorCode.ets"};
    std::vector<std::string> fileContents = {source};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const size_t start = source.find("obj.nam") + std::string("obj.").size();
    const size_t length = std::string("nam").size();

    auto fixResult = GetFixes(context, start, length, PropertyNonexistentCodes());

    // AddLocalVariableForClass shares this error code and also fires on the assignment target
    ASSERT_EQ(fixResult.size(), 2U);
    auto spellingFixes = CollectSpellingFixes(fixResult);
    ASSERT_EQ(spellingFixes.size(), 1U);
    ValidateSpellingFix(spellingFixes[0], filePaths[0], "name", start, length);
    ASSERT_TRUE(HasFixNamed(fixResult, "AddLocalVariableForClass"));

    initializer.DestroyContext(context);
}

// Test: fix-all entry (FixSpellingForProperty::GetAllCodeActions) through the public
// GetCombinedCodeFixImpl wrapper: a real PROPERTY_NONEXISTENT diagnostic rewrites the
// misspelled property to the closest member name.
TEST_F(FixSpellingForPropertyTests2, FixAllReplacesMisspelledProperty)
{
    const std::string source = R"(
class MyClass {
    name: string = "hello";
}
function foo(): void {
    let obj = new MyClass();
    obj.nam;
}
)";
    std::vector<std::string> fileNames = {"FixSpellingForPropertyFixAll.ets"};
    std::vector<std::string> fileContents = {source};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto combined = ark::es2panda::lsp::GetCombinedCodeFixImpl(context, EXPECTED_FIX_NAME.data(), options);

    ASSERT_EQ(combined.changes_.size(), 1U);
    ASSERT_EQ(combined.changes_[0].fileName, filePaths[0]);
    ASSERT_EQ(combined.changes_[0].textChanges.size(), 1U);
    const auto &change = combined.changes_[0].textChanges[0];
    const size_t expectedStart = source.find("obj.nam") + std::string("obj.").size();
    ASSERT_EQ(change.span.start, expectedStart);
    ASSERT_EQ(change.span.length, std::string("nam").size());
    ASSERT_EQ(change.newText, "name");

    initializer.DestroyContext(context);
}

// Test: fix-all entry with a misspelled property that has no close candidate produces no edits:
// the per-diagnostic lambda bails out before any change is recorded.
TEST_F(FixSpellingForPropertyTests2, FixAllSkipsPropertyWithoutCandidate)
{
    const std::string source = R"(
class MyClass {
    name: string = "hello";
}
function foo(): void {
    let obj = new MyClass();
    obj.xyzabc;
}
)";
    std::vector<std::string> fileNames = {"FixSpellingForPropertyFixAllNoCandidate.ets"};
    std::vector<std::string> fileContents = {source};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto combined = ark::es2panda::lsp::GetCombinedCodeFixImpl(context, EXPECTED_FIX_NAME.data(), options);

    // No close candidate means no edit is produced by the fix-all lambda.
    ASSERT_TRUE(combined.changes_.empty());

    initializer.DestroyContext(context);
}

// Test: without a spelling candidate the shared error code returns only AddLocalVariableForClass
TEST_F(FixSpellingForPropertyTests2, SharedErrorCodeFiltersToAddLocalVariableForClass)
{
    const std::string source = R"(
class MyClass {
    name: string = "hello";
}
let obj = new MyClass();
obj.nam = "x";
obj.unknown = 1;
)";
    std::vector<std::string> fileNames = {"FixSpellingSharedErrorCode.ets"};
    std::vector<std::string> fileContents = {source};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const size_t start = source.find("obj.unknown") + std::string("obj.").size();
    const size_t length = std::string("unknown").size();

    auto fixResult = GetFixes(context, start, length, PropertyNonexistentCodes());

    ASSERT_EQ(fixResult.size(), 1U);
    auto spellingFixes = CollectSpellingFixes(fixResult);
    ASSERT_TRUE(spellingFixes.empty());
    ASSERT_EQ(fixResult[0].fixName_, "AddLocalVariableForClass");
    ASSERT_FALSE(HasFixNamed(fixResult, "AddLocalVariable"));

    initializer.DestroyContext(context);
}

}  // namespace

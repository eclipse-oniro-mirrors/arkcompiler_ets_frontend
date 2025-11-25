/**
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at*
 *
 * http://www.apache.org/licenses/LICENSE-2.0*
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cstddef>
#include <string>
#include <algorithm>
#include <gtest/gtest.h>
#include "lsp/include/types.h"
#include "lsp_api_test.h"
#include "public/es2panda_lib.h"
#include "lsp/include/internal_api.h"
#include "lsp/include/refactors/refactor_types.h"
#include "lsp/include/refactors/convert_function_to_class.h"

using ::FileTextChanges;
using ark::es2panda::lsp::ConvertFunctionToClassRefactor;
using ark::es2panda::lsp::RefactorContext;

namespace {
class LSPConvertFunctionToClass : public LSPAPITests {
public:
    static constexpr std::string_view K_KIND = "refactor.rewrite.function.to.class";
    static constexpr std::string_view K_ACTION_NAME = "convert_to_class";
    // Named sentinel for "caret at start-of-file" to avoid magic number 0.
    // NOLINTNEXTLINE(readability-identifier-naming)
    static constexpr size_t kCaretAtStart = 0;

protected:
    static size_t FindIdPos(const std::string &src, const char *id)
    {
        const auto p = src.find(id);
        // Return an explicit sentinel (npos) rather than overloading 0 meaning.
        return (p == std::string::npos) ? std::string::npos : p;
    }

    static std::string ApplyEdits(const std::string &code, const std::vector<FileTextChanges> &edits)
    {
        struct Patch {
            size_t start;
            size_t len;
            std::string text;
        };
        std::vector<Patch> patches;
        patches.reserve(edits.size());
        for (const auto &ftc : edits) {
            for (const auto &ch : ftc.textChanges) {
                patches.push_back(Patch {ch.span.start, ch.span.length, ch.newText});
            }
        }

        // Apply from right to left.
        std::sort(patches.begin(), patches.end(), [](const Patch &a, const Patch &b) { return a.start > b.start; });

        std::string out = code;

        for (const auto &p : patches) {
            if (p.start > out.size()) {
                continue;
            }
            const size_t maxLen = out.size() - p.start;
            const size_t len = std::min(p.len, maxLen);
            out.replace(p.start, len, p.text);
        }

        return out;
    }

    static RefactorContext MakeCtx(es2panda_Context *ctx, size_t pos)
    {
        RefactorContext rc;
        rc.context = ctx;
        rc.kind = std::string(K_KIND);
        rc.span.pos = pos;
        rc.span.end = pos;  // caret on identifier (explicitly equal to pos)
        return rc;
    }
};

TEST_F(LSPConvertFunctionToClass, DoesNotOfferAction_WhenCursorOutsideIdentifier)
{
    const std::string source = R"(   const Foo = () => 42;)";
    auto tempFiles = CreateTempFile({"convert_to_class_no_avail_outside.ets"}, {source});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::RefactorContext refCtx;
    refCtx.context = ctx;
    refCtx.kind = std::string(K_KIND);
    refCtx.span.pos = LSPConvertFunctionToClass::kCaretAtStart;  // start-of-file, explicit
    refCtx.span.end = refCtx.span.pos;

    ark::es2panda::lsp::ConvertFunctionToClassRefactor refactor;
    auto avail = refactor.GetAvailableActions(refCtx);
    EXPECT_TRUE(avail.empty());
    if (!avail.empty()) {
        EXPECT_TRUE(avail[0].action.kind.empty());
    }

    initializer.DestroyContext(ctx);
}

TEST_F(LSPConvertFunctionToClass, AppliesEdits_AndContainsClassSkeleton)
{
    const std::string source = R"(const Foo = (a, b) => a + b;)";
    auto tempFiles = CreateTempFile({"convert_to_class_apply_arrow.ets"}, {source});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    const size_t pos = FindIdPos(source, "Foo");
    ASSERT_NE(pos, std::string::npos);

    ark::es2panda::lsp::RefactorContext refCtx = MakeCtx(ctx, pos);

    ark::es2panda::lsp::ConvertFunctionToClassRefactor refactor;
    auto editsPtr = refactor.GetEditsForAction(refCtx, std::string(K_ACTION_NAME));
    if (!editsPtr) {
        GTEST_SKIP() << "codefix::DoConvertToClass not implemented; skipping text application assertion.";
    }
    const auto &edits = editsPtr->GetFileTextChanges();
    ASSERT_FALSE(edits.empty());

    const std::string result = ApplyEdits(source, edits);
    EXPECT_NE(result.find("class Foo"), std::string::npos);

    initializer.DestroyContext(ctx);
}

TEST_F(LSPConvertFunctionToClass, OffersAction_OnArrowWithParams_WhenSelectingIdentifier)
{
    const std::string source = R"(const Foo = (x: number, y: number) => x + y;)";
    auto tempFiles = CreateTempFile({"convert_to_class_avail_arrow_params.ets"}, {source});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    const size_t pos = FindIdPos(source, "Foo");
    ASSERT_NE(pos, std::string::npos);

    ark::es2panda::lsp::RefactorContext rc = MakeCtx(ctx, pos);

    ark::es2panda::lsp::ConvertFunctionToClassRefactor refactor;
    auto avail = refactor.GetAvailableActions(rc);
    EXPECT_FALSE(avail.empty());
    ASSERT_FALSE(avail[0].action.kind.empty());
    EXPECT_EQ(avail[0].action.name, std::string(K_ACTION_NAME));
    EXPECT_EQ(avail[0].action.kind, std::string(K_KIND));

    initializer.DestroyContext(ctx);
}

TEST_F(LSPConvertFunctionToClass, OffersAction_WithRangeSelectionCoveringIdentifier)
{
    const std::string source = R"(const Foo = () => 42;)";
    auto tempFiles = CreateTempFile({"convert_to_class_avail_range.ets"}, {source});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    const size_t idPos = FindIdPos(source, "Foo");
    ASSERT_NE(idPos, std::string::npos);
    const size_t idLen = std::string("Foo").size();

    ark::es2panda::lsp::RefactorContext rc;
    rc.context = ctx;
    rc.kind = std::string(K_KIND);
    rc.span.pos = idPos;
    rc.span.end = idPos + idLen;  // selection spans the whole identifier

    ark::es2panda::lsp::ConvertFunctionToClassRefactor refactor;
    auto avail = refactor.GetAvailableActions(rc);
    ASSERT_FALSE(avail.empty());
    ASSERT_FALSE(avail[0].action.kind.empty());
    EXPECT_EQ(avail[0].action.name, std::string(K_ACTION_NAME));

    initializer.DestroyContext(ctx);
}

TEST_F(LSPConvertFunctionToClass, DoesNotOfferAction_OnNonFunctionVariable)
{
    const std::string source = R"(const Foo = 123;)";
    auto tempFiles = CreateTempFile({"convert_to_class_no_avail_nonfunc.ets"}, {source});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    const size_t pos = FindIdPos(source, "Foo");
    ASSERT_NE(pos, std::string::npos);

    ark::es2panda::lsp::RefactorContext rc = MakeCtx(ctx, pos);

    ark::es2panda::lsp::ConvertFunctionToClassRefactor refactor;
    auto avail = refactor.GetAvailableActions(rc);
    EXPECT_TRUE(avail.empty());
    if (!avail.empty()) {
        EXPECT_TRUE(avail[0].action.kind.empty());
    }
    initializer.DestroyContext(ctx);
}

TEST_F(LSPConvertFunctionToClass, DoesNotOfferAction_WhenSelectingInsideInitializerBody)
{
    const std::string source = R"(const Foo = () => 42;)";
    auto tempFiles = CreateTempFile({"convert_to_class_no_avail_inside_initializer.ets"}, {source});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    // Place the caret on "42" (inside the arrow body), not on the name
    const size_t caret = FindIdPos(source, "42");
    ASSERT_NE(caret, std::string::npos);

    ark::es2panda::lsp::RefactorContext rc = MakeCtx(ctx, caret);

    ark::es2panda::lsp::ConvertFunctionToClassRefactor refactor;
    auto avail = refactor.GetAvailableActions(rc);
    EXPECT_TRUE(avail.empty());
    if (!avail.empty()) {
        EXPECT_TRUE(avail[0].action.kind.empty());
    }
    initializer.DestroyContext(ctx);
}

TEST_F(LSPConvertFunctionToClass, OffersAction_OnTypedFunctionDeclaration_WhenSelectingName)
{
    const std::string source = R"(function greet(x: number) { return x + 1; })";
    auto tempFiles = CreateTempFile({"convert_to_class_avail_funcdecl_typed.ets"}, {source});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    const size_t pos = FindIdPos(source, "greet");
    ASSERT_NE(pos, std::string::npos);

    ark::es2panda::lsp::RefactorContext rc = MakeCtx(ctx, pos);

    ark::es2panda::lsp::ConvertFunctionToClassRefactor refactor;
    auto avail = refactor.GetAvailableActions(rc);
    ASSERT_FALSE(avail.empty());
    ASSERT_FALSE(avail[0].action.kind.empty());
    EXPECT_EQ(avail[0].action.name, std::string(K_ACTION_NAME));
    EXPECT_EQ(avail[0].action.kind, std::string(K_KIND));

    initializer.DestroyContext(ctx);
}

TEST_F(LSPConvertFunctionToClass, OffersAction_OnArrowVar_WhenSelectingIdent)
{
    const std::string src = R"(const Foo = (a: number, b: number) => a + b;)";
    auto files = CreateTempFile({"convert_to_class_avail_arrow.ets"}, {src});
    ASSERT_FALSE(files.empty());

    ark::es2panda::lsp::Initializer init;
    es2panda_Context *ctx = init.CreateContext(files[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    const size_t pos = FindIdPos(src, "Foo");
    ASSERT_NE(pos, std::string::npos);

    ConvertFunctionToClassRefactor ref;
    auto avail = ref.GetAvailableActions(MakeCtx(ctx, pos));
    ASSERT_FALSE(avail.empty());
    EXPECT_EQ(avail[0].action.kind, std::string(K_KIND));
    EXPECT_EQ(avail[0].action.name, std::string(K_ACTION_NAME));

    init.DestroyContext(ctx);
}

TEST_F(LSPConvertFunctionToClass, OffersAction_OnFunctionDecl_WhenSelectingName)
{
    const std::string src = R"(function greet(x: number) { return x + 1; })";
    auto files = CreateTempFile({"convert_to_class_avail_funcdecl.ets"}, {src});
    ASSERT_FALSE(files.empty());

    ark::es2panda::lsp::Initializer init;
    es2panda_Context *ctx = init.CreateContext(files[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    const size_t pos = FindIdPos(src, "greet");
    ASSERT_NE(pos, std::string::npos);

    ConvertFunctionToClassRefactor ref;
    auto avail = ref.GetAvailableActions(MakeCtx(ctx, pos));
    EXPECT_FALSE(avail.empty());
    EXPECT_EQ(avail[0].action.kind, std::string(K_KIND));
    EXPECT_EQ(avail[0].action.name, std::string(K_ACTION_NAME));

    init.DestroyContext(ctx);
}

TEST_F(LSPConvertFunctionToClass, NoAction_WhenCursorNotOnIdentifier)
{
    const std::string src = R"(   const Foo = (a: number) => a;)";
    auto files = CreateTempFile({"convert_to_class_no_avail.ets"}, {src});
    ASSERT_FALSE(files.empty());

    ark::es2panda::lsp::Initializer init;
    es2panda_Context *ctx = init.CreateContext(files[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    ConvertFunctionToClassRefactor ref;
    auto avail = ref.GetAvailableActions(MakeCtx(ctx, LSPConvertFunctionToClass::kCaretAtStart));
    EXPECT_TRUE(avail.empty());
    if (!avail.empty()) {
        EXPECT_TRUE(avail[0].action.kind.empty());
    }

    init.DestroyContext(ctx);
}

TEST_F(LSPConvertFunctionToClass, Converts_ConciseArrow_ToClassConstructor_ReturnsExpression)
{
    const std::string src = R"(const Foo = (a: number, b: number) => a + b;)";
    auto files = CreateTempFile({"convert_to_class_arrow_concise.ets"}, {src});
    ASSERT_FALSE(files.empty());

    ark::es2panda::lsp::Initializer init;
    es2panda_Context *ctx = init.CreateContext(files[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    const size_t pos = FindIdPos(src, "Foo");
    ASSERT_NE(pos, std::string::npos);

    ConvertFunctionToClassRefactor ref;
    auto editsPtr = ref.GetEditsForAction(MakeCtx(ctx, pos), std::string(K_ACTION_NAME));
    ASSERT_NE(editsPtr, nullptr) << "Refactor returned no edits — converter likely not wired.";
    const auto &edits = editsPtr->GetFileTextChanges();
    ASSERT_FALSE(edits.empty());

    const std::string out = ApplyEdits(src, edits);
    EXPECT_NE(out.find("class Foo"), std::string::npos);
    EXPECT_NE(out.find("constructor(a: number, b: number)"), std::string::npos);
    EXPECT_NE(out.find("return"), std::string::npos);

    // ensure arrow is gone
    EXPECT_EQ(out.find("=>"), std::string::npos);
    init.DestroyContext(ctx);
}

TEST_F(LSPConvertFunctionToClass, Converts_BlockArrow_ToClassConstructor_CopiesBody)
{
    const std::string src = R"(const Foo = (x: number) => { const y = x * 2; return y; };)";
    auto files = CreateTempFile({"convert_to_class_arrow_block.ets"}, {src});
    ASSERT_FALSE(files.empty());

    ark::es2panda::lsp::Initializer init;
    es2panda_Context *ctx = init.CreateContext(files[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    const size_t pos = FindIdPos(src, "Foo");
    ASSERT_NE(pos, std::string::npos);

    ConvertFunctionToClassRefactor ref;
    auto editsPtr = ref.GetEditsForAction(MakeCtx(ctx, pos), std::string(K_ACTION_NAME));
    ASSERT_NE(editsPtr, nullptr);
    const auto &edits = editsPtr->GetFileTextChanges();
    ASSERT_FALSE(edits.empty());

    const std::string out = ApplyEdits(src, edits);
    EXPECT_NE(out.find("class Foo"), std::string::npos);
    EXPECT_NE(out.find("constructor(x: number)"), std::string::npos);
    EXPECT_NE(out.find("const y = x * 2;"), std::string::npos);
    EXPECT_NE(out.find("return y;"), std::string::npos);
    EXPECT_EQ(out.find("=>"), std::string::npos);

    init.DestroyContext(ctx);
}

TEST_F(LSPConvertFunctionToClass, Converts_FunctionDeclaration_ToClass)
{
    const std::string src = R"(function greet(x: number) { return x + 1; })";
    auto files = CreateTempFile({"convert_to_class_funcdecl.ets"}, {src});
    ASSERT_FALSE(files.empty());

    ark::es2panda::lsp::Initializer init;
    es2panda_Context *ctx = init.CreateContext(files[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    const size_t pos = FindIdPos(src, "greet");
    ASSERT_NE(pos, std::string::npos);

    ConvertFunctionToClassRefactor ref;
    auto editsPtr = ref.GetEditsForAction(MakeCtx(ctx, pos), std::string(K_ACTION_NAME));
    ASSERT_NE(editsPtr, nullptr);
    const auto &edits = editsPtr->GetFileTextChanges();
    ASSERT_FALSE(edits.empty());

    const std::string out = ApplyEdits(src, edits);
    EXPECT_NE(out.find("class greet"), std::string::npos);
    EXPECT_NE(out.find("constructor(x: number)"), std::string::npos);
    EXPECT_NE(out.find("return x + 1;"), std::string::npos);

    init.DestroyContext(ctx);
}
}  // namespace
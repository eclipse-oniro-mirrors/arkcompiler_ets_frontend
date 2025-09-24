/**
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 */

#include <cstddef>
#include <string>
#include <algorithm>
#include <gtest/gtest.h>
#include "lsp_api_test.h"
#include "public/es2panda_lib.h"
#include "lsp/include/internal_api.h"
#include "lsp/include/refactors/convert_chain.h"
#include "lsp/include/refactors/refactor_types.h"

using ::FileTextChanges;
using ark::es2panda::lsp::ApplicableRefactorInfo;
using ark::es2panda::lsp::ConvertToOptionalChainExpressionRefactor;
using ark::es2panda::lsp::RefactorContext;

namespace {
class LSPConvertChainRefactorTests : public LSPAPITests {
public:
    static constexpr std::string_view kKind = "refactor.rewrite.expression.optionalChain";
    static constexpr std::string_view kActionName = "Convert to optional chain expression";
    static constexpr size_t kCaretAtStart = 0;
    static constexpr std::string_view kTriggerMarker = "/*1*/";
    static constexpr size_t kAfterMarkerOffset = 8;

protected:
    static size_t FindPos(const std::string &src, const char *needle)
    {
        const auto p = src.find(needle);
        return (p == std::string::npos) ? std::string::npos : p;
    }

    static size_t PosAfterMarker(const std::string &src)
    {
        const size_t pos = FindPos(src, std::string(kTriggerMarker).c_str());
        return (pos == std::string::npos) ? std::string::npos : (pos + kAfterMarkerOffset);
    }

    struct Patch {
        size_t start;
        size_t len;
        std::string text;
    };

    static std::string ApplyEdits(const std::string &code, const std::vector<FileTextChanges> &edits)
    {
        std::vector<Patch> patches;
        for (const auto &ftc : edits) {
            for (const auto &ch : ftc.textChanges) {
                patches.push_back(Patch {ch.span.start, ch.span.length, ch.newText});
            }
        }

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
        rc.kind = std::string(kKind);
        rc.span.pos = pos;
        rc.span.end = pos;
        return rc;
    }
};

TEST_F(LSPConvertChainRefactorTests, OffersAction_OnLogicalAndChain_SimpleDotChain)
{
    const std::string src = R"(
let a = { b: { c: 1 } };
let r = /*1*/a && a.b && a.b.c/*2*/;
)";
    auto files = CreateTempFile({"chain_offer_simple.ets"}, {src});
    ASSERT_FALSE(files.empty());

    ark::es2panda::lsp::Initializer init;
    es2panda_Context *ctx = init.CreateContext(files[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    const size_t caret = PosAfterMarker(src);
    ASSERT_NE(caret, std::string::npos);

    ConvertToOptionalChainExpressionRefactor ref;
    auto avail = ref.GetAvailableActions(MakeCtx(ctx, caret));
    ASSERT_FALSE(avail.action.kind.empty());
    EXPECT_EQ(avail.action.name, std::string(kActionName));
    EXPECT_EQ(avail.action.kind, std::string(kKind));

    init.DestroyContext(ctx);
}

TEST_F(LSPConvertChainRefactorTests, OffersAction_OnLogicalAndChain_WithCall)
{
    const std::string src = R"(
let a = { b: () => 0 };
let r = /*1*/a && a.b && a.b()/*2*/;
)";
    auto files = CreateTempFile({"chain_offer_call.ets"}, {src});
    ASSERT_FALSE(files.empty());

    ark::es2panda::lsp::Initializer init;
    es2panda_Context *ctx = init.CreateContext(files[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    const size_t caret = PosAfterMarker(src);
    ASSERT_NE(caret, std::string::npos);

    ConvertToOptionalChainExpressionRefactor ref;
    auto avail = ref.GetAvailableActions(MakeCtx(ctx, caret));
    ASSERT_FALSE(avail.action.kind.empty());
    EXPECT_EQ(avail.action.name, std::string(kActionName));

    init.DestroyContext(ctx);
}

TEST_F(LSPConvertChainRefactorTests, OffersAction_OnConditional_Ternary)
{
    const std::string src = R"(
interface A { baz: string; }
interface Foo { bar?: A; }
declare let foo: Foo;
let ccc = /*1*/foo.bar ? foo.bar.baz : "x"/*2*/;
)";
    auto files = CreateTempFile({"chain_offer_conditional.ets"}, {src});
    ASSERT_FALSE(files.empty());

    ark::es2panda::lsp::Initializer init;
    es2panda_Context *ctx = init.CreateContext(files[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    const size_t caret = PosAfterMarker(src);
    ASSERT_NE(caret, std::string::npos);

    ConvertToOptionalChainExpressionRefactor ref;
    auto avail = ref.GetAvailableActions(MakeCtx(ctx, caret));
    ASSERT_FALSE(avail.action.kind.empty());
    EXPECT_EQ(avail.action.name, std::string(kActionName));

    init.DestroyContext(ctx);
}

TEST_F(LSPConvertChainRefactorTests, DoesNotOffer_WhenUsingLogicalOr)
{
    const std::string src = R"(
let a = { b: { c: 1 } };
let r = /*1*/a || (a && a.b && a.b.c)/*2*/;
)";
    auto files = CreateTempFile({"chain_no_offer_or.ets"}, {src});
    ASSERT_FALSE(files.empty());

    ark::es2panda::lsp::Initializer init;
    es2panda_Context *ctx = init.CreateContext(files[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    const size_t caret = PosAfterMarker(src);
    ASSERT_NE(caret, std::string::npos);

    ConvertToOptionalChainExpressionRefactor ref;
    auto avail = ref.GetAvailableActions(MakeCtx(ctx, caret));
    EXPECT_TRUE(avail.action.kind.empty());

    init.DestroyContext(ctx);
}

TEST_F(LSPConvertChainRefactorTests, DoesNotOffer_WhenCursorOutside)
{
    const std::string src = R"(   let a = { b: 1 }; let r = a && a.b; )";
    auto files = CreateTempFile({"chain_no_offer_outside.ets"}, {src});
    ASSERT_FALSE(files.empty());

    ark::es2panda::lsp::Initializer init;
    es2panda_Context *ctx = init.CreateContext(files[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    ConvertToOptionalChainExpressionRefactor ref;
    auto avail = ref.GetAvailableActions(MakeCtx(ctx, kCaretAtStart));
    EXPECT_TRUE(avail.action.kind.empty());

    init.DestroyContext(ctx);
}

TEST_F(LSPConvertChainRefactorTests, DoesNotOffer_WhenNoAccessChainRoot)
{
    const std::string src = R"(
let a = { b: 1 };
let r = /*1*/a?.b/*2*/;
)";
    auto files = CreateTempFile({"chain_no_offer_no_root.ets"}, {src});
    ASSERT_FALSE(files.empty());

    ark::es2panda::lsp::Initializer init;
    es2panda_Context *ctx = init.CreateContext(files[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    const size_t caret = PosAfterMarker(src);
    ASSERT_NE(caret, std::string::npos);

    ConvertToOptionalChainExpressionRefactor ref;
    auto avail = ref.GetAvailableActions(MakeCtx(ctx, caret));
    EXPECT_TRUE(avail.action.kind.empty());

    init.DestroyContext(ctx);
}

TEST_F(LSPConvertChainRefactorTests, AppliesEdits_OnLogicalAndChain_Simple)
{
    const std::string src = R"(
let a = { b: { c: 1 } };
let r = /*1*/a && a.b && a.b.c/*2*/;
)";
    auto files = CreateTempFile({"chain_apply_simple.ets"}, {src});
    ASSERT_FALSE(files.empty());

    ark::es2panda::lsp::Initializer init;
    es2panda_Context *ctx = init.CreateContext(files[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    const size_t caret = PosAfterMarker(src);
    ASSERT_NE(caret, std::string::npos);

    ConvertToOptionalChainExpressionRefactor ref;
    auto editsPtr = ref.GetEditsForAction(MakeCtx(ctx, caret), std::string(kActionName));
    ASSERT_NE(editsPtr, nullptr);
    const auto &edits = editsPtr->GetFileTextChanges();
    ASSERT_FALSE(edits.empty());

    const std::string out = ApplyEdits(src, edits);
    EXPECT_NE(out.find("a?.b?.c"), std::string::npos);

    init.DestroyContext(ctx);
}

TEST_F(LSPConvertChainRefactorTests, AppliesEdits_OnConditional_ToNullishCoalesce)
{
    const std::string src = R"(
interface A { baz: string }
interface Foo { bar?: A }
declare let foo: Foo;
let ccc = /*1*/foo.bar ? foo.bar.baz : "whenFalse"/*2*/;
)";
    auto files = CreateTempFile({"chain_apply_conditional.ets"}, {src});
    ASSERT_FALSE(files.empty());

    ark::es2panda::lsp::Initializer init;
    es2panda_Context *ctx = init.CreateContext(files[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    const size_t caret = PosAfterMarker(src);
    ASSERT_NE(caret, std::string::npos);

    ConvertToOptionalChainExpressionRefactor ref;
    auto editsPtr = ref.GetEditsForAction(MakeCtx(ctx, caret), std::string(kActionName));
    ASSERT_NE(editsPtr, nullptr);
    const auto &edits = editsPtr->GetFileTextChanges();
    ASSERT_FALSE(edits.empty());

    const std::string out = ApplyEdits(src, edits);
    EXPECT_NE(out.find(R"(foo?.bar?.baz ?? "whenFalse")"), std::string::npos);

    init.DestroyContext(ctx);
}
}  // namespace
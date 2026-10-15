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

#include <gtest/gtest.h>
#include <string>
#include <tuple>
#include "lsp_api_test.h"
#include "lsp/include/internal_api.h"
#include "lsp/include/references.h"
#include "public/es2panda_lib.h"

namespace {

using ark::es2panda::ir::AstNode;
using ark::es2panda::lsp::DeclInfoType;
using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::IsValidReference;
using ark::es2panda::lsp::ResolveInfo;

class LspReferencesInternalsTests : public LSPAPITests {
protected:
    AstNode *FindNode(AstNode *root, std::function<bool(AstNode *)> pred)
    {
        AstNode *result = nullptr;
        root->FindChild([&result, &pred](AstNode *n) {
            if (result == nullptr && pred(n)) {
                result = n;
            }
            return false;
        });
        return result;
    }
};

// =============================================================================
// P2E-REF-01: IsValidReference_TrueForIdentifier
// =============================================================================
TEST_F(LspReferencesInternalsTests, IsValidReference_TrueForIdentifier)
{
    const std::string source = "let x = 1;";
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("ref01.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    // Find the 'x' identifier
    auto *ident = FindNode(ast, [](AstNode *n) { return n->IsIdentifier() && n->AsIdentifier()->Name().Is("x"); });
    ASSERT_NE(ident, nullptr);

    // IsValidReference returns true for IDENTIFIER nodes
    EXPECT_TRUE(IsValidReference(ident));

    initializer.DestroyContext(ctx);
}

// =============================================================================
// P2E-REF-02: IsValidReference_FalseForLiteral
// =============================================================================
TEST_F(LspReferencesInternalsTests, IsValidReference_FalseForLiteral)
{
    const std::string source = "let x = 42;";
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("ref02.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_NE(ctx, nullptr);

    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    // Find a NumberLiteral
    auto *numLit = FindNode(ast, [](AstNode *n) { return n->IsNumberLiteral(); });
    ASSERT_NE(numLit, nullptr);

    // IsValidReference returns false for non-IDENTIFIER nodes
    EXPECT_FALSE(IsValidReference(numLit));

    initializer.DestroyContext(ctx);
}

// =============================================================================
// P2E-REF-03: ResolveInfo_ReturnsReferenceInfo
// =============================================================================
// IsValidReference/ResolveInfo have no production callers outside references.cpp;
// pipeline tests use GetDeclInfoImpl/GetReferencesAtPositionImpl instead.
TEST_F(LspReferencesInternalsTests, ResolveInfo_ReturnsReferenceInfo)
{
    // Valid input: fileName and positionInfo in "filename:start:end" format
    DeclInfoType validInfo = std::make_tuple("test.ets", "someContent:10:20");
    auto refInfo = ResolveInfo(validInfo);
    EXPECT_EQ(refInfo.fileName, "test.ets");
    EXPECT_EQ(refInfo.start, 10U);
    EXPECT_EQ(refInfo.length, 10U);  // 20 - 10 = 10

    // Empty fileName should produce default-constructed ReferenceInfo
    DeclInfoType emptyFileInfo = std::make_tuple("", "content:0:5");
    auto emptyRef = ResolveInfo(emptyFileInfo);
    EXPECT_TRUE(emptyRef.fileName.empty());
    EXPECT_EQ(emptyRef.start, 0U);
    EXPECT_EQ(emptyRef.length, 0U);

    // Invalid position format (no colon) should produce default-constructed ReferenceInfo
    DeclInfoType badPosInfo = std::make_tuple("test.ets", "noColonHere");
    auto badRef = ResolveInfo(badPosInfo);
    EXPECT_TRUE(badRef.fileName.empty());
    EXPECT_EQ(badRef.start, 0U);
    EXPECT_EQ(badRef.length, 0U);

    // End < start should produce default-constructed ReferenceInfo
    DeclInfoType reversedInfo = std::make_tuple("test.ets", "content:20:10");
    auto reversedRef = ResolveInfo(reversedInfo);
    EXPECT_TRUE(reversedRef.fileName.empty());
    EXPECT_EQ(reversedRef.start, 0U);
    EXPECT_EQ(reversedRef.length, 0U);
}

}  // namespace
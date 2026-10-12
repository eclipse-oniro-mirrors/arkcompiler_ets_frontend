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

#include "lsp_api_test.h"

#include <gtest/gtest.h>

#include "ir/astNode.h"
#include "lsp/include/services/import_utils.h"

namespace {

using ark::es2panda::lsp::BuildImportDeclarationText;
using ark::es2panda::lsp::DeduplicateNamedImports;
using ark::es2panda::lsp::FormatImportSpecifier;
using ark::es2panda::lsp::GetImportDeclarationShape;
using ark::es2panda::lsp::GetImportMergePriority;
using ark::es2panda::lsp::ImportDeclarationShape;
using ark::es2panda::lsp::ImportMergePriority;
using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::IsLineBreak;

class LspImportUtilsTests : public LSPAPITests {};

// Canonical home of IsLineBreak (defined in services/import_utils.cpp); the duplicate
// IsLineBreak_ChecksBothChars wrapper in extract_symbol_internal_test.cpp was removed.
TEST_F(LspImportUtilsTests, IsLineBreak_TrueForLF)
{
    EXPECT_TRUE(IsLineBreak('\n'));
}

TEST_F(LspImportUtilsTests, IsLineBreak_TrueForCR)
{
    EXPECT_TRUE(IsLineBreak('\r'));
}

TEST_F(LspImportUtilsTests, IsLineBreak_FalseForSpace)
{
    EXPECT_FALSE(IsLineBreak(' '));
}

TEST_F(LspImportUtilsTests, IsLineBreak_FalseForLetter)
{
    EXPECT_FALSE(IsLineBreak('a'));
}

TEST_F(LspImportUtilsTests, FormatImportSpecifier_NamedWithAlias)
{
    EXPECT_EQ(FormatImportSpecifier({"foo", "bar"}), "foo as bar");
}

TEST_F(LspImportUtilsTests, DeduplicateNamedImports_RemovesDuplicates)
{
    const std::vector<std::pair<std::string, std::string>> imports = {
        {"foo", "foo"}, {"bar", "bar"}, {"foo", "foo"}, {"bar", "b"}, {"baz", "baz"}};

    const auto result = DeduplicateNamedImports(imports);

    ASSERT_EQ(result.size(), 4U);
    EXPECT_EQ(result[0], std::make_pair(std::string("foo"), std::string("foo")));
    EXPECT_EQ(result[1], std::make_pair(std::string("bar"), std::string("bar")));
    const size_t barAliasIdx = 2;
    const size_t bazIdx = 3;
    EXPECT_EQ(result[barAliasIdx], std::make_pair(std::string("bar"), std::string("b")));
    EXPECT_EQ(result[bazIdx], std::make_pair(std::string("baz"), std::string("baz")));
}

TEST_F(LspImportUtilsTests, DeduplicateNamedImports_AllUnique)
{
    const std::vector<std::pair<std::string, std::string>> imports = {{"foo", "foo"}, {"bar", "bar"}, {"baz", "b"}};

    EXPECT_EQ(DeduplicateNamedImports(imports), imports);
}

TEST_F(LspImportUtilsTests, DeduplicateNamedImports_EmptyInput)
{
    EXPECT_TRUE(DeduplicateNamedImports({}).empty());
}

TEST_F(LspImportUtilsTests, BuildImportDeclarationText_NamedOnly)
{
    const std::vector<std::pair<std::string, std::string>> imports = {{"foo", "foo"}, {"bar", "bar"}};

    EXPECT_EQ(BuildImportDeclarationText(std::nullopt, imports, "./mod"), "import { foo, bar } from './mod';");
}

TEST_F(LspImportUtilsTests, BuildImportDeclarationText_DefaultOnly)
{
    EXPECT_EQ(BuildImportDeclarationText(std::optional<std::string>("Foo"), {}, "./mod"), "import Foo from './mod';");
}

TEST_F(LspImportUtilsTests, BuildImportDeclarationText_DefaultAndNamed)
{
    const std::vector<std::pair<std::string, std::string>> imports = {{"foo", "foo"}, {"bar", "b"}};

    EXPECT_EQ(BuildImportDeclarationText(std::optional<std::string>("Default"), imports, "./mod"),
              "import Default, { foo, bar as b } from './mod';");
}

TEST_F(LspImportUtilsTests, GetImportMergePriority_EmptyImport)
{
    const ImportDeclarationShape shape;

    EXPECT_EQ(GetImportMergePriority(shape, false), ImportMergePriority::EMPTY_IMPORT);
}

TEST_F(LspImportUtilsTests, GetImportMergePriority_HasNamedImport)
{
    ImportDeclarationShape shape;
    shape.namedImports.emplace_back("foo", "foo");

    EXPECT_EQ(GetImportMergePriority(shape, false), ImportMergePriority::HAS_NAMED_IMPORT);
}

TEST_F(LspImportUtilsTests, GetImportMergePriority_HasDefaultImport)
{
    ImportDeclarationShape shape;
    shape.defaultImport = "Foo";

    EXPECT_EQ(GetImportMergePriority(shape, false), ImportMergePriority::HAS_DEFAULT_IMPORT);
}

TEST_F(LspImportUtilsTests, GetImportDeclarationShape_ExtractsNamedImports)
{
    const char *source = R"(import {foo, bar as b} from "./mod";)";
    Initializer initializer;
    auto *context = initializer.CreateContext("import_shape.ets", ES2PANDA_STATE_PARSED, source);
    ASSERT_NE(context, nullptr);

    auto *ast = GetAstFromContext<ark::es2panda::ir::AstNode>(context);
    ASSERT_NE(ast, nullptr);
    auto *node = ast->FindChild([](ark::es2panda::ir::AstNode *child) { return child->IsETSImportDeclaration(); });
    ASSERT_NE(node, nullptr);

    const auto shape = GetImportDeclarationShape(node->AsETSImportDeclaration());
    ASSERT_EQ(shape.namedImports.size(), 2U);
    EXPECT_EQ(shape.namedImports[0], std::make_pair(std::string("foo"), std::string("foo")));
    EXPECT_EQ(shape.namedImports[1], std::make_pair(std::string("bar"), std::string("b")));

    initializer.DestroyContext(context);
}

}  // namespace

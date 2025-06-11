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

#include "lsp/include/create_type_help_items.h"
#include "lsp_api_test.h"
#include <gtest/gtest.h>
#include <cstddef>
#include <iostream>
#include <vector>
#include "ir/astNode.h"
#include "lsp/include/internal_api.h"
#include "public/es2panda_lib.h"
#include "libarkbase/utils/arena_containers.h"

namespace {

class LSPCreateTypeHelpItems : public LSPAPITests {};

TEST_F(LSPCreateTypeHelpItems, GetTypeHelpItemClassTest)
{
    const auto fileClassTest = "getTypeHelpItemClassTest.ets";

    const auto classTestText = R"(
    class GenericClass<T, U> {
        constructor(param1: T, param2: U) {}
    }
)";
    const size_t expectedTypeParamCount = 4;
    const size_t index0 = 0;
    std::vector<std::string> files = {fileClassTest};
    std::vector<std::string> texts = {classTestText};
    auto filePaths = CreateTempFile(files, texts);
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext(files.at(index0).c_str(), ES2PANDA_STATE_CHECKED, texts.at(index0).c_str());
    auto context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto astNode = reinterpret_cast<ark::es2panda::ir::AstNode *>(context->parserProgram->Ast());
    std::vector<ark::es2panda::checker::Type *> list;
    astNode->FindChild([&list](const ark::es2panda::ir::AstNode *child) {
        ark::es2panda::lsp::GetLocalTypeParametersOfClassOrInterfaceOrTypeAlias(child, list);
        return false;
    });
    EXPECT_EQ(list.size(), expectedTypeParamCount);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCreateTypeHelpItems, GetTypeHelpItemInterfaceTest)
{
    const auto fileInterfaceTest = "getTypeHelpItemInterfaceTest.ets";
    const auto interfaceTestText = R"(
    interface GenericInterface<T, U> {
        method(param: T): U;
    }
)";
    const size_t expectedTypeParamCount = 4;
    const size_t index0 = 0;
    std::vector<std::string> files = {fileInterfaceTest};
    std::vector<std::string> texts = {interfaceTestText};
    auto filePaths = CreateTempFile(files, texts);
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext(files.at(index0).c_str(), ES2PANDA_STATE_CHECKED, texts.at(index0).c_str());
    auto context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto astNode = reinterpret_cast<ark::es2panda::ir::AstNode *>(context->parserProgram->Ast());
    std::vector<ark::es2panda::checker::Type *> list;
    astNode->FindChild([&list](const ark::es2panda::ir::AstNode *child) {
        ark::es2panda::lsp::GetLocalTypeParametersOfClassOrInterfaceOrTypeAlias(child, list);
        return false;
    });
    EXPECT_EQ(list.size(), expectedTypeParamCount);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCreateTypeHelpItems, GetTypeHelpItemGenericTypeTest)
{
    const auto fileGenericTypeTest = "getTypeHelpItemGenericTypeTest.ets";
    const auto genericTypeTestText = R"(
    type GenericType<T, U> = { first: T; second: U };
)";
    const size_t expectedTypeParamCount = 4;
    const size_t index0 = 0;
    std::vector<std::string> files = {fileGenericTypeTest};
    std::vector<std::string> texts = {genericTypeTestText};
    auto filePaths = CreateTempFile(files, texts);
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext(files.at(index0).c_str(), ES2PANDA_STATE_CHECKED, texts.at(index0).c_str());
    auto context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto astNode = reinterpret_cast<ark::es2panda::ir::AstNode *>(context->parserProgram->Ast());
    std::vector<ark::es2panda::checker::Type *> list;
    astNode->FindChild([&list](const ark::es2panda::ir::AstNode *child) {
        ark::es2panda::lsp::GetLocalTypeParametersOfClassOrInterfaceOrTypeAlias(child, list);
        return false;
    });
    EXPECT_EQ(list.size(), expectedTypeParamCount);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPCreateTypeHelpItems, GetTypeHelpItemTest)
{
    const auto classTestText = R"(
    class GenericClass<T, U> {
        constructor(param1: T, param2: U) {}
    }
)";
    const auto fileClassTest = "getTypeHelpItemTest.ets";

    const auto genericText = "GenericClass";
    const auto punctuationText = "<";
    const auto suffixText = ">";
    const auto typeText = "T";
    const auto typeText2 = "U";
    const auto expectedPrefixPartsCount = 4;  // "GenericClass" and "<"
    const auto expectedSuffixPartsCount = 2;  // ">"
    const auto expectedParametersCount = 2;   // "T" and "U"
    const size_t index1 = 1;
    const size_t index2 = 2;
    const size_t index3 = 3;
    const size_t index0 = 0;
    std::vector<std::string> files = {fileClassTest};
    std::vector<std::string> texts = {classTestText};
    auto filePaths = CreateTempFile(files, texts);
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctxTypeHelp;
    {
        ctxTypeHelp = initializer.CreateContext(files.at(0).c_str(), ES2PANDA_STATE_CHECKED, texts.at(0).c_str());
        auto context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctxTypeHelp);
        auto astNode = reinterpret_cast<ark::es2panda::ir::AstNode *>(context->parserProgram->Ast());
        SignatureHelpItem result;
        std::vector<ark::es2panda::checker::Type *> typeParameters;
        astNode->FindChild([&typeParameters, &result](const ark::es2panda::ir::AstNode *child) {
            if (child->IsClassDeclaration()) {
                ark::es2panda::lsp::GetTypeHelpItem(&typeParameters, child, result);
            }
            return false;
        });
        ASSERT_EQ(result.GetPrefixDisplayParts().size(), expectedPrefixPartsCount);
        EXPECT_EQ(result.GetPrefixDisplayParts()[index2].GetText(), genericText);
        EXPECT_EQ(result.GetPrefixDisplayParts()[index3].GetText(), punctuationText);
        ASSERT_EQ(result.GetSuffixDisplayParts().size(), expectedSuffixPartsCount);
        EXPECT_EQ(result.GetSuffixDisplayParts()[index0].GetText(), suffixText);
        ASSERT_EQ(result.GetParameters().size(), expectedParametersCount);
        EXPECT_EQ(result.GetParameters()[index0].GetName(), typeText);
        EXPECT_EQ(result.GetParameters()[index1].GetName(), typeText2);
    }
    initializer.DestroyContext(ctxTypeHelp);
}

}  // namespace

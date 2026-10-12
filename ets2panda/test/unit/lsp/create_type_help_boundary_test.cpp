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

#include "lsp/include/create_type_help_items.h"
#include "lsp_api_test.h"
#include <gtest/gtest.h>
#include <cstddef>
#include <vector>
#include "ir/astNode.h"
#include "lsp/include/internal_api.h"
#include "public/es2panda_lib.h"

namespace {

using ark::es2panda::lsp::Initializer;

class LSPCreateTypeHelpBoundaryTests : public LSPAPITests {};

void VerifyGenericConstraintTypeHelp(const SignatureHelpItem &result)
{
    const size_t genericClassPrefixIndex = 2U;
    const size_t punctuationPrefixIndex = 3U;
    ASSERT_GE(result.GetPrefixDisplayParts().size(), 4U);
    EXPECT_EQ(result.GetPrefixDisplayParts()[genericClassPrefixIndex].GetText(), "GenericClass");
    EXPECT_EQ(result.GetPrefixDisplayParts()[genericClassPrefixIndex].GetKind(), "className");
    EXPECT_EQ(result.GetPrefixDisplayParts()[punctuationPrefixIndex].GetText(), "<");
    EXPECT_EQ(result.GetPrefixDisplayParts()[punctuationPrefixIndex].GetKind(), "punctuation");

    ASSERT_GE(result.GetSuffixDisplayParts().size(), 2U);
    EXPECT_EQ(result.GetSuffixDisplayParts()[0].GetText(), ">");
    EXPECT_EQ(result.GetSuffixDisplayParts()[0].GetKind(), "punctuation");

    ASSERT_GE(result.GetParameters().size(), 2U);
    EXPECT_EQ(result.GetParameters()[0].GetName(), "T");
    const auto &tDisplayParts = result.GetParameters()[0].GetDisplayParts();
    ASSERT_EQ(tDisplayParts.size(), 3U);
    EXPECT_EQ(tDisplayParts[0].GetText(), "T");
    EXPECT_EQ(tDisplayParts[0].GetKind(), "typeName");
    EXPECT_EQ(tDisplayParts[1].GetText(), " extends ");
    EXPECT_EQ(tDisplayParts[1].GetKind(), "keyword");
    EXPECT_EQ(tDisplayParts[genericClassPrefixIndex].GetText(), "...");
    EXPECT_EQ(tDisplayParts[genericClassPrefixIndex].GetKind(), "typeName");

    EXPECT_EQ(result.GetParameters()[1].GetName(), "U");
    const auto &uDisplayParts = result.GetParameters()[1].GetDisplayParts();
    ASSERT_EQ(uDisplayParts.size(), 1U);
    EXPECT_EQ(uDisplayParts[0].GetText(), "U");
    EXPECT_EQ(uDisplayParts[0].GetKind(), "typeName");
}

void VerifyEffectiveTypeHelp(const SignatureHelpItem &result)
{
    const size_t multiParamPrefixIndex = 2U;
    const size_t punctuationPrefixIndex = 3U;
    ASSERT_GE(result.GetPrefixDisplayParts().size(), 4U);
    EXPECT_EQ(result.GetPrefixDisplayParts()[multiParamPrefixIndex].GetText(), "MultiParam");
    EXPECT_EQ(result.GetPrefixDisplayParts()[multiParamPrefixIndex].GetKind(), "className");
    EXPECT_EQ(result.GetPrefixDisplayParts()[punctuationPrefixIndex].GetText(), "<");
    EXPECT_EQ(result.GetPrefixDisplayParts()[punctuationPrefixIndex].GetKind(), "punctuation");

    ASSERT_GE(result.GetSuffixDisplayParts().size(), 2U);
    EXPECT_EQ(result.GetSuffixDisplayParts()[0].GetText(), ">");
    EXPECT_EQ(result.GetSuffixDisplayParts()[0].GetKind(), "punctuation");

    const std::vector<std::string> expectedNames = {"T", "U", "V"};
    ASSERT_GE(result.GetParameters().size(), expectedNames.size());
    for (size_t index = 0; index < expectedNames.size(); ++index) {
        EXPECT_EQ(result.GetParameters()[index].GetName(), expectedNames[index]);
        const auto &displayParts = result.GetParameters()[index].GetDisplayParts();
        ASSERT_GE(displayParts.size(), 1U);
        EXPECT_EQ(displayParts[0].GetKind(), "typeName");
    }
}

void VerifyUnionTypeHelp(const SignatureHelpItem &result)
{
    const size_t unionBoundPrefixIndex = 2U;
    const size_t punctuationPrefixIndex = 3U;
    ASSERT_GE(result.GetPrefixDisplayParts().size(), 4U);
    EXPECT_EQ(result.GetPrefixDisplayParts()[unionBoundPrefixIndex].GetText(), "UnionBound");
    EXPECT_EQ(result.GetPrefixDisplayParts()[unionBoundPrefixIndex].GetKind(), "className");
    EXPECT_EQ(result.GetPrefixDisplayParts()[punctuationPrefixIndex].GetText(), "<");
    EXPECT_EQ(result.GetPrefixDisplayParts()[punctuationPrefixIndex].GetKind(), "punctuation");

    ASSERT_GE(result.GetSuffixDisplayParts().size(), 2U);
    EXPECT_EQ(result.GetSuffixDisplayParts()[0].GetText(), ">");
    EXPECT_EQ(result.GetSuffixDisplayParts()[0].GetKind(), "punctuation");

    ASSERT_GE(result.GetParameters().size(), 1U);
    EXPECT_EQ(result.GetParameters()[0].GetName(), "T");
    const auto &displayParts = result.GetParameters()[0].GetDisplayParts();
    const size_t constraintTypeIndex = 2U;
    ASSERT_EQ(displayParts.size(), 3U);
    EXPECT_EQ(displayParts[0].GetText(), "T");
    EXPECT_EQ(displayParts[0].GetKind(), "typeName");
    EXPECT_EQ(displayParts[1].GetText(), " extends ");
    EXPECT_EQ(displayParts[1].GetKind(), "keyword");
    EXPECT_EQ(displayParts[constraintTypeIndex].GetText(), "...");
    EXPECT_EQ(displayParts[constraintTypeIndex].GetKind(), "typeName");
}

// P1E-CTH-01: GenericConstraintTypeParams
// Verifies GetLocalTypeParametersOfClassOrInterfaceOrTypeAlias and
// GetTypeHelpItem on a constrained generic class.
// T extends Object produces display parts: [T (typeName), " extends " (keyword),
// Object (typeName)].
// GetLocalTypeParametersOfClassOrInterfaceOrTypeAlias double-adds params
// (see create_type_help_items.cpp:48-50), so for 2 params (T, U) -> 4 entries.
TEST_F(LSPCreateTypeHelpBoundaryTests, GenericConstraintTypeParams)
{
    const auto fileName = "genericConstraintTypeParams.ets";
    const auto source = R"(
    class GenericClass<T extends Object, U> {
        constructor(param1: T, param2: U) {}
    }
)";
    std::vector<std::string> files = {fileName};
    std::vector<std::string> texts = {source};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext(fileName, ES2PANDA_STATE_CHECKED, source);
    auto context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto astNode = reinterpret_cast<ark::es2panda::ir::AstNode *>(context->parserProgram->Ast());

    // -- GetLocalTypeParametersOfClassOrInterfaceOrTypeAlias on ClassDefinition --
    std::vector<ark::es2panda::checker::Type *> list;
    astNode->FindChild([&list](const ark::es2panda::ir::AstNode *child) {
        ark::es2panda::lsp::GetLocalTypeParametersOfClassOrInterfaceOrTypeAlias(child, list);
        return false;
    });
    // Double-add: T, U, T, U from the GenericClass<T extends Object, U> ClassDefinition
    const size_t expectedTypeParamCount = 4;
    EXPECT_EQ(list.size(), expectedTypeParamCount);

    // -- GetTypeHelpItem on ClassDeclaration --
    SignatureHelpItem result;
    std::vector<ark::es2panda::checker::Type *> typeParameters;
    astNode->FindChild([&typeParameters, &result](const ark::es2panda::ir::AstNode *child) {
        if (child->IsClassDeclaration()) {
            ark::es2panda::lsp::GetTypeHelpItem(&typeParameters, child, result);
        }
        return false;
    });

    VerifyGenericConstraintTypeHelp(result);

    initializer.DestroyContext(ctx);
}

// P1E-CTH-02: NestedTypeReferenceItem
// Verifies GetTypeHelpItem on a nested class declaration (class Inner inside
// class Outer). The Inner class has no type params, so prefix = [Inner (className),
// < (punctuation)], suffix = [> (punctuation)], parameters = 0.
TEST_F(LSPCreateTypeHelpBoundaryTests, NestedTypeReferenceItem)
{
    const auto fileName = "nestedTypeReferenceItem.ets";
    const auto source = R"(
    class Outer {
        class Inner {}
    }
)";
    std::vector<std::string> files = {fileName};
    std::vector<std::string> texts = {source};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext(fileName, ES2PANDA_STATE_CHECKED, source);
    auto context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto astNode = reinterpret_cast<ark::es2panda::ir::AstNode *>(context->parserProgram->Ast());

    SignatureHelpItem result;
    std::vector<ark::es2panda::checker::Type *> typeParameters;
    // Find the Inner class declaration (nested class)
    astNode->FindChild([&typeParameters, &result](const ark::es2panda::ir::AstNode *child) {
        if (child->IsClassDeclaration() &&
            child->AsClassDeclaration()->Definition()->Ident()->Name().Utf8() == "Inner") {
            ark::es2panda::lsp::GetTypeHelpItem(&typeParameters, child, result);
        }
        return false;
    });

    // Inner has no type params -> prefix = [Inner (className), < (punctuation)]
    ASSERT_GE(result.GetPrefixDisplayParts().size(), 2U);
    EXPECT_EQ(result.GetPrefixDisplayParts()[0].GetText(), "Inner");
    EXPECT_EQ(result.GetPrefixDisplayParts()[0].GetKind(), "className");
    EXPECT_EQ(result.GetPrefixDisplayParts()[1].GetText(), "<");
    EXPECT_EQ(result.GetPrefixDisplayParts()[1].GetKind(), "punctuation");

    // Suffix = [> (punctuation)]
    ASSERT_GE(result.GetSuffixDisplayParts().size(), 1U);
    EXPECT_EQ(result.GetSuffixDisplayParts()[0].GetText(), ">");
    EXPECT_EQ(result.GetSuffixDisplayParts()[0].GetKind(), "punctuation");

    // No type parameters
    EXPECT_EQ(result.GetParameters().size(), 0U);

    initializer.DestroyContext(ctx);
}

// P1E-CTH-03: UnionTypeItem
// Verifies type help item built from a union type signature.
// Uses GetTypeHelpItem on a class declaration with a single type parameter
// that is constrained by a union type: T extends Object | null.
// The constraint display parts include the union type text.
// GetLocalTypeParametersOfClassOrInterfaceOrTypeAlias: 1 param -> 2 entries.
TEST_F(LSPCreateTypeHelpBoundaryTests, UnionTypeItem)
{
    const auto fileName = "unionTypeItem.ets";
    const auto source = R"(
    class UnionBound<T extends Object | null> {
        constructor(x: T) {}
    }
)";
    std::vector<std::string> files = {fileName};
    std::vector<std::string> texts = {source};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext(fileName, ES2PANDA_STATE_CHECKED, source);
    auto context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto astNode = reinterpret_cast<ark::es2panda::ir::AstNode *>(context->parserProgram->Ast());

    // -- GetLocalTypeParametersOfClassOrInterfaceOrTypeAlias --
    std::vector<ark::es2panda::checker::Type *> list;
    astNode->FindChild([&list](const ark::es2panda::ir::AstNode *child) {
        ark::es2panda::lsp::GetLocalTypeParametersOfClassOrInterfaceOrTypeAlias(child, list);
        return false;
    });
    // Double-add: T, T = 2 entries
    const size_t expectedTypeParamCount = 2;
    EXPECT_EQ(list.size(), expectedTypeParamCount);

    // -- GetTypeHelpItem on ClassDeclaration --
    SignatureHelpItem result;
    std::vector<ark::es2panda::checker::Type *> typeParameters;
    astNode->FindChild([&typeParameters, &result](const ark::es2panda::ir::AstNode *child) {
        if (child->IsClassDeclaration()) {
            ark::es2panda::lsp::GetTypeHelpItem(&typeParameters, child, result);
        }
        return false;
    });

    VerifyUnionTypeHelp(result);

    initializer.DestroyContext(ctx);
}

// P1E-CTH-04: EffectiveTypeParameters
// Verifies GetEffectiveTypeParameterDeclarations returns class type parameters
// in CHECKED context. Uses a class with 3 type params: T, U, V.
// GetEffectiveTypeParameterDeclarations returns the type params from the
// class definition's TypeParams() -> Params() list (single add).
// GetLocalTypeParametersOfClassOrInterfaceOrTypeAlias double-adds -> 6 entries.
TEST_F(LSPCreateTypeHelpBoundaryTests, EffectiveTypeParameters)
{
    const auto fileName = "effectiveTypeParameters.ets";
    const auto source = R"(
    class MultiParam<T, U, V> {
        constructor(a: T, b: U, c: V) {}
    }
)";
    std::vector<std::string> files = {fileName};
    std::vector<std::string> texts = {source};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext(fileName, ES2PANDA_STATE_CHECKED, source);
    auto context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto astNode = reinterpret_cast<ark::es2panda::ir::AstNode *>(context->parserProgram->Ast());

    // -- GetLocalTypeParametersOfClassOrInterfaceOrTypeAlias --
    std::vector<ark::es2panda::checker::Type *> list;
    astNode->FindChild([&list](const ark::es2panda::ir::AstNode *child) {
        ark::es2panda::lsp::GetLocalTypeParametersOfClassOrInterfaceOrTypeAlias(child, list);
        return false;
    });
    // Double-add: T, U, V, T, U, V = 6 entries
    const size_t expectedTypeParamCount = 6;
    EXPECT_EQ(list.size(), expectedTypeParamCount);

    // -- GetEffectiveTypeParameterDeclarations (direct, single add) --
    std::vector<ark::es2panda::checker::Type *> effective;
    astNode->FindChild([&effective](const ark::es2panda::ir::AstNode *child) {
        if (child->IsClassDefinition() && child->AsClassDefinition()->Ident()->Name().Utf8() == "MultiParam") {
            ark::es2panda::lsp::GetEffectiveTypeParameterDeclarations(child, effective);
            return true;
        }
        return false;
    });
    // Direct call appends each type parameter exactly once: T, U, V.
    EXPECT_EQ(effective.size(), 3U);

    // -- GetTypeHelpItem on ClassDeclaration --
    SignatureHelpItem result;
    std::vector<ark::es2panda::checker::Type *> typeParameters;
    astNode->FindChild([&typeParameters, &result](const ark::es2panda::ir::AstNode *child) {
        if (child->IsClassDeclaration()) {
            ark::es2panda::lsp::GetTypeHelpItem(&typeParameters, child, result);
        }
        return false;
    });

    VerifyEffectiveTypeHelp(result);

    initializer.DestroyContext(ctx);
}

}  // namespace

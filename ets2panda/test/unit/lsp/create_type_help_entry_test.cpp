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

#include <cstddef>
#include <functional>
#include <string>
#include <vector>
#include "gtest/gtest.h"
#include "ir/astNode.h"
#include "lsp/include/create_type_help_items.h"
#include "lsp_api_test.h"
#include "public/es2panda_lib.h"

namespace {
using ark::es2panda::checker::Type;
using ark::es2panda::lsp::Initializer;

class LSPCreateTypeHelpEntryTests : public LSPAPITests {};

const ark::es2panda::ir::AstNode *FindNode(const ark::es2panda::ir::AstNode *ast,
                                           const std::function<bool(const ark::es2panda::ir::AstNode *)> &pred)
{
    const ark::es2panda::ir::AstNode *found = nullptr;
    ast->FindChild([&found, &pred](const ark::es2panda::ir::AstNode *node) {
        if (pred(node)) {
            found = node;
            return true;
        }
        return false;
    });
    return found;
}

// Index of the stringified constraint part in an interface type-parameter display:
// [typeName, " extends ", constraint typeName].
constexpr size_t CONSTRAINT_DISPLAY_IDX = 2;
// Index of the duplicated type-alias parameters in `type Pair<T, U> = ...`:
// [T, U, T, U].
constexpr size_t ALIAS_DUPLICATE_T_IDX = 2;
constexpr size_t ALIAS_DUPLICATE_U_IDX = 3;

// PL-005 (create_type_help_items.cpp:22-38): a null node returns the empty
// SignatureHelpItems without touching any type-parameter walk.
TEST_F(LSPCreateTypeHelpEntryTests, CreateTypeHelpItemsNullNodeReturnsEmpty)
{
    auto items = ark::es2panda::lsp::CreateTypeHelpItems(nullptr, ark::es2panda::lexer::SourceRange(), TextSpan(0, 0));
    EXPECT_EQ(items.GetItems().size(), 0U);
    EXPECT_EQ(items.GetApplicableSpan().start, 0U);
    EXPECT_EQ(items.GetApplicableSpan().length, 0U);
}

// PL-005 (create_type_help_items.cpp:22-38): the full pipeline on a real CHECKED
// generic class context fills exactly one item. The declaration node itself
// fails the GetLocalTypeParameters gate (a ClassDeclaration is not a
// ClassDefinition), so the parameters are exactly the type params that
// GetTypeHelpItem appends at create_type_help_items.cpp:106-109: T then U.
TEST_F(LSPCreateTypeHelpEntryTests, CreateTypeHelpItemsFullPipelineOnGenericClass)
{
    const auto fileName = "typeHelpEntryGenericClass.ets";
    const auto source =
        "\nclass Client<T, U extends object> {\n"
        "    make(a: T, b: U): void {}\n"
        "}\n";
    std::vector<std::string> files = {fileName};
    std::vector<std::string> texts = {source};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext(fileName, ES2PANDA_STATE_CHECKED, source);
    auto context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    const auto *ast = context->parserProgram->Ast();
    // At CHECKED the synthetic ETSGLOBAL class declaration wraps the file and is
    // the first IsClassDeclaration node, so locate the user class by name.
    const auto *classDecl = FindNode(ast, [](const ark::es2panda::ir::AstNode *node) {
        return node->IsClassDeclaration() &&
               node->AsClassDeclaration()->Definition()->Ident()->Name().Utf8() == "Client";
    });
    ASSERT_NE(classDecl, nullptr);

    ark::es2panda::lexer::SourceRange location(classDecl->Start(), classDecl->End());
    auto items = ark::es2panda::lsp::CreateTypeHelpItems(classDecl, location, TextSpan(1, 4));

    ASSERT_EQ(items.GetItems().size(), 1U);
    const auto &item = items.GetItems().at(0);
    ASSERT_GE(item.GetPrefixDisplayParts().size(), 2U);
    EXPECT_EQ(item.GetPrefixDisplayParts()[0].GetText(), "Client");
    EXPECT_EQ(item.GetPrefixDisplayParts()[0].GetKind(), "className");
    EXPECT_EQ(item.GetPrefixDisplayParts()[1].GetText(), "<");
    EXPECT_EQ(item.GetPrefixDisplayParts()[1].GetKind(), "punctuation");
    ASSERT_GE(item.GetSuffixDisplayParts().size(), 1U);
    EXPECT_EQ(item.GetSuffixDisplayParts()[0].GetText(), ">");
    EXPECT_EQ(item.GetSuffixDisplayParts()[0].GetKind(), "punctuation");

    const std::vector<std::string> expectedNames = {"T", "U"};
    ASSERT_EQ(item.GetParameters().size(), expectedNames.size());
    for (size_t index = 0; index < expectedNames.size(); ++index) {
        EXPECT_EQ(item.GetParameters()[index].GetName(), expectedNames[index]) << index;
    }

    EXPECT_EQ(items.GetApplicableSpan().start, 1U);
    EXPECT_EQ(items.GetApplicableSpan().length, 4U);
    EXPECT_EQ(items.GetSelectedItemIndex(), location.start.index);
    EXPECT_EQ(items.GetArgumentIndex(), location.start.index);
    EXPECT_EQ(items.GetArgumentCount(), location.end.index - location.start.index);
    initializer.DestroyContext(ctx);
}

// PL-005 (create_type_help_items.cpp:22-38): a non-generic class yields the
// prefix/suffix punctuation pair with an empty parameter list.
TEST_F(LSPCreateTypeHelpEntryTests, CreateTypeHelpItemsNonGenericClassHasNoParameters)
{
    const auto fileName = "typeHelpEntryPlainClass.ets";
    const auto source = "\nclass Plain {}\n";
    std::vector<std::string> files = {fileName};
    std::vector<std::string> texts = {source};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext(fileName, ES2PANDA_STATE_CHECKED, source);
    auto context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    const auto *ast = context->parserProgram->Ast();
    const auto *classDecl = FindNode(ast, [](const ark::es2panda::ir::AstNode *node) {
        return node->IsClassDeclaration() &&
               node->AsClassDeclaration()->Definition()->Ident()->Name().Utf8() == "Plain";
    });
    ASSERT_NE(classDecl, nullptr);

    auto items =
        ark::es2panda::lsp::CreateTypeHelpItems(classDecl, ark::es2panda::lexer::SourceRange(), TextSpan(0, 0));
    ASSERT_EQ(items.GetItems().size(), 1U);
    const auto &item = items.GetItems().at(0);
    EXPECT_EQ(item.GetPrefixDisplayParts().size(), 2U);
    EXPECT_EQ(item.GetPrefixDisplayParts()[0].GetText(), "Plain");
    EXPECT_EQ(item.GetParameters().size(), 0U);
    initializer.DestroyContext(ctx);
}

// PL-006 (create_type_help_items.cpp:90-92): an interface declaration renders
// its identifier as the className prefix and its constrained parameter through
// the " extends " keyword arm.
TEST_F(LSPCreateTypeHelpEntryTests, GetTypeHelpItemInterfaceKind)
{
    const auto fileName = "typeHelpEntryInterface.ets";
    const auto source = "\ninterface Provider<T extends object> {\n    provide(p: T): void;\n}\n";
    std::vector<std::string> files = {fileName};
    std::vector<std::string> texts = {source};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext(fileName, ES2PANDA_STATE_CHECKED, source);
    auto context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    const auto *ast = context->parserProgram->Ast();
    const auto *interfaceDecl =
        FindNode(ast, [](const ark::es2panda::ir::AstNode *node) { return node->IsTSInterfaceDeclaration(); });
    ASSERT_NE(interfaceDecl, nullptr);

    SignatureHelpItem result;
    std::vector<Type *> typeParameters;
    ark::es2panda::lsp::GetTypeHelpItem(&typeParameters, interfaceDecl, result);

    ASSERT_GE(result.GetPrefixDisplayParts().size(), 2U);
    EXPECT_EQ(result.GetPrefixDisplayParts()[0].GetText(), "Provider");
    EXPECT_EQ(result.GetPrefixDisplayParts()[0].GetKind(), "className");
    EXPECT_EQ(result.GetPrefixDisplayParts()[1].GetText(), "<");
    ASSERT_EQ(result.GetParameters().size(), 1U);
    EXPECT_EQ(result.GetParameters()[0].GetName(), "T");
    const auto &displayParts = result.GetParameters()[0].GetDisplayParts();
    ASSERT_EQ(displayParts.size(), 3U);
    EXPECT_EQ(displayParts[0].GetText(), "T");
    EXPECT_EQ(displayParts[0].GetKind(), "typeName");
    EXPECT_EQ(displayParts[1].GetText(), " extends ");
    EXPECT_EQ(displayParts[1].GetKind(), "keyword");
    // The constraint TypeNode stringifies to "..." through ToString().
    EXPECT_EQ(displayParts[CONSTRAINT_DISPLAY_IDX].GetText(), "...");
    EXPECT_EQ(displayParts[CONSTRAINT_DISPLAY_IDX].GetKind(), "typeName");
    initializer.DestroyContext(ctx);
}

// PL-006 (create_type_help_items.cpp:93-96): an ETS struct declaration renders
// through the struct-name prefix arm.
TEST_F(LSPCreateTypeHelpEntryTests, GetTypeHelpItemStructKind)
{
    const auto fileName = "typeHelpEntryStruct.ets";
    const auto source = "\nstruct Store<T> {\n    value: T;\n}\n";
    std::vector<std::string> files = {fileName};
    std::vector<std::string> texts = {source};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext(fileName, ES2PANDA_STATE_CHECKED, source);
    auto context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    const auto *ast = context->parserProgram->Ast();
    const auto *structDecl =
        FindNode(ast, [](const ark::es2panda::ir::AstNode *node) { return node->IsETSStructDeclaration(); });
    ASSERT_NE(structDecl, nullptr);

    SignatureHelpItem result;
    std::vector<Type *> typeParameters;
    ark::es2panda::lsp::GetTypeHelpItem(&typeParameters, structDecl, result);

    ASSERT_GE(result.GetPrefixDisplayParts().size(), 2U);
    EXPECT_EQ(result.GetPrefixDisplayParts()[0].GetText(), "Store");
    EXPECT_EQ(result.GetPrefixDisplayParts()[0].GetKind(), "structName");
    EXPECT_EQ(result.GetPrefixDisplayParts()[1].GetText(), "<");
    EXPECT_EQ(result.GetPrefixDisplayParts()[1].GetKind(), "punctuation");
    ASSERT_EQ(result.GetParameters().size(), 1U);
    EXPECT_EQ(result.GetParameters()[0].GetName(), "T");
    initializer.DestroyContext(ctx);
}

// PL-006 (create_type_help_items.cpp:83-134): ETS has no FunctionDeclaration
// node (functions are MethodDefinitions), so a method declaration matches none
// of the four kind arms and renders the punctuation-only fallback pair with no
// parameters.
TEST_F(LSPCreateTypeHelpEntryTests, GetTypeHelpItemFallbackForNonDeclarationKind)
{
    const auto fileName = "typeHelpEntryFallback.ets";
    const auto source = "\nfunction serve(p: number): void {}\n";
    std::vector<std::string> files = {fileName};
    std::vector<std::string> texts = {source};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext(fileName, ES2PANDA_STATE_CHECKED, source);
    auto context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    const auto *ast = context->parserProgram->Ast();
    EXPECT_EQ(FindNode(ast, [](const ark::es2panda::ir::AstNode *node) { return node->IsFunctionDeclaration(); }),
              nullptr);
    const auto *methodDecl =
        FindNode(ast, [](const ark::es2panda::ir::AstNode *node) { return node->IsMethodDefinition(); });
    ASSERT_NE(methodDecl, nullptr);

    SignatureHelpItem result;
    std::vector<Type *> typeParameters;
    ark::es2panda::lsp::GetTypeHelpItem(&typeParameters, methodDecl, result);

    ASSERT_EQ(result.GetPrefixDisplayParts().size(), 1U);
    EXPECT_EQ(result.GetPrefixDisplayParts()[0].GetText(), "<");
    EXPECT_EQ(result.GetPrefixDisplayParts()[0].GetKind(), "punctuation");
    ASSERT_EQ(result.GetSuffixDisplayParts().size(), 1U);
    EXPECT_EQ(result.GetSuffixDisplayParts()[0].GetText(), ">");
    EXPECT_EQ(result.GetParameters().size(), 0U);
    initializer.DestroyContext(ctx);
}

// PL-006/PL-007 (create_type_help_items.cpp:69-74, 97-102): enum declarations
// only exist before lowering; the CHECKED context sees no TSEnumDeclaration, so
// the enum arms of GetTypeHelpItem are unreachable through checked contexts.
// The direct GetEffectiveTypeParameterDeclarations call on a real PARSED enum
// node exercises the member loop. The member checker types are still
// uninitialized garbage at PARSED (non-null small values), so feeding them into
// the reinterpret-then-deref at create_type_help_items.cpp:118-119 would crash;
// GetTypeHelpItem is deliberately not invoked on the enum node.
TEST_F(LSPCreateTypeHelpEntryTests, EnumKindReachabilityAndEffectiveParameters)
{
    const auto fileName = "typeHelpEntryEnum.ets";
    const auto source = "\nenum Mode { ON, OFF }\n";
    std::vector<std::string> files = {fileName};
    std::vector<std::string> texts = {source};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    Initializer initializer = Initializer();
    es2panda_Context *ctxChecked = initializer.CreateContext(fileName, ES2PANDA_STATE_CHECKED, source);
    auto context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctxChecked);
    const auto *astChecked = context->parserProgram->Ast();
    EXPECT_EQ(FindNode(astChecked, [](const ark::es2panda::ir::AstNode *node) { return node->IsTSEnumDeclaration(); }),
              nullptr);
    initializer.DestroyContext(ctxChecked);

    es2panda_Context *ctxParsed = initializer.CreateContext(fileName, ES2PANDA_STATE_PARSED, source);
    auto contextParsed = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctxParsed);
    const auto *astParsed = contextParsed->parserProgram->Ast();
    const auto *enumDecl =
        FindNode(astParsed, [](const ark::es2panda::ir::AstNode *node) { return node->IsTSEnumDeclaration(); });
    ASSERT_NE(enumDecl, nullptr);
    std::vector<Type *> enumParams;
    ark::es2panda::lsp::GetEffectiveTypeParameterDeclarations(enumDecl, enumParams);
    EXPECT_EQ(enumParams.size(), 2U);
    initializer.DestroyContext(ctxParsed);
}

// PL-007 (create_type_help_items.cpp:54-68): effective type-parameter
// declarations on a real struct definition and a null node; the struct branch
// reads the nested definition type params and null returns the untouched
// vector. (The enum arm is covered by EnumKindReachabilityAndEffectiveParameters
// on a PARSED context.)
TEST_F(LSPCreateTypeHelpEntryTests, EffectiveTypeParameterDeclarationsStructNull)
{
    const auto fileName = "typeHelpEntryEffective.ets";
    const auto source = "\nstruct PairBox<T> {\n    first: T;\n}\n";
    std::vector<std::string> files = {fileName};
    std::vector<std::string> texts = {source};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext(fileName, ES2PANDA_STATE_CHECKED, source);
    auto context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    const auto *ast = context->parserProgram->Ast();

    const auto *structDecl =
        FindNode(ast, [](const ark::es2panda::ir::AstNode *node) { return node->IsETSStructDeclaration(); });
    ASSERT_NE(structDecl, nullptr);
    std::vector<Type *> structParams;
    ark::es2panda::lsp::GetEffectiveTypeParameterDeclarations(structDecl, structParams);
    ASSERT_EQ(structParams.size(), 1U);
    auto *typeParamNode = reinterpret_cast<ark::es2panda::ir::TSTypeParameter *>(structParams[0]);
    EXPECT_EQ(typeParamNode->Name()->ToString(), "T");

    std::vector<Type *> nullParams;
    ark::es2panda::lsp::GetEffectiveTypeParameterDeclarations(nullptr, nullParams);
    EXPECT_EQ(nullParams.size(), 0U);
    initializer.DestroyContext(ctx);
}

// PL-007 (create_type_help_items.cpp:40-52): the gate walks only interface,
// class-definition, class-expression and type-alias nodes; a null node returns
// immediately and a method definition (the only ETS function shape) is skipped
// without touching the list.
TEST_F(LSPCreateTypeHelpEntryTests, LocalTypeParametersGateNullAndSkippedKinds)
{
    const auto fileName = "typeHelpEntryGate.ets";
    const auto source = "\nfunction plain(p: number): void {}\n";
    std::vector<std::string> files = {fileName};
    std::vector<std::string> texts = {source};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext(fileName, ES2PANDA_STATE_CHECKED, source);
    auto context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    const auto *ast = context->parserProgram->Ast();

    std::vector<Type *> list;
    ark::es2panda::lsp::GetLocalTypeParametersOfClassOrInterfaceOrTypeAlias(nullptr, list);
    EXPECT_EQ(list.size(), 0U);

    const auto *methodDecl =
        FindNode(ast, [](const ark::es2panda::ir::AstNode *node) { return node->IsMethodDefinition(); });
    ASSERT_NE(methodDecl, nullptr);
    ark::es2panda::lsp::GetLocalTypeParametersOfClassOrInterfaceOrTypeAlias(methodDecl, list);
    EXPECT_EQ(list.size(), 0U);
    initializer.DestroyContext(ctx);
}

// PL-007 (create_type_help_items.cpp:40-52, 83-134): a local type-alias scope
// passes the gate and contributes its parameters twice; through the entry the
// alias itself renders the punctuation-only fallback pair because
// GetTypeHelpItem has no type-alias arm.
TEST_F(LSPCreateTypeHelpEntryTests, TypeAliasLocalScopeThroughEntry)
{
    const auto fileName = "typeHelpEntryAlias.ets";
    const auto source = "\ntype Pair<T, U> = {\n    first: T;\n    second: U;\n};\n";
    std::vector<std::string> files = {fileName};
    std::vector<std::string> texts = {source};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext(fileName, ES2PANDA_STATE_CHECKED, source);
    auto context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    const auto *ast = context->parserProgram->Ast();

    const auto *aliasDecl =
        FindNode(ast, [](const ark::es2panda::ir::AstNode *node) { return node->IsTSTypeAliasDeclaration(); });
    ASSERT_NE(aliasDecl, nullptr);

    std::vector<Type *> list;
    ark::es2panda::lsp::GetLocalTypeParametersOfClassOrInterfaceOrTypeAlias(aliasDecl, list);
    ASSERT_EQ(list.size(), 4U);

    auto items =
        ark::es2panda::lsp::CreateTypeHelpItems(aliasDecl, ark::es2panda::lexer::SourceRange(), TextSpan(0, 0));
    ASSERT_EQ(items.GetItems().size(), 1U);
    const auto &item = items.GetItems().at(0);
    ASSERT_EQ(item.GetPrefixDisplayParts().size(), 1U);
    EXPECT_EQ(item.GetPrefixDisplayParts()[0].GetText(), "<");
    ASSERT_EQ(item.GetParameters().size(), 4U);
    EXPECT_EQ(item.GetParameters()[0].GetName(), "T");
    EXPECT_EQ(item.GetParameters()[1].GetName(), "U");
    EXPECT_EQ(item.GetParameters()[ALIAS_DUPLICATE_T_IDX].GetName(), "T");
    EXPECT_EQ(item.GetParameters()[ALIAS_DUPLICATE_U_IDX].GetName(), "U");
    initializer.DestroyContext(ctx);
}
}  // namespace

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

#include <string>
#include <vector>
#include "gtest/gtest.h"
#include "lsp/include/isolated_declaration.h"
#include "lsp_api_test.h"

namespace {
using ark::es2panda::lsp::Initializer;

class LSPIsolatedDeclarationShapesTests : public LSPAPITests {};

ark::es2panda::ir::Identifier *FindIdentifier(ark::es2panda::ir::AstNode *ast, const std::string &name)
{
    ark::es2panda::ir::Identifier *found = nullptr;
    ast->FindChild([&found, &name](ark::es2panda::ir::AstNode *node) {
        if (node->IsIdentifier() && node->AsIdentifier()->Name().Utf8() == name) {
            found = node->AsIdentifier();
            return true;
        }
        return false;
    });
    return found;
}

// PL-001 (isolated_declaration.cpp:185-201, 220-221, 288-309): an explicit
// array return annotation is checked as ETSArrayType, so HandleETSSpecificTypes
// finally routes ETS_ARRAY into the zero-hit HandleArrayType. The annotation is
// present, so GetTextChange replaces its exact span instead of inserting.
TEST_F(LSPIsolatedDeclarationShapesTests, ArrayReturnAnnotationReplacesSpan)
{
    const std::string source =
        "class A {\n"
        "    n1(): number[] { return [1, 2]; }\n"
        "    n2(): number[][] { return [[1]]; }\n"
        "    s1(): string[] { return [\"a\"]; }\n"
        "}\n";
    std::vector<std::string> files = {"IsolatedDeclArrayMatrix.ets"};
    std::vector<std::string> texts = {source};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    Initializer initializer = Initializer();
    auto context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
    auto checker = reinterpret_cast<ark::es2panda::checker::ETSChecker *>(ctx->GetChecker());
    auto ast = ctx->parserProgram->Ast();

    struct Case {
        const char *method;
        const char *annotation;
    };
    const std::vector<Case> cases = {{"n1", "number[]"}, {"n2", "number[][]"}, {"s1", "string[]"}};
    for (const auto &testCase : cases) {
        auto id = FindIdentifier(ast, testCase.method);
        ASSERT_NE(id, nullptr) << testCase.method;
        auto textChange = ark::es2panda::lsp::ProcessIdentifier(id, checker, ctx->parserProgram);
        ASSERT_EQ(textChange.has_value(), true) << testCase.method;
        const size_t annStart = source.find(testCase.annotation);
        ASSERT_NE(annStart, std::string::npos);
        EXPECT_EQ(textChange.value().newText, testCase.annotation) << testCase.method;
        EXPECT_EQ(textChange.value().span.start, annStart) << testCase.method;
        EXPECT_EQ(textChange.value().span.length, std::string(testCase.annotation).size()) << testCase.method;
    }
    initializer.DestroyContext(context);
}

// PL-002 (isolated_declaration.cpp:112-127, 159-183): a `(A | B)[]` annotation
// is checked as an Array object type with a union type argument, so it renders
// through the GenObjectType "Array" union branch as the comma tuple "[A, B]"
// with class elements named individually.
TEST_F(LSPIsolatedDeclarationShapesTests, UnionElementArrayUsesParentheses)
{
    const std::string source =
        "class A {}\n"
        "class B {}\n"
        "class Holder {\n"
        "    sel(f: number): (A | B)[] {\n"
        "        if (f > 0) { return [new A(), new B()]; }\n"
        "        return [new B()];\n"
        "    }\n"
        "}\n";
    std::vector<std::string> files = {"IsolatedDeclUnionArray.ets"};
    std::vector<std::string> texts = {source};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    Initializer initializer = Initializer();
    auto context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
    auto checker = reinterpret_cast<ark::es2panda::checker::ETSChecker *>(ctx->GetChecker());
    auto ast = ctx->parserProgram->Ast();
    auto id = FindIdentifier(ast, "sel");
    ASSERT_NE(id, nullptr);
    auto textChange = ark::es2panda::lsp::ProcessIdentifier(id, checker, ctx->parserProgram);
    ASSERT_EQ(textChange.has_value(), true);
    EXPECT_EQ(textChange.value().newText, "[A, B]");
    const size_t annStart = source.find("(A | B)[]");
    ASSERT_NE(annStart, std::string::npos);
    EXPECT_EQ(textChange.value().span.start, annStart);
    EXPECT_EQ(textChange.value().span.length, std::string("(A | B)[]").size());
    initializer.DestroyContext(context);
}

// PL-002 (isolated_declaration.cpp:52-86, 159-183): a union with repeated
// string and number constituents is deduplicated by FilterUnionTypes to a
// single "string | number" insertion after the parameter list.
TEST_F(LSPIsolatedDeclarationShapesTests, UnionDuplicateStringNumberConstituentsDedup)
{
    const std::string source =
        "function pick(n: number) {\n"
        "    if (n < 1) { return \"a\"; }\n"
        "    if (n < 2) { return \"b\"; }\n"
        "    if (n < 3) { return 1; }\n"
        "    return 2;\n"
        "}\n";
    std::vector<std::string> files = {"IsolatedDeclUnionDedup.ets"};
    std::vector<std::string> texts = {source};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    Initializer initializer = Initializer();
    auto context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
    auto checker = reinterpret_cast<ark::es2panda::checker::ETSChecker *>(ctx->GetChecker());
    auto ast = ctx->parserProgram->Ast();
    auto id = FindIdentifier(ast, "pick");
    ASSERT_NE(id, nullptr);
    auto textChange = ark::es2panda::lsp::ProcessIdentifier(id, checker, ctx->parserProgram);
    ASSERT_EQ(textChange.has_value(), true);
    EXPECT_EQ(textChange.value().newText, ": number | string");
    const size_t insertPos = source.find(") {");
    ASSERT_NE(insertPos, std::string::npos);
    EXPECT_EQ(textChange.value().span.start, insertPos + 1);
    EXPECT_EQ(textChange.value().span.length, 0U);
    initializer.DestroyContext(context);
}

// PL-002 (isolated_declaration.cpp:109-111): a return whose object type is
// literally named Exception maps to the display name "Error"; the edit is an
// insertion after the parameter list because no annotation exists.
TEST_F(LSPIsolatedDeclarationShapesTests, ExceptionReturnMapsToError)
{
    const std::string source =
        "class Exception {}\n"
        "function boom() {\n"
        "    return new Exception();\n"
        "}\n";
    std::vector<std::string> files = {"IsolatedDeclException.ets"};
    std::vector<std::string> texts = {source};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    Initializer initializer = Initializer();
    auto context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
    auto checker = reinterpret_cast<ark::es2panda::checker::ETSChecker *>(ctx->GetChecker());
    auto ast = ctx->parserProgram->Ast();
    auto id = FindIdentifier(ast, "boom");
    ASSERT_NE(id, nullptr);
    auto textChange = ark::es2panda::lsp::ProcessIdentifier(id, checker, ctx->parserProgram);
    ASSERT_EQ(textChange.has_value(), true);
    EXPECT_EQ(textChange.value().newText, ": Error");
    const size_t insertPos = source.find(") {");
    ASSERT_NE(insertPos, std::string::npos);
    EXPECT_EQ(textChange.value().span.start, insertPos + 1);
    EXPECT_EQ(textChange.value().span.length, 0U);
    initializer.DestroyContext(context);
}

// PL-002 (isolated_declaration.cpp:112-127, 175-179): an Array<T | U> return
// takes the GenObjectType "Array" union branch and renders the tuple-style
// "[number, string]" with the comma separator.
TEST_F(LSPIsolatedDeclarationShapesTests, ArrayOfUnionRendersCommaTuple)
{
    const std::string source =
        "function both(f: number): Array<number | string> {\n"
        "    if (f > 0) { return [f, \"a\"]; }\n"
        "    return [\"b\"];\n"
        "}\n";
    std::vector<std::string> files = {"IsolatedDeclArrayUnion.ets"};
    std::vector<std::string> texts = {source};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    Initializer initializer = Initializer();
    auto context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
    auto checker = reinterpret_cast<ark::es2panda::checker::ETSChecker *>(ctx->GetChecker());
    auto ast = ctx->parserProgram->Ast();
    auto id = FindIdentifier(ast, "both");
    ASSERT_NE(id, nullptr);
    auto textChange = ark::es2panda::lsp::ProcessIdentifier(id, checker, ctx->parserProgram);
    ASSERT_EQ(textChange.has_value(), true);
    EXPECT_EQ(textChange.value().newText, "[number, string]");
    const size_t annStart = source.find("Array<number | string>");
    ASSERT_NE(annStart, std::string::npos);
    EXPECT_EQ(textChange.value().span.start, annStart);
    EXPECT_EQ(textChange.value().span.length, strlen("Array<number | string>"));
    initializer.DestroyContext(context);
}

// PL-002 (isolated_declaration.cpp:135-137): a Partial<T> return renders the
// internal partial marker through the "%%partial-" prefix arm.
TEST_F(LSPIsolatedDeclarationShapesTests, PartialTypeRendersPartialMarker)
{
    const std::string source =
        "interface Opt { x: number; }\n"
        "function part(p: Partial<Opt>): Partial<Opt> {\n"
        "    return p;\n"
        "}\n";
    std::vector<std::string> files = {"IsolatedDeclPartial.ets"};
    std::vector<std::string> texts = {source};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    Initializer initializer = Initializer();
    auto context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
    auto checker = reinterpret_cast<ark::es2panda::checker::ETSChecker *>(ctx->GetChecker());
    auto ast = ctx->parserProgram->Ast();
    auto id = FindIdentifier(ast, "part");
    ASSERT_NE(id, nullptr);
    auto textChange = ark::es2panda::lsp::ProcessIdentifier(id, checker, ctx->parserProgram);
    ASSERT_EQ(textChange.has_value(), true);
    EXPECT_EQ(textChange.value().newText, "Partial<%%partial-Opt>");
    const size_t annStart = source.find("Partial<Opt> {\n    return p;");
    ASSERT_NE(annStart, std::string::npos);
    EXPECT_EQ(textChange.value().span.start, annStart);
    EXPECT_EQ(textChange.value().span.length, strlen("Partial<Opt>"));
    initializer.DestroyContext(context);
}

// PL-003 (isolated_declaration.cpp:203-228 default, 245-266, 268-274): a
// function-typed return hits the HandleETSSpecificTypes default arm, so
// GenReturnTypeStr fails and GetReturnTypeStr falls back to "void", replacing
// the annotation span.
TEST_F(LSPIsolatedDeclarationShapesTests, FunctionTypeReturnFallsBackToVoid)
{
    const std::string source =
        "function apply(f: (x: number) => number): (x: number) => number {\n"
        "    return f;\n"
        "}\n";
    std::vector<std::string> files = {"IsolatedDeclFuncType.ets"};
    std::vector<std::string> texts = {source};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    Initializer initializer = Initializer();
    auto context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
    auto checker = reinterpret_cast<ark::es2panda::checker::ETSChecker *>(ctx->GetChecker());
    auto ast = ctx->parserProgram->Ast();
    auto id = FindIdentifier(ast, "apply");
    ASSERT_NE(id, nullptr);
    auto textChange = ark::es2panda::lsp::ProcessIdentifier(id, checker, ctx->parserProgram);
    ASSERT_EQ(textChange.has_value(), true);
    EXPECT_EQ(textChange.value().newText, "void");
    const size_t retAnnStart = source.rfind("(x: number) => number");
    ASSERT_NE(retAnnStart, std::string::npos);
    EXPECT_EQ(textChange.value().span.start, retAnnStart);
    EXPECT_EQ(textChange.value().span.length, strlen("(x: number) => number"));
    initializer.DestroyContext(context);
}

// PL-003 (isolated_declaration.cpp:231-243): enum, char and bigint annotated
// returns resolve through HandleBasicTypes/HandleObjectType keyword mappings.
TEST_F(LSPIsolatedDeclarationShapesTests, EnumCharBigintKeywordMatrix)
{
    const std::string source =
        "enum Color { R }\n"
        "function en(c: Color): Color { return c; }\n"
        "function ch(c: char): char { return c; }\n"
        "function bg(b: bigint): bigint { return b; }\n";
    std::vector<std::string> files = {"IsolatedDeclKeywords.ets"};
    std::vector<std::string> texts = {source};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    Initializer initializer = Initializer();
    auto context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
    auto checker = reinterpret_cast<ark::es2panda::checker::ETSChecker *>(ctx->GetChecker());
    auto ast = ctx->parserProgram->Ast();

    struct Case {
        const char *function;
        const char *annotation;
        const char *expected;
    };
    const std::vector<Case> cases = {{"en", "Color", "Color"}, {"ch", "char", "string"}, {"bg", "bigint", "bigint"}};
    for (const auto &testCase : cases) {
        auto id = FindIdentifier(ast, testCase.function);
        ASSERT_NE(id, nullptr) << testCase.function;
        auto textChange = ark::es2panda::lsp::ProcessIdentifier(id, checker, ctx->parserProgram);
        ASSERT_EQ(textChange.has_value(), true) << testCase.function;
        EXPECT_EQ(textChange.value().newText, testCase.expected) << testCase.function;
        const size_t annStart = source.rfind(testCase.annotation);
        ASSERT_NE(annStart, std::string::npos);
        EXPECT_EQ(textChange.value().span.start, annStart) << testCase.function;
        EXPECT_EQ(textChange.value().span.length, strlen(testCase.annotation)) << testCase.function;
    }
    initializer.DestroyContext(context);
}

// PL-003 (isolated_declaration.cpp:276-286): direct GetFuncSignature calls on
// real checked function types clear both remaining arms: the arrow signature of
// a lambda type and the CallSignatures()[0] fallback when no method definition
// is supplied.
TEST_F(LSPIsolatedDeclarationShapesTests, FuncSignatureArrowAndCallSignatureArms)
{
    const std::string source =
        "class Server {\n"
        "    ping(p: number): number { return p; }\n"
        "}\n"
        "let lambda = (x: number): number => x;\n";
    std::vector<std::string> files = {"IsolatedDeclSig.ets"};
    std::vector<std::string> texts = {source};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    Initializer initializer = Initializer();
    auto context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
    auto ast = ctx->parserProgram->Ast();

    ark::es2panda::ir::ArrowFunctionExpression *arrow = nullptr;
    ast->FindChild([&arrow](ark::es2panda::ir::AstNode *node) {
        if (node->IsArrowFunctionExpression()) {
            arrow = node->AsArrowFunctionExpression();
            return true;
        }
        return false;
    });
    ASSERT_NE(arrow, nullptr);
    auto *arrowFuncType = arrow->TsType()->AsETSFunctionType();
    const ark::es2panda::checker::Signature *arrowSig = ark::es2panda::lsp::GetFuncSignature(arrowFuncType, nullptr);
    ASSERT_NE(arrowSig, nullptr);

    ark::es2panda::ir::MethodDefinition *method = nullptr;
    ast->FindChild([&method](ark::es2panda::ir::AstNode *node) {
        if (node->IsMethodDefinition() &&
            node->AsMethodDefinition()->Kind() == ark::es2panda::ir::MethodDefinitionKind::METHOD) {
            auto *key = node->AsMethodDefinition()->Key();
            if (key->IsIdentifier() && key->AsIdentifier()->Name().Utf8() == "ping") {
                method = node->AsMethodDefinition();
                return true;
            }
        }
        return false;
    });
    ASSERT_NE(method, nullptr);
    auto *methodFuncType = method->TsType()->AsETSFunctionType();
    const ark::es2panda::checker::Signature *callSig = ark::es2panda::lsp::GetFuncSignature(methodFuncType, nullptr);
    ASSERT_NE(callSig, nullptr);
    initializer.DestroyContext(context);
}

// PL-003 (isolated_declaration.cpp:335-354): identifiers that resolve to
// declarations which are neither methods, arrow declarations nor arrow-valued
// class properties produce no isolated-declaration edit.
TEST_F(LSPIsolatedDeclarationShapesTests, NonFunctionIdentifiersReturnNullopt)
{
    const std::string source =
        "class Box {\n"
        "    plain = 5;\n"
        "}\n"
        "let b = new Box();\n";
    std::vector<std::string> files = {"IsolatedDeclNonFunc.ets"};
    std::vector<std::string> texts = {source};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    Initializer initializer = Initializer();
    auto context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
    auto checker = reinterpret_cast<ark::es2panda::checker::ETSChecker *>(ctx->GetChecker());
    auto ast = ctx->parserProgram->Ast();
    auto id = FindIdentifier(ast, "plain");
    ASSERT_NE(id, nullptr);
    auto textChange = ark::es2panda::lsp::ProcessIdentifier(id, checker, ctx->parserProgram);
    EXPECT_EQ(textChange, std::nullopt);
    initializer.DestroyContext(context);
}
}  // namespace

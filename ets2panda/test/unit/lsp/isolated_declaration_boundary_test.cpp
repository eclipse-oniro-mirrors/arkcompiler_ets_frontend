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
#include "lsp/include/isolated_declaration.h"

namespace {

using ark::es2panda::lsp::Initializer;

class LSPIsolatedDeclarationBoundaryTests : public LSPAPITests {};

// P1E-IDB-01: GenericFunctionReturnType
// Verifies ProcessIdentifier on a generic function with a type parameter return.
// Inference: signature return type is T (ETSTypeParameter), GetReturnTypeStr
// returns "T" (HandleETSSpecificTypes ETS_TYPE_PARAMETER -> ToString()).
// Expected: newText = ": T" inserted after ')', span.length = 0.
TEST_F(LSPIsolatedDeclarationBoundaryTests, GenericFunctionReturnType)
{
    std::vector<std::string> files = {"GenericFunctionReturnType.ets"};
    std::vector<std::string> texts = {R"(export function foo<T>(x: T) {
  return x;
}
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    Initializer initializer = Initializer();
    auto context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
    auto checker = reinterpret_cast<ark::es2panda::checker::ETSChecker *>(ctx->GetChecker());
    auto ast = ctx->parserProgram->Ast();
    auto id = ast->FindChild([](ark::es2panda::ir::AstNode *childNode) {
        return childNode->IsIdentifier() && childNode->AsIdentifier()->Name() == "foo";
    });
    ASSERT_NE(id, nullptr);
    auto textChange = ark::es2panda::lsp::ProcessIdentifier(id->AsIdentifier(), checker, ctx->parserProgram);
    const size_t expectedStart = 28;
    ASSERT_EQ(textChange.has_value(), true);
    ASSERT_EQ(textChange.value().newText, ": T");
    ASSERT_EQ(textChange.value().span.start, expectedStart);
    ASSERT_EQ(textChange.value().span.length, 0U);

    initializer.DestroyContext(context);
}

// P1E-IDB-02: AsyncFunctionReturnType
// Verifies ProcessIdentifier on an async function with inferred return type.
// The checker infers Promise<int> for async function (InferReturnType line 808-810
// in ETSAnalyzerHelpers.cpp: CreatePromiseOf). GetReturnTypeStr(Promise<int>)
// -> GenObjectType returns "Promise" (name only, no type args).
// Expected: newText = ": Promise" inserted after ')', span.length = 0.
TEST_F(LSPIsolatedDeclarationBoundaryTests, AsyncFunctionReturnType)
{
    std::vector<std::string> files = {"AsyncFunctionReturnType.ets"};
    std::vector<std::string> texts = {R"(export async function foo() {
  return 1;
}
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    Initializer initializer = Initializer();
    auto context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
    auto checker = reinterpret_cast<ark::es2panda::checker::ETSChecker *>(ctx->GetChecker());
    auto ast = ctx->parserProgram->Ast();
    auto id = ast->FindChild([](ark::es2panda::ir::AstNode *childNode) {
        return childNode->IsIdentifier() && childNode->AsIdentifier()->Name() == "foo";
    });
    ASSERT_NE(id, nullptr);
    auto textChange = ark::es2panda::lsp::ProcessIdentifier(id->AsIdentifier(), checker, ctx->parserProgram);
    const size_t expectedStart = 27;
    ASSERT_EQ(textChange.has_value(), true);
    ASSERT_EQ(textChange.value().newText, ": Promise");
    ASSERT_EQ(textChange.value().span.start, expectedStart);
    ASSERT_EQ(textChange.value().span.length, 0U);

    initializer.DestroyContext(context);
}

// P1E-IDB-03: NestedClassReturnType
// Verifies ProcessIdentifier on a function returning a nested class instance.
// Nested class type ETSObjectType name is "Inner" (classDef->Ident()->Name()).
// GenObjectType returns "Inner".
// Expected: newText = ": Inner" inserted after ')', span.length = 0.
TEST_F(LSPIsolatedDeclarationBoundaryTests, NestedClassReturnType)
{
    std::vector<std::string> files = {"NestedClassReturnType.ets"};
    std::vector<std::string> texts = {R"(class Outer {
  class Inner {}
}
function foo() {
  return new Outer.Inner();
}
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    Initializer initializer = Initializer();
    auto context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
    auto checker = reinterpret_cast<ark::es2panda::checker::ETSChecker *>(ctx->GetChecker());
    auto ast = ctx->parserProgram->Ast();
    auto id = ast->FindChild([](ark::es2panda::ir::AstNode *childNode) {
        return childNode->IsIdentifier() && childNode->AsIdentifier()->Name() == "foo";
    });
    ASSERT_NE(id, nullptr);
    auto textChange = ark::es2panda::lsp::ProcessIdentifier(id->AsIdentifier(), checker, ctx->parserProgram);
    const size_t expectedStart = 47;
    ASSERT_EQ(textChange.has_value(), true);
    ASSERT_EQ(textChange.value().newText, ": Inner");
    ASSERT_EQ(textChange.value().span.start, expectedStart);
    ASSERT_EQ(textChange.value().span.length, 0U);

    initializer.DestroyContext(context);
}

// P1E-IDB-04: NoReturnType_VoidOrNullopt
// Verifies ProcessIdentifier on a function with no return type annotation and
// no return statement. DoReturnTypeInference (ETSAnalyzer.cpp:1899) returns
// GlobalETSUndefinedType. GetReturnTypeStr(undefined) -> "undefined".
// Expected: newText = ": undefined" inserted after ')', span.length = 0.
TEST_F(LSPIsolatedDeclarationBoundaryTests, NoReturnType_VoidOrNullopt)
{
    std::vector<std::string> files = {"NoReturnType_VoidOrNullopt.ets"};
    std::vector<std::string> texts = {R"(function foo() {
}
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    Initializer initializer = Initializer();
    auto context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
    auto checker = reinterpret_cast<ark::es2panda::checker::ETSChecker *>(ctx->GetChecker());
    auto ast = ctx->parserProgram->Ast();
    auto id = ast->FindChild([](ark::es2panda::ir::AstNode *childNode) {
        return childNode->IsIdentifier() && childNode->AsIdentifier()->Name() == "foo";
    });
    ASSERT_NE(id, nullptr);
    auto textChange = ark::es2panda::lsp::ProcessIdentifier(id->AsIdentifier(), checker, ctx->parserProgram);
    const size_t expectedStart = 14;
    ASSERT_EQ(textChange.has_value(), true);
    // DoReturnTypeInference returns GlobalETSUndefinedType for empty body
    // (ETSAnalyzer.cpp:1899), which ToString() -> "undefined"
    ASSERT_EQ(textChange.value().newText, ": undefined");
    ASSERT_EQ(textChange.value().span.start, expectedStart);
    ASSERT_EQ(textChange.value().span.length, 0U);

    initializer.DestroyContext(context);
}

// P1E-IDB-05 (ConstAssertionReturnType) is BLOCKED_DESIGN_MAPPING: the ETS
// parser rejects 'as const' at parse time (ETSparserExpressions.cpp:826-829
// logs AS_CONST_USAGE and returns nullptr), so no const-assertion AST exists
// for ProcessIdentifier to infer from. See ledger
// lsp_test_design_v4_phase1e_mapping.tsv for the authoritative entry.

}  // namespace

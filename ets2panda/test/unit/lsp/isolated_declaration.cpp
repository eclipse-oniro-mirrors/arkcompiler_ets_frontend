/**
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

class LSPIsolatedDeclarationTests : public LSPAPITests {};

TEST_F(LSPIsolatedDeclarationTests, IsolatedDeclaration1)
{
    std::vector<std::string> files = {"IsolatedDeclaration1_export.ets", "IsolatedDeclaration1.ets"};
    std::vector<std::string> texts = {R"(export class C {
  pro1: number = 123;
  pro2: string = "hello";
  pro3: boolean = false;
}
)",
                                      R"(import {C} from './IsolatedDeclaration1_export';
export function foo(n: number) {
  if (n < 3) {
    return n + 2;
  }
  const c = new C();
  if (c.pro1 < 100) {
    return c;
  }
  if (c.pro3) {
    return c.pro3;
  }
  return c.pro2;
}
)"};
    auto filePaths = CreateTempFile(files, texts);
    const size_t expectedFileCount = 2;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    Initializer initializer = Initializer();
    auto context = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
    auto checker = reinterpret_cast<ark::es2panda::checker::ETSChecker *>(ctx->GetChecker());
    auto ast = ctx->parserProgram->Ast();
    auto id = ast->FindChild([](ark::es2panda::ir::AstNode *childNode) {
        return childNode->IsIdentifier() && childNode->AsIdentifier()->Name() == "foo";
    });
    auto textChange = ark::es2panda::lsp::ProcessIdentifier(id->AsIdentifier(), checker, ctx->parserProgram);
    const size_t expectedStart = 79;
    ASSERT_EQ(textChange.has_value(), true);
    ASSERT_EQ(textChange.value().newText, ": string | boolean | number | C");
    ASSERT_EQ(textChange.value().span.start, expectedStart);
    ASSERT_EQ(textChange.value().span.length, 0);

    initializer.DestroyContext(context);
}

TEST_F(LSPIsolatedDeclarationTests, IsolatedDeclaration2)
{
    std::vector<std::string> files = {"IsolatedDeclaration2_export.ets", "IsolatedDeclaration2.ets"};
    std::vector<std::string> texts = {R"(export const arr = [1, 2, 3];
export const tuple = [1, "hello", true];
)",
                                      R"(import {arr, tuple} from './IsolatedDeclaration2_export';
export function foo(n: number) {
  if (n < 3) {
    return n + 2;
  }
  if (n < 7) {
    return arr;
  }
  return tuple;
}
)"};
    auto filePaths = CreateTempFile(files, texts);
    const size_t expectedFileCount = 2;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    Initializer initializer = Initializer();
    auto context = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
    auto checker = reinterpret_cast<ark::es2panda::checker::ETSChecker *>(ctx->GetChecker());
    auto ast = ctx->parserProgram->Ast();
    auto id = ast->FindChild([](ark::es2panda::ir::AstNode *childNode) {
        return childNode->IsIdentifier() && childNode->AsIdentifier()->Name() == "foo";
    });
    auto textChange = ark::es2panda::lsp::ProcessIdentifier(id->AsIdentifier(), checker, ctx->parserProgram);
    const size_t expectedStart = 88;
    ASSERT_EQ(textChange.has_value(), true);
    ASSERT_EQ(textChange.value().newText, ": [number, string, boolean] | number | number[]");
    ASSERT_EQ(textChange.value().span.start, expectedStart);
    ASSERT_EQ(textChange.value().span.length, 0);

    initializer.DestroyContext(context);
}

TEST_F(LSPIsolatedDeclarationTests, IsolatedDeclaration3)
{
    std::vector<std::string> files = {"IsolatedDeclaration3_export.ets", "IsolatedDeclaration3.ets"};
    std::vector<std::string> texts = {R"(export class C {
  pro1: number = 123;
  pro2: string = "hello";
  pro3: boolean = false;
}
)",
                                      R"(import {C} from './IsolatedDeclaration3_export';
export const foo = () => {
  const c = new C();
  if (c.pro1 < 100) {
    return c.pro1;
  }
  return c.pro2;
};
)"};
    auto filePaths = CreateTempFile(files, texts);
    const size_t expectedFileCount = 2;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    Initializer initializer = Initializer();
    auto context = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
    auto checker = reinterpret_cast<ark::es2panda::checker::ETSChecker *>(ctx->GetChecker());
    auto ast = ctx->parserProgram->Ast();
    auto id = ast->FindChild([](ark::es2panda::ir::AstNode *childNode) {
        return childNode->IsIdentifier() && childNode->AsIdentifier()->Name() == "foo";
    });
    auto textChange = ark::es2panda::lsp::ProcessIdentifier(id->AsIdentifier(), checker, ctx->parserProgram);
    const size_t expectedStart = 70;
    ASSERT_EQ(textChange.has_value(), true);
    ASSERT_EQ(textChange.value().newText, ": number | string");
    ASSERT_EQ(textChange.value().span.start, expectedStart);
    ASSERT_EQ(textChange.value().span.length, 0);

    initializer.DestroyContext(context);
}

TEST_F(LSPIsolatedDeclarationTests, IsolatedDeclaration4)
{
    std::vector<std::string> files = {"IsolatedDeclaration4_export.ets", "IsolatedDeclaration4.ets"};
    std::vector<std::string> texts = {R"(export const arr = [1, 2, 3];
export const tuple = [1, "hello", true];
)",
                                      R"(import {arr, tuple} from './IsolatedDeclaration4_export';
export class A {
  public foo(n: number) {
    if (n < 3) {
        return n + 2;
    }
    if (n < 7) {
        return arr;
    }
    return tuple;
  }
}
)"};
    auto filePaths = CreateTempFile(files, texts);
    const size_t expectedFileCount = 2;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    Initializer initializer = Initializer();
    auto context = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
    auto checker = reinterpret_cast<ark::es2panda::checker::ETSChecker *>(ctx->GetChecker());
    auto ast = ctx->parserProgram->Ast();
    auto id = ast->FindChild([](ark::es2panda::ir::AstNode *childNode) {
        return childNode->IsIdentifier() && childNode->AsIdentifier()->Name() == "foo";
    });
    auto textChange = ark::es2panda::lsp::ProcessIdentifier(id->AsIdentifier(), checker, ctx->parserProgram);
    const size_t expectedStart = 98;
    ASSERT_EQ(textChange.has_value(), true);
    ASSERT_EQ(textChange.value().newText, ": [number, string, boolean] | number | number[]");
    ASSERT_EQ(textChange.value().span.start, expectedStart);
    ASSERT_EQ(textChange.value().span.length, 0);

    initializer.DestroyContext(context);
}

TEST_F(LSPIsolatedDeclarationTests, IsolatedDeclaration5)
{
    std::vector<std::string> files = {"IsolatedDeclaration5_export.ets", "IsolatedDeclaration5.ets"};
    std::vector<std::string> texts = {R"(export const arr = [1, 2, 3];
export const tuple = [1, "hello", true];
)",
                                      R"(import {arr, tuple} from './IsolatedDeclaration5_export';
export class A {
  public foo = (n: number) => {
    if (n < 3) {
        return n + 2;
    }
    if (n < 7) {
        return arr;
    }
    return tuple;
  }
}
)"};
    auto filePaths = CreateTempFile(files, texts);
    const size_t expectedFileCount = 2;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    Initializer initializer = Initializer();
    auto context = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
    auto checker = reinterpret_cast<ark::es2panda::checker::ETSChecker *>(ctx->GetChecker());
    auto ast = ctx->parserProgram->Ast();
    auto id = ast->FindChild([](ark::es2panda::ir::AstNode *childNode) {
        return childNode->IsIdentifier() && childNode->AsIdentifier()->Name() == "foo";
    });
    auto textChange = ark::es2panda::lsp::ProcessIdentifier(id->AsIdentifier(), checker, ctx->parserProgram);
    const size_t expectedStart = 101;
    ASSERT_EQ(textChange.has_value(), true);
    ASSERT_EQ(textChange.value().newText, ": [number, string, boolean] | number | number[]");
    ASSERT_EQ(textChange.value().span.start, expectedStart);
    ASSERT_EQ(textChange.value().span.length, 0);

    initializer.DestroyContext(context);
}
}  // namespace
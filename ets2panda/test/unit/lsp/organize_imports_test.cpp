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

#include <gtest/gtest.h>
#include "lsp/include/organize_imports.h"
#include "lsp/include/api.h"
#include "lsp/include/internal_api.h"
#include "test/unit/lsp/lsp_api_test.h"

namespace {
using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::OrganizeImports;
}  // namespace

class OrganizeImportsTest : public LSPAPITests {};

TEST_F(OrganizeImportsTest, NormalImports)
{
    std::vector<std::string> files = {"normal-imports-test.ets", "organize-imports-1.ets", "organize-imports-2.ets"};
    std::vector<std::string> texts = {
        R"(
        import {B, C, A} from "./organize-imports-1";
        import { X } from "./organize-imports-2";
        const a = B;
        const b = C;
        )",
        R"(export const A = 1; export const B = 2; export const C = 3;)", R"(export const X = 1;)"};
    auto filePaths = CreateTempFile(files, texts);

    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    std::vector<FileTextChanges> changes = OrganizeImports::Organize(ctx, filePaths[0]);

    ASSERT_EQ(changes.size(), 1);
    ASSERT_EQ(changes[0].textChanges.size(), 1);

    std::string result = changes[0].textChanges[0].newText;

    EXPECT_TRUE(result.find("import { X } from \'./organize-imports-2\';") == std::string::npos);
    EXPECT_TRUE(result.find("import { B, C } from \'./organize-imports-1\';") != std::string::npos);

    initializer.DestroyContext(ctx);
}

TEST_F(OrganizeImportsTest, TypeOnlyImports)
{
    std::vector<std::string> files = {"typeonly-imports-test.ets", "typeonly-index.ets"};
    std::vector<std::string> texts = {
        R"(
        import type { T } from "./typeonly-index";
        import type {T as T1} from "./typeonly-index";
        import type {T as T2} from "./typeonly-index";
        let t: T1;
        function foo(arg: T2) {};
        )",
        R"(export type T = string;)"};

    auto filePaths = CreateTempFile(files, texts);
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    std::vector<FileTextChanges> changes = OrganizeImports::Organize(ctx, filePaths[0]);

    ASSERT_EQ(changes.size(), 1);

    std::string result = changes[0].textChanges[0].newText;

    EXPECT_TRUE(result.find("import type { T } from \'./typeonly-index\';") == std::string::npos);
    EXPECT_TRUE(result.find("import type { T as T1 } from \'./typeonly-index\';") != std::string::npos);
    EXPECT_TRUE(result.find("import type { T as T2 } from \'./typeonly-index\';") != std::string::npos);

    initializer.DestroyContext(ctx);
}

TEST_F(OrganizeImportsTest, NamespaceImports)
{
    std::vector<std::string> files = {"namespace-imports-test.ets", "namespace-imports-index.ets"};
    std::vector<std::string> texts = {
        R"(
        import * as NS1 from "./namespace-imports-index";
        import * as NS2 from "./namespace-imports-index";
        const b = NS1.color.red;
        )",
        R"(export const A = 1; export const B = 2;export enum color{red, blue, yellow};)"};
    auto filePaths = CreateTempFile(files, texts);

    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    std::vector<FileTextChanges> changes = OrganizeImports::Organize(ctx, filePaths[0]);

    ASSERT_EQ(changes.size(), 1);

    std::string result = changes[0].textChanges[0].newText;

    EXPECT_TRUE(result.find("import * as NS1 from \'./namespace-imports-index\';") != std::string::npos);
    EXPECT_TRUE(result.find("import * as NS2 from \'./namespace-imports-index\';") == std::string::npos);

    initializer.DestroyContext(ctx);
}

TEST_F(OrganizeImportsTest, AliasImports)
{
    std::vector<std::string> files = {"alias-imports-test.ets", "alias-imports-index.ets"};
    std::vector<std::string> texts = {
        R"(
        import {B as B1, C} from "./alias-imports-index";
        import { B as B2 } from "./alias-imports-index";
        const b = B1;
        )",
        R"(export const B = 2; export const C = 3;)"};

    auto filePaths = CreateTempFile(files, texts);
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    std::vector<FileTextChanges> changes = OrganizeImports::Organize(ctx, filePaths[0]);

    ASSERT_EQ(changes.size(), 1);

    std::string result = changes[0].textChanges[0].newText;

    EXPECT_TRUE(result.find("import { B as B1 } from \'./alias-imports-index\'") != std::string::npos);
    EXPECT_TRUE(result.find("import { B as B2 } from \'./alias-imports-index\'") == std::string::npos);

    initializer.DestroyContext(ctx);
}

TEST_F(OrganizeImportsTest, DefaultImports)
{
    std::vector<std::string> files = {"default-imports-test.ets", "default-imports-index.ets"};
    std::vector<std::string> texts = {
        R"(
        import B from "./default-imports-index";
        import {C as C1} from "./default-imports-index";
        const b = B;
        const c = C1;
        )",
        R"(export default const B = 2; export const C = 3;)"};

    auto filePaths = CreateTempFile(files, texts);
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    std::vector<FileTextChanges> changes = OrganizeImports::Organize(ctx, filePaths[0]);

    ASSERT_EQ(changes.size(), 1);

    std::string result = changes[0].textChanges[0].newText;

    EXPECT_TRUE(result.find("import B from \'./default-imports-index\'") != std::string::npos);
    EXPECT_TRUE(result.find("import { C as C1 } from \'./default-imports-index\'") != std::string::npos);

    initializer.DestroyContext(ctx);
}

TEST_F(OrganizeImportsTest, SystemDefaultImports)
{
    std::vector<std::string> files = {"system-default-imports-test.ets", "system-default-index.ets"};
    std::vector<std::string> texts = {
        R"(
        import * as T1 from "./system-default-index";
        import * as T2 from "./system-default-index";
        let t: T2.T = "hello";
        )",
        R"(export type T = string;)"};

    auto filePaths = CreateTempFile(files, texts);
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    std::vector<FileTextChanges> changes = OrganizeImports::Organize(ctx, filePaths[0]);

    ASSERT_EQ(changes.size(), 1);

    std::string result = changes[0].textChanges[0].newText;

    EXPECT_TRUE(result.find("import * as T1 from \'./system-default-index\';") == std::string::npos);
    EXPECT_TRUE(result.find("import * as T2 from \'./system-default-index\';") != std::string::npos);

    initializer.DestroyContext(ctx);
}

TEST_F(OrganizeImportsTest, UnusedImports)
{
    std::vector<std::string> files = {"UnusedImports_import.ets", "UnusedImports_export.ets"};
    std::vector<std::string> texts = {
        R"(
        import { Foo, Bar } from './UnusedImports_export';
        export { Bar } from  './UnusedImports_export';
        )",
        R"(
        export class Foo {
            name: string = "john";
        }
        export class Bar {
            name: string = "john";
        }
        )"};

    auto filePaths = CreateTempFile(files, texts);
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    std::vector<FileTextChanges> changes = OrganizeImports::Organize(ctx, filePaths[0]);

    ASSERT_EQ(changes.size(), 1);

    auto result = changes[0].textChanges[0];

    const size_t expectedStart = 9;
    const size_t expectedLength = 50;
    ASSERT_EQ(result.span.start, expectedStart);
    ASSERT_EQ(result.span.length, expectedLength);
    EXPECT_EQ(result.newText, "");

    initializer.DestroyContext(ctx);
}

TEST_F(OrganizeImportsTest, UnusedDefaultImports)
{
    std::vector<std::string> files = {"UnusedDefaultImports_import.ets", "UnusedDefaultImports_export.ets"};
    std::vector<std::string> texts = {
        R"(
        import Foo from "./UnusedDefaultImports_export";
        )",
        R"(
        class Foo {
            name: string = "john";
        }
        export default Foo;
        )"};

    auto filePaths = CreateTempFile(files, texts);
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    std::vector<FileTextChanges> changes = OrganizeImports::Organize(ctx, filePaths[0]);

    ASSERT_EQ(changes.size(), 1);

    auto result = changes[0].textChanges[0];

    const size_t expectedStart = 9;
    const size_t expectedLength = 48;
    ASSERT_EQ(result.span.start, expectedStart);
    ASSERT_EQ(result.span.length, expectedLength);
    ASSERT_EQ(result.newText, "");

    initializer.DestroyContext(ctx);
}

TEST_F(OrganizeImportsTest, ExtractDefaultImport1)
{
    std::vector<std::string> files = {"ExtractDefaultImport1_import.ets", "ExtractDefaultImport1_export.ets"};
    std::vector<std::string> texts = {
        R"(
        import {Foo, one} from './ExtractDefaultImport1_export';
        let foo: Foo = new Foo();
        let a: number = one();
        )",
        R"(
        export function one(): number { return 1; }
        class Foo {
            name: string = "john";
        }
        export default Foo;
        )"};

    auto filePaths = CreateTempFile(files, texts);
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    std::vector<FileTextChanges> changes = OrganizeImports::Organize(ctx, filePaths[0]);

    ASSERT_EQ(changes.size(), 1);

    auto result = changes[0].textChanges[0];

    const size_t expectedStart = 9;
    const size_t expectedLength = 56;
    ASSERT_EQ(result.span.start, expectedStart);
    ASSERT_EQ(result.span.length, expectedLength);
    ASSERT_EQ(result.newText, "import Foo, { one } from './ExtractDefaultImport1_export';");

    initializer.DestroyContext(ctx);
}

TEST_F(OrganizeImportsTest, ExtractDefaultImport2)
{
    std::vector<std::string> files = {"ExtractDefaultImport2_import.ets", "ExtractDefaultImport2_export.ets"};
    std::vector<std::string> texts = {
        R"(
        import {Foo} from './ExtractDefaultImport2_export';
        let foo: Foo = new Foo();
        )",
        R"(
        class Foo {
            name: string = "john";
        }
        export default Foo;
        )"};

    auto filePaths = CreateTempFile(files, texts);
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    std::vector<FileTextChanges> changes = OrganizeImports::Organize(ctx, filePaths[0]);

    ASSERT_EQ(changes.size(), 1);

    auto result = changes[0].textChanges[0];

    const size_t expectedStart = 9;
    const size_t expectedLength = 51;
    ASSERT_EQ(result.span.start, expectedStart);
    ASSERT_EQ(result.span.length, expectedLength);
    ASSERT_EQ(result.newText, "import Foo from './ExtractDefaultImport2_export';");

    initializer.DestroyContext(ctx);
}

TEST_F(OrganizeImportsTest, ExtractDefaultImport3)
{
    std::vector<std::string> files = {"ExtractDefaultImport3_import.ets", "ExtractDefaultImport3_export.ets"};
    std::vector<std::string> texts = {
        R"(
        import { one, two, Foo } from './ExtractDefaultImport3_export';
        let foo: Foo = new Foo();
        let a: number = one();
        let b: number = two();
        )",
        R"(
        export function one(): number { return 1; }
        export function two(): number { return 2; }
        class Foo {
            name: string = "john";
        }
        export default Foo;
        )"};

    auto filePaths = CreateTempFile(files, texts);
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    std::vector<FileTextChanges> changes = OrganizeImports::Organize(ctx, filePaths[0]);

    ASSERT_EQ(changes.size(), 1);

    auto result = changes[0].textChanges[0];

    const size_t expectedStart = 9;
    const size_t expectedLength = 63;
    ASSERT_EQ(result.span.start, expectedStart);
    ASSERT_EQ(result.span.length, expectedLength);
    ASSERT_EQ(result.newText, "import Foo, { one, two } from './ExtractDefaultImport3_export';");

    initializer.DestroyContext(ctx);
}

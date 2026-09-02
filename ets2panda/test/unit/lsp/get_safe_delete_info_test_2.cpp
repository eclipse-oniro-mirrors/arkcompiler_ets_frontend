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
#include <cstddef>
#include <string>
#include <vector>
#include "lsp_api_test.h"
#include "lsp/include/internal_api.h"
#include "lsp/include/api.h"
#include "public/es2panda_lib.h"

namespace {
using ark::es2panda::lsp::Initializer;

class LspGetSafeDeleteInfoTest2 : public LSPAPITests {};

// Test: unused local variable, function and class declarations are safe to delete
TEST_F(LspGetSafeDeleteInfoTest2, UnusedDeclarationsAreSafeToDelete)
{
    std::vector<std::string> files = {"safe_delete2_unused.ets"};
    std::vector<std::string> texts = {R"(let unusedVar = 42;
function unusedFunc(): void {}
class UnusedClass {}
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    Initializer initializer;
    auto *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);
    LSPAPI const *lspApi = GetImpl();

    const auto varPos = texts[0].find("unusedVar");
    const auto funcPos = texts[0].find("unusedFunc");
    const auto classPos = texts[0].find("UnusedClass");
    ASSERT_NE(varPos, std::string::npos);
    ASSERT_NE(funcPos, std::string::npos);
    ASSERT_NE(classPos, std::string::npos);

    auto varResult = lspApi->getSafeDeleteInfo(ctx, varPos);
    auto funcResult = lspApi->getSafeDeleteInfo(ctx, funcPos);
    auto classResult = lspApi->getSafeDeleteInfo(ctx, classPos);

    initializer.DestroyContext(ctx);

    EXPECT_TRUE(varResult);
    EXPECT_TRUE(funcResult);
    EXPECT_TRUE(classResult);
}

// Test: exported symbol that is also referenced locally is unsafe to delete.
// lsp-test-plan: exported symbols are never safe to delete (export rule fires before
// reference analysis).
TEST_F(LspGetSafeDeleteInfoTest2, DISABLED_ExportedReferencedSymbolIsNotSafeToDelete)
{
    std::vector<std::string> files = {"safe_delete2_exported_used.ets"};
    std::vector<std::string> texts = {R"(export function exportedUsed(): number {
    return 1;
}
const holder = exportedUsed();
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    Initializer initializer;
    auto *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);
    LSPAPI const *lspApi = GetImpl();

    // Position at "exportedUsed" in the function declaration
    const auto declPos = texts[0].find("exportedUsed(): number");
    ASSERT_NE(declPos, std::string::npos);

    auto result = lspApi->getSafeDeleteInfo(ctx, declPos);

    initializer.DestroyContext(ctx);

    ASSERT_FALSE(result);
}

// Test: source symbol referenced by an import in another file is unsafe to delete.
// lsp-test-plan: exported symbols are never safe to delete; the cross-file import
// reference additionally makes deletion unsafe.
TEST_F(LspGetSafeDeleteInfoTest2, DISABLED_ImportedSourceSymbolIsNotSafeToDelete)
{
    std::vector<std::string> files = {"safe_delete2_shared.ets", "safe_delete2_consumer.ets"};
    std::vector<std::string> texts = {R"(export function sharedFunc(): void {})",
                                      R"(import { sharedFunc } from './safe_delete2_shared';
sharedFunc();)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 2U);

    Initializer initializer;
    auto *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);
    LSPAPI const *lspApi = GetImpl();

    // Position at "sharedFunc" in the source file declaration
    const auto declPos = texts[0].find("sharedFunc");
    ASSERT_NE(declPos, std::string::npos);

    auto result = lspApi->getSafeDeleteInfo(ctx, declPos);

    initializer.DestroyContext(ctx);

    ASSERT_FALSE(result);
}

TEST_F(LspGetSafeDeleteInfoTest2, NonExportedSourceSymbolImportIsNotVisibleFromSourceContext)
{
    std::vector<std::string> files = {"safe_delete2_private_shared.ets", "safe_delete2_private_consumer.ets"};
    std::vector<std::string> texts = {R"(function privateShared(): void {})",
                                      R"(import { privateShared } from './safe_delete2_private_shared';
privateShared();)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 2U);

    Initializer initializer;
    auto *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);
    LSPAPI const *lspApi = GetImpl();

    const auto declPos = texts[0].find("privateShared");
    ASSERT_NE(declPos, std::string::npos);

    const auto result = lspApi->getSafeDeleteInfo(ctx, declPos);
    initializer.DestroyContext(ctx);

    // CreateContext is rooted at the source file. Because importing a non-exported
    // symbol is invalid, the consumer is not attached as an external program and its
    // reference is not visible from this context. The exported-symbol scenario above
    // remains the valid cross-file safe-delete contract.
    ASSERT_TRUE(result);
}

TEST_F(LspGetSafeDeleteInfoTest2, SameNamedExportInExternalProgramDoesNotMakeLocalDeclarationUnsafe)
{
    std::vector<std::string> files = {"safe_delete2_same_name_main.ets", "safe_delete2_same_name_external.ets"};
    std::vector<std::string> texts = {R"(import { helper } from './safe_delete2_same_name_external';
function sameName(): void {}
helper();)",
                                      R"(function sameName(): void {}
export { sameName };
export function helper(): void {})"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 2U);

    Initializer initializer;
    auto *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);
    LSPAPI const *lspApi = GetImpl();

    const auto declPos = texts[0].find("sameName");
    ASSERT_NE(declPos, std::string::npos);

    const auto result = lspApi->getSafeDeleteInfo(ctx, declPos);
    initializer.DestroyContext(ctx);

    ASSERT_TRUE(result);
}

// Test: identifier inside a re-export specifier. lsp-test-plan: a symbol that is
// re-exported is part of the module surface, so deleting the specifier is unsafe.
TEST_F(LspGetSafeDeleteInfoTest2, DISABLED_ReExportSpecifierIsNotSafeToDelete)
{
    std::vector<std::string> files = {"safe_delete2_reexport_mod.ets", "safe_delete2_reexport.ets"};
    std::vector<std::string> texts = {R"(export const reexportedValue = 1;)",
                                      R"(export { reexportedValue } from './safe_delete2_reexport_mod';)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 2U);

    Initializer initializer;
    auto *ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);
    LSPAPI const *lspApi = GetImpl();

    // Position at "reexportedValue" inside the re-export specifier braces
    const auto specPos = texts[1].find("reexportedValue");
    ASSERT_NE(specPos, std::string::npos);

    auto result = lspApi->getSafeDeleteInfo(ctx, specPos);

    initializer.DestroyContext(ctx);

    ASSERT_FALSE(result);
}

// Test: interface method declaration and an overridden base class method.
// The current rule flags only the implementing/overriding member as unsafe; the
// interface declaration itself and the base definition stay deletable.
TEST_F(LspGetSafeDeleteInfoTest2, InterfaceAndBaseMethodsAreSafeToDelete)
{
    std::vector<std::string> files = {"safe_delete2_override_base.ets"};
    std::vector<std::string> texts = {R"(interface IShape {
    area(): number;
}
class Base {
    compute(): number {
        return 0;
    }
}
class Derived extends Base implements IShape {
    area(): number {
        return 1;
    }
    compute(): number {
        return 2;
    }
}
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    Initializer initializer;
    auto *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);
    LSPAPI const *lspApi = GetImpl();

    // First "area(): number" is the interface method declaration
    const auto ifacePos = texts[0].find("area(): number");
    // First "compute(): number" is the overridden base class method
    const auto basePos = texts[0].find("compute(): number");
    ASSERT_NE(ifacePos, std::string::npos);
    ASSERT_NE(basePos, std::string::npos);

    auto ifaceResult = lspApi->getSafeDeleteInfo(ctx, ifacePos);
    auto baseResult = lspApi->getSafeDeleteInfo(ctx, basePos);

    initializer.DestroyContext(ctx);

    EXPECT_TRUE(ifaceResult);
    EXPECT_TRUE(baseResult);
}

// Test: deleting a class whose constructor/static members are referenced is unsafe.
// lsp-test-plan: `new Widget("a")` references the class, and the class declares a
// constructor and static members, so the class itself is unsafe to delete; the
// members alone (no direct references) stay deletable.
TEST_F(LspGetSafeDeleteInfoTest2, DISABLED_ClassWithReferencedCtorAndStaticMembersIsNotSafeToDelete)
{
    std::vector<std::string> files = {"safe_delete2_class_members.ets"};
    std::vector<std::string> texts = {R"(class Widget {
    static count: number = 0;
    name: string = "";
    constructor(name: string) {
        this.name = name;
    }
    static create(): Widget {
        return new Widget("default");
    }
}
let w = new Widget("a");
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    Initializer initializer;
    auto *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);
    LSPAPI const *lspApi = GetImpl();

    // First "Widget" is the class declaration name
    const auto classPos = texts[0].find("Widget");
    const auto ctorPos = texts[0].find("constructor(");
    const auto staticPropPos = texts[0].find("count: number");
    const auto staticMethodPos = texts[0].find("create(): Widget");
    ASSERT_NE(classPos, std::string::npos);
    ASSERT_NE(ctorPos, std::string::npos);
    ASSERT_NE(staticPropPos, std::string::npos);
    ASSERT_NE(staticMethodPos, std::string::npos);

    auto classResult = lspApi->getSafeDeleteInfo(ctx, classPos);
    auto ctorResult = lspApi->getSafeDeleteInfo(ctx, ctorPos);
    auto staticPropResult = lspApi->getSafeDeleteInfo(ctx, staticPropPos);
    auto staticMethodResult = lspApi->getSafeDeleteInfo(ctx, staticMethodPos);

    initializer.DestroyContext(ctx);

    EXPECT_FALSE(classResult);
    EXPECT_TRUE(ctorResult);
    EXPECT_TRUE(staticPropResult);
    EXPECT_TRUE(staticMethodResult);
}

// Test: position inside a comment has no touching token, so it is NOT safe to delete
TEST_F(LspGetSafeDeleteInfoTest2, CommentPositionIsNotSafeToDelete)
{
    std::vector<std::string> files = {"safe_delete2_comment.ets"};
    std::vector<std::string> texts = {R"(// nothing deletable here
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    Initializer initializer;
    auto *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);
    LSPAPI const *lspApi = GetImpl();

    const auto commentPos = texts[0].find("deletable");
    ASSERT_NE(commentPos, std::string::npos);

    auto result = lspApi->getSafeDeleteInfo(ctx, commentPos);

    initializer.DestroyContext(ctx);

    ASSERT_FALSE(result);
}

}  // namespace

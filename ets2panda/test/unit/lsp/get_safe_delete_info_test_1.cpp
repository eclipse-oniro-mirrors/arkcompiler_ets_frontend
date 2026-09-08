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

class LspGetSafeDeleteInfoTest1 : public LSPAPITests {};

// Test: exported symbol (class) should be safe to delete
TEST_F(LspGetSafeDeleteInfoTest1, DISABLED_ExportedClassIsNotSafeToDelete)
{
    std::vector<std::string> files = {"safe_delete_exported_class.ets"};
    std::vector<std::string> texts = {R"(export class ExportedClass {
    value: number = 0;
})"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    Initializer initializer;
    auto *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);
    LSPAPI const *lspApi = GetImpl();

    // Position at "ExportedClass" in the class declaration
    const auto pos = texts[0].find("ExportedClass");
    ASSERT_NE(pos, std::string::npos);

    auto result = lspApi->getSafeDeleteInfo(ctx, pos);
    // lsp-test-plan: exported symbols are never safe to delete.
    ASSERT_FALSE(result);

    initializer.DestroyContext(ctx);
}

// Test: exported function should be safe to delete
TEST_F(LspGetSafeDeleteInfoTest1, DISABLED_ExportedFunctionIsNotSafeToDelete)
{
    std::vector<std::string> files = {"safe_delete_exported_func.ets"};
    std::vector<std::string> texts = {R"(export function exportedFunc(): void {
    console.log("hello");
})"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    Initializer initializer;
    auto *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);
    LSPAPI const *lspApi = GetImpl();

    // Position at "exportedFunc" in the function declaration
    const auto pos = texts[0].find("exportedFunc");
    ASSERT_NE(pos, std::string::npos);

    auto result = lspApi->getSafeDeleteInfo(ctx, pos);
    // lsp-test-plan: exported symbols are never safe to delete.
    ASSERT_FALSE(result);

    initializer.DestroyContext(ctx);
}

// Test: imported symbol should be safe to delete (the import binding, not the source)
TEST_F(LspGetSafeDeleteInfoTest1, ImportedSymbolIsSafeToDelete)
{
    std::vector<std::string> files = {"safe_delete_import_source.ets", "safe_delete_import_consumer.ets"};
    std::vector<std::string> texts = {R"(export function importedFunc(): void {})",
                                      R"(import { importedFunc } from './safe_delete_import_source';
importedFunc();)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 2U);

    Initializer initializer;
    auto *ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);
    LSPAPI const *lspApi = GetImpl();

    // Position at "importedFunc" in the import statement
    const auto pos = texts[1].find("importedFunc }");
    ASSERT_NE(pos, std::string::npos);

    auto result = lspApi->getSafeDeleteInfo(ctx, pos);
    // Import binding should be safe to delete
    ASSERT_TRUE(result);

    initializer.DestroyContext(ctx);
}

// Test: class method that overrides a base class method should be safe to delete
TEST_F(LspGetSafeDeleteInfoTest1, DISABLED_OverrideMethodIsNotSafeToDelete)
{
    std::vector<std::string> files = {"safe_delete_override.ets"};
    std::vector<std::string> texts = {R"(class Base {
    greet(): void {
        console.log("Hello");
    }
}
class Derived extends Base {
    greet(): void {
        super.greet();
    }
})"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    Initializer initializer;
    auto *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);
    LSPAPI const *lspApi = GetImpl();

    // Position at "greet" in the Derived class override
    const auto derivedGreet = texts[0].rfind("greet(): void");
    ASSERT_NE(derivedGreet, std::string::npos);

    auto result = lspApi->getSafeDeleteInfo(ctx, derivedGreet);
    // lsp-test-plan: an override member is never safe to delete.
    ASSERT_FALSE(result);

    initializer.DestroyContext(ctx);
}

// Test: interface method implementation should be safe to delete
TEST_F(LspGetSafeDeleteInfoTest1, DISABLED_InterfaceImplementationIsNotSafeToDelete)
{
    std::vector<std::string> files = {"safe_delete_interface_impl.ets"};
    std::vector<std::string> texts = {R"(interface IShape {
    area(): number;
}
class Circle implements IShape {
    radius: number = 1;
    area(): number {
        return this.radius * this.radius * 3;
    }
})"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    Initializer initializer;
    auto *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);
    LSPAPI const *lspApi = GetImpl();

    // Position at "area" in the Circle class implementation
    const auto implArea = texts[0].rfind("area(): number");
    ASSERT_NE(implArea, std::string::npos);

    auto result = lspApi->getSafeDeleteInfo(ctx, implArea);
    // lsp-test-plan: an interface-implementation member is never safe to delete.
    ASSERT_FALSE(result);

    initializer.DestroyContext(ctx);
}

// Test: builtin type reference should NOT be safe to delete
TEST_F(LspGetSafeDeleteInfoTest1, BuiltinTypeReferenceIsNotSafeToDelete)
{
    std::vector<std::string> files = {"safe_delete_builtin.ets"};
    std::vector<std::string> texts = {R"(let value: Number = 42;
let text: String = "hello";
let flag: Boolean = true;)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    Initializer initializer;
    auto *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);
    LSPAPI const *lspApi = GetImpl();

    // Position at "Number" (builtin type - capitalized form in BUILTIN_TYPES)
    const auto numberPos = texts[0].find("Number");
    ASSERT_NE(numberPos, std::string::npos);

    auto result = lspApi->getSafeDeleteInfo(ctx, numberPos);
    // Builtin type reference should NOT be safe to delete
    ASSERT_FALSE(result);

    initializer.DestroyContext(ctx);
}

// Test: type parameter should NOT be safe to delete
TEST_F(LspGetSafeDeleteInfoTest1, TypeParameterIsNotSafeToDelete)
{
    std::vector<std::string> files = {"safe_delete_typeparam.ets"};
    std::vector<std::string> texts = {R"(function identity<T>(value: T): T {
    return value;
})"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    Initializer initializer;
    auto *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);
    LSPAPI const *lspApi = GetImpl();

    // Position at "T" type parameter in the generic declaration
    const auto tPos = texts[0].find("<T>");
    ASSERT_NE(tPos, std::string::npos);
    const auto tIdentPos = tPos + 1;  // position at "T"

    auto result = lspApi->getSafeDeleteInfo(ctx, tIdentPos);
    // Type parameter should NOT be safe to delete
    ASSERT_FALSE(result);

    initializer.DestroyContext(ctx);
}

// Test: local variable declaration should be safe to delete
TEST_F(LspGetSafeDeleteInfoTest1, LocalVariableIsSafeToDelete)
{
    std::vector<std::string> files = {"safe_delete_local_var.ets"};
    std::vector<std::string> texts = {R"(function compute(): void {
    let temp: number = 0;
    let result: number = temp + 1;
})"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    Initializer initializer;
    auto *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);
    LSPAPI const *lspApi = GetImpl();

    // Position at "temp" variable declaration
    const auto tempPos = texts[0].find("temp: number");
    ASSERT_NE(tempPos, std::string::npos);

    auto result = lspApi->getSafeDeleteInfo(ctx, tempPos);
    // Local variable declaration should be safe to delete
    ASSERT_TRUE(result);

    initializer.DestroyContext(ctx);
}

// Test: enum declaration should be safe to delete
TEST_F(LspGetSafeDeleteInfoTest1, EnumDeclarationIsSafeToDelete)
{
    std::vector<std::string> files = {"safe_delete_enum.ets"};
    std::vector<std::string> texts = {R"(enum Direction {
    Up,
    Down,
    Left,
    Right
})"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    Initializer initializer;
    auto *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);
    LSPAPI const *lspApi = GetImpl();

    // Position at "Direction" enum name
    const auto dirPos = texts[0].find("Direction");
    ASSERT_NE(dirPos, std::string::npos);

    auto result = lspApi->getSafeDeleteInfo(ctx, dirPos);
    // Enum declaration should be safe to delete
    ASSERT_TRUE(result);

    initializer.DestroyContext(ctx);
}

// Test: class property should be safe to delete
TEST_F(LspGetSafeDeleteInfoTest1, ClassPropertyIsSafeToDelete)
{
    std::vector<std::string> files = {"safe_delete_property.ets"};
    std::vector<std::string> texts = {R"(class Container {
    items: number = 0;
    capacity: number = 100;
})"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    Initializer initializer;
    auto *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);
    LSPAPI const *lspApi = GetImpl();

    // Position at "capacity" property declaration
    const auto capPos = texts[0].find("capacity: number");
    ASSERT_NE(capPos, std::string::npos);

    auto result = lspApi->getSafeDeleteInfo(ctx, capPos);
    // Class property declaration should be safe to delete
    ASSERT_TRUE(result);

    initializer.DestroyContext(ctx);
}

}  // namespace

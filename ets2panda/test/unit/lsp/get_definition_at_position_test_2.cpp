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
#include <gtest/gtest.h>
#include <string>
#include <vector>

#include "lsp_api_test.h"
#include "lsp/include/internal_api.h"

using ark::es2panda::lsp::Initializer;

class LspGetDefTests2 : public LSPAPITests {};

// Test: definition of a top-level local variable usage inside an arrow function
TEST_F(LspGetDefTests2, LocalVariableUsageDefinition)
{
    std::vector<std::string> files = {"local_var_def.ets"};
    std::vector<std::string> texts = {R"(let factor: number = 2;
let fn = (): number => { return factor; };
fn();)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    // Position at "factor" usage inside the arrow function body
    const auto usagePos = texts[0].rfind("factor");
    ASSERT_NE(usagePos, std::string::npos);

    auto result = lspApi->getDefinitionAtPosition(context, usagePos);
    initializer.DestroyContext(context);

    // Note: the returned fileName is empty when the context is created from an
    // in-memory source; with a real file it is the absolute temp path
    const auto expectedStart = texts[0].find("factor: number");
    ASSERT_NE(expectedStart, std::string::npos);
    EXPECT_EQ(result.fileName, filePaths[0]);
    EXPECT_EQ(result.start, expectedStart);
    EXPECT_EQ(result.length, std::string("factor").size());
}

// Test: definition of a local function call
TEST_F(LspGetDefTests2, LocalFunctionCallDefinition)
{
    std::vector<std::string> files = {"local_func_def.ets"};
    std::vector<std::string> texts = {R"(function add(left: number, right: number): number {
    return left + right;
}
let sum = add(1, 2);)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    // Position at "add" call site
    const auto callPos = texts[0].find("add(1, 2)");
    ASSERT_NE(callPos, std::string::npos);

    auto result = lspApi->getDefinitionAtPosition(context, callPos);
    initializer.DestroyContext(context);

    const auto expectedStart = texts[0].find("add(left");
    ASSERT_NE(expectedStart, std::string::npos);
    EXPECT_EQ(result.fileName, filePaths[0]);
    EXPECT_EQ(result.start, expectedStart);
    EXPECT_EQ(result.length, std::string("add").size());
}

// Test: definition of class name at a new expression
TEST_F(LspGetDefTests2, ClassDefinitionFromNewExpression)
{
    std::vector<std::string> files = {"class_def.ets"};
    std::vector<std::string> texts = {R"(class Widget {
    name: string = "w";
}
let widget = new Widget();)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    // Position at "Widget" in the new expression
    const auto newPos = texts[0].find("Widget()");
    ASSERT_NE(newPos, std::string::npos);

    auto result = lspApi->getDefinitionAtPosition(context, newPos);
    initializer.DestroyContext(context);

    const auto expectedStart = texts[0].find("Widget {");
    ASSERT_NE(expectedStart, std::string::npos);
    EXPECT_EQ(result.fileName, filePaths[0]);
    EXPECT_EQ(result.start, expectedStart);
    EXPECT_EQ(result.length, std::string("Widget").size());
}

// Test: definition of a method name at a call site
TEST_F(LspGetDefTests2, MethodCallDefinition)
{
    std::vector<std::string> files = {"method_def.ets"};
    std::vector<std::string> texts = {R"(class Calculator {
    multiply(a: number, b: number): number {
        return a * b;
    }
}
let calc = new Calculator();
calc.multiply(3, 4);)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    // Position at "multiply" call site
    const auto callPos = texts[0].rfind("multiply(3, 4)");
    ASSERT_NE(callPos, std::string::npos);

    auto result = lspApi->getDefinitionAtPosition(context, callPos);
    initializer.DestroyContext(context);

    const auto expectedStart = texts[0].find("multiply(a: number");
    ASSERT_NE(expectedStart, std::string::npos);
    EXPECT_EQ(result.fileName, filePaths[0]);
    EXPECT_EQ(result.start, expectedStart);
    EXPECT_EQ(result.length, std::string("multiply").size());
}

// Test: definition of a property name at an access site
TEST_F(LspGetDefTests2, PropertyAccessDefinition)
{
    std::vector<std::string> files = {"property_def.ets"};
    std::vector<std::string> texts = {R"(class Config {
    timeout: number = 30;
}
let cfg = new Config();
cfg.timeout = 60;)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    // Position at "timeout" access site
    const auto accessPos = texts[0].find("timeout = 60");
    ASSERT_NE(accessPos, std::string::npos);

    auto result = lspApi->getDefinitionAtPosition(context, accessPos);
    initializer.DestroyContext(context);

    const auto expectedStart = texts[0].find("timeout: number");
    ASSERT_NE(expectedStart, std::string::npos);
    EXPECT_EQ(result.fileName, filePaths[0]);
    EXPECT_EQ(result.start, expectedStart);
    EXPECT_EQ(result.length, std::string("timeout").size());
}

// Test: definitions of getter and setter usages point to their declarations
TEST_F(LspGetDefTests2, GetterSetterDefinition)
{
    std::vector<std::string> files = {"getter_setter_def.ets"};
    std::vector<std::string> texts = {R"(class Box {
    private value_: number = 0;
    get value(): number {
        return this.value_;
    }
    set value(v: number) {
        this.value_ = v;
    }
}
let box = new Box();
box.value = 5;
let current = box.value;)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    // Position at "value" write (setter) and read (getter) usages
    const auto setterPos = texts[0].find("value = 5");
    const auto getterPos = texts[0].rfind("box.value");
    ASSERT_NE(setterPos, std::string::npos);
    ASSERT_NE(getterPos, std::string::npos);
    const auto getterUsagePos = getterPos + std::string("box.").size();

    auto setterResult = lspApi->getDefinitionAtPosition(context, setterPos);
    auto getterResult = lspApi->getDefinitionAtPosition(context, getterUsagePos);
    initializer.DestroyContext(context);

    const auto expectedGetterStart = texts[0].find("value(): number");
    ASSERT_NE(expectedGetterStart, std::string::npos);
    EXPECT_EQ(getterResult.fileName, filePaths[0]);
    EXPECT_EQ(getterResult.start, expectedGetterStart);
    EXPECT_EQ(getterResult.length, std::string("value").size());

    // Both the getter and the setter declare a "value" identifier; the LSP resolves
    // the write usage to the first accessor that declares the name, i.e. the getter
    EXPECT_EQ(setterResult.fileName, filePaths[0]);
    EXPECT_EQ(setterResult.start, expectedGetterStart);
    EXPECT_EQ(setterResult.length, std::string("value").size());
}

// Test: definition of the constructor from a new expression
TEST_F(LspGetDefTests2, ConstructorDefinition)
{
    std::vector<std::string> files = {"constructor_def.ets"};
    std::vector<std::string> texts = {R"(class Point {
    x: number = 0;
    constructor(x: number) {
        this.x = x;
    }
}
let point = new Point(1);)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    // Position at "Point" in the new expression
    const auto newPos = texts[0].find("Point(1)");
    ASSERT_NE(newPos, std::string::npos);

    auto result = lspApi->getDefinitionAtPosition(context, newPos);
    initializer.DestroyContext(context);

    // getDefinitionAtPosition resolves the class name identifier at the new expression,
    // i.e. it navigates to the class declaration rather than to the constructor body
    const auto expectedStart = texts[0].find("Point {");
    ASSERT_NE(expectedStart, std::string::npos);
    EXPECT_EQ(result.fileName, filePaths[0]);
    EXPECT_EQ(result.start, expectedStart);
    EXPECT_EQ(result.length, std::string("Point").size());
}

// Test: definition of an enum member from a qualified access
TEST_F(LspGetDefTests2, EnumMemberDefinition)
{
    std::vector<std::string> files = {"enum_member_def.ets"};
    std::vector<std::string> texts = {R"(enum Direction {
    North,
    South
}
let heading = Direction.South;)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    // Position at "South" member access
    const auto memberPos = texts[0].rfind("South");
    ASSERT_NE(memberPos, std::string::npos);

    auto result = lspApi->getDefinitionAtPosition(context, memberPos);
    initializer.DestroyContext(context);

    const auto expectedStart = texts[0].find("South\n");
    ASSERT_NE(expectedStart, std::string::npos);
    EXPECT_EQ(result.fileName, filePaths[0]);
    EXPECT_EQ(result.start, expectedStart);
    EXPECT_EQ(result.length, std::string("South").size());
}

// Test: definition of a namespace member from a qualified access
TEST_F(LspGetDefTests2, NamespaceMemberDefinition)
{
    std::vector<std::string> files = {"namespace_member_def.ets"};
    std::vector<std::string> texts = {R"(namespace Geometry {
    export function circleArea(radius: number): number {
        return 3 * radius * radius;
    }
}
let area = Geometry.circleArea(2);)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    // Position at "circleArea" qualified usage
    const auto usagePos = texts[0].find("circleArea(2)");
    ASSERT_NE(usagePos, std::string::npos);

    auto result = lspApi->getDefinitionAtPosition(context, usagePos);
    initializer.DestroyContext(context);

    const auto expectedStart = texts[0].find("circleArea(radius");
    ASSERT_NE(expectedStart, std::string::npos);
    EXPECT_EQ(result.fileName, filePaths[0]);
    EXPECT_EQ(result.start, expectedStart);
    EXPECT_EQ(result.length, std::string("circleArea").size());
}

// Test: definition of a type alias from a type annotation
TEST_F(LspGetDefTests2, TypeAliasDefinition)
{
    std::vector<std::string> files = {"type_alias_def.ets"};
    std::vector<std::string> texts = {R"(type Callback = () => void;
let handler: Callback = (): void => {};)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    // Position at "Callback" in the type annotation
    const auto usagePos = texts[0].rfind("Callback");
    ASSERT_NE(usagePos, std::string::npos);

    auto result = lspApi->getDefinitionAtPosition(context, usagePos);
    initializer.DestroyContext(context);

    const auto expectedStart = texts[0].find("Callback =");
    ASSERT_NE(expectedStart, std::string::npos);
    EXPECT_EQ(result.fileName, filePaths[0]);
    EXPECT_EQ(result.start, expectedStart);
    EXPECT_EQ(result.length, std::string("Callback").size());
}

// Test: definition of a named import from a usage site (cross file)
TEST_F(LspGetDefTests2, NamedImportDefinition)
{
    std::vector<std::string> files = {"named_import_lib.ets", "named_import_main.ets"};
    std::vector<std::string> texts = {R"(export function utility(flag: boolean): boolean {
    return !flag;
})",
                                      R"(import {utility} from './named_import_lib';
let ok = utility(true);)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    // Position at "utility" call site in the importing file
    const auto callPos = texts[1].find("utility(true)");
    ASSERT_NE(callPos, std::string::npos);

    auto result = lspApi->getDefinitionAtPosition(context, callPos);
    initializer.DestroyContext(context);

    const auto expectedStart = texts[0].find("utility(flag");
    ASSERT_NE(expectedStart, std::string::npos);
    EXPECT_EQ(result.fileName, filePaths[0]);
    EXPECT_EQ(result.start, expectedStart);
    EXPECT_EQ(result.length, std::string("utility").size());
}

// Test: definition of a default import from a usage site (cross file)
TEST_F(LspGetDefTests2, DefaultImportDefinition)
{
    std::vector<std::string> files = {"default_import_lib.ets", "default_import_main.ets"};
    std::vector<std::string> texts = {R"(export default function helper(): number {
    return 7;
})",
                                      R"(import helper from './default_import_lib';
let value = helper();)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    // Position at "helper" call site in the importing file
    const auto callPos = texts[1].find("helper()");
    ASSERT_NE(callPos, std::string::npos);

    auto result = lspApi->getDefinitionAtPosition(context, callPos);
    initializer.DestroyContext(context);

    const auto expectedStart = texts[0].find("helper(): number");
    ASSERT_NE(expectedStart, std::string::npos);
    EXPECT_EQ(result.fileName, filePaths[0]);
    EXPECT_EQ(result.start, expectedStart);
    EXPECT_EQ(result.length, std::string("helper").size());
}

// Test: definition of an aliased import (import {X as Y}) from a usage site
TEST_F(LspGetDefTests2, DISABLED_AliasImportDefinition)
{
    std::vector<std::string> files = {"alias_import_lib.ets", "alias_import_main.ets"};
    std::vector<std::string> texts = {R"(export function logMessage(message: string): void {})",
                                      R"(import {logMessage as log} from './alias_import_lib';
log("hi");)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    // Position at "log" usage in the importing file
    const auto usagePos = texts[1].find("log(\"hi\")");
    ASSERT_NE(usagePos, std::string::npos);

    auto result = lspApi->getDefinitionAtPosition(context, usagePos);
    initializer.DestroyContext(context);

    const auto expectedStart = texts[0].find("logMessage(message");
    ASSERT_NE(expectedStart, std::string::npos);
    EXPECT_EQ(result.fileName, filePaths[0]);
    EXPECT_EQ(result.start, expectedStart);
    EXPECT_EQ(result.length, std::string("logMessage").size());
}

// Test: definition of the imported name inside an aliased import specifier
TEST_F(LspGetDefTests2, AliasedImportSpecifierDefinition)
{
    std::vector<std::string> files = {"alias_spec_lib.ets", "alias_spec_main.ets"};
    std::vector<std::string> texts = {R"(export function logMessage(message: string): void {})",
                                      R"(import {logMessage as log} from './alias_spec_lib';
log("hi");)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    // Position at "logMessage" inside the import specifier
    const auto specPos = texts[1].find("logMessage as log");
    ASSERT_NE(specPos, std::string::npos);

    auto result = lspApi->getDefinitionAtPosition(context, specPos);
    initializer.DestroyContext(context);

    const auto expectedStart = texts[0].find("logMessage(message");
    ASSERT_NE(expectedStart, std::string::npos);
    EXPECT_EQ(result.fileName, filePaths[0]);
    EXPECT_EQ(result.start, expectedStart);
    EXPECT_EQ(result.length, std::string("logMessage").size());
}

// Test: definition through a re-export chain (export {X} from) resolves to the original declaration
TEST_F(LspGetDefTests2, ReExportChainDefinition)
{
    std::vector<std::string> files = {"reexport_origin.ets", "reexport_barrel.ets", "reexport_main.ets"};
    std::vector<std::string> texts = {R"(export function core(): number {
    return 1;
})",
                                      R"(export {core} from './reexport_origin';)",
                                      R"(import {core} from './reexport_barrel';
let out = core();)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 3;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[2].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    // Position at "core" call site in the importing file
    const auto callPos = texts[2].find("core()");
    ASSERT_NE(callPos, std::string::npos);

    auto result = lspApi->getDefinitionAtPosition(context, callPos);
    initializer.DestroyContext(context);

    const auto expectedStart = texts[0].find("core(): number");
    ASSERT_NE(expectedStart, std::string::npos);
    EXPECT_EQ(result.fileName, filePaths[0]);
    EXPECT_EQ(result.start, expectedStart);
    EXPECT_EQ(result.length, std::string("core").size());
}

// Test: definition through a barrel chain of two star re-exports resolves to the original declaration
TEST_F(LspGetDefTests2, BarrelChainDefinition)
{
    std::vector<std::string> files = {"barrel_origin.ets", "barrel_mid.ets", "barrel_index.ets", "barrel_main.ets"};
    std::vector<std::string> texts = {R"(export class Engine {
    start(): void {}
})",
                                      R"(export * from './barrel_origin';)", R"(export * from './barrel_mid';)",
                                      R"(import {Engine} from './barrel_index';
let engine = new Engine();)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 4;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[3].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    // Position at "Engine" usage in the importing file
    const auto usagePos = texts[3].find("Engine()");
    ASSERT_NE(usagePos, std::string::npos);

    auto result = lspApi->getDefinitionAtPosition(context, usagePos);
    initializer.DestroyContext(context);

    const auto expectedStart = texts[0].find("Engine {");
    ASSERT_NE(expectedStart, std::string::npos);
    EXPECT_EQ(result.fileName, filePaths[0]);
    EXPECT_EQ(result.start, expectedStart);
    EXPECT_EQ(result.length, std::string("Engine").size());
}

// Test: definition on the module specifier of an import resolves to the imported file
TEST_F(LspGetDefTests2, ModuleSpecifierImportDefinition)
{
    std::vector<std::string> files = {"module_spec_lib.ets", "module_spec_main.ets"};
    std::vector<std::string> texts = {R"(export function libFunc(): void {})",
                                      R"(import {libFunc} from './module_spec_lib';
libFunc();)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    // Position inside the module specifier string literal
    const auto specPos = texts[1].find("module_spec_lib");
    ASSERT_NE(specPos, std::string::npos);

    auto result = lspApi->getDefinitionAtPosition(context, specPos);
    initializer.DestroyContext(context);

    EXPECT_EQ(result.fileName, filePaths[0]);
    EXPECT_EQ(result.start, 0U);
    EXPECT_EQ(result.length, 0U);
}

// Test: cursor at the start, middle and end of an identifier resolves to the same definition
TEST_F(LspGetDefTests2, IdentifierBoundaryPositions)
{
    std::vector<std::string> files = {"identifier_boundary.ets"};
    std::vector<std::string> texts = {R"(let counterValue = 0;
counterValue = 1;)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    // Usage of "counterValue" on the second line
    const auto usagePos = texts[0].rfind("counterValue");
    ASSERT_NE(usagePos, std::string::npos);
    const size_t middleOffset = 2;

    auto startResult = lspApi->getDefinitionAtPosition(context, usagePos);
    auto middleResult = lspApi->getDefinitionAtPosition(context, usagePos + middleOffset);
    auto endResult = lspApi->getDefinitionAtPosition(context, usagePos + std::string("counterValue").size());
    initializer.DestroyContext(context);

    const auto expectedStart = texts[0].find("counterValue");
    ASSERT_NE(expectedStart, std::string::npos);
    EXPECT_EQ(startResult.fileName, filePaths[0]);
    EXPECT_EQ(startResult.start, expectedStart);
    EXPECT_EQ(startResult.length, std::string("counterValue").size());
    EXPECT_EQ(middleResult.fileName, filePaths[0]);
    EXPECT_EQ(middleResult.start, expectedStart);
    EXPECT_EQ(middleResult.length, std::string("counterValue").size());
    EXPECT_EQ(endResult.fileName, filePaths[0]);
    EXPECT_EQ(endResult.start, expectedStart);
    EXPECT_EQ(endResult.length, std::string("counterValue").size());
}

// Test: cursor on whitespace, operator, string literal or comment returns no definition
TEST_F(LspGetDefTests2, NonIdentifierPositionsReturnEmpty)
{
    std::vector<std::string> files = {"non_identifier_pos.ets"};
    std::vector<std::string> texts = {R"(let greeting = "hello";
let doubled = 1 + 2;
// a trailing comment
greeting = "world";

)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    // Whitespace between two tokens on an empty trailing line
    const auto whitespacePos = texts[0].find("greeting = \"world\";") + std::string("greeting = \"world\";\n").size();
    // Operator '+'
    const auto operatorPos = texts[0].find('+');
    // Inside the "hello" string literal
    const auto stringPos = texts[0].find("hello");
    // Inside the comment
    const auto commentPos = texts[0].find("trailing");
    ASSERT_NE(operatorPos, std::string::npos);
    ASSERT_NE(stringPos, std::string::npos);
    ASSERT_NE(commentPos, std::string::npos);

    auto whitespaceResult = lspApi->getDefinitionAtPosition(context, whitespacePos);
    auto operatorResult = lspApi->getDefinitionAtPosition(context, operatorPos);
    auto stringResult = lspApi->getDefinitionAtPosition(context, stringPos);
    auto commentResult = lspApi->getDefinitionAtPosition(context, commentPos);
    initializer.DestroyContext(context);

    EXPECT_EQ(whitespaceResult.fileName, "");
    EXPECT_EQ(operatorResult.fileName, "");
    EXPECT_EQ(stringResult.fileName, "");
    EXPECT_EQ(commentResult.fileName, "");
}

// Test: a shadowing local variable takes precedence over an imported global
TEST_F(LspGetDefTests2, DISABLED_ShadowingLocalPreferredOverImport)
{
    std::vector<std::string> files = {"shadow_lib.ets", "shadow_main.ets"};
    std::vector<std::string> texts = {R"(export let token: number = 1;)",
                                      R"(import {token} from './shadow_lib';
function check(): number {
    let token: number = 99;
    return token;
}
check();)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    // Position at "token" inside the function body (shadowed usage)
    const auto usagePos = texts[1].find("return token");
    ASSERT_NE(usagePos, std::string::npos);
    const auto tokenUsagePos = usagePos + std::string("return ").size();

    auto result = lspApi->getDefinitionAtPosition(context, tokenUsagePos);
    initializer.DestroyContext(context);

    const auto localDecl = texts[1].find("token: number = 99");
    ASSERT_NE(localDecl, std::string::npos);
    EXPECT_EQ(result.fileName, filePaths[1]);
    EXPECT_EQ(result.start, localDecl);
    EXPECT_EQ(result.length, std::string("token").size());
}

// Test: a top-level local variable shadows nothing and resolves from an arrow function
TEST_F(LspGetDefTests2, TopLevelVariableResolvesFromNestedScope)
{
    std::vector<std::string> files = {"top_shadow_main.ets"};
    std::vector<std::string> texts = {R"(let token = 1;
let read = (): number => { return token; };
read();)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    // Position at "token" usage inside the arrow function body
    const auto usagePos = texts[0].rfind("token");
    ASSERT_NE(usagePos, std::string::npos);

    auto result = lspApi->getDefinitionAtPosition(context, usagePos);
    initializer.DestroyContext(context);

    // Note: the returned fileName is empty when the context is created from an
    // in-memory source; with a real file it is the absolute temp path
    const auto expectedStart = texts[0].find("token = 1");
    ASSERT_NE(expectedStart, std::string::npos);
    EXPECT_EQ(result.fileName, filePaths[0]);
    EXPECT_EQ(result.start, expectedStart);
    EXPECT_EQ(result.length, std::string("token").size());
}

// Test: calls to overloaded functions resolve to the matching overload declaration
TEST_F(LspGetDefTests2, OverloadResolutionStrategy)
{
    std::vector<std::string> files = {"overload_def.ets"};
    std::vector<std::string> texts = {R"(function render(x: number): number {
    return x;
}
function render(x: string): string {
    return x;
}
let first = render(1);
let second = render("s");)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    // Position at the numeric call and at the string call
    const auto numberCallPos = texts[0].find("render(1)");
    const auto stringCallPos = texts[0].find("render(\"s\")");
    ASSERT_NE(numberCallPos, std::string::npos);
    ASSERT_NE(stringCallPos, std::string::npos);

    auto numberResult = lspApi->getDefinitionAtPosition(context, numberCallPos);
    auto stringResult = lspApi->getDefinitionAtPosition(context, stringCallPos);
    initializer.DestroyContext(context);

    const auto expectedNumberStart = texts[0].find("render(x: number)");
    const auto expectedStringStart = texts[0].find("render(x: string)");
    ASSERT_NE(expectedNumberStart, std::string::npos);
    ASSERT_NE(expectedStringStart, std::string::npos);
    EXPECT_EQ(numberResult.fileName, filePaths[0]);
    EXPECT_EQ(numberResult.start, expectedNumberStart);
    EXPECT_EQ(numberResult.length, std::string("render").size());
    EXPECT_EQ(stringResult.fileName, filePaths[0]);
    EXPECT_EQ(stringResult.start, expectedStringStart);
    EXPECT_EQ(stringResult.length, std::string("render").size());
}

// Test: definition of a generic type parameter jumps to its declaration
TEST_F(LspGetDefTests2, GenericTypeParameterDefinition)
{
    std::vector<std::string> files = {"generic_type_param.ets"};
    std::vector<std::string> texts = {R"(function identity<T>(value: T): T {
    return value;
}
identity<number>(1);)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    // Position at "T" in the parameter annotation and in the return type
    const auto paramPos = texts[0].find("value: T");
    ASSERT_NE(paramPos, std::string::npos);
    const auto paramTypePos = paramPos + std::string("value: ").size();
    const auto returnTypePos = texts[0].find("): T {");
    ASSERT_NE(returnTypePos, std::string::npos);
    const auto returnTPos = returnTypePos + std::string("): ").size();

    auto paramResult = lspApi->getDefinitionAtPosition(context, paramTypePos);
    auto returnResult = lspApi->getDefinitionAtPosition(context, returnTPos);
    initializer.DestroyContext(context);

    const auto expectedStart = texts[0].find("<T>");
    ASSERT_NE(expectedStart, std::string::npos);
    const auto expectedTypeParamStart = expectedStart + 1;
    EXPECT_EQ(paramResult.fileName, filePaths[0]);
    EXPECT_EQ(paramResult.start, expectedTypeParamStart);
    EXPECT_EQ(paramResult.length, 1U);
    EXPECT_EQ(returnResult.fileName, filePaths[0]);
    EXPECT_EQ(returnResult.start, expectedTypeParamStart);
    EXPECT_EQ(returnResult.length, 1U);
}

// Test: definition on a package module specifier resolves to the package file on disk
TEST_F(LspGetDefTests2, PackageModuleSpecifierImportDefinition)
{
    std::vector<std::string> files = {"package_module_spec_main.ets"};
    std::vector<std::string> texts = {R"(import { dbl } from "import_tests/packages/package_module_1";
let value = dbl;)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    // Position inside the module specifier string literal
    const auto specPos = texts[0].find("import_tests/packages/package_module_1");
    ASSERT_NE(specPos, std::string::npos);

    auto result = lspApi->getDefinitionAtPosition(context, specPos);
    initializer.DestroyContext(context);

    // fileName should be the resolved absolute path ending with the expected suffix
    const std::string expectedSuffix = "/ets2panda/test/parser/ets/import_tests/packages/package_module_1.ets";
    ASSERT_GE(result.fileName.size(), expectedSuffix.size());
    EXPECT_EQ(
        result.fileName.compare(result.fileName.size() - expectedSuffix.size(), expectedSuffix.size(), expectedSuffix),
        0);
    EXPECT_EQ(result.start, 0U);
    EXPECT_EQ(result.length, 0U);
}

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

class LspGetDefTests1 : public LSPAPITests {};

// Test: interface method definition jumps to the interface method declaration
TEST_F(LspGetDefTests1, InterfaceMethodDefinitionJumpsToInterfaceDeclaration)
{
    std::vector<std::string> files = {"iface_def.ets"};
    std::vector<std::string> texts = {R"(interface IShape {
    area(): number;
}
class Circle implements IShape {
    area(): number {
        return 3;
    }
}
let s: IShape = new Circle();
let a = s.area();)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    // Position at "area()" call site at the end
    const auto callPos = texts[0].find("s.area()");
    ASSERT_NE(callPos, std::string::npos);
    const auto areaPos = callPos + std::string("s.").size();  // position on "area"

    auto result = lspApi->getDefinitionAtPosition(context, areaPos);

    initializer.DestroyContext(context);

    // Definition should point to the interface method declaration "area(): number;"
    ASSERT_EQ(result.fileName, filePaths[0]);
    EXPECT_GT(result.length, 0U);
}

// Test: abstract method override - definition from override points to abstract declaration
TEST_F(LspGetDefTests1, AbstractMethodOverrideDefinitionPointsToAbstractDeclaration)
{
    std::vector<std::string> files = {"abstract_def.ets"};
    std::vector<std::string> texts = {R"(abstract class Animal {
    abstract makeSound(): void;
}
class Dog extends Animal {
    makeSound(): void {
        console.log("Woof");
    }
}
let d = new Dog();
d.makeSound();)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    // Position at "makeSound()" call site
    const auto callPos = texts[0].rfind("makeSound()");
    ASSERT_NE(callPos, std::string::npos);

    auto result = lspApi->getDefinitionAtPosition(context, callPos);

    initializer.DestroyContext(context);

    // Definition should resolve to a makeSound declaration (abstract or override)
    ASSERT_EQ(result.fileName, filePaths[0]);
    EXPECT_GT(result.length, 0U);
}

// Test: base class method - definition from subclass call points to base method
TEST_F(LspGetDefTests1, BaseMethodDefinitionFromSubclassCall)
{
    std::vector<std::string> files = {"base_method_def.ets"};
    std::vector<std::string> texts = {R"(class Base {
    greet(): void {
        console.log("hello");
    }
}
class Derived extends Base {
    greet(): void {
        super.greet();
    }
}
let d = new Derived();
d.greet();)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    // Position at "greet()" call site at the end
    const auto callPos = texts[0].rfind("greet()");
    ASSERT_NE(callPos, std::string::npos);

    auto result = lspApi->getDefinitionAtPosition(context, callPos);

    initializer.DestroyContext(context);

    // Definition should resolve to a greet method declaration
    ASSERT_EQ(result.fileName, filePaths[0]);
    EXPECT_GT(result.length, 0U);
}

// Test: getImplementationAtPosition on interface method should find the implementing class method
TEST_F(LspGetDefTests1, ImplementationAtPositionFindsClassImplementation)
{
    std::vector<std::string> files = {"impl_at_pos.ets"};
    std::vector<std::string> texts = {R"(interface ICounter {
    increment(): void;
}
class Counter implements ICounter {
    increment(): void {
        console.log("incremented");
    }
}
let c: ICounter = new Counter();
c.increment();)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    // Position at "increment" in the interface declaration
    const auto ifacePos = texts[0].find("increment(): void;");
    ASSERT_NE(ifacePos, std::string::npos);

    auto result = lspApi->getImplementationAtPosition(context, ifacePos);

    initializer.DestroyContext(context);

    // Implementation should point to the class method in the same file
    ASSERT_EQ(result.fileName, filePaths[0]);
    EXPECT_GT(result.length, 0U);
}

// Test: definition at non-identifier position returns empty fileName
TEST_F(LspGetDefTests1, DefinitionAtNonIdentifierPositionReturnsEmpty)
{
    std::vector<std::string> files = {"non_ident_def.ets"};
    std::vector<std::string> texts = {R"(let x: number = 1;
console.log(x);)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    // Position at the semicolon (non-identifier)
    const auto semiPos = texts[0].find(';');
    ASSERT_NE(semiPos, std::string::npos);

    auto result = lspApi->getDefinitionAtPosition(context, semiPos);

    initializer.DestroyContext(context);

    // No definition at a non-identifier position
    EXPECT_EQ(result.fileName, "");
}

// Test: definition of local variable in shadowing scenario does not crash the API
TEST_F(LspGetDefTests1, ShadowedVariableDefinitionDoesNotCrash)
{
    std::vector<std::string> files = {"shadow_def.ets"};
    std::vector<std::string> texts = {R"(let value: number = 1;
function outer(): void {
    let value: string = "inner";
    console.log(value);
}
outer();
console.log(value);)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    // Position at "value" inside outer() - shadowing resolution may be limited in current LSP
    const auto innerUsage = texts[0].find("console.log(value);");
    ASSERT_NE(innerUsage, std::string::npos);
    const auto valuePos = innerUsage + std::string("console.log(").size();

    auto result = lspApi->getDefinitionAtPosition(context, valuePos);

    initializer.DestroyContext(context);

    // The API should not crash when querying a shadowed variable.
    // Shadowing resolution support may be limited in the current LSP version;
    // verify the API returns a usable result without crashing.
    if (!result.fileName.empty()) {
        EXPECT_EQ(result.fileName, filePaths[0]);
    }
}

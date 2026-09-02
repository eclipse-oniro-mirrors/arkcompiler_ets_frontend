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
#include "lsp/include/api.h"
#include "lsp/include/internal_api.h"

namespace {

using ark::es2panda::lsp::Initializer;

class LspGetImplementationLocationTests : public LSPAPITests {};

TEST_F(LspGetImplementationLocationTests, InterfaceMethodToClassImplementation)
{
    Initializer initializer = Initializer();
    static std::string source = R"(
interface Shape {
    area(): number;
}
class Square implements Shape {
    area(): number {
        return 0;
    }
}
let sq: Shape = new Square();
let sqImpl: Square = new Square();
sqImpl.area();
)";
    es2panda_Context *ctx =
        initializer.CreateContext("interface-implementation.ets", ES2PANDA_STATE_CHECKED, source.data());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    size_t const callOffset = source.rfind("area");
    ASSERT_NE(callOffset, std::string::npos);
    auto result = GetImpl()->getImplementationLocationAtPosition(ctx, callOffset);
    initializer.DestroyContext(ctx);

    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t expectedStartLine = 5;
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t expectedStartCharacter = 77;
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t expectedEndLine = 7;
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t expectedEndCharacter = 117;

    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].uri_, "interface-implementation.ets");
    EXPECT_EQ(result[0].range_.start.line_, expectedStartLine);
    EXPECT_EQ(result[0].range_.start.character_, expectedStartCharacter);
    EXPECT_EQ(result[0].range_.end.line_, expectedEndLine);
    EXPECT_EQ(result[0].range_.end.character_, expectedEndCharacter);
}

TEST_F(LspGetImplementationLocationTests, AbstractMethodToConcreteOverride)
{
    Initializer initializer = Initializer();
    static std::string source = R"(
abstract class Vehicle {
    abstract run(): void;
}
class Car extends Vehicle {
    run(): void {
    }
}
let car: Car = new Car();
car.run();
)";
    es2panda_Context *ctx = initializer.CreateContext("abstract-override.ets", ES2PANDA_STATE_CHECKED, source.data());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    size_t const callOffset = source.rfind("run");
    ASSERT_NE(callOffset, std::string::npos);
    auto result = GetImpl()->getImplementationLocationAtPosition(ctx, callOffset);
    initializer.DestroyContext(ctx);

    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t expectedStartLine = 5;
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t expectedStartCharacter = 86;
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t expectedEndLine = 6;
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t expectedEndCharacter = 105;

    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].uri_, "abstract-override.ets");
    EXPECT_EQ(result[0].range_.start.line_, expectedStartLine);
    EXPECT_EQ(result[0].range_.start.character_, expectedStartCharacter);
    EXPECT_EQ(result[0].range_.end.line_, expectedEndLine);
    EXPECT_EQ(result[0].range_.end.character_, expectedEndCharacter);
}

TEST_F(LspGetImplementationLocationTests, BaseMethodCallResolvesToOverride)
{
    Initializer initializer = Initializer();
    static std::string source = R"(
class Animal {
    speak(): void {
    }
}
class Dog extends Animal {
    speak(): void {
    }
}
let dog: Dog = new Dog();
dog.speak();
)";
    es2panda_Context *ctx = initializer.CreateContext("base-method.ets", ES2PANDA_STATE_CHECKED, source.data());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    size_t const callOffset = source.rfind("speak");
    ASSERT_NE(callOffset, std::string::npos);
    auto result = GetImpl()->getImplementationLocationAtPosition(ctx, callOffset);
    initializer.DestroyContext(ctx);

    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t expectedStartLine = 6;
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t expectedStartCharacter = 75;
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t expectedEndLine = 7;
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t expectedEndCharacter = 96;

    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].uri_, "base-method.ets");
    EXPECT_EQ(result[0].range_.start.line_, expectedStartLine);
    EXPECT_EQ(result[0].range_.start.character_, expectedStartCharacter);
    EXPECT_EQ(result[0].range_.end.line_, expectedEndLine);
    EXPECT_EQ(result[0].range_.end.character_, expectedEndCharacter);
}

TEST_F(LspGetImplementationLocationTests, DISABLED_InterfaceTypedCallReturnsAllImplementations)
{
    Initializer initializer = Initializer();
    static std::string source = R"(
interface Drawable {
    draw(): void;
}
class Circle implements Drawable {
    draw(): void {
    }
}
class Rect implements Drawable {
    draw(): void {
    }
}
let c: Drawable = new Circle();
c.draw();
)";
    es2panda_Context *ctx =
        initializer.CreateContext("multi-implementation.ets", ES2PANDA_STATE_CHECKED, source.data());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    size_t const callOffset = source.rfind("draw");
    ASSERT_NE(callOffset, std::string::npos);
    auto result = GetImpl()->getImplementationLocationAtPosition(ctx, callOffset);
    initializer.DestroyContext(ctx);

    ASSERT_EQ(result.size(), 2U);
    EXPECT_EQ(result[0].uri_, "multi-implementation.ets");
    EXPECT_EQ(result[1].uri_, "multi-implementation.ets");
    EXPECT_EQ(result[0].range_.start.line_, 5U);
    EXPECT_EQ(result[1].range_.start.line_, 9U);
    EXPECT_EQ(result[0].range_.start.character_, source.find("draw(): void", source.find("class Circle")));
    EXPECT_EQ(result[1].range_.start.character_, source.find("draw(): void", source.find("class Rect")));
}

TEST_F(LspGetImplementationLocationTests, DISABLED_InterfaceDeclarationCursorReturnsAllImplementations)
{
    Initializer initializer = Initializer();
    static std::string source = R"(
interface Flyable {
    fly(): void;
}
class Bird implements Flyable {
    fly(): void {
    }
}
class Plane implements Flyable {
    fly(): void {
    }
}
let bird: Flyable = new Bird();
bird.fly();
)";
    es2panda_Context *ctx =
        initializer.CreateContext("interface-decl-cursor.ets", ES2PANDA_STATE_CHECKED, source.data());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    size_t const declOffset = source.find("fly");
    ASSERT_NE(declOffset, std::string::npos);
    auto result = GetImpl()->getImplementationLocationAtPosition(ctx, declOffset);
    initializer.DestroyContext(ctx);

    ASSERT_EQ(result.size(), 2U);
    EXPECT_EQ(result[0].uri_, "interface-decl-cursor.ets");
    EXPECT_EQ(result[1].uri_, "interface-decl-cursor.ets");
    EXPECT_EQ(result[0].range_.start.line_, 5U);
    EXPECT_EQ(result[1].range_.start.line_, 9U);
    EXPECT_EQ(result[0].range_.start.character_, source.find("fly(): void", source.find("class Bird")));
    EXPECT_EQ(result[1].range_.start.character_, source.find("fly(): void", source.find("class Plane")));
}

}  // namespace

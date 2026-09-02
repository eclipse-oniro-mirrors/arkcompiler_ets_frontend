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

#include <gtest/gtest.h>

#include "generated/code_fix_register.h"
#include "lsp/include/api.h"
#include "lsp/include/cancellation_token.h"
#include "lsp/include/register_code_fix/add_missing_declare_property.h"
#include "lsp/include/register_code_fix/fix_class_doesnt_implement_inherited_abstract_member.h"
#include "lsp/include/register_code_fix/fix_class_incorrectly_implements_interface.h"
#include "lsp/include/register_code_fix/fix_extends_interface_becomes_implements.h"
#include "lsp/include/register_code_fix/fix_property_assignment.h"

namespace {

using ark::es2panda::lsp::codefixes::EXTENDS_INTERFACE_BECOMES_IMPLEMENTS;
using ark::es2panda::lsp::codefixes::FIX_CLASS_INCORRECTLY_IMPLEMENTS_INTERFACE_FOR_GETTER;
using ark::es2panda::lsp::codefixes::FIX_CLASS_INCORRECTLY_IMPLEMENTS_INTERFACE_FOR_SETTER;
using ark::es2panda::lsp::codefixes::FIX_CLASS_NOT_IMPLEMENTING_INHERITED_MEMBERS;
using ark::es2panda::lsp::codefixes::FIX_PROPERTY_ASSIGNMENT;

constexpr int DEFAULT_THROTTLE = 20;

class ClassInterfaceCodefixesTests : public LSPAPITests {
public:
    static ark::es2panda::lsp::CancellationToken CreateNonCancellationToken()
    {
        return ark::es2panda::lsp::CancellationToken(DEFAULT_THROTTLE, &GetNullHost());
    }

    static std::vector<int> &AbstractErrorCodes()
    {
        static auto codes = [] {
            auto supported = FIX_CLASS_NOT_IMPLEMENTING_INHERITED_MEMBERS.GetSupportedCodeNumbers();
            return std::vector<int>(supported.begin(), supported.end());
        }();
        return codes;
    }

    static std::vector<int> &GetterErrorCodes()
    {
        static auto codes = [] {
            auto supported = FIX_CLASS_INCORRECTLY_IMPLEMENTS_INTERFACE_FOR_GETTER.GetSupportedCodeNumbers();
            return std::vector<int>(supported.begin(), supported.end());
        }();
        return codes;
    }

    static std::vector<int> &SetterErrorCodes()
    {
        static auto codes = [] {
            auto supported = FIX_CLASS_INCORRECTLY_IMPLEMENTS_INTERFACE_FOR_SETTER.GetSupportedCodeNumbers();
            return std::vector<int>(supported.begin(), supported.end());
        }();
        return codes;
    }

    static std::vector<int> &ExtendsErrorCodes()
    {
        static auto codes = [] {
            auto supported = EXTENDS_INTERFACE_BECOMES_IMPLEMENTS.GetSupportedCodeNumbers();
            return std::vector<int>(supported.begin(), supported.end());
        }();
        return codes;
    }

    static std::vector<int> &PropertyAssignmentErrorCodes()
    {
        static auto codes = [] {
            auto supported = FIX_PROPERTY_ASSIGNMENT.GetSupportedCodeNumbers();
            return std::vector<int>(supported.begin(), supported.end());
        }();
        return codes;
    }

    static CodeFixOptions &MakeOptions()
    {
        static CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
        return options;
    }

private:
    class NullCancellationToken : public ark::es2panda::lsp::HostCancellationToken {
    public:
        bool IsCancellationRequested() override
        {
            return false;
        }
    };

    static NullCancellationToken &GetNullHost()
    {
        static NullCancellationToken instance;
        return instance;
    }
};

TEST_F(ClassInterfaceCodefixesTests, DISABLED_AddsAbstractGetterWithAccessorKeyword)
{
    const std::string code = R"(abstract class Shape {
  abstract area(): number;
  abstract describe();
  abstract get kind(): string;
}
class Circle extends Shape {
  radius: number = 1;
}
)";
    ark::es2panda::lsp::Initializer initializer;
    auto *ctx = initializer.CreateContext("abstract_getter_stub.ets", ES2PANDA_STATE_CHECKED, code.c_str());
    ASSERT_NE(ctx, nullptr);

    const auto pos = code.find("Circle extends Shape");
    ASSERT_NE(pos, std::string::npos);
    auto fixes = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(ctx, pos, pos + 1, AbstractErrorCodes(), MakeOptions());
    ASSERT_EQ(fixes.size(), 1);
    EXPECT_EQ(fixes[0].fixName_, FIX_CLASS_NOT_IMPLEMENTING_INHERITED_MEMBERS.GetFixId());
    EXPECT_EQ(fixes[0].fixId_, FIX_CLASS_NOT_IMPLEMENTING_INHERITED_MEMBERS.GetFixId());
    EXPECT_EQ(fixes[0].description_, "Add missing inherited abstract members");
    ASSERT_EQ(fixes[0].changes_.size(), 1);
    EXPECT_EQ(fixes[0].changes_[0].fileName, "abstract_getter_stub.ets");
    ASSERT_EQ(fixes[0].changes_[0].textChanges.size(), 1);
    const auto &textChange = fixes[0].changes_[0].textChanges[0];
    EXPECT_EQ(textChange.span.start, code.find('{', code.find("class Circle")) + 1);
    EXPECT_EQ(textChange.span.length, 0);
    EXPECT_EQ(textChange.newText, "  area(): number {}\n  describe() {}\n  get kind(): string {}\n");
    initializer.DestroyContext(ctx);
}

TEST_F(ClassInterfaceCodefixesTests, DISABLED_AddsAbstractSetterStub)
{
    const std::string code = R"(abstract class NamedBase {
  abstract set name(value: string);
}
class NamedImpl extends NamedBase {
}
)";
    ark::es2panda::lsp::Initializer initializer;
    auto *ctx = initializer.CreateContext("abstract_setter_stub.ets", ES2PANDA_STATE_CHECKED, code.c_str());
    ASSERT_NE(ctx, nullptr);

    const auto pos = code.find("NamedImpl extends");
    ASSERT_NE(pos, std::string::npos);
    auto fixes = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(ctx, pos, pos + 1, AbstractErrorCodes(), MakeOptions());
    ASSERT_EQ(fixes.size(), 1U);
    ASSERT_EQ(fixes[0].changes_.size(), 1U);
    ASSERT_EQ(fixes[0].changes_[0].textChanges.size(), 1U);
    const auto &change = fixes[0].changes_[0].textChanges[0];
    EXPECT_EQ(change.span.start, code.find('{', code.find("class NamedImpl")) + 1);
    EXPECT_EQ(change.newText, "  set name(value: string) {}\n");

    initializer.DestroyContext(ctx);
}

TEST_F(ClassInterfaceCodefixesTests, DISABLED_AddsAbstractPropertyOfDirectAbstractBase)
{
    const std::string code = R"(abstract class Shape {
  abstract area(): number;
  abstract size: number;
}
class Circle extends Shape {
}
)";
    ark::es2panda::lsp::Initializer initializer;
    auto *ctx = initializer.CreateContext("abstract_property_skip.ets", ES2PANDA_STATE_CHECKED, code.c_str());
    ASSERT_NE(ctx, nullptr);

    const auto pos = code.find("Circle extends Shape");
    ASSERT_NE(pos, std::string::npos);
    auto fixes = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(ctx, pos, pos + 1, AbstractErrorCodes(), MakeOptions());
    ASSERT_EQ(fixes.size(), 1);
    EXPECT_EQ(fixes[0].fixName_, FIX_CLASS_NOT_IMPLEMENTING_INHERITED_MEMBERS.GetFixId());
    ASSERT_EQ(fixes[0].changes_.size(), 1);
    ASSERT_EQ(fixes[0].changes_[0].textChanges.size(), 1);
    const auto &textChange = fixes[0].changes_[0].textChanges[0];
    EXPECT_EQ(textChange.span.start, code.find('{', code.find("class Circle")) + 1);
    EXPECT_EQ(textChange.span.length, 0);
    EXPECT_EQ(textChange.newText, "  area(): number {}\n  public size: number\n");
    initializer.DestroyContext(ctx);
}

// Class extending an abstract class and implementing an interface at the same time:
// each fix family covers its own declaration clause independently.
TEST_F(ClassInterfaceCodefixesTests, DISABLED_ExtendsAbstractAndImplementsInterface)
{
    const std::string code = R"(interface Base {
  get id(): string;
}
interface Child extends Base {
  set label(value: string);
}
abstract class Shape {
  abstract area(): number;
}
class Circle extends Shape implements Child {
}
)";
    ark::es2panda::lsp::Initializer initializer;
    auto *ctx = initializer.CreateContext("extends_and_implements.ets", ES2PANDA_STATE_CHECKED, code.c_str());
    ASSERT_NE(ctx, nullptr);

    const auto pos = code.find("Circle extends Shape");
    ASSERT_NE(pos, std::string::npos);
    auto options = MakeOptions();

    auto abstractFixes =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(ctx, pos, pos + 1, AbstractErrorCodes(), options);
    ASSERT_EQ(abstractFixes.size(), 1);
    EXPECT_EQ(abstractFixes[0].fixName_, FIX_CLASS_NOT_IMPLEMENTING_INHERITED_MEMBERS.GetFixId());
    ASSERT_EQ(abstractFixes[0].changes_[0].textChanges.size(), 1);
    const auto &abstractChange = abstractFixes[0].changes_[0].textChanges[0];
    EXPECT_EQ(abstractChange.span.start, code.find('{', code.find("class Circle")) + 1);
    EXPECT_EQ(abstractChange.newText, "  area(): number {}\n");

    auto setterFixes = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(ctx, pos, pos + 1, SetterErrorCodes(), options);
    ASSERT_EQ(setterFixes.size(), 1);
    EXPECT_EQ(setterFixes[0].fixName_, FIX_CLASS_INCORRECTLY_IMPLEMENTS_INTERFACE_FOR_SETTER.GetFixId());
    EXPECT_EQ(setterFixes[0].description_, "Add missing interface setter implementations");
    ASSERT_EQ(setterFixes[0].changes_[0].textChanges.size(), 1);
    const auto &setterChange = setterFixes[0].changes_[0].textChanges[0];
    // Empty class: insert immediately after the opening brace so the edit stays
    // inside the class body instead of falling back to an invalid file offset.
    EXPECT_EQ(setterChange.span.start, code.find('{', code.find("class Circle")) + 1);
    EXPECT_EQ(setterChange.span.length, 0);
    EXPECT_EQ(setterChange.newText, "\n\n  set label(value: string) {\n  }\n");
    initializer.DestroyContext(ctx);
}

// Interface extending interface: the checker reports only the directly declared members
// of the child interface; the inherited base accessor is not fixed by this codefix.
TEST_F(ClassInterfaceCodefixesTests, InterfaceInheritanceFixesDirectMembersOnly)
{
    const std::string code = R"(interface Base {
  get id(): string;
}
interface Child extends Base {
  set label(value: string);
}
class Widget implements Child {
}
)";
    ark::es2panda::lsp::Initializer initializer;
    auto *ctx = initializer.CreateContext("interface_extends_interface.ets", ES2PANDA_STATE_CHECKED, code.c_str());
    ASSERT_NE(ctx, nullptr);

    const auto pos = code.find("Widget implements");
    ASSERT_NE(pos, std::string::npos);
    auto options = MakeOptions();

    std::vector<int> accessorErrorCodes = GetterErrorCodes();
    accessorErrorCodes.insert(accessorErrorCodes.end(), SetterErrorCodes().begin(), SetterErrorCodes().end());
    auto fixes = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(ctx, pos, pos + 1, accessorErrorCodes, options);
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t expectedFixCount = 2;
    ASSERT_EQ(fixes.size(), expectedFixCount);
    EXPECT_EQ(fixes[0].fixName_, FIX_CLASS_INCORRECTLY_IMPLEMENTS_INTERFACE_FOR_SETTER.GetFixId());
    EXPECT_EQ(fixes[0].description_, "Add missing interface setter implementations");
    ASSERT_EQ(fixes[0].changes_[0].textChanges.size(), 1);
    EXPECT_EQ(fixes[0].changes_[0].textChanges[0].newText, "\n\n  set label(value: string) {\n  }\n");
    EXPECT_EQ(fixes[1].fixName_, fixes[0].fixName_);
    EXPECT_EQ(fixes[1].changes_[0].textChanges[0].newText, fixes[0].changes_[0].textChanges[0].newText);
    initializer.DestroyContext(ctx);
}

// Getter and setter of the same interface property: the checker reports the missing
// getter; the registered provider answers with its getter fix regardless of which of the
// two accessor error codes is passed to the query.
TEST_F(ClassInterfaceCodefixesTests, AddsMissingGetterAndSetterAsSeparateFixes)
{
    const std::string code = R"(interface Named {
  get name(): string;
  set name(value: string);
}
class Person implements Named {
}
)";
    ark::es2panda::lsp::Initializer initializer;
    auto *ctx = initializer.CreateContext("getter_and_setter.ets", ES2PANDA_STATE_CHECKED, code.c_str());
    ASSERT_NE(ctx, nullptr);

    const auto pos = code.find("Person implements");
    ASSERT_NE(pos, std::string::npos);
    auto options = MakeOptions();

    auto getterFixes = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(ctx, pos, pos + 1, GetterErrorCodes(), options);
    ASSERT_EQ(getterFixes.size(), 1);
    EXPECT_EQ(getterFixes[0].fixName_, FIX_CLASS_INCORRECTLY_IMPLEMENTS_INTERFACE_FOR_GETTER.GetFixId());
    EXPECT_EQ(getterFixes[0].fixId_, FIX_CLASS_INCORRECTLY_IMPLEMENTS_INTERFACE_FOR_GETTER.GetFixId());
    EXPECT_EQ(getterFixes[0].description_, "Add missing interface getter implementations");
    ASSERT_EQ(getterFixes[0].changes_[0].textChanges.size(), 1);
    EXPECT_EQ(getterFixes[0].changes_[0].textChanges[0].newText, "\n\n  get name(): string {\n    return null;\n  }\n");

    auto setterFixes = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(ctx, pos, pos + 1, SetterErrorCodes(), options);
    ASSERT_EQ(setterFixes.size(), 1);
    EXPECT_EQ(setterFixes[0].fixName_, FIX_CLASS_INCORRECTLY_IMPLEMENTS_INTERFACE_FOR_GETTER.GetFixId());
    EXPECT_EQ(setterFixes[0].changes_[0].textChanges[0].newText, getterFixes[0].changes_[0].textChanges[0].newText);
    initializer.DestroyContext(ctx);
}

// Part of the interface members already implemented: only the missing setter is added,
// the existing getter implementation is kept untouched.
TEST_F(ClassInterfaceCodefixesTests, AddsOnlyMissingMemberWhenPartiallyImplemented)
{
    const std::string code = R"(interface Two {
  get alpha(): string;
  set beta(value: string);
}
class Impl implements Two {
  get alpha(): string {
    return "a";
  }
}
)";
    ark::es2panda::lsp::Initializer initializer;
    auto *ctx = initializer.CreateContext("partial_implementation.ets", ES2PANDA_STATE_CHECKED, code.c_str());
    ASSERT_NE(ctx, nullptr);

    const auto pos = code.find("Impl implements");
    ASSERT_NE(pos, std::string::npos);
    auto fixes = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(ctx, pos, pos + 1, SetterErrorCodes(), MakeOptions());
    ASSERT_EQ(fixes.size(), 1);
    EXPECT_EQ(fixes[0].fixName_, FIX_CLASS_INCORRECTLY_IMPLEMENTS_INTERFACE_FOR_SETTER.GetFixId());
    ASSERT_EQ(fixes[0].changes_[0].textChanges.size(), 1);
    const auto &textChange = fixes[0].changes_[0].textChanges[0];
    EXPECT_EQ(textChange.span.length, 0);
    EXPECT_EQ(textChange.newText, "\n\n  set beta(value: string) {\n  }\n");
    EXPECT_EQ(textChange.newText.find("alpha"), std::string::npos);
    initializer.DestroyContext(ctx);
}

// Insertion position stays inside the class body: the stub is appended right after the
// last existing member, keeping the field and constructor order intact.
TEST_F(ClassInterfaceCodefixesTests, InsertsStubAfterLastMemberWithoutBreakingOrder)
{
    const std::string code = R"(interface Named {
  get name(): string;
}
class Person implements Named {
  age: number = 0;
  constructor() {}
}
)";
    ark::es2panda::lsp::Initializer initializer;
    auto *ctx = initializer.CreateContext("insert_after_last_member.ets", ES2PANDA_STATE_CHECKED, code.c_str());
    ASSERT_NE(ctx, nullptr);

    const auto pos = code.find("Person implements");
    ASSERT_NE(pos, std::string::npos);
    auto fixes = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(ctx, pos, pos + 1, GetterErrorCodes(), MakeOptions());
    ASSERT_EQ(fixes.size(), 1);
    ASSERT_EQ(fixes[0].changes_[0].textChanges.size(), 1);
    const auto &textChange = fixes[0].changes_[0].textChanges[0];
    const auto fieldPos = code.find("age: number");
    const auto ctorPos = code.find("constructor() {}");
    ASSERT_NE(fieldPos, std::string::npos);
    ASSERT_NE(ctorPos, std::string::npos);
    EXPECT_EQ(textChange.span.start, ctorPos + std::string("constructor() {}").size());
    EXPECT_EQ(textChange.span.length, 0);
    EXPECT_GT(textChange.span.start, fieldPos);
    EXPECT_EQ(textChange.newText, "\n\n  get name(): string {\n    return null;\n  }\n");
    initializer.DestroyContext(ctx);
}

// Overloaded abstract methods: only the first overload declaration is visible in the
// class body, so a single stub is generated for the overloaded name.
TEST_F(ClassInterfaceCodefixesTests, AddsSingleStubForOverloadedAbstractMethod)
{
    const std::string code = R"(abstract class Printer {
  abstract print(value: string): void;
  abstract print(value: number): void;
}
class ConsolePrinter extends Printer {
}
)";
    ark::es2panda::lsp::Initializer initializer;
    auto *ctx = initializer.CreateContext("overloaded_abstract.ets", ES2PANDA_STATE_CHECKED, code.c_str());
    ASSERT_NE(ctx, nullptr);

    const auto pos = code.find("ConsolePrinter extends");
    ASSERT_NE(pos, std::string::npos);
    auto fixes = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(ctx, pos, pos + 1, AbstractErrorCodes(), MakeOptions());
    ASSERT_EQ(fixes.size(), 1);
    EXPECT_EQ(fixes[0].fixName_, FIX_CLASS_NOT_IMPLEMENTING_INHERITED_MEMBERS.GetFixId());
    ASSERT_EQ(fixes[0].changes_[0].textChanges.size(), 1);
    const auto &textChange = fixes[0].changes_[0].textChanges[0];
    EXPECT_EQ(textChange.span.start, code.find('{', code.find("class ConsolePrinter")) + 1);
    EXPECT_EQ(textChange.newText, "  print(value: string): void {}\n");
    initializer.DestroyContext(ctx);
}

// Generic abstract methods keep their type parameter and parameter/return types when
// the inherited-member codefix generates the concrete stub.
TEST_F(ClassInterfaceCodefixesTests, AddsStubForGenericAbstractMethod)
{
    const std::string code = R"(abstract class GenericBase {
  abstract map<T>(value: T): T;
}
class GenericImpl extends GenericBase {
}
)";
    ark::es2panda::lsp::Initializer initializer;
    auto *ctx = initializer.CreateContext("generic_abstract_method.ets", ES2PANDA_STATE_CHECKED, code.c_str());
    ASSERT_NE(ctx, nullptr);

    const auto pos = code.find("GenericImpl extends");
    ASSERT_NE(pos, std::string::npos);
    auto fixes = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(ctx, pos, pos + 1, AbstractErrorCodes(), MakeOptions());
    ASSERT_EQ(fixes.size(), 1U);
    ASSERT_EQ(fixes[0].changes_.size(), 1U);
    ASSERT_EQ(fixes[0].changes_[0].textChanges.size(), 1U);
    EXPECT_EQ(fixes[0].changes_[0].textChanges[0].newText, "  map<T>(value: T): T {}\n");
    initializer.DestroyContext(ctx);
}

// Wrong '=' assignment for a readonly interface property inside an object literal.
TEST_F(ClassInterfaceCodefixesTests, FixesInvalidAssignmentOfReadonlyProperty)
{
    const std::string code = R"(interface Cfg {
  readonly flag: boolean;
  count: number;
}
const cfg: Cfg = {
  flag = true,
  count = 5
}
)";
    ark::es2panda::lsp::Initializer initializer;
    auto *ctx = initializer.CreateContext("readonly_property_assignment.ets", ES2PANDA_STATE_CHECKED, code.c_str());
    ASSERT_NE(ctx, nullptr);

    const auto flagPos = code.find("flag = true");
    ASSERT_NE(flagPos, std::string::npos);
    const auto cursor = flagPos + std::string("flag ").size();
    auto fixes = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(ctx, cursor, cursor + 1, PropertyAssignmentErrorCodes(),
                                                                MakeOptions());
    ASSERT_EQ(fixes.size(), 1);
    EXPECT_EQ(fixes[0].fixName_, FIX_PROPERTY_ASSIGNMENT.GetFixId());
    EXPECT_EQ(fixes[0].fixId_, FIX_PROPERTY_ASSIGNMENT.GetFixId());
    EXPECT_EQ(fixes[0].description_, "Change '=' to ':' in object property");
    ASSERT_EQ(fixes[0].changes_.size(), 1);
    EXPECT_EQ(fixes[0].changes_[0].fileName, "readonly_property_assignment.ets");
    ASSERT_EQ(fixes[0].changes_[0].textChanges.size(), 1);
    const auto &textChange = fixes[0].changes_[0].textChanges[0];
    EXPECT_EQ(textChange.span.start, flagPos);
    EXPECT_EQ(textChange.span.length, std::string("flag = true").size());
    EXPECT_EQ(textChange.newText, "flag: true");
    initializer.DestroyContext(ctx);
}

TEST_F(ClassInterfaceCodefixesTests, ReadonlyClassFieldReassignmentHasNoUnsafeQuickFix)
{
    const std::string code = R"(class Cfg {
  readonly flag: boolean = false;
}
let cfg = new Cfg();
cfg.flag = true;
)";
    ark::es2panda::lsp::Initializer initializer;
    auto *ctx =
        initializer.CreateContext("readonly_class_field_reassignment.ets", ES2PANDA_STATE_CHECKED, code.c_str());
    ASSERT_NE(ctx, nullptr);

    const auto cursor = code.find("flag = true");
    ASSERT_NE(cursor, std::string::npos);
    // ESE4002 (FIELD_ASSIGN_TO_READONLY) intentionally has no registered codefix:
    // silently removing `readonly` would change the declaration contract.
    constexpr int fieldAssignToReadonly = 4002;
    std::vector<int> readonlyFieldDiagnostic = {fieldAssignToReadonly};
    auto fixes =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(ctx, cursor, cursor + 1, readonlyFieldDiagnostic, MakeOptions());
    EXPECT_TRUE(fixes.empty());
    initializer.DestroyContext(ctx);
}

// Fix-all for the property assignment fix: a well-formed object literal has no
// diagnostic to repair, so the combined result stays empty.
TEST_F(ClassInterfaceCodefixesTests, CombinedFixIgnoresValidObjectLiteral)
{
    const std::string code = R"(interface Cfg {
  flag: boolean;
  count: number;
}
const cfg: Cfg = {
  flag: true,
  count: 5
}
)";
    ark::es2panda::lsp::Initializer initializer;
    auto *ctx = initializer.CreateContext("combined_property_assignment.ets", ES2PANDA_STATE_CHECKED, code.c_str());
    ASSERT_NE(ctx, nullptr);

    CombinedCodeActionsInfo result =
        ark::es2panda::lsp::GetCombinedCodeFixImpl(ctx, FIX_PROPERTY_ASSIGNMENT.GetFixId().data(), MakeOptions());
    EXPECT_TRUE(result.changes_.empty());
    initializer.DestroyContext(ctx);
}

// The 'extends' to 'implements' rewrite is applied by keyword shape only: for two
// classes it is not the correct fix, so verify the produced span and text explicitly.
TEST_F(ClassInterfaceCodefixesTests, RewritesExtendsKeywordBetweenClassesByShape)
{
    const std::string code = R"(class A {}
class B extends A {}
)";
    ark::es2panda::lsp::Initializer initializer;
    auto *ctx = initializer.CreateContext("extends_class_not_interface.ets", ES2PANDA_STATE_CHECKED, code.c_str());
    ASSERT_NE(ctx, nullptr);

    const auto pos = code.find("extends");
    ASSERT_NE(pos, std::string::npos);
    auto fixes = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(ctx, pos, pos + 1, ExtendsErrorCodes(), MakeOptions());
    ASSERT_EQ(fixes.size(), 1);
    EXPECT_EQ(fixes[0].fixName_, EXTENDS_INTERFACE_BECOMES_IMPLEMENTS.GetFixId());
    EXPECT_EQ(fixes[0].description_, "Change 'extends' to 'implements'");
    ASSERT_EQ(fixes[0].changes_[0].textChanges.size(), 1);
    const auto &textChange = fixes[0].changes_[0].textChanges[0];
    EXPECT_EQ(textChange.span.start, pos);
    EXPECT_EQ(textChange.span.length, std::string("extends").size());
    EXPECT_EQ(textChange.newText, "implements");
    initializer.DestroyContext(ctx);
}

// 'class Bar extends IFoo' where IFoo is an interface: the 'extends' keyword span is
// replaced with 'implements'.
TEST_F(ClassInterfaceCodefixesTests, FixesExtendsInterfaceToImplements)
{
    const std::string code = R"(interface IFoo {}
class Bar extends IFoo {}
)";
    ark::es2panda::lsp::Initializer initializer;
    auto *ctx = initializer.CreateContext("extends_interface.ets", ES2PANDA_STATE_CHECKED, code.c_str());
    ASSERT_NE(ctx, nullptr);

    const auto pos = code.find("extends");
    ASSERT_NE(pos, std::string::npos);
    auto fixes = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(ctx, pos, pos + 1, ExtendsErrorCodes(), MakeOptions());
    ASSERT_EQ(fixes.size(), 1);
    EXPECT_EQ(fixes[0].fixName_, EXTENDS_INTERFACE_BECOMES_IMPLEMENTS.GetFixId());
    EXPECT_EQ(fixes[0].fixId_, EXTENDS_INTERFACE_BECOMES_IMPLEMENTS.GetFixId());
    EXPECT_EQ(fixes[0].description_, "Change 'extends' to 'implements'");
    ASSERT_EQ(fixes[0].changes_.size(), 1);
    EXPECT_EQ(fixes[0].changes_[0].fileName, "extends_interface.ets");
    ASSERT_EQ(fixes[0].changes_[0].textChanges.size(), 1);
    const auto &textChange = fixes[0].changes_[0].textChanges[0];
    EXPECT_EQ(textChange.span.start, pos);
    EXPECT_EQ(textChange.span.length, std::string("extends").size());
    EXPECT_EQ(textChange.newText, "implements");
    initializer.DestroyContext(ctx);
}

// AddMissingDeclareProperty inserts the "declare " prefix at the offset of the reported
// diagnostic, following the pattern of the existing declare property suite. The scenario
// uses a syntax overload declaration so the checker drives the phase pipeline that logs
// semantic diagnostics into the diagnostic storage.
TEST_F(ClassInterfaceCodefixesTests, AddsDeclarePrefixAtUninitializedProperty)
{
    std::vector<std::string> fileNames = {"declare_property_prefix.ets"};
    std::vector<std::string> fileContents = {R"(declare class ExtBase {
  extMethod(): void;
}
function overloaded(): void;
class MyClass extends ExtBase {
  count: number;
}
function touch(): void {
  let obj: MyClass = new MyClass();
  obj.count = 1;
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    ark::es2panda::lsp::Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);
    auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
    const auto &diagnostics =
        ctx->diagnosticEngine->GetDiagnosticStorage(ark::es2panda::util::DiagnosticType::SEMANTIC);
    ASSERT_EQ(diagnostics.size(), 1);
    EXPECT_EQ(diagnostics[0]->Message(), "Only abstract or native methods can't have body.");
    const auto &source = ctx->parserProgram->SourceCode();
    const auto expectedOffset = source.find("overloaded(): void");
    ASSERT_NE(expectedOffset, std::string::npos);
    auto index = ark::es2panda::lexer::LineIndex(source);
    const auto offset = index.GetOffset(
        ark::es2panda::lexer::SourceLocation(diagnostics[0]->Line(), diagnostics[0]->Offset(), ctx->parserProgram));
    ASSERT_EQ(offset, expectedOffset);

    std::vector<ark::es2panda::ir::AstNode *> fixedNodes;
    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    TextChangesContext textChangesContext {{}, formatContext, {}};
    auto tracker = ark::es2panda::lsp::ChangeTracker::FromContext(textChangesContext);
    ark::es2panda::lsp::MakeChangeAddMissing(tracker, context, offset, fixedNodes);
    auto changes = tracker.GetChanges();
    ASSERT_EQ(changes.size(), 1);
    ASSERT_EQ(changes[0].textChanges.size(), 1);
    const auto &textChange = changes[0].textChanges[0];
    EXPECT_EQ(textChange.newText, "declare ");
    EXPECT_EQ(textChange.span.start, expectedOffset);
    EXPECT_EQ(textChange.span.length, 0);
    EXPECT_EQ(fixedNodes.size(), 1);
    initializer.DestroyContext(context);
}

// A property initialized inside the constructor produces no diagnostic, so the declare
// prefix fix has nothing to insert for it.
TEST_F(ClassInterfaceCodefixesTests, SkipsDeclarePrefixForConstructorAssignedProperty)
{
    const std::string code = R"(declare class ExtBase {
  extMethod(): void;
}
class MyClass extends ExtBase {
  title: string;
  constructor() {
    this.title = "t";
  }
}
)";
    ark::es2panda::lsp::Initializer initializer;
    auto *context = initializer.CreateContext("declare_property_skip.ets", ES2PANDA_STATE_CHECKED, code.c_str());
    ASSERT_NE(context, nullptr);
    auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
    const auto &diagnostics =
        ctx->diagnosticEngine->GetDiagnosticStorage(ark::es2panda::util::DiagnosticType::SEMANTIC);
    EXPECT_TRUE(diagnostics.empty());
    initializer.DestroyContext(context);
}

}  // namespace

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

#include <algorithm>
#include <array>
#include <cstdint>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>
#include "checker/ets/assignAnalyzer.h"
#include "ir/base/classDefinition.h"
#include "parser/program/program.h"
#include "varbinder/ETSBinder.h"
#include "test/utils/ast_verifier_test.h"
#include "util/diagnosticEngine.h"
#include "util/options.h"

namespace ark::es2panda::checker {

class StrictInitializationTest : public ::test::utils::AstVerifierTest {
protected:
    static std::string MakeConstraintChain(size_t length, const char *constraint, const char *body,
                                           bool nonNullish = false)
    {
        std::ostringstream source;
        source << "function f<T0 extends " << constraint;
        for (size_t i = 1; i < length; ++i) {
            source << ", T" << i << " extends T" << (i - 1);
        }
        source << ">(v: " << (nonNullish ? "NonNullable<T" : "T") << (length - 1) << (nonNullish ? ">" : "")
               << "): void { " << body << " }";
        return source.str();
    }

    void ExpectStrictDiagnostic(const std::string &source, uint32_t id)
    {
        // Apply the opt-in policy to an AST accepted by the legacy checker.
        CONTEXT(ES2PANDA_STATE_CHECKED, source.c_str())
        {
            auto &engine = *reinterpret_cast<public_lib::Context *>(GetContext())->diagnosticEngine;
            AssignAnalyzer(GetChecker(), InitializationPolicy {true}).Analyze(GetAst());
            const auto &errors = engine.GetDiagnosticStorage(util::DiagnosticType::SEMANTIC);
            EXPECT_EQ(errors.size(), 1U);
            if (!errors.empty()) {
                EXPECT_EQ(errors.front()->GetId(), id);
            }
            // The fixture config shares diagnostics between source contexts.
            engine.ClearDiagnostics();
        }
    }

    void ExpectModeDiagnostics(std::initializer_list<const char *> options, const char *source,
                               std::initializer_list<uint32_t> expected)
    {
        std::vector<const char *> args {::test::utils::PandaExecutablePathGetter::Get()[0], "--extension=ets"};
        args.insert(args.end(), options.begin(), options.end());
        auto *config = GetImpl()->CreateConfig(static_cast<int>(args.size()), args.data());
        ASSERT_NE(config, nullptr);
        auto *context = GetImpl()->CreateContextFromString(config, source, "strict-mode.ats");
        ASSERT_NE(context, nullptr);
        GetImpl()->ProceedToState(context, ES2PANDA_STATE_CHECKED);
        EXPECT_EQ(GetImpl()->ContextState(context),
                  expected.size() == 0 ? ES2PANDA_STATE_CHECKED : ES2PANDA_STATE_ERROR);
        auto &engine = *reinterpret_cast<public_lib::Context *>(context)->diagnosticEngine;
        const auto &errors = engine.GetDiagnosticStorage(util::DiagnosticType::SEMANTIC);
        EXPECT_EQ(errors.size(), expected.size());
        size_t index = 0;
        for (const auto id : expected) {
            if (index < errors.size()) {
                EXPECT_EQ(errors[index]->GetId(), id);
            }
            ++index;
        }
        GetImpl()->DestroyContext(context);
        GetImpl()->DestroyConfig(config);
    }

    void CheckStdlibMode(const char *option, bool strict)
    {
        const auto executable = ::test::utils::PandaExecutablePathGetter::Get()[0];
        const std::array args {executable, "--extension=ets", "--gen-stdlib=true", option};
        auto *config = GetImpl()->CreateConfig(option == nullptr ? 3 : 4, args.data());
        ASSERT_NE(config, nullptr);
        EXPECT_EQ(reinterpret_cast<public_lib::ConfigImpl *>(config)->options->IsStrictMode(), strict);
        // A stdlib compilation needs the complete std source graph. Its checker
        // integration is exercised by the etsstdlib build, not a string fixture.
        GetImpl()->DestroyConfig(config);
    }

    void CheckStdlibInitializerMetadata(const char *source, bool strict, size_t expectedErrors)
    {
        CONTEXT(ES2PANDA_STATE_CHECKED, source)
        {
            auto *context = reinterpret_cast<public_lib::Context *>(GetContext());
            const auto executable = ::test::utils::PandaExecutablePathGetter::Get()[0];
            const std::array args {executable, "--extension=ets", "--gen-stdlib=true",
                                   strict ? "--strict=true" : "--strict=false"};
            auto *config = GetImpl()->CreateConfig(args.size(), args.data());
            ASSERT_NE(config, nullptr);
            auto *previousConfig = context->config;
            auto *globalClass = GetChecker()->VarBinder()->Program()->GlobalClass();
            auto &body = globalClass->BodyForUpdate();
            auto previousBody = body;
            // GEN_STD_LIB does not emit the lowered declaration assignments.
            // Retain the declaration metadata while omitting that static block.
            body.erase(std::remove_if(body.begin(), body.end(),
                                      [](const ir::AstNode *node) { return node->IsClassStaticBlock(); }),
                       body.end());
            context->config = reinterpret_cast<public_lib::ConfigImpl *>(config);
            auto &engine = *context->diagnosticEngine;
            engine.ClearDiagnostics();
            AssignAnalyzer(GetChecker(), InitializationPolicy {strict}).Analyze(GetAst());
            const auto &errors = engine.GetDiagnosticStorage(util::DiagnosticType::SEMANTIC);
            EXPECT_EQ(errors.size(), expectedErrors);
            for (const auto &error : errors) {
                EXPECT_EQ(error->GetId(), diagnostic::USE_BEFORE_INIT.Id());
            }
            engine.ClearDiagnostics();
            context->config = previousConfig;
            body = std::move(previousBody);
            GetImpl()->DestroyConfig(config);
        }
    }

    void CheckConfig(const char *option, bool strict)
    {
        const auto executable = ::test::utils::PandaExecutablePathGetter::Get()[0];
        const std::array args {executable, option};
        auto *config = GetImpl()->CreateConfig(option == nullptr ? 1 : 2, args.data());
        ASSERT_NE(config, nullptr);
        EXPECT_EQ(reinterpret_cast<public_lib::ConfigImpl *>(config)->options->IsStrictMode(), strict);
        auto *context = GetImpl()->CreateContextFromString(config, "function f(): int { let x: int; return x; }",
                                                           "strict-init.ats");
        ASSERT_NE(context, nullptr);
        GetImpl()->ProceedToState(context, ES2PANDA_STATE_CHECKED);
        EXPECT_EQ(GetImpl()->ContextState(context), strict ? ES2PANDA_STATE_ERROR : ES2PANDA_STATE_CHECKED);
        auto &engine = *reinterpret_cast<public_lib::Context *>(context)->diagnosticEngine;
        const auto &errors = engine.GetDiagnosticStorage(util::DiagnosticType::SEMANTIC);
        EXPECT_EQ(errors.size(), strict ? 1U : 0U);
        if (!errors.empty()) {
            EXPECT_EQ(errors.front()->GetId(), diagnostic::USE_BEFORE_INIT.Id());
        }
        GetImpl()->DestroyContext(context);
        GetImpl()->DestroyConfig(config);
    }
};

TEST_F(StrictInitializationTest, ConfigControlsChecker)
{
    CheckConfig(nullptr, false);
    CheckConfig("--strict=true", true);
    CheckConfig("--strict=false", false);
    CheckConfig(nullptr, false);
}

TEST_F(StrictInitializationTest, RejectsUnassignedValueTypes)
{
    const std::array<std::string_view, 9U> types {"number", "byte",   "short", "int",    "long",
                                                  "float",  "double", "char",  "boolean"};
    for (const auto type : types) {
        SCOPED_TRACE(type);
        const std::string name {type};
        std::ostringstream local;
        local << "function f(): " << name << " { let x: " << name << "; return x; }";
        ExpectStrictDiagnostic(local.str(), diagnostic::USE_BEFORE_INIT.Id());
        ExpectStrictDiagnostic("class A { x: " + name + "; }", diagnostic::PROPERTY_MAYBE_MISSING_INIT.Id());
        ExpectStrictDiagnostic("class A { static x: " + name + "; }", diagnostic::PROPERTY_MAYBE_MISSING_INIT.Id());
    }
}

TEST_F(StrictInitializationTest, ChecksNullComparisons)
{
    for (const auto *op : {"==", "!="}) {
        for (const auto *value : {"null", "undefined"}) {
            ExpectStrictDiagnostic(std::string {"function f(): void { let x: Object; console.log(x "} + op + " " +
                                       value + "); }",
                                   diagnostic::USE_BEFORE_INIT.Id());
        }
    }
}

TEST_F(StrictInitializationTest, ChecksConstantConditionOperands)
{
    ExpectStrictDiagnostic("function f(): void { let x: int; if (x == null) { console.log(1); } }",
                           diagnostic::USE_BEFORE_INIT.Id());
}

TEST_F(StrictInitializationTest, MergesShortCircuitExpressionStatements)
{
    for (const auto *op : {"&&", "||"}) {
        for (const auto *type : {"Object", "int"}) {
            const std::string name {type};
            const std::string value = name == "Object" ? "new Object()" : "1";
            std::ostringstream source;
            source << "function value(): " << name << " { return " << value << "; } function f(c: boolean): " << name
                   << " { let x: " << name << "; c " << op << " ((x = value()) === value()); return x; }";
            ExpectStrictDiagnostic(source.str(), diagnostic::USE_BEFORE_INIT.Id());
        }
    }
}

TEST_F(StrictInitializationTest, PreservesUndefinedDefaultInStrictPolicy)
{
    // Protect the boundary of the new default-value policy: undefined unions
    // must not be rejected together with non-null value types.
    CONTEXT(ES2PANDA_STATE_CHECKED, "function f(): int | undefined { let x: int | undefined; return x; }")
    {
        auto &engine = *reinterpret_cast<public_lib::Context *>(GetContext())->diagnosticEngine;
        const auto before = engine.Save();
        AssignAnalyzer(GetChecker(), InitializationPolicy {true}).Analyze(GetAst());
        EXPECT_FALSE(engine.HasErrorsSince(before));
    }
}

TEST_F(StrictInitializationTest, StrictControlsBooleanChecks)
{
    const auto source = "function f(n: int): void { if (n) {} }";
    ExpectModeDiagnostics({}, source, {});
    ExpectModeDiagnostics({"--strict=false"}, source, {});
    ExpectModeDiagnostics({"--strict=true"}, source, {diagnostic::STRICT_BOOLEAN_REQUIRED.Id()});
    ExpectModeDiagnostics({}, source, {});
}

TEST_F(StrictInitializationTest, StdlibDefaultsToStrictWithExplicitOverride)
{
    CheckStdlibMode(nullptr, true);
    CheckStdlibMode("--strict=false", false);
    CheckStdlibMode("--strict=true", true);
}

TEST_F(StrictInitializationTest, StrictBooleanOperandsAndErrorRecovery)
{
    const auto id = diagnostic::STRICT_BOOLEAN_REQUIRED.Id();
    ExpectModeDiagnostics({"--strict=true"}, "function f(n: int, s: string): void { n && s; }", {id, id});
    ExpectModeDiagnostics({"--strict=true"}, "function f(n: int): void { if (!!n) {} }", {id});
    ExpectModeDiagnostics({"--strict=true"}, "function f(b: boolean): void { b && console.log(b); }", {id});
}

TEST_F(StrictInitializationTest, StrictChecksDeadOperandsAndRestoresDiagnostics)
{
    const auto id = diagnostic::STRICT_BOOLEAN_REQUIRED.Id();
    ExpectModeDiagnostics({"--strict=true"}, "function f(n: int): void { false && n; true || n; }", {id, id});
    ExpectModeDiagnostics({"--strict=true"}, "function f(n: int): void { const x = true ? true : !n; }", {id});
    ExpectModeDiagnostics({"--strict=false"}, "function f(n: int): void { const x = true ? true : !n; }", {});
}

TEST_F(StrictInitializationTest, StrictPreservesBooleanTypeRelationsAndNarrowing)
{
    ExpectModeDiagnostics({"--strict=true"}, R"(
        type Flag = boolean;
        function f<T extends boolean>(v: T, a: Flag, b: boolean | undefined): boolean {
            if (b === undefined || b) {
                if (v && a) {}
            }
            return v || a;
        }
    )",
                          {});
    ExpectModeDiagnostics({"--strict=true"}, "function f(v: Any): void { if (v) {} }",
                          {diagnostic::STRICT_BOOLEAN_REQUIRED.Id()});
    ExpectModeDiagnostics({"--strict=true"}, "function f(v: boolean | undefined): void { if (v) {} }",
                          {diagnostic::STRICT_BOOLEAN_REQUIRED.Id()});
}

TEST_F(StrictInitializationTest, StrictBooleanAcceptsLongConstraintChains)
{
    for (size_t length : {9U, 16U, 64U}) {
        const auto source = MakeConstraintChain(length, "boolean", "if (v) {}");
        SCOPED_TRACE(source);
        ExpectModeDiagnostics({"--strict=true"}, source.c_str(), {});
    }
}

TEST_F(StrictInitializationTest, StrictBooleanRejectsLongNonBooleanConstraintChains)
{
    for (const char *constraint : {"int", "boolean | undefined"}) {
        const auto source = MakeConstraintChain(16U, constraint, "if (v) {}");
        SCOPED_TRACE(source);
        ExpectModeDiagnostics({"--strict=true"}, source.c_str(), {diagnostic::STRICT_BOOLEAN_REQUIRED.Id()});
    }
}

TEST_F(StrictInitializationTest, StrictBooleanPreservesNonNullishLongConstraintChains)
{
    const auto source = MakeConstraintChain(16U, "boolean | undefined", "if (v) {}", true);
    ExpectModeDiagnostics({"--strict=true"}, source.c_str(), {});
}

TEST_F(StrictInitializationTest, StrictBooleanLongConstraintsSupportLogicalOperands)
{
    const auto source = MakeConstraintChain(16U, "boolean", "const a = v && v; const b = v || v; if (!v ? a : b) {}");
    ExpectModeDiagnostics({"--strict=true"}, source.c_str(), {});
}

TEST_F(StrictInitializationTest, StrictInternalDynamicAndRecoveryBoundaries)
{
    const std::array args {::test::utils::PandaExecutablePathGetter::Get()[0], "--strict=true"};
    auto *config = GetImpl()->CreateConfig(args.size(), args.data());
    ASSERT_NE(config, nullptr);
    auto *opaqueContext = GetImpl()->CreateContextFromString(config, "function f(): void {}", "strict-mode.ats");
    ASSERT_NE(opaqueContext, nullptr);
    GetImpl()->ProceedToState(opaqueContext, ES2PANDA_STATE_CHECKED);
    ASSERT_EQ(GetImpl()->ContextState(opaqueContext), ES2PANDA_STATE_CHECKED);
    auto *context = reinterpret_cast<public_lib::Context *>(opaqueContext);
    auto *checker = static_cast<ETSChecker *>(context->GetChecker());
    const lexer::SourcePosition pos {};
    auto *relation = checker->Relation();
    const auto flags = relation->GetTypeRelationFlags();
    auto *node = relation->GetNode();
    relation->Result(true);
    EXPECT_TRUE(checker->CheckStrictBooleanType(checker->GlobalETSBooleanType(), pos));
    EXPECT_TRUE(relation->IsTrue());
    EXPECT_EQ(relation->GetTypeRelationFlags(), flags);
    EXPECT_EQ(relation->GetNode(), node);
    EXPECT_FALSE(checker->CheckStrictBooleanType(checker->GlobalETSRelaxedAnyType(), pos));
    EXPECT_TRUE(checker->CheckStrictBooleanType(nullptr, pos));
    EXPECT_TRUE(checker->CheckStrictBooleanType(checker->GlobalTypeError(), pos));
    const auto &errors = context->diagnosticEngine->GetDiagnosticStorage(util::DiagnosticType::SEMANTIC);
    EXPECT_EQ(errors.size(), 1U);
    if (!errors.empty()) {
        EXPECT_EQ(errors.front()->GetId(), diagnostic::STRICT_BOOLEAN_REQUIRED.Id());
    }
    GetImpl()->DestroyContext(opaqueContext);
    GetImpl()->DestroyConfig(config);
}

TEST_F(StrictInitializationTest, StdlibPreservesExplicitInitializerMetadata)
{
    CheckStdlibInitializerMetadata("const n: int = 24; function f(): int { return n; }", true, 0U);
    CheckStdlibInitializerMetadata("const n: int = 24 * 60; function f(): int { return n; }", true, 0U);
    CheckStdlibInitializerMetadata("const n: Object = new Object(); function f(): Object { return n; }", true, 0U);
    CheckStdlibInitializerMetadata("let n: int = 24; function f(): int { return n; }", true, 1U);
    CheckStdlibInitializerMetadata("let n: int; function f(): int { return n; }", true, 1U);
    CheckStdlibInitializerMetadata("let n: int; function f(): int { return n; }", false, 0U);
}

TEST_F(StrictInitializationTest, StrictConstantFoldingCannotEraseInvalidOperands)
{
    const std::array expressions {"1 ? 1 : 2", "!1", "!!1", "true && 1", "false || 1", "true ? true : !1"};
    for (const auto *expression : expressions) {
        const auto source = std::string("function f(): Any { return ") + expression + "; }";
        ExpectModeDiagnostics({"--strict=true"}, source.c_str(), {diagnostic::STRICT_BOOLEAN_REQUIRED.Id()});
        ExpectModeDiagnostics({"--strict=false"}, source.c_str(), {});
    }
    ExpectModeDiagnostics({"--strict=true"}, "function f(): boolean { return !(true && false); }", {});
}

}  // namespace ark::es2panda::checker

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

#include <algorithm>
#include <array>
#include <cstddef>
#include <memory>
#include <string>
#include <string_view>

#include "es2panda.h"
#include "libarkbase/utils/span.h"
#include "test/utils/asm_test.h"
#include "util/diagnosticEngine.h"
#include "util/options.h"

#ifndef ES2PANDA_BIN_PATH
#error "ES2PANDA_BIN_PATH is not defined (pass it from CMakeLists.txt)"
#endif

namespace ark::es2panda::test {

namespace {

// Compiles 'src' in-process. Returns nullptr when compilation fails: Compiler::Compile
// returns an empty program map on parse errors, which must not be dereferenced.
std::unique_ptr<pandasm::Program> CompileSource(Span<const char *const> args, std::string_view src)
{
    if (args.empty() || args[0] == nullptr) {
        return nullptr;
    }

    auto diagnosticEngine = util::DiagnosticEngine();
    auto options = std::make_unique<util::Options>(args[0], diagnosticEngine);
    if (!options->Parse(args)) {
        return nullptr;
    }

    Compiler compiler(options->GetExtension(), options->GetThread());
    SourceFile input("line_number_overflow.ets", src, options->IsModule());
    auto output = compiler.Compile(input, *options, diagnosticEngine);
    if (output.empty()) {
        return nullptr;
    }
    return std::move(output.begin()->second);
}

// Builds a source whose statement starts on 1-based line 65537: one line past the
// uint16_t line range stored in lexer::CompressedSourceRange on every AST node.
// 'statement' must not start with a newline.
std::string MakeSourceBeyondUint16Lines(std::string_view statement)
{
    constexpr size_t headerLines = 3U;  // 'function foo', '}', 'function main'
    constexpr size_t targetLine = 65537U;
    constexpr size_t blankLines = targetLine - headerLines - 1U;

    std::string src = "function foo(): void {\n}\nfunction main(): void {\n";
    src.reserve(src.size() + blankLines + statement.size() + 3U);
    src.append(blankLines, '\n');
    src.append(statement);
    src.append("\n}\n");
    return src;
}

// Returns true if any identifier operand of the instruction refers to 'foo' (a call target).
bool InstructionRefersToFoo(const pandasm::Ins &ins)
{
    for (size_t i = 0; i < ins.IDSize(); ++i) {
        if (ins.GetID(i).find("ETSGLOBAL.foo:void;") != std::string::npos) {
            return true;
        }
    }
    return false;
}

// Checks whether the compiled 'main' contains a call to 'foo'. Scoped to main on
// purpose: in the ASI case the reference to 'foo' is lowered into a lambda wrapper
// whose invoke method does call 'foo'.
bool MainContainsCallToFoo(const pandasm::Program &program)
{
    for (const auto &[name, function] : program.functionStaticTable) {
        if (name.find("ETSGLOBAL.main") != std::string::npos &&
            std::any_of(function.ins.begin(), function.ins.end(), InstructionRefersToFoo)) {
            return true;
        }
    }
    return false;
}

}  // namespace

// AsmTest provides the ScopedAllocatorsManager lifecycle required by in-process compilation.
class LineNumberOverflowTest : public ::test::utils::AsmTest {};

TEST_F(LineNumberOverflowTest, SameLineCallPastUint16LineLimitCompilesAsCall)
{
    std::array args = {ES2PANDA_BIN_PATH, "--ets-unnamed", "--opt-level=0"};

    // 'foo()' on line 65537 must parse as a single call expression: the uint16_t line
    // saturation of AST node ranges must not make the parser treat '(' as starting a
    // new statement (previously produced bogus syntax errors and no call was emitted).
    auto program = CompileSource({args.data(), args.size()}, MakeSourceBeyondUint16Lines("foo()"));
    ASSERT_NE(program, nullptr) << "compilation of a same-line call past line 65536 failed";
    EXPECT_TRUE(MainContainsCallToFoo(*program)) << "'foo()' past line 65536 must compile to a call";
}

TEST_F(LineNumberOverflowTest, NewlineBeforeArgumentsPastUint16LineLimitKeepsTwoStatements)
{
    std::array args = {ES2PANDA_BIN_PATH, "--ets-unnamed", "--opt-level=0"};

    // ASI semantics must be preserved beyond the uint16_t line range: a '(' on a new
    // line ends the statement instead of becoming call arguments, so 'main' must not
    // contain a direct call to 'foo'.
    auto program = CompileSource({args.data(), args.size()}, MakeSourceBeyondUint16Lines("foo\n(0)"));
    ASSERT_NE(program, nullptr) << "compilation of a newline-separated call past line 65536 failed";
    EXPECT_FALSE(MainContainsCallToFoo(*program)) << "'foo' newline '(0)' must stay two statements";
}

}  // namespace ark::es2panda::test

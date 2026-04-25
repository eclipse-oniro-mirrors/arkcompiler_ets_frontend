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

#include <array>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

#include "assembly-function.h"
#include "assembly-program.h"
#include "test/utils/asm_test.h"

#ifndef ES2PANDA_BIN_PATH
#error "ES2PANDA_BIN_PATH is not defined (pass it from CMakeLists.txt)"
#endif

namespace ark::es2panda::compiler::test {

class ScopeLineInfoTest : public ::test::utils::AsmTest {
public:
    struct LambdaInvokeGuardCheckResult {
        bool sawLambdaInvoke {false};
        bool sawNextUserLine {false};
    };

    const pandasm::Function *FindFunctionByName(const pandasm::Program &program, const std::string &name)
    {
        auto itS = program.functionStaticTable.find(name);
        if (itS != program.functionStaticTable.end()) {
            return &itS->second;
        }
        auto itI = program.functionInstanceTable.find(name);
        if (itI != program.functionInstanceTable.end()) {
            return &itI->second;
        }
        return nullptr;
    }

    static std::vector<std::string> SplitLines(std::string_view text)
    {
        std::vector<std::string> lines;
        std::string cur;
        cur.reserve(text.size());
        for (char c : text) {
            if (c == '\n') {
                lines.emplace_back(cur);
                cur.clear();
            } else {
                cur.push_back(c);
            }
        }
        lines.emplace_back(cur);
        return lines;
    }

    static uint32_t FindLineNo(const std::vector<std::string> &lines, const std::string &needle)
    {
        for (size_t i = 0; i < lines.size(); i++) {
            if (lines[i].find(needle) != std::string::npos) {
                return static_cast<uint32_t>(i + 1U);  // 1-based
            }
        }
        return 0U;
    }

    template <class Fn>
    static void ForEachFunction(const pandasm::Program &program, Fn &&cb)
    {
        for (const auto &[name, fn] : program.functionStaticTable) {
            cb(name, fn);
        }
        for (const auto &[name, fn] : program.functionInstanceTable) {
            cb(name, fn);
        }
    }

    static bool IsIllegalLine(uint32_t ln) noexcept
    {
        return ln > static_cast<uint32_t>(std::numeric_limits<int32_t>::max());
    }

    static bool ExpectNextUserLine(const pandasm::Function &func, size_t startIndex, uint32_t nextUserLine)
    {
        for (size_t j = startIndex; j < func.ins.size(); j++) {
            const uint32_t lineNo = func.ins[j].insDebug.LineNumber();
            if (IsIllegalLine(lineNo)) {
                continue;
            }

            EXPECT_EQ(lineNo, nextUserLine) << "Step-out should continue on the next user-authored line.";
            return true;
        }

        return false;
    }

    static LambdaInvokeGuardCheckResult CheckLambdaInvokeGuardLineInfo(const pandasm::Function &func, uint32_t callLine,
                                                                       uint32_t nextUserLine)
    {
        LambdaInvokeGuardCheckResult result;

        for (size_t i = 0; i + 2U < func.ins.size(); i++) {
            const auto &ins = func.ins[i];
            if (ins.opcode != pandasm::Opcode::CALL_VIRT_SHORT || ins.insDebug.LineNumber() != callLine) {
                continue;
            }

            const auto &guardStore = func.ins[i + 1U];
            const auto &guardCheckCast = func.ins[i + 2U];
            if (guardStore.opcode != pandasm::Opcode::STA_OBJ || guardCheckCast.opcode != pandasm::Opcode::CHECKCAST) {
                continue;
            }

            result.sawLambdaInvoke = true;
            EXPECT_TRUE(IsIllegalLine(guardStore.insDebug.LineNumber()))
                << "The compiler-generated store after lambda invoke must not map to the source call line.";
            EXPECT_TRUE(IsIllegalLine(guardCheckCast.insDebug.LineNumber()))
                << "The compiler-generated checkcast after lambda invoke must not map to the source call line.";
            result.sawNextUserLine = ExpectNextUserLine(func, i + 3U, nextUserLine);
            break;
        }

        return result;
    }
};

TEST_F(ScopeLineInfoTest, LoweringGeneratedCode_LineNumberValidityAndInvalidMarker)
{
    std::string_view text = R"(// KNOWN ISSUE: the 1st line code is setted to invailid line number
        let a = [1, 2, 3];
        let b = [-1, ...a, -2];
        (() => { console.log(a); })();
    )";

    std::array args = {
        ES2PANDA_BIN_PATH,
        "--debug-info=true",
        "--opt-level=0",
        "--ets-unnamed",
    };

    auto program = GetCurrentProgramWithArgs({args.data(), args.size()}, text);
    ASSERT_NE(program, nullptr);

    const auto lines = SplitLines(text);
    const auto maxLine = static_cast<uint32_t>(lines.size());

    // ---- A) Global check: scan ALL functions' instructions ----
    bool sawIllegal = false;

    ForEachFunction(*program, [&](const std::string &fnName, const pandasm::Function &func) {
        for (const auto &ins : func.ins) {
            const uint32_t ln = ins.insDebug.LineNumber();
            if (IsIllegalLine(ln)) {
                sawIllegal = true;
                continue;
            }

            // Any non-illegal line must point to a valid source line.
            EXPECT_GE(ln, 1U) << "fn=" << fnName;
            EXPECT_LE(ln, maxLine) << "fn=" << fnName;
        }
    });

    EXPECT_TRUE(sawIllegal) << "At least one lowering-generated instructionto have an illegal line number.";

    // ---- B) Local sanity: ensure user-authored anchor lines in cctor map to legal line numbers ----
    const auto *cctor = FindFunctionByName(*program, "ETSGLOBAL.<cctor>:void;");
    ASSERT_NE(cctor, nullptr);

    const uint32_t sprLine = FindLineNo(lines, "let b = [-1, ...a, -2]");
    const uint32_t lamLine = FindLineNo(lines, "(() => {");
    ASSERT_GT(sprLine, 0U);
    ASSERT_GT(lamLine, 0U);

    bool sawSpr = false;
    bool sawLam = false;

    for (const auto &ins : cctor->ins) {
        const uint32_t ln = ins.insDebug.LineNumber();
        // Here we only care that at least some instructions map back to those user lines
        // with legal line numbers.
        if (ln == sprLine) {
            sawSpr = true;
        }
        if (ln == lamLine) {
            sawLam = true;
        }
    }

    EXPECT_TRUE(sawSpr) << "No instruction in ETSGLOBAL.foo:void; mapped to the user spread line.";
    EXPECT_TRUE(sawLam) << "No instruction in ETSGLOBAL.foo:void; mapped to the user lambda line.";
}

TEST_F(ScopeLineInfoTest, LambdaInvokeGuardUsesIllegalLineNumberInFunction)
{
    std::string_view text = R"(function func():void{
    let test:()=>void=()=>{
        console.log("111");
        return;
    }
    test();
    console.log("2222222");
}
)";

    std::array args = {
        ES2PANDA_BIN_PATH,
        "--debug-info=true",
        "--opt-level=0",
        "--ets-unnamed",
    };

    auto program = GetCurrentProgramWithArgs({args.data(), args.size()}, text);
    ASSERT_NE(program, nullptr);

    const auto lines = SplitLines(text);
    const auto *func = FindFunctionByName(*program, "ETSGLOBAL.func:void;");
    ASSERT_NE(func, nullptr);

    const uint32_t callLine = FindLineNo(lines, "test();");
    const uint32_t nextUserLine = FindLineNo(lines, R"(console.log("2222222");)");
    ASSERT_GT(callLine, 0U);
    ASSERT_GT(nextUserLine, 0U);

    const auto checkResult = CheckLambdaInvokeGuardLineInfo(*func, callLine, nextUserLine);
    EXPECT_TRUE(checkResult.sawLambdaInvoke)
        << "Failed to find the lambda invoke followed by its compiler-generated store and checkcast.";
    EXPECT_TRUE(checkResult.sawNextUserLine) << "Failed to find the next user-authored line after the lambda invoke.";
}

}  // namespace ark::es2panda::compiler::test

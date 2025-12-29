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
#include <string>
#include <vector>
#include <algorithm>

#include "assembly-function.h"
#include "assembly-program.h"
#include "test/utils/asm_test.h"

#ifndef ES2PANDA_BIN_PATH
#error "ES2PANDA_BIN_PATH is not defined (pass it from CMakeLists.txt)"
#endif

namespace ark::es2panda::compiler::test {

class ScopeLvtDebugTest : public ::test::utils::AsmTest {
public:
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

    const auto &GetLvtDebugInfo(const pandasm::Function &func)
    {
        return func.localVariableDebug;
    }

    bool HasVar(const pandasm::Function &fn, const std::string &varName)
    {
        const auto &vars = GetLvtDebugInfo(fn);
        return std::any_of(vars.begin(), vars.end(), [&varName](const auto &v) { return v.name == varName; });
    }

    void AssertVarRangeValid(const pandasm::Function &fn, const std::string &varName)
    {
        const auto &vars = GetLvtDebugInfo(fn);
        const auto insCnt = fn.ins.size();
        bool found = false;

        for (const auto &v : vars) {
            if (v.name != varName) {
                continue;
            }
            found = true;

            EXPECT_LT(v.start, insCnt) << "var=" << varName;
            EXPECT_GT(v.length, 0U) << "var=" << varName;
            EXPECT_LE(v.start + v.length, insCnt) << "var=" << varName;
            break;
        }

        ASSERT_TRUE(found) << "Missing var in LVT: " << varName;
    }
};

TEST_F(ScopeLvtDebugTest, ForLoop_NoPredecessorScopeStart_VarsInLvt)
{
    std::string_view text = R"(
        function foo() {
            for (let i = 0; i < 10; i++) {
                for (;;) {
                    let d = 4;
                }
            }
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

    const auto *fn = FindFunctionByName(*program, "ETSGLOBAL.foo:void;");

    ASSERT_NE(fn, nullptr);

    ASSERT_TRUE(HasVar(*fn, "i"));
    ASSERT_TRUE(HasVar(*fn, "d"));

    AssertVarRangeValid(*fn, "i");
    AssertVarRangeValid(*fn, "d");
}

TEST_F(ScopeLvtDebugTest, FunctionBlockScope_VarsInLvt)
{
    std::string_view text = R"(
        function foo() {
            { let test1 = 0; let test2 = 1; }
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

    const auto *fn = FindFunctionByName(*program, "ETSGLOBAL.foo:void;");

    ASSERT_NE(fn, nullptr);

    ASSERT_TRUE(HasVar(*fn, "test1"));
    ASSERT_TRUE(HasVar(*fn, "test2"));

    AssertVarRangeValid(*fn, "test1");
    AssertVarRangeValid(*fn, "test2");
}

}  // namespace ark::es2panda::compiler::test
/*
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

#include <iostream>
#include <string>
#include <utility>
#include <vector>
#include "assembly-function.h"
#include "assembly-ins.h"
#include "assembly-parser.h"
#include "assembly-program.h"
#include "test/utils/checker_test.h"

namespace ark::es2panda::compiler::test {

class AnyInsTest : public ::test::utils::CheckerTest {
public:
    AnyInsTest() = default;

    ~AnyInsTest() override = default;

    void AssertInstructions(const pandasm::Function &func, const std::vector<pandasm::Opcode> &expectedInstructions)
    {
        size_t index = 0;
        size_t expectedInsIndex = 0;
        while (index < func.ins.size()) {
            if (expectedInsIndex >= expectedInstructions.size()) {
                break;
            }
            if (func.ins.at(index).opcode == expectedInstructions[expectedInsIndex]) {
                expectedInsIndex++;
            }
            index++;
        }
        ASSERT_EQ(expectedInsIndex, expectedInstructions.size())
            << "Expected instructions order do not match actual instructions in function: " << func.name;
    }

    void AssertFunctionInstructions(pandasm::Program *program, const std::string &functionName,
                                    const std::vector<pandasm::Opcode> &expectedInstructions)
    {
        const auto &functionTable = program->functionStaticTable;

        auto found = functionTable.find(functionName);
        ASSERT_NE(found, functionTable.end()) << "Function not found: " << functionName;

        AssertInstructions(found->second, expectedInstructions);
    }

private:
    NO_COPY_SEMANTIC(AnyInsTest);
    NO_MOVE_SEMANTIC(AnyInsTest);
};

// CC-OFFNXT(huge_depth) solid logic
TEST_F(AnyInsTest, any_ins_test_01)
{
    std::string_view text = R"(
    // Assume this is dynamic class
    export declare class X {
        x: X

        constructor()
        constructor(x: X)
        constructor(p1: number, p2: string, p3: string)
    }

    function foo() {
        let x: X = new X()
        x = new X(x)
        x = new X(1, "", "")
    })";

    auto doNode = [this](ir::AstNode *ast) {
        if (ast->IsClassDefinition() && ast->AsClassDefinition()->IsGlobal()) {
            auto cls = ast->AsClassDefinition();
            cls->IterateRecursively([this](ir::AstNode *astInClass) {
                if (!astInClass->IsTyped()) {
                    return;
                }
                auto tstype = astInClass->AsTyped()->TsType();
                if (tstype != nullptr && tstype->IsETSObjectType() && tstype->AsETSObjectType()->Name() != "String") {
                    astInClass->AsTyped()->SetTsType(Checker()->GlobalETSAnyType());
                }
            });
        }
    };
    // Manually replace ETSObject type to ETSAnyType for testing purpose.
    auto program = RunCheckerWithCustomFunc("dummy.ets", text, doNode);
    ASSERT_NE(program, nullptr);

    const std::vector<pandasm::Opcode> expectedOps = {
        pandasm::Opcode::ANY_CALL_NEW_0, pandasm::Opcode::ANY_CALL_NEW_SHORT, pandasm::Opcode::ANY_CALL_NEW_RANGE};
    AssertFunctionInstructions(program.get(), "dummy.ETSGLOBAL.foo:void;", expectedOps);
}

// CC-OFFNXT(huge_depth) solid logic
TEST_F(AnyInsTest, any_ins_test_02)
{
    std::string_view text = R"(
    // Assume this is dynamic class
    export declare class X {
        x: X

        constructor()
    }

    function foo() {
        let x: X = new X()
        x.x = new X()
    })";

    auto doNode = [this](ir::AstNode *ast) {
        if (ast->IsClassDefinition() && ast->AsClassDefinition()->IsGlobal()) {
            auto cls = ast->AsClassDefinition();
            cls->IterateRecursively([this](ir::AstNode *astInClass) {
                if (!astInClass->IsTyped()) {
                    return;
                }
                auto tstype = astInClass->AsTyped()->TsType();
                if (tstype != nullptr && tstype->IsETSObjectType() && tstype->AsETSObjectType()->Name() != "String") {
                    astInClass->AsTyped()->SetTsType(Checker()->GlobalETSAnyType());
                }
            });
        }
    };
    // Manually replace ETSObject type to ETSAnyType for testing purpose.
    auto program = RunCheckerWithCustomFunc("dummy.ets", text, doNode);
    ASSERT_NE(program, nullptr);

    const std::vector<pandasm::Opcode> expectedOps = {pandasm::Opcode::ANY_CALL_NEW_0, pandasm::Opcode::ANY_CALL_NEW_0,
                                                      pandasm::Opcode::ANY_STBYNAME};
    AssertFunctionInstructions(program.get(), "dummy.ETSGLOBAL.foo:void;", expectedOps);
}

// CC-OFFNXT(huge_depth) solid logic
TEST_F(AnyInsTest, any_ins_test_03)
{
    std::string_view text = R"(
    // Assume this is dynamic class
    export declare class X {
        constructor()
    }

    function foo() {
        let x: X = new X()
        x instanceof X
    })";

    auto doNode = [this](ir::AstNode *ast) {
        if (ast->IsClassDefinition() && ast->AsClassDefinition()->IsGlobal()) {
            auto cls = ast->AsClassDefinition();
            cls->IterateRecursively([this](ir::AstNode *astInClass) {
                if (!astInClass->IsTyped()) {
                    return;
                }
                auto tstype = astInClass->AsTyped()->TsType();
                if (tstype != nullptr && tstype->IsETSObjectType() && tstype->AsETSObjectType()->Name() != "String") {
                    astInClass->AsTyped()->SetTsType(Checker()->GlobalETSRelaxedAnyType());
                }
            });
        }
    };
    // Manually replace ETSObject type to ETSAnyType for testing purpose.
    auto program = RunCheckerWithCustomFunc("dummy.ets", text, doNode);
    ASSERT_NE(program, nullptr);

    const std::vector<pandasm::Opcode> expectedOps = {pandasm::Opcode::ANY_CALL_NEW_0, pandasm::Opcode::ANY_ISINSTANCE};
    AssertFunctionInstructions(program.get(), "dummy.ETSGLOBAL.foo:void;", expectedOps);
}

// CC-OFFNXT(huge_depth) solid logic
TEST_F(AnyInsTest, any_ins_test_04)
{
    std::string_view text = R"(
    // Assume this is dynamic class
    export declare class X {
        constructor()

        foo(): void
        foo(x: X): void
        foo(p1: number, p2: string, p3: string): void
    }

    function bar() {
        let x : X = new X()
        x.foo()
        x.foo(x)
        x.foo(1, "", "")
    })";

    auto doNode = [this](ir::AstNode *ast) {
        if (ast->IsClassDefinition() && ast->AsClassDefinition()->IsGlobal()) {
            auto cls = ast->AsClassDefinition();
            cls->IterateRecursively([this](ir::AstNode *astInClass) {
                if (!astInClass->IsTyped()) {
                    return;
                }
                auto tstype = astInClass->AsTyped()->TsType();
                if (tstype != nullptr && tstype->IsETSObjectType() && tstype->AsETSObjectType()->Name() != "String") {
                    astInClass->AsTyped()->SetTsType(Checker()->GlobalETSAnyType());
                }
                if (tstype != nullptr && tstype->IsETSFunctionType() && tstype->AsETSFunctionType()->Name() == "foo") {
                    astInClass->AsTyped()->SetTsType(Checker()->GlobalETSAnyType());
                }
            });
        }
    };
    // Manually replace ETSObject type to ETSAnyType for testing purpose.
    auto program = RunCheckerWithCustomFunc("dummy.ets", text, doNode);
    ASSERT_NE(program, nullptr);

    const std::vector<pandasm::Opcode> expectedOps = {pandasm::Opcode::ANY_CALL_NEW_0, pandasm::Opcode::ANY_CALL_THIS_0,
                                                      pandasm::Opcode::ANY_CALL_THIS_SHORT,
                                                      pandasm::Opcode::ANY_CALL_THIS_RANGE};
    AssertFunctionInstructions(program.get(), "dummy.ETSGLOBAL.bar:void;", expectedOps);
}

// CC-OFFNXT(huge_depth) solid logic
TEST_F(AnyInsTest, any_ins_test_05)
{
    std::string_view text = R"(
    // Assume this is dynamic class
    export declare class X {
        s: string
    }

    class Y {
        s: string = ""
    }

    function foo(v: X | Y) {
        v.s
        if (v instanceof X) {
            v as X == v
        }
        if (v instanceof Y) {
            v as Y == v
        }
    })";

    auto doNode = [this](ir::AstNode *ast) {
        if (ast->IsClassDefinition() && ast->AsClassDefinition()->IsGlobal()) {
            auto cls = ast->AsClassDefinition();
            cls->IterateRecursively([this](ir::AstNode *astInClass) {
                if (!astInClass->IsTyped()) {
                    return;
                }
                auto tstype = astInClass->AsTyped()->TsType();
                if (tstype != nullptr && tstype->IsETSObjectType() && tstype->AsETSObjectType()->Name() != "Y") {
                    astInClass->AsTyped()->SetTsType(Checker()->GlobalETSRelaxedAnyType());
                }
            });
        }
    };
    // Manually replace ETSObject type to ETSAnyType for testing purpose.
    auto program = RunCheckerWithCustomFunc("dummy.ets", text, doNode);
    ASSERT_NE(program, nullptr);

    const std::vector<pandasm::Opcode> expectedOps = {pandasm::Opcode::ANY_ISINSTANCE, pandasm::Opcode::ISINSTANCE};
    AssertFunctionInstructions(program.get(), "dummy.ETSGLOBAL.foo:{Udummy.X,dummy.Y};void;", expectedOps);
}

// CC-OFFNXT(huge_depth) solid logic
TEST_F(AnyInsTest, any_ins_test_06)
{
    std::string_view text = R"(
    export declare class X {
    }

    function foo() {
        let x = new X()
        x == x
        x === x
        x ? true : false
    })";

    auto doNode = [this](ir::AstNode *ast) {
        if (ast->IsClassDefinition() && ast->AsClassDefinition()->IsGlobal()) {
            auto cls = ast->AsClassDefinition();
            cls->IterateRecursively([this](ir::AstNode *astInClass) {
                if (!astInClass->IsTyped()) {
                    return;
                }
                auto tstype = astInClass->AsTyped()->TsType();
                if (tstype != nullptr && tstype->IsETSObjectType()) {
                    astInClass->AsTyped()->SetTsType(Checker()->GlobalETSAnyType());
                }
            });
        }
    };
    // Manually replace ETSObject type to ETSAnyType for testing purpose.
    auto program = RunCheckerWithCustomFunc("dummy.ets", text, doNode);
    ASSERT_NE(program, nullptr);

    const std::vector<pandasm::Opcode> expectedOps = {pandasm::Opcode::ETS_EQUALS, pandasm::Opcode::ETS_STRICTEQUALS};
    AssertFunctionInstructions(program.get(), "dummy.ETSGLOBAL.foo:void;", expectedOps);
}

}  // namespace ark::es2panda::compiler::test
/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

#ifndef ETS_BYTECODE_OPTIMIZER_TESTS_ETS_OPT_H
#define ETS_BYTECODE_OPTIMIZER_TESTS_ETS_OPT_H

#include <gtest/gtest.h>
#include <string>
#include <string_view>

#include "assembler/assembly-emitter.h"
#include "assembler/assembly-parser.h"
#include "assembler/assembly-program.h"
#include "assembler/extensions/extensions.h"
#include "bytecode_optimizer/canonicalization.h"
#include "compiler/compiler_options.h"
#include "compiler/optimizer/analysis/rpo.h"
#include "compiler/optimizer/ir/inst.h"
#include "compiler/optimizer/ir/ir_constructor.h"
#include "compiler/optimizer/optimizations/regalloc/reg_alloc_linear_scan.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include "mem/arena_allocator.h"
#include "mem/pool_manager.h"
#include "optimizer/ir/inst.h"
#include "bytecode_optimizer/reg_acc_alloc.h"
#include "bytecode_optimizer/optimize_bytecode.h"
#include "bytecode_optimizer/panda_gen_options/generated/bytecodeopt_options_gen.h"
#include "parser/ETSparser.h"

namespace ark::bytecodeopt {

class EtsOptTest : public ::testing::Test {
public:
    EtsOptTest()
    {
        ark::compiler::g_options.SetCompilerUseSafepoint(false);
        ark::compiler::g_options.SetCompilerSupportInitObjectInst(true);

        // NOLINTNEXTLINE(readability-magic-numbers)
        mem::MemConfig::Initialize(128_MB, 64_MB, 64_MB, 32_MB, 0, 0);
        PoolManager::Initialize();
        allocator_ = new ArenaAllocator(SpaceType::SPACE_TYPE_INTERNAL);
        localAllocator_ = new ArenaAllocator(SpaceType::SPACE_TYPE_INTERNAL);
        builder_ = new compiler::IrConstructor();

        Logger::InitializeStdLogging(Logger::Level::ERROR,
                                     ark::Logger::ComponentMask().set(Logger::Component::BYTECODE_OPTIMIZER));
    }
    ~EtsOptTest() override
    {
        delete allocator_;
        delete localAllocator_;
        delete builder_;
        PoolManager::Finalize();
        mem::MemConfig::Finalize();

        Logger::Destroy();
    }

    NO_COPY_SEMANTIC(EtsOptTest);
    NO_MOVE_SEMANTIC(EtsOptTest);

    ArenaAllocator *GetAllocator()
    {
        return allocator_;
    }
    ArenaAllocator *GetLocalAllocator()
    {
        return localAllocator_;
    }

    compiler::Graph *CreateEmptyGraph(bool isDynamic = false)
    {
        auto *graph =
            GetAllocator()->New<compiler::Graph>(GetAllocator(), GetLocalAllocator(), Arch::NONE, isDynamic, true);
        return graph;
    }

    compiler::Graph *GetGraph()
    {
        return graph_;
    }

    void SetGraph(compiler::Graph *graph)
    {
        graph_ = graph;
    }

    bool FuncHasInst(pandasm::Function *func, pandasm::Opcode opcode)
    {
        for (const auto &inst : func->ins) {
            if (inst.opcode == opcode) {
                return true;
            }
        }
        return false;
    }

protected:
    // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
    compiler::IrConstructor *builder_;

private:
    ArenaAllocator *allocator_;
    ArenaAllocator *localAllocator_;
    compiler::Graph *graph_ {nullptr};
};

}  // namespace ark::bytecodeopt

#endif  // ETS_BYTECODE_OPTIMIZER_TESTS_ETS_OPT_H
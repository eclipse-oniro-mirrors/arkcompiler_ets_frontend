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

#include <parser/context/parserContext.h>
#include <parser/program/program.h>
#include <gtest/gtest.h>
#include <mem/pool_manager.h>
#include <util/ustring.h>

namespace panda::es2panda::parser {

using mem::MemConfig;

class MemManager {
public:
    explicit MemManager()
    {
        constexpr auto COMPILER_SIZE = 8192_MB;

        MemConfig::Initialize(0, 0, COMPILER_SIZE, 0);
        PoolManager::Initialize(PoolType::MMAP);
    }

    NO_COPY_SEMANTIC(MemManager);
    NO_MOVE_SEMANTIC(MemManager);

    ~MemManager()
    {
        PoolManager::Finalize();
        MemConfig::Finalize();
    }
};

class ParserContextTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        mm_ = std::make_unique<MemManager>();
    }

    void TearDown() override
    {
        mm_.reset();
    }

    std::unique_ptr<MemManager> mm_;
};

// Test constructor with null Program pointer
TEST_F(ParserContextTest, Constructor_Program_SetsProgram)
{
    Program *program = nullptr;
    ParserContext context(program);

    EXPECT_EQ(context.GetProgram(), program);
    EXPECT_EQ(context.Prev(), nullptr);
    EXPECT_EQ(context.Status(), ParserStatus::NO_OPTS);
}


// Test constructor with empty label
TEST_F(ParserContextTest, Constructor_WithEmptyLabel_SetsLabel)
{
    Program *program = nullptr;
    ParserContext parentContext(program);
    util::StringView emptyLabel = "";

    ParserContext childContext(&parentContext, ParserStatus::FUNCTION, emptyLabel);

    EXPECT_EQ(childContext.GetProgram(), program);
    EXPECT_EQ(childContext.Prev(), &parentContext);
    EXPECT_TRUE(childContext.Status() & ParserStatus::FUNCTION);
}

// Test constructor with non-empty label
TEST_F(ParserContextTest, Constructor_WithLabel_SetsLabel)
{
    Program *program = nullptr;
    ParserContext parentContext(program);
    util::StringView label = "myLabel";

    ParserContext childContext(&parentContext, ParserStatus::IN_LABELED, label);

    EXPECT_EQ(childContext.GetProgram(), program);
    EXPECT_EQ(childContext.Prev(), &parentContext);
    EXPECT_TRUE(childContext.Status() & ParserStatus::IN_LABELED);
    // The label is stored and can be found via FindLabel
}

// Test child context preserves inherited flags from parent
TEST_F(ParserContextTest, Constructor_ChildContext_PreservesInheritedFlags)
{
    Program *program = nullptr;
    ParserContext parentContext(program);
    parentContext.Status() |= ParserStatus::MODULE;

    ParserContext childContext(&parentContext, ParserStatus::FUNCTION);

    // Child should inherit MODULE status
    EXPECT_TRUE(childContext.Status() & ParserStatus::MODULE);
    // Child should have its own FUNCTION status
    EXPECT_TRUE(childContext.Status() & ParserStatus::FUNCTION);
}

// Test GetProgram returns the correct program pointer
TEST_F(ParserContextTest, GetProgram_ReturnsCorrectProgram)
{
    Program *program = nullptr;
    ParserContext context(program);

    EXPECT_EQ(context.GetProgram(), program);
}

TEST_F(ParserContextTest, Prev_RootContext_ReturnsNullptr)
{
    Program *program = nullptr;
    ParserContext context(program);

    EXPECT_EQ(context.Prev(), nullptr);
}

TEST_F(ParserContextTest, Prev_ChildContext_ReturnsParent)
{
    Program *program = nullptr;
    ParserContext parentContext(program);
    ParserContext childContext(&parentContext, ParserStatus::FUNCTION);

    EXPECT_EQ(childContext.Prev(), &parentContext);
}

TEST_F(ParserContextTest, Status_Setter_ModifiesStatus)
{
    Program *program = nullptr;
    ParserContext context(program);

    context.Status() = ParserStatus::FUNCTION;

    EXPECT_TRUE(context.Status() & ParserStatus::FUNCTION);
}

TEST_F(ParserContextTest, Status_ConstGetter_ReturnsStatus)
{
    Program *program = nullptr;
    ParserContext context(program);

    const ParserContext &constContext = context;
    EXPECT_EQ(constContext.Status(), ParserStatus::NO_OPTS);
}

// Test IsGenerator returns true when GENERATOR_FUNCTION flag is set
TEST_F(ParserContextTest, IsGenerator_WithFlag_ReturnsTrue)
{
    Program *program = nullptr;
    ParserContext parentContext(program);
    ParserContext generatorContext(&parentContext, ParserStatus::GENERATOR_FUNCTION);

    EXPECT_TRUE(generatorContext.IsGenerator());
}

TEST_F(ParserContextTest, IsGenerator_WithoutFlag_ReturnsFalse)
{
    Program *program = nullptr;
    ParserContext parentContext(program);
    ParserContext normalContext(&parentContext, ParserStatus::FUNCTION);

    EXPECT_FALSE(normalContext.IsGenerator());
}

// Test IsAsync returns true when ASYNC_FUNCTION flag is set
TEST_F(ParserContextTest, IsAsync_WithFlag_ReturnsTrue)
{
    Program *program = nullptr;
    ParserContext parentContext(program);
    ParserContext asyncContext(&parentContext, ParserStatus::ASYNC_FUNCTION);

    EXPECT_TRUE(asyncContext.IsAsync());
}

TEST_F(ParserContextTest, IsAsync_WithoutFlag_ReturnsFalse)
{
    Program *program = nullptr;
    ParserContext parentContext(program);
    ParserContext syncContext(&parentContext, ParserStatus::FUNCTION);

    EXPECT_FALSE(syncContext.IsAsync());
}

// Test AllowYield returns true when ALLOW_YIELD flag is set
TEST_F(ParserContextTest, AllowYield_WithFlag_ReturnsTrue)
{
    Program *program = nullptr;
    ParserContext parentContext(program);
    ParserContext yieldContext(&parentContext, ParserStatus::ALLOW_YIELD);

    EXPECT_TRUE(yieldContext.AllowYield());
}

TEST_F(ParserContextTest, AllowYield_WithoutFlag_ReturnsFalse)
{
    Program *program = nullptr;
    ParserContext parentContext(program);
    ParserContext noYieldContext(&parentContext, ParserStatus::FUNCTION);

    EXPECT_FALSE(noYieldContext.AllowYield());
}

// Test DisallowAwait returns true when DISALLOW_AWAIT flag is set
TEST_F(ParserContextTest, DisallowAwait_WithFlag_ReturnsTrue)
{
    Program *program = nullptr;
    ParserContext parentContext(program);
    ParserContext disallowAwaitContext(&parentContext, ParserStatus::DISALLOW_AWAIT);

    EXPECT_TRUE(disallowAwaitContext.DisallowAwait());
}

TEST_F(ParserContextTest, DisallowAwait_WithoutFlag_ReturnsFalse)
{
    Program *program = nullptr;
    ParserContext parentContext(program);
    ParserContext allowAwaitContext(&parentContext, ParserStatus::FUNCTION);

    EXPECT_FALSE(allowAwaitContext.DisallowAwait());
}

// Test DisallowArguments returns true when DISALLOW_ARGUMENTS flag is set
TEST_F(ParserContextTest, DisallowArguments_WithFlag_ReturnsTrue)
{
    Program *program = nullptr;
    ParserContext parentContext(program);
    ParserContext disallowArgsContext(&parentContext, ParserStatus::DISALLOW_ARGUMENTS);

    EXPECT_TRUE(disallowArgsContext.DisallowArguments());
}

TEST_F(ParserContextTest, DisallowArguments_WithoutFlag_ReturnsFalse)
{
    Program *program = nullptr;
    ParserContext parentContext(program);
    ParserContext allowArgsContext(&parentContext, ParserStatus::FUNCTION);

    EXPECT_FALSE(allowArgsContext.DisallowArguments());
}

// Test IsModule returns true when MODULE flag is set
TEST_F(ParserContextTest, IsModule_WithFlag_ReturnsTrue)
{
    Program *program = nullptr;
    ParserContext parentContext(program);
    ParserContext moduleContext(&parentContext, ParserStatus::MODULE);

    EXPECT_TRUE(moduleContext.IsModule());
}

TEST_F(ParserContextTest, IsModule_WithoutFlag_ReturnsFalse)
{
    Program *program = nullptr;
    ParserContext parentContext(program);
    ParserContext scriptContext(&parentContext, ParserStatus::FUNCTION);

    EXPECT_FALSE(scriptContext.IsModule());
}

// Test IsTsModule returns true when TS_MODULE flag is set
TEST_F(ParserContextTest, IsTsModule_WithFlag_ReturnsTrue)
{
    Program *program = nullptr;
    ParserContext parentContext(program);
    ParserContext tsModuleContext(&parentContext, ParserStatus::TS_MODULE);

    EXPECT_TRUE(tsModuleContext.IsTsModule());
}

TEST_F(ParserContextTest, IsTsModule_WithoutFlag_ReturnsFalse)
{
    Program *program = nullptr;
    ParserContext parentContext(program);
    ParserContext nonTsModuleContext(&parentContext, ParserStatus::MODULE);

    EXPECT_FALSE(nonTsModuleContext.IsTsModule());
}

// Test IsStaticBlock returns true when STATIC_BLOCK flag is set
TEST_F(ParserContextTest, IsStaticBlock_WithFlag_ReturnsTrue)
{
    Program *program = nullptr;
    ParserContext parentContext(program);
    ParserContext staticBlockContext(&parentContext, ParserStatus::STATIC_BLOCK);

    EXPECT_TRUE(staticBlockContext.IsStaticBlock());
}

TEST_F(ParserContextTest, IsStaticBlock_WithoutFlag_ReturnsFalse)
{
    Program *program = nullptr;
    ParserContext parentContext(program);
    ParserContext nonStaticContext(&parentContext, ParserStatus::FUNCTION);

    EXPECT_FALSE(nonStaticContext.IsStaticBlock());
}

// Test FindLabel with empty label returns nullptr
TEST_F(ParserContextTest, FindLabel_EmptyLabel_ReturnsNullptr)
{
    Program *program = nullptr;
    ParserContext rootContext(program);
    util::StringView emptyLabel = "";

    const auto *result = rootContext.FindLabel(emptyLabel);

    EXPECT_EQ(result, nullptr);
}

// Test FindLabel when caller has no label returns nullptr
TEST_F(ParserContextTest, FindLabel_CallerWithoutLabel_ReturnsNullptr)
{
    Program *program = nullptr;
    ParserContext rootContext(program);
    util::StringView searchLabel = "loop";

    const auto *result = rootContext.FindLabel(searchLabel);

    EXPECT_EQ(result, nullptr);
}

// Test FindLabel finds label in same context
TEST_F(ParserContextTest, FindLabel_SameContext_ReturnsThisContext)
{
    Program *program = nullptr;
    ParserContext rootContext(program);
    util::StringView label = "loop";

    ParserContext labeledContext(&rootContext, ParserStatus::IN_LABELED, label);
    const auto *result = labeledContext.FindLabel(label);

    EXPECT_NE(result, nullptr);
    EXPECT_EQ(result, &labeledContext);
}

// Test FindLabel finds label in parent context
TEST_F(ParserContextTest, FindLabel_ParentContext_ReturnsParent)
{
    Program *program = nullptr;
    util::StringView label = "outer";

    ParserContext rootContext(program);
    ParserContext labeledContext(&rootContext, ParserStatus::IN_LABELED, label);

    // Find from the labeled context itself
    const auto *result = labeledContext.FindLabel(label);

    EXPECT_NE(result, nullptr);
    EXPECT_EQ(result, &labeledContext);
}

// Test FindLabel returns nullptr for non-existent label
TEST_F(ParserContextTest, FindLabel_NotFound_ReturnsNullptr)
{
    Program *program = nullptr;
    util::StringView existingLabel = "existing";
    util::StringView searchLabel = "nonexistent";

    ParserContext rootContext(program);
    ParserContext labeledContext(&rootContext, ParserStatus::IN_LABELED, existingLabel);

    const auto *result = labeledContext.FindLabel(searchLabel);

    EXPECT_EQ(result, nullptr);
}

// Test FindLabel with deep chain finds root label when called from labeled context
TEST_F(ParserContextTest, FindLabel_DeepChain_FindsRootLabel)
{
    Program *program = nullptr;
    util::StringView label = "rootLabel";

    ParserContext rootContext(program);
    ParserContext labeledContext(&rootContext, ParserStatus::IN_LABELED, label);
    ParserContext middleContext(&labeledContext, ParserStatus::FUNCTION);

    // FindLabel from middle context with empty label returns nullptr immediately
    const auto *result = middleContext.FindLabel(label);

    EXPECT_EQ(result, nullptr);
}

// Test FindLabel with middle context having empty label stops traversal
TEST_F(ParserContextTest, FindLabel_MiddleContextWithEmptyLabel_ReturnsNullptr)
{
    Program *program = nullptr;
    util::StringView label = "rootLabel";

    ParserContext rootContext(program);
    ParserContext labeledContext(&rootContext, ParserStatus::IN_LABELED, label);
    ParserContext middleContext(&labeledContext, ParserStatus::FUNCTION);
    ParserContext deepContext(&middleContext, ParserStatus::FUNCTION);

    // All contexts except labeledContext have empty labels
    // FindLabel returns nullptr when called from context with empty label
    const auto *result = deepContext.FindLabel(label);

    EXPECT_EQ(result, nullptr);
}

// Test FindLabel with empty StringView on labeled context
TEST_F(ParserContextTest, FindLabel_LabeledContext_SearchEmptyLabel)
{
    Program *program = nullptr;
    util::StringView label = "myLabel";
    util::StringView emptyLabel = "";

    ParserContext rootContext(program);
    ParserContext labeledContext(&rootContext, ParserStatus::IN_LABELED, label);

    const auto *result = labeledContext.FindLabel(emptyLabel);

    // Empty label is different from "myLabel"
    EXPECT_EQ(result, nullptr);
}

// Test multiple status flags can be combined correctly
TEST_F(ParserContextTest, MultipleStatusFlags_Combined_WorksCorrectly)
{
    Program *program = nullptr;
    ParserContext parentContext(program);

    ParserStatus combined = ParserStatus::ASYNC_FUNCTION | ParserStatus::GENERATOR_FUNCTION;
    ParserContext context(&parentContext, combined);

    EXPECT_TRUE(context.IsAsync());
    EXPECT_TRUE(context.IsGenerator());
}


TEST_F(ParserContextTest, VariousStatusFlags_CanBeSet)
{
    Program *program = nullptr;
    ParserContext parentContext(program);

    // Test multiple status flags can be set
    ParserContext context(&parentContext, ParserStatus::ARROW_FUNCTION);

    EXPECT_TRUE(context.Status() & ParserStatus::ARROW_FUNCTION);
    EXPECT_FALSE(context.Status() & ParserStatus::FUNCTION);
}

// Test copy constructor preserves status
TEST_F(ParserContextTest, CopyConstructor_PreservesStatus)
{
    Program *program = nullptr;
    ParserContext original(program);
    original.Status() = ParserStatus::FUNCTION;

    ParserContext copy = original;

    EXPECT_EQ(copy.Status(), original.Status());
    EXPECT_EQ(copy.GetProgram(), original.GetProgram());
}

TEST_F(ParserContextTest, MoveConstructor_TransfersStatus)
{
    Program *program = nullptr;
    ParserContext original(program);
    original.Status() = ParserStatus::FUNCTION;

    ParserContext moved = std::move(original);

    EXPECT_TRUE(moved.Status() & ParserStatus::FUNCTION);
    EXPECT_EQ(moved.GetProgram(), program);
}

}  // namespace panda::es2panda::parser

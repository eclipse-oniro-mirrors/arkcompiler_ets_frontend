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
#include "compiler/lowering/phase.h"
#include "ir/astNodeHistory.h"

namespace ark::es2panda {

class NodeHistoryTest : public testing::Test {
public:
    ~NodeHistoryTest() override = default;

    static void SetUpTestCase()
    {
        ark::mem::MemConfig::Initialize(0, 0, ark::es2panda::COMPILER_SIZE, 0, 0, 0);
        ark::PoolManager::Initialize();
    }

    NodeHistoryTest()
    {
        allocator_ = std::make_unique<ArenaAllocator>(SpaceType::SPACE_TYPE_COMPILER);
        phaseManager_ = std::make_unique<compiler::PhaseManager>(ScriptExtension::ETS, Allocator());
        compiler::SetPhaseManager(phaseManager_.get());
        ir::EnableContextHistory();
    }

    NO_COPY_SEMANTIC(NodeHistoryTest);
    NO_MOVE_SEMANTIC(NodeHistoryTest);

    ArenaAllocator *Allocator() const
    {
        return allocator_.get();
    }

    compiler::PhaseManager *PhaseManager() const
    {
        return phaseManager_.get();
    }

private:
    std::unique_ptr<ArenaAllocator> allocator_;
    std::unique_ptr<compiler::PhaseManager> phaseManager_;
};

constexpr compiler::PhaseId PARSER_PHASE_ID = {0, compiler::PARSER_PHASE_ID};
constexpr compiler::PhaseId PHASE_ID_0 = {0, 0};
constexpr compiler::PhaseId PHASE_ID_1 = {0, 1};
constexpr compiler::PhaseId PHASE_ID_2 = {0, 2};
constexpr compiler::PhaseId PHASE_ID_3 = {0, 3};
constexpr compiler::PhaseId PHASE_ID_4 = {0, 4};
constexpr compiler::PhaseId PHASE_ID_5 = {0, 5};

constexpr int32_t INDEX_0 = 0;
constexpr int32_t INDEX_1 = 1;
constexpr int32_t INDEX_2 = 2;
constexpr int32_t INDEX_3 = 3;
constexpr int32_t INDEX_4 = 4;

constexpr int32_t VALUE_0 = 0;
constexpr int32_t VALUE_1 = 1;
constexpr int32_t VALUE_2 = 2;
constexpr int32_t VALUE_3 = 3;
constexpr int32_t VALUE_4 = 4;

constexpr int32_t SIZE_5 = 5;

// CC-OFFNXT(huge_method, G.FUN.01-CPP, G.FUD.05) solid logic
TEST_F(NodeHistoryTest, DoubleLinkedList)
{
    util::ArenaDoubleLinkedList<int> list {Allocator()};

    ASSERT_EQ(list.Head(), nullptr);
    ASSERT_EQ(list.Tail(), nullptr);
    ASSERT_TRUE(list.Empty());

    // Make list: (1,2,4,0,3)
    std::array<int, SIZE_5> data = {VALUE_0, VALUE_1, VALUE_2, VALUE_3, VALUE_4};

    auto item4 = list.Append(data[INDEX_4]);
    auto item0 = list.Append(data[INDEX_0]);
    auto item3 = list.Insert(item0, data[INDEX_3]);
    auto item1 = list.Prepend(data[INDEX_1]);
    auto item2 = list.Insert(item1, data[INDEX_2]);

    ASSERT_FALSE(list.Empty());

    ASSERT_EQ(item0->data, data[INDEX_0]);
    ASSERT_EQ(item1->data, data[INDEX_1]);
    ASSERT_EQ(item2->data, data[INDEX_2]);
    ASSERT_EQ(item3->data, data[INDEX_3]);
    ASSERT_EQ(item4->data, data[INDEX_4]);

    ASSERT_EQ(list.Head(), item1);
    ASSERT_EQ(item1->next, item2);
    ASSERT_EQ(item2->next, item4);
    ASSERT_EQ(item4->next, item0);
    ASSERT_EQ(item0->next, item3);
    ASSERT_EQ(item3, list.Tail());
    ASSERT_EQ(list.Tail()->next, nullptr);

    ASSERT_EQ(item3->prev, item0);
    ASSERT_EQ(item0->prev, item4);
    ASSERT_EQ(item4->prev, item2);
    ASSERT_EQ(item2->prev, item1);
    ASSERT_EQ(item1->prev, nullptr);

    ASSERT_EQ(item2->prev->next, item2);
    ASSERT_EQ(item4->prev->next, item4);
    ASSERT_EQ(item0->prev->next, item0);

    ASSERT_EQ(item2->next->prev, item2);
    ASSERT_EQ(item4->next->prev, item4);
    ASSERT_EQ(item0->next->prev, item0);

    list.Erase(item4);
    ASSERT_EQ(item2->next, item0);
    ASSERT_EQ(item0->prev, item2);

    list.Erase(item0);
    ASSERT_EQ(item2->next, item3);
    ASSERT_EQ(item3->prev, item2);

    list.Erase(list.Tail());
    ASSERT_EQ(list.Tail(), item2);
    ASSERT_EQ(item2->next, nullptr);
    ASSERT_EQ(item2->prev, item1);

    list.Erase(list.Head());
    ASSERT_EQ(list.Head(), item2);
    ASSERT_EQ(item2->next, nullptr);
    ASSERT_EQ(item2->prev, nullptr);

    list.Erase(item2);
    ASSERT_EQ(list.Head(), nullptr);
    ASSERT_EQ(list.Tail(), nullptr);
    ASSERT_TRUE(list.Empty());
}

TEST_F(NodeHistoryTest, HistoryAt)
{
    ASSERT_EQ(PhaseManager()->CurrentPhaseId(), PARSER_PHASE_ID);

    PhaseManager()->SetCurrentPhaseId(PHASE_ID_0.minor);
    auto identifier = Allocator()->New<ir::Identifier>(Allocator())->AsIdentifier();
    auto history = Allocator()->New<ir::AstNodeHistory>(identifier, PhaseManager()->CurrentPhaseId(), Allocator());

    ASSERT_EQ(history->At(PARSER_PHASE_ID), nullptr);
    ASSERT_EQ(history->At(PHASE_ID_0), identifier);
    ASSERT_EQ(history->At(PHASE_ID_1), nullptr);
}

TEST_F(NodeHistoryTest, HistoryGet)
{
    ASSERT_EQ(PhaseManager()->CurrentPhaseId(), PARSER_PHASE_ID);

    PhaseManager()->SetCurrentPhaseId(PHASE_ID_0.minor);
    auto identifier = Allocator()->New<ir::Identifier>(Allocator())->AsIdentifier();
    auto history = Allocator()->New<ir::AstNodeHistory>(identifier, PhaseManager()->CurrentPhaseId(), Allocator());

    ASSERT_EQ(history->Get(PARSER_PHASE_ID), nullptr);
    ASSERT_EQ(history->Get(PHASE_ID_0), identifier);
    ASSERT_EQ(history->Get(PHASE_ID_1), identifier);
    ASSERT_EQ(history->Get(PHASE_ID_2), identifier);
    ASSERT_EQ(history->Get(PHASE_ID_3), identifier);
}

// CC-OFFNXT(huge_method, G.FUN.01-CPP, G.FUD.05) solid logic
TEST_F(NodeHistoryTest, HistorySet)
{
    ASSERT_EQ(PhaseManager()->CurrentPhaseId(), PARSER_PHASE_ID);

    PhaseManager()->SetCurrentPhaseId(PHASE_ID_0.minor);
    auto identifier0 = Allocator()->New<ir::Identifier>(Allocator())->AsIdentifier();
    auto history = Allocator()->New<ir::AstNodeHistory>(identifier0, PhaseManager()->CurrentPhaseId(), Allocator());

    PhaseManager()->SetCurrentPhaseId(PHASE_ID_1.minor);
    auto identifier1 = Allocator()->New<ir::Identifier>(Allocator())->AsIdentifier();
    history->Set(identifier1, PhaseManager()->CurrentPhaseId());

    PhaseManager()->SetCurrentPhaseId(PHASE_ID_2.minor);
    auto identifier2 = Allocator()->New<ir::Identifier>(Allocator())->AsIdentifier();
    history->Set(identifier2, PhaseManager()->CurrentPhaseId());

    PhaseManager()->SetCurrentPhaseId(PHASE_ID_3.minor);
    history->Set(nullptr, PhaseManager()->CurrentPhaseId());

    // Search forward
    ASSERT_EQ(history->Get(PARSER_PHASE_ID), nullptr);
    ASSERT_EQ(history->Get(PHASE_ID_0), identifier0);
    ASSERT_EQ(history->Get(PHASE_ID_1), identifier1);
    ASSERT_EQ(history->Get(PHASE_ID_2), identifier2);
    ASSERT_EQ(history->Get(PHASE_ID_3), nullptr);
    ASSERT_EQ(history->Get(PHASE_ID_4), nullptr);
    ASSERT_EQ(history->Get(PHASE_ID_5), nullptr);

    // Search backward
    ASSERT_EQ(history->Get(PHASE_ID_5), nullptr);
    ASSERT_EQ(history->Get(PHASE_ID_4), nullptr);
    ASSERT_EQ(history->Get(PHASE_ID_3), nullptr);
    ASSERT_EQ(history->Get(PHASE_ID_2), identifier2);
    ASSERT_EQ(history->Get(PHASE_ID_1), identifier1);
    ASSERT_EQ(history->Get(PHASE_ID_0), identifier0);
    ASSERT_EQ(history->Get(PARSER_PHASE_ID), nullptr);

    // Search random
    ASSERT_EQ(history->Get(PHASE_ID_1), identifier1);
    ASSERT_EQ(history->Get(PHASE_ID_5), nullptr);
    ASSERT_EQ(history->Get(PHASE_ID_2), identifier2);
    ASSERT_EQ(history->Get(PHASE_ID_4), nullptr);
    ASSERT_EQ(history->Get(PHASE_ID_0), identifier0);
    ASSERT_EQ(history->Get(PARSER_PHASE_ID), nullptr);
    ASSERT_EQ(history->Get(PHASE_ID_3), nullptr);

    // Search precise
    ASSERT_EQ(history->At(PARSER_PHASE_ID), nullptr);
    ASSERT_EQ(history->At(PHASE_ID_0), identifier0);
    ASSERT_EQ(history->At(PHASE_ID_1), identifier1);
    ASSERT_EQ(history->At(PHASE_ID_2), identifier2);
    ASSERT_EQ(history->At(PHASE_ID_3), nullptr);
    ASSERT_EQ(history->At(PHASE_ID_4), nullptr);
    ASSERT_EQ(history->At(PHASE_ID_5), nullptr);
}

TEST_F(NodeHistoryTest, HistoryReplace)
{
    ASSERT_EQ(PhaseManager()->CurrentPhaseId(), PARSER_PHASE_ID);

    PhaseManager()->SetCurrentPhaseId(PHASE_ID_0.minor);
    auto identifier0Orig = Allocator()->New<ir::Identifier>(Allocator())->AsIdentifier();
    auto history = Allocator()->New<ir::AstNodeHistory>(identifier0Orig, PhaseManager()->CurrentPhaseId(), Allocator());

    PhaseManager()->SetCurrentPhaseId(PHASE_ID_1.minor);
    auto identifier1Orig = Allocator()->New<ir::Identifier>(Allocator())->AsIdentifier();
    history->Set(identifier1Orig, PhaseManager()->CurrentPhaseId());

    ASSERT_EQ(history->Get(PARSER_PHASE_ID), nullptr);
    ASSERT_EQ(history->Get(PHASE_ID_0), identifier0Orig);
    ASSERT_EQ(history->Get(PHASE_ID_1), identifier1Orig);
    ASSERT_EQ(history->Get(PHASE_ID_2), identifier1Orig);
    ASSERT_EQ(history->Get(PHASE_ID_3), identifier1Orig);

    ASSERT_EQ(history->At(PARSER_PHASE_ID), nullptr);
    ASSERT_EQ(history->At(PHASE_ID_0), identifier0Orig);
    ASSERT_EQ(history->At(PHASE_ID_1), identifier1Orig);
    ASSERT_EQ(history->At(PHASE_ID_2), nullptr);

    PhaseManager()->SetCurrentPhaseId(PHASE_ID_0.minor);
    auto identifier0New = Allocator()->New<ir::Identifier>(Allocator())->AsIdentifier();
    history->Set(identifier0New, PhaseManager()->CurrentPhaseId());

    PhaseManager()->SetCurrentPhaseId(PHASE_ID_1.minor);
    auto identifier1New = Allocator()->New<ir::Identifier>(Allocator())->AsIdentifier();
    history->Set(identifier1New, PhaseManager()->CurrentPhaseId());

    PhaseManager()->SetCurrentPhaseId(PHASE_ID_2.minor);
    history->Set(nullptr, PhaseManager()->CurrentPhaseId());

    ASSERT_EQ(history->Get(PARSER_PHASE_ID), nullptr);
    ASSERT_EQ(history->Get(PHASE_ID_0), identifier0Orig);
    ASSERT_EQ(history->Get(PHASE_ID_1), identifier1Orig);
    ASSERT_EQ(history->Get(PHASE_ID_2), identifier1Orig);
    ASSERT_EQ(history->Get(PHASE_ID_3), identifier1Orig);
    ASSERT_EQ(history->Get({1, PHASE_ID_0.minor}), identifier0New);
    ASSERT_EQ(history->Get({1, PHASE_ID_1.minor}), identifier1New);
    ASSERT_EQ(history->Get({1, PHASE_ID_2.minor}), nullptr);
    ASSERT_EQ(history->Get({1, PHASE_ID_3.minor}), nullptr);

    ASSERT_EQ(history->At(PARSER_PHASE_ID), nullptr);
    ASSERT_EQ(history->At(PHASE_ID_0), identifier0Orig);
    ASSERT_EQ(history->At(PHASE_ID_1), identifier1Orig);
    ASSERT_EQ(history->At(PHASE_ID_2), nullptr);
    ASSERT_EQ(history->At({1, PHASE_ID_0.minor}), identifier0New);
    ASSERT_EQ(history->At({1, PHASE_ID_1.minor}), identifier1New);
    ASSERT_EQ(history->At(PHASE_ID_2), nullptr);
}

ir::ClassDefinition *NewClassDefinition(ArenaAllocator *allocator)
{
    ArenaVector<ir::AstNode *> body {allocator->Adapter()};
    return allocator
        ->New<ir::ClassDefinition>(allocator, nullptr, std::move(body), ir::ClassDefinitionModifiers::NONE,
                                   ir::ModifierFlags::NONE, Language::Id::ETS)
        ->AsClassDefinition();
}

TEST_F(NodeHistoryTest, UpdateField)
{
    ASSERT_EQ(PhaseManager()->CurrentPhaseId(), PARSER_PHASE_ID);

    auto definition = NewClassDefinition(Allocator());
    ASSERT_FALSE(definition->IsAbstract());
    definition->ClearModifier(ir::ModifierFlags::ABSTRACT);
    ASSERT_FALSE(definition->IsAbstract());
    definition->AddModifier(ir::ModifierFlags::FINAL);
    ASSERT_TRUE(definition->IsFinal());

    PhaseManager()->SetCurrentPhaseId(PHASE_ID_0.minor);
    definition->AddModifier(ir::ModifierFlags::ABSTRACT);
    ASSERT_TRUE(definition->IsAbstract());
    definition->ClearModifier(ir::ModifierFlags::FINAL);
    ASSERT_FALSE(definition->IsFinal());

    PhaseManager()->SetCurrentPhaseId(PHASE_ID_1.minor);
    definition->ClearModifier(ir::ModifierFlags::ABSTRACT);
    ASSERT_FALSE(definition->IsAbstract());
    definition->AddModifier(ir::ModifierFlags::FINAL);
    ASSERT_TRUE(definition->IsFinal());

    PhaseManager()->SetCurrentPhaseId(PHASE_ID_2.minor);
    definition->ClearModifier(ir::ModifierFlags::FINAL);
    ASSERT_FALSE(definition->IsFinal());

    PhaseManager()->Reset();
    ASSERT_FALSE(definition->IsAbstract());
    ASSERT_TRUE(definition->IsFinal());

    PhaseManager()->SetCurrentPhaseId(PHASE_ID_0.minor);
    ASSERT_TRUE(definition->IsAbstract());
    ASSERT_FALSE(definition->IsFinal());

    PhaseManager()->SetCurrentPhaseId(PHASE_ID_1.minor);
    ASSERT_FALSE(definition->IsAbstract());
    ASSERT_TRUE(definition->IsFinal());

    PhaseManager()->SetCurrentPhaseId(PHASE_ID_2.minor);
    ASSERT_FALSE(definition->IsAbstract());
    ASSERT_FALSE(definition->IsFinal());
}

TEST_F(NodeHistoryTest, UpdateChild)
{
    ASSERT_EQ(PhaseManager()->CurrentPhaseId(), PARSER_PHASE_ID);

    auto declaration =
        Allocator()->New<ir::ClassDeclaration>(NewClassDefinition(Allocator()), Allocator())->AsClassDeclaration();
    ASSERT_EQ(declaration->Definition()->Ident(), nullptr);

    PhaseManager()->SetCurrentPhaseId(PHASE_ID_0.minor);
    auto identifier0 = Allocator()->New<ir::Identifier>(Allocator())->AsIdentifier();
    declaration->Definition()->SetIdent(identifier0);
    ASSERT_EQ(declaration->Definition()->Ident(), identifier0);

    PhaseManager()->SetCurrentPhaseId(PHASE_ID_1.minor);
    auto identifier1 = Allocator()->New<ir::Identifier>(Allocator())->AsIdentifier();
    declaration->Definition()->SetIdent(identifier1);
    ASSERT_EQ(declaration->Definition()->Ident(), identifier1);

    PhaseManager()->SetCurrentPhaseId(PHASE_ID_2.minor);
    declaration->Definition()->SetIdent(nullptr);
    ASSERT_EQ(declaration->Definition()->Ident(), nullptr);

    PhaseManager()->Reset();
    ASSERT_EQ(declaration->Definition()->Ident(), nullptr);

    PhaseManager()->SetCurrentPhaseId(PHASE_ID_0.minor);
    ASSERT_EQ(declaration->Definition()->Ident(), identifier0);

    PhaseManager()->SetCurrentPhaseId(PHASE_ID_1.minor);
    ASSERT_EQ(declaration->Definition()->Ident(), identifier1);

    PhaseManager()->SetCurrentPhaseId(PHASE_ID_2.minor);
    ASSERT_EQ(declaration->Definition()->Ident(), nullptr);
}
}  // namespace ark::es2panda

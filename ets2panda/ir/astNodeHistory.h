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

#ifndef ES2PANDA_IR_AST_NODE_HISTORY_H
#define ES2PANDA_IR_AST_NODE_HISTORY_H

#include <mutex>

#include "ir/astNode.h"
#include "compiler/lowering/phase_id.h"
#include "util/doubleLinkedList.h"

namespace ark::es2panda::ir {

class AstNodeHistory {
public:
    AstNodeHistory(AstNode *node, compiler::PhaseId phaseId, ArenaAllocator *allocator);

    AstNode *At(compiler::PhaseId phaseId);
    AstNode *Get(compiler::PhaseId phaseId);
    void Set(AstNode *node, compiler::PhaseId phaseId);
    compiler::PhaseId FirstCreated()
    {
        return list_.Head()->data.phaseId;
    }

private:
    struct HistoryRecord {
        AstNode *node;
        compiler::PhaseId phaseId;
    };

    using HistoryList = util::ArenaDoubleLinkedList<HistoryRecord>;

    AstNode *FindBackwardEquals(compiler::PhaseId phaseId);
    AstNode *FindForwardEquals(compiler::PhaseId phaseId);
    HistoryList::Item *FindLessOrEquals(compiler::PhaseId phaseId);

    HistoryList list_;                   // Node history list
    HistoryList::Item *item_ {nullptr};  // Last accessed history record
    std::mutex itemMutex_ {};
};
}  // namespace ark::es2panda::ir
#endif

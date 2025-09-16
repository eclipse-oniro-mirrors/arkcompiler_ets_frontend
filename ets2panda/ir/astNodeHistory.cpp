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

#include "astNodeHistory.h"
#include "compiler/lowering/phase.h"

namespace ark::es2panda::ir {

AstNodeHistory::AstNodeHistory(AstNode *node, compiler::PhaseId phaseId, ArenaAllocator *allocator) : list_ {allocator}
{
    Set(node, phaseId);
}

AstNode *AstNodeHistory::FindBackwardEquals(compiler::PhaseId phaseId)
{
    auto item = item_;

    while ((item = item->prev) != nullptr) {
        if (LIKELY(item->data.phaseId == phaseId)) {
            item_ = item;
            return item->data.node;
        }
    }

    return nullptr;
}

AstNode *AstNodeHistory::FindForwardEquals(compiler::PhaseId phaseId)
{
    auto item = item_;

    do {
        if (LIKELY(item->data.phaseId == phaseId)) {
            item_ = item;
            return item->data.node;
        }
    } while ((item = item->next) != nullptr);

    return nullptr;
}

// Find node state precisely at phase with a given ID
// (e.g. find the node history record with `phaseId` equal to a given value)
AstNode *AstNodeHistory::At(compiler::PhaseId phaseId)
{
    std::lock_guard<std::mutex> lock(itemMutex_);
    if (LIKELY(item_->data.phaseId == phaseId)) {
        // Start searching with last accessed item
        // In most cases last accessed item is the one we are looking for
        return item_->data.node;
    }
    if (LIKELY(item_->data.phaseId > phaseId)) {
        // Search backwards starting at last accessed node for the node at previous phase
        return FindBackwardEquals(phaseId);
    }
    // Search forward starting at last accessed node for the node at next phase
    return FindForwardEquals(phaseId);
}

// Find node state at phase with a given ID
// (e.g. find last node history record with `phaseId` less or equal to a given value)
AstNode *AstNodeHistory::Get(compiler::PhaseId phaseId)
{
    std::lock_guard<std::mutex> lock(itemMutex_);
    auto found = FindLessOrEquals(phaseId);
    if (LIKELY(found != nullptr)) {
        item_ = found;
        return item_->data.node;
    }

    return nullptr;
}

// Find node state at phase with a given ID and set its new value, insert new history record if not found
void AstNodeHistory::Set(AstNode *node, compiler::PhaseId phaseId)
{
    std::lock_guard<std::mutex> lock(itemMutex_);
    HistoryRecord record {node, phaseId};
    if (LIKELY(list_.Empty() || list_.Tail()->data.phaseId < phaseId)) {
        item_ = list_.Append(record);
    } else if (list_.Head()->data.phaseId > phaseId) {
        item_ = list_.Prepend(record);
    } else if (auto found = FindLessOrEquals(phaseId); found != nullptr) {
        if (found->data.phaseId == phaseId) {
            item_ = found;
            item_->data.node = node;
        } else {
            item_ = list_.Insert(found, record);
            ES2PANDA_ASSERT(item_->prev->data.phaseId < item_->data.phaseId &&
                            item_->data.phaseId < item_->next->data.phaseId);
        }
    } else {
        ES2PANDA_UNREACHABLE();
    }
}

// Find node state at phase with a given ID
// (e.g. find last node history record with `phaseId` less or equal to a given value)
AstNodeHistory::HistoryList::Item *AstNodeHistory::FindLessOrEquals(compiler::PhaseId phaseId)
{
    // Start searching with last accessed item
    auto item = item_;

    if (LIKELY(item->data.phaseId == phaseId)) {
        // In most cases last accessed item is the one we are looking for
        return item;
    }
    if (LIKELY(item->data.phaseId > phaseId)) {
        // Search backwards starting at last accessed node for the node at previous phase
        while ((item = item->prev) != nullptr) {
            if (item->data.phaseId <= phaseId) {
                return item;
            }
        }
    } else {
        // Search forward starting at last accessed node for the node at next phase
        while (item->next != nullptr) {
            if (item->data.phaseId == phaseId) {
                return item;
            }
            if (item->data.phaseId > phaseId && item->prev != nullptr) {
                item = item->prev;
                return item;
            }
            item = item->next;
        }
        if (item->data.phaseId <= phaseId) {
            return item;
        }
        if (item->data.phaseId > phaseId && item->prev != nullptr) {
            item = item->prev;
            return item;
        }
    }

    return nullptr;
}
}  // namespace ark::es2panda::ir

/**
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at*
 *
 * http://www.apache.org/licenses/LICENSE-2.0*
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MOVE_TO_NEW_FILE_H
#define MOVE_TO_NEW_FILE_H

#include <vector>
#include "refactor_types.h"
#include "../services/text_change/change_tracker.h"

namespace ark::es2panda::lsp {

constexpr RefactorActionView TO_MOVE_TO_NEW_FILE_ACTION {"Move to a new file", "Move to a new file",
                                                         "refactor.move.newFile"};
class MoveToNewFileRefactor : public Refactor {
private:
    mutable std::vector<ark::es2panda::ir::AstNode *> statementsToMove_;
    mutable std::vector<ark::es2panda::ir::AstNode *> importStatementsOfOldFile_;

    void DoChange(es2panda_Context *context, ChangeTracker &tracker, const SourceFile *oldFile) const;
    void GetStatementsToMove(const RefactorContext &refContext) const;
    bool NodeIsMissing(ir::AstNode *node) const;
    std::string GetSourceTextOfNodeFromSourceFile(es2panda_Context *context, util::StringView sourceCode,
                                                  ir::AstNode *node) const;
    void FillTempFileAndDeleteNodes(es2panda_Context *context, ChangeTracker &tracker, const std::string &tempNewFile,
                                    const SourceFile *oldFile) const;

public:
    MoveToNewFileRefactor();
    std::vector<ApplicableRefactorInfo> GetAvailableActions(const RefactorContext &refContext) const override;
    std::unique_ptr<RefactorEditInfo> GetEditsForAction(const RefactorContext &refContext,
                                                        const std::string &actionName) const override;
};
bool GetIsNodeHasExport(es2panda_Context *context, ir::AstNode *node);

}  // namespace ark::es2panda::lsp

#endif
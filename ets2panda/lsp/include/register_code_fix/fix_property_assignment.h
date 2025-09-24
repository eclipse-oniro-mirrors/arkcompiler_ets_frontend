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

#ifndef FIX_PROPERTY_ASSIGNMENT_H
#define FIX_PROPERTY_ASSIGNMENT_H

#include <vector>
#include <cstddef>
#include "lsp/include/types.h"
#include "lsp/include/code_fixes/code_fix_types.h"
#include "lsp/include/services/text_change/change_tracker.h"

namespace ark::es2panda::lsp {
class FixPropertyAssignment : public CodeFixRegistration {
public:
    FixPropertyAssignment();
    std::vector<CodeFixAction> GetCodeActions(const CodeFixContext &context) override;
    CombinedCodeActions GetAllCodeActions(const CodeFixAllContext &codeFixAll) override;
    static std::vector<FileTextChanges> GetCodeActionsToFix(const CodeFixContext &context);

private:
    static void MakeChange(ChangeTracker &changeTracker, es2panda_Context *context, size_t pos,
                           std::vector<ark::es2panda::ir::AstNode *> &fixedNodes);
    static ir::Property *GetShorthandProperty(const ir::AstNode *node);
    static ir::Property *GetInvalidEqualsProperty(const ir::AstNode *node);
};
}  // namespace ark::es2panda::lsp
#endif  // FIX_PROPERTY_ASSIGNMENT_H
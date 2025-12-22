/**
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef CONSTRUCTOR_FOR_DERIVED_NEED_SUPER_CALL_H
#define CONSTRUCTOR_FOR_DERIVED_NEED_SUPER_CALL_H

#include "lsp/include/code_fixes/code_fix_types.h"
#include "lsp/include/services/text_change/change_tracker.h"
#include "lsp/include/types.h"
#include <cstddef>
#include <vector>

namespace ark::es2panda::lsp {
class ConstructorDerivedNeedSuper : public CodeFixRegistration {
public:
    ConstructorDerivedNeedSuper();
    std::vector<CodeFixAction> GetCodeActions(const CodeFixContext &context) override;
    CombinedCodeActions GetAllCodeActions(const CodeFixAllContext &codeFixAll) override;
    static std::vector<FileTextChanges> GetCodeActionsToFix(const CodeFixContext &context);

private:
    static void MakeChange(ChangeTracker &changeTracker, es2panda_Context *context, size_t pos,
                           std::vector<ark::es2panda::ir::AstNode *> &fixedNodes);
    static bool IsValidTarget(const ir::AstNode *node);
    static const ir::AstNode *FindEnclosingClassNode(const ir::AstNode *start);
    static ir::ClassDefinition *ExtractClassDefinition(const ir::AstNode *classNode);
    static ir::MethodDefinition *GetConstructorMethodFromDefinition(ir::ClassDefinition *definition);
    static bool NeedsSuperCall(ir::ScriptFunction *scriptFunc);
    static ir::Statement *CreateSuperStatement(es2panda_Context *context);
};
}  // namespace ark::es2panda::lsp

#endif  // CONSTRUCTOR_FOR_DERIVED_NEED_SUPER_CALL_H
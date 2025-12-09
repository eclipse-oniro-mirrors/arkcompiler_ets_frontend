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

#ifndef FIX_CLASS_DOESNT_IMPLEMENT_INHERITED_ABSTRACT_MEMBER_H
#define FIX_CLASS_DOESNT_IMPLEMENT_INHERITED_ABSTRACT_MEMBER_H

#include "lsp/include/code_fixes/code_fix_types.h"
#include "public/es2panda_lib.h"
#include "lsp/include/services/text_change/change_tracker.h"

namespace ark::es2panda::lsp {

class FixClassNotImplementingInheritedMembers : public CodeFixRegistration {
public:
    FixClassNotImplementingInheritedMembers();

    std::vector<CodeFixAction> GetCodeActions(const CodeFixContext &context) override;
    CombinedCodeActions GetAllCodeActions(const CodeFixAllContext &codeFixAll) override;

private:
    void MakeTextChangeForNotImplementedMembers(ChangeTracker &changeTracker, es2panda_Context *context, size_t pos);
    std::string MakeNewText(ir::AstNode *node);
    std::string MakeMethodSignature(ir::AstNode *node);
    ir::AstNode *GetSuperClassDefinition(ir::AstNode *node);
    std::vector<FileTextChanges> GetCodeActionsForAbstractMissingMembers(const CodeFixContext &context);
};

}  // namespace ark::es2panda::lsp

#endif  // FIX_CLASS_DOESNT_IMPLEMENT_INHERITED_ABSTRACT_MEMBER_H
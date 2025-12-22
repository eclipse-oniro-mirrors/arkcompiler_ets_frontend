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

#ifndef FIX_CLASS_INCORRECTLY_IMPLEMENTS_INTERFACE_H
#define FIX_CLASS_INCORRECTLY_IMPLEMENTS_INTERFACE_H

#include "lsp/include/code_fixes/code_fix_types.h"
#include "lsp/include/services/text_change/change_tracker.h"
#include "public/es2panda_lib.h"

namespace ark::es2panda::lsp {

class FixClassIncorrectlyImplementsInterface : public CodeFixRegistration {
public:
    FixClassIncorrectlyImplementsInterface();

    std::vector<CodeFixAction> GetCodeActions(const CodeFixContext &context) override;

    CombinedCodeActions GetAllCodeActions(const CodeFixAllContext &codeFixAll) override;

private:
    void MakeChangeForMissingInterfaceMembers(ChangeTracker &changeTracker, es2panda_Context *context, size_t pos);
    std::vector<FileTextChanges> GetCodeActionsToImplementMissingMembers(const CodeFixContext &context);

    std::string MakeNewTextForMember(ir::AstNode *node);
    std::vector<ir::AstNode *> FindMissingInterfaceMembers(ir::ClassDefinition *classDef);
    std::vector<ir::AstNode *> GetInterfaceDefinitions(ir::ClassDefinition *classDef);
    bool IsMemberImplemented(ir::ClassDefinition *classDef, const std::string &memberSignature);

    ir::AstNode *FindInterfaceDefinition(ir::TSClassImplements *implement);
    void ProcessInterfaceMembers(ir::TSInterfaceDeclaration *interface, ir::ClassDefinition *classDef,
                                 std::vector<ir::AstNode *> &missingMembers);
    std::string GenerateMemberSignature(ir::MethodDefinition *methodDef, const std::string &memberName);
    void GroupMissingMembers(const std::vector<ir::AstNode *> &missingMembers,
                             std::vector<ir::AstNode *> &missingGetters, std::vector<ir::AstNode *> &missingSetters);
    void CreateCodeActionForType(const std::vector<ir::AstNode *> &members, const CodeFixContext &context,
                                 bool isGetter, std::vector<CodeFixAction> &returnedActions);
};

}  // namespace ark::es2panda::lsp

#endif  // FIX_CLASS_INCORRECTLY_IMPLEMENTS_INTERFACE_H
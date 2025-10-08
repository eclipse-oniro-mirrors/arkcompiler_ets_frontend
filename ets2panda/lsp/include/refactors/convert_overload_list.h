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

#ifndef CONVERT_OVERLOAD_LIST_H
#define CONVERT_OVERLOAD_LIST_H

#include "refactor_types.h"
#include "ir/astNode.h"
#include "checker/types/signature.h"
#include "varbinder/declaration.h"
#include <vector>

namespace ark::es2panda::lsp {

struct OverloadsInfo {
private:
    const ir::ScriptFunction *scriptFunction_ {nullptr};
    const ir::AstNode *body_ {nullptr};
    bool hasBody_ {false};

public:
    const ir::ScriptFunction *GetScriptFunction() const
    {
        return scriptFunction_;
    }
    const ir::AstNode *GetBody() const
    {
        return body_;
    }
    bool HasBody() const
    {
        return hasBody_;
    }

    void SetScriptFunction(const ir::ScriptFunction *func)
    {
        scriptFunction_ = func;
    }
    void SetBody(const ir::AstNode *bodyNode)
    {
        body_ = bodyNode;
    }
    void SetHasBody(bool hasBody)
    {
        hasBody_ = hasBody;
    }

    bool IsPositionInBody(size_t pos) const
    {
        if (body_ == nullptr) {
            return false;
        }
        auto bodyStart = body_->Start().index;
        auto bodyEnd = body_->End().index;
        return pos >= bodyStart && pos <= bodyEnd;
    }
};

constexpr RefactorActionView CONVERT_OVERLOAD_LIST_ACTION {refactor_name::CONVERT_OVERLOAD_LIST_REFACTOR_NAME,
                                                           refactor_description::CONVERT_OVERLOAD_LIST_REFACTOR_DESC,
                                                           "refactor.rewrite.function.overloadList"};

struct OverloadGroupInfo {
private:
    std::vector<ir::AstNode *> declarations_ {};
    ir::AstNode *implementationNode_ {nullptr};

public:
    const std::vector<ir::AstNode *> &GetDeclarations() const
    {
        return declarations_;
    }
    std::vector<ir::AstNode *> &GetDeclarations()
    {
        return declarations_;
    }
    ir::AstNode *GetImplementationNode() const
    {
        return implementationNode_;
    }

    void SetDeclarations(std::vector<ir::AstNode *> decls)
    {
        declarations_ = std::move(decls);
    }
    void SetImplementationNode(ir::AstNode *node)
    {
        implementationNode_ = node;
    }
};

class ConvertOverloadListRefactor : public Refactor {
public:
    ConvertOverloadListRefactor();
    ApplicableRefactorInfo GetAvailableActions(const RefactorContext &context) const override;
    std::unique_ptr<RefactorEditInfo> GetEditsForAction(const RefactorContext &context,
                                                        const std::string &actionName) const override;
};

OverloadGroupInfo GetOverloadGroupAtPosition(const RefactorContext &context);
bool ValidateOverloadGroup(const OverloadGroupInfo &group);
std::vector<const checker::Signature *> ExtractSignatures(const OverloadGroupInfo &group);
std::string GenerateSignatureParametersToTuple(const checker::Signature *signature);
std::string GenerateUnionParameterType(const std::vector<const checker::Signature *> &signatures);
std::string GenerateConvertedOverloadSignature(const OverloadGroupInfo &group,
                                               const std::vector<const checker::Signature *> &signatures);
std::vector<FileTextChanges> GetEditInfoForConvertOverloadList(const RefactorContext &context,
                                                               const OverloadGroupInfo &group);

OverloadsInfo SetOverloadsInfo(const ir::AstNode *node);

}  // namespace ark::es2panda::lsp

#endif  // CONVERT_OVERLOAD_LIST_H

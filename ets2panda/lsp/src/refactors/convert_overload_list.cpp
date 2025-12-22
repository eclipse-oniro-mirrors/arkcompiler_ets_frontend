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

#include "refactors/convert_overload_list.h"
#include "refactor_provider.h"
#include "internal_api.h"
#include "services/text_change/change_tracker.h"
#include "ir/statements/functionDeclaration.h"
#include "ir/base/classDefinition.h"
#include "checker/ETSchecker.h"
#include "checker/types/signature.h"
#include "public/public.h"
#include <string>

namespace ark::es2panda::lsp {

OverloadsInfo SetOverloadsInfo(const ir::AstNode *node)
{
    OverloadsInfo info {};

    if (node->IsMethodDefinition()) {
        auto *methodDef = node->AsMethodDefinition();
        info.SetScriptFunction(methodDef->Function());
        info.SetBody(methodDef->Function()->Body());
        info.SetHasBody(info.GetBody() != nullptr);
    }

    return info;
}

bool IsInFunctionBody(const ir::AstNode *node, const RefactorContext &context)
{
    auto nodeInfo = SetOverloadsInfo(node);
    return nodeInfo.IsPositionInBody(context.span.pos);
}

std::vector<ir::AstNode *> FindOverloadDeclarations(const ir::AstNode *containingDecl)
{
    std::vector<ir::AstNode *> declarations;

    if (!containingDecl->IsMethodDefinition()) {
        return declarations;
    }

    auto *methodDef = const_cast<ir::MethodDefinition *>(containingDecl->AsMethodDefinition());

    declarations.push_back(methodDef);

    const auto &overloads = methodDef->Overloads();
    for (auto *overload : overloads) {
        declarations.push_back(overload);
    }

    return declarations;
}

bool ValidateSignatureDeclarations(const std::vector<ir::AstNode *> &declarations)
{
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t kMinimumOverloadCount = 2;
    if (declarations.size() < kMinimumOverloadCount) {
        return false;
    }

    for (auto *decl : declarations) {
        auto nodeInfo = SetOverloadsInfo(decl);
        if (nodeInfo.GetScriptFunction() != nullptr && nodeInfo.GetScriptFunction()->TypeParams() != nullptr) {
            return false;
        }
    }
    return true;
}

const checker::Signature *GetSignatureFromNode(const ir::AstNode *node)
{
    auto nodeInfo = SetOverloadsInfo(node);
    return nodeInfo.GetScriptFunction() != nullptr ? nodeInfo.GetScriptFunction()->Signature() : nullptr;
}

std::string GenerateSignatureParametersToTuple(const checker::Signature *signature)
{
    if (signature == nullptr) {
        return "";
    }

    const checker::SignatureInfo *signatureInfo = signature->GetSignatureInfo();
    std::string result = "[";

    for (auto it = signatureInfo->params.begin(); it != signatureInfo->params.end(); ++it) {
        auto *param = *it;
        std::string paramName = std::string(param->Name().Utf8());
        result += paramName;

        if (param->HasFlag(varbinder::VariableFlags::OPTIONAL)) {
            result += "?";
        }
        result += ": ";
        if (param->TsType() != nullptr) {
            result += param->TsType()->ToString();
        } else {
            result += "any";
        }

        if (std::next(it) != signatureInfo->params.end()) {
            result += ", ";
        }
    }

    if (signatureInfo->restVar != nullptr) {
        if (!signatureInfo->params.empty()) {
            result += ", ";
        }
        result += "...";

        std::string paramName = std::string(signatureInfo->restVar->Name().Utf8());
        result += paramName;

        result += ": ";

        std::string paramType = signatureInfo->restVar->TsType()->ToString();
        result += paramType;
    }

    result += "]";
    return result;
}

std::string GenerateUnionParameterType(const std::vector<const checker::Signature *> &signatures)
{
    if (signatures.empty()) {
        return "";
    }

    std::string result = "(...args: ";
    std::vector<std::string> tupleTypes;
    tupleTypes.reserve(signatures.size());

    for (const auto *sig : signatures) {
        tupleTypes.push_back(GenerateSignatureParametersToTuple(sig));
    }

    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t kFirstIndex = 0;
    for (size_t i = kFirstIndex; i < tupleTypes.size(); ++i) {
        if (i > kFirstIndex) {
            result += " | ";
        }
        result += tupleTypes[i];
    }
    result += ")";

    return result;
}

ConvertOverloadListRefactor::ConvertOverloadListRefactor()
{
    AddKind(std::string(CONVERT_OVERLOAD_LIST_ACTION.kind));
}

std::vector<ApplicableRefactorInfo> ConvertOverloadListRefactor::GetAvailableActions(
    const RefactorContext &context) const
{
    ApplicableRefactorInfo applicableRef;
    std::vector<ApplicableRefactorInfo> res;

    if (!context.kind.empty() && !IsKind(context.kind)) {
        return res;
    }

    auto group = GetOverloadGroupAtPosition(context);
    if (group.GetDeclarations().empty() || !ValidateOverloadGroup(group)) {
        return res;
    }

    applicableRef.name = refactor_name::CONVERT_OVERLOAD_LIST_REFACTOR_NAME;
    applicableRef.description = std::string(CONVERT_OVERLOAD_LIST_ACTION.description);
    applicableRef.action.name = std::string(CONVERT_OVERLOAD_LIST_ACTION.name);
    applicableRef.action.description = std::string(CONVERT_OVERLOAD_LIST_ACTION.description);
    applicableRef.action.kind = std::string(CONVERT_OVERLOAD_LIST_ACTION.kind);
    res.push_back(applicableRef);
    return res;
}

std::unique_ptr<RefactorEditInfo> ConvertOverloadListRefactor::GetEditsForAction(const RefactorContext &context,
                                                                                 const std::string &actionName) const
{
    if (!actionName.empty() && actionName != CONVERT_OVERLOAD_LIST_ACTION.name) {
        return nullptr;
    }

    auto group = GetOverloadGroupAtPosition(context);
    if (group.GetDeclarations().empty()) {
        return nullptr;
    }

    auto edits = GetEditInfoForConvertOverloadList(context, group);
    if (edits.empty()) {
        return nullptr;
    }

    return std::make_unique<RefactorEditInfo>(std::move(edits));
}

OverloadGroupInfo GetOverloadGroupAtPosition(const RefactorContext &context)
{
    OverloadGroupInfo info;

    auto *node = GetTouchingToken(context.context, context.span.pos, false);
    if (node == nullptr) {
        return info;
    }

    ir::NodePredicate cb = [](ir::AstNode *ancestorNode) { return ancestorNode->IsMethodDefinition(); };
    auto *containingDecl = FindAncestor(node, cb);

    if (containingDecl == nullptr) {
        return info;
    }

    if (IsInFunctionBody(containingDecl, context)) {
        return info;
    }

    auto declarations = FindOverloadDeclarations(containingDecl);
    if (!ValidateSignatureDeclarations(declarations)) {
        return info;
    }

    ir::AstNode *implementationNode = nullptr;
    for (auto *decl : declarations) {
        auto nodeInfo = SetOverloadsInfo(decl);
        if (nodeInfo.HasBody()) {
            implementationNode = decl;
            break;
        }
    }

    info.SetDeclarations(std::move(declarations));
    info.SetImplementationNode(implementationNode);

    return info;
}

bool ValidateOverloadGroup(const OverloadGroupInfo &group)
{
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t kMinimumOverloadCount = 2;
    if (group.GetDeclarations().size() < kMinimumOverloadCount) {
        return false;
    }

    std::vector<const checker::Signature *> signatures = ExtractSignatures(group);
    return !signatures.empty();
}

std::vector<const checker::Signature *> ExtractSignatures(const OverloadGroupInfo &group)
{
    std::vector<const checker::Signature *> signatures;

    for (auto *decl : group.GetDeclarations()) {
        const auto *signature = GetSignatureFromNode(decl);
        if (signature != nullptr) {
            signatures.push_back(signature);
        }
    }

    return signatures;
}

std::string GenerateConvertedOverloadSignature(const OverloadGroupInfo &group,
                                               const std::vector<const checker::Signature *> &signatures)
{
    if (signatures.empty() || group.GetDeclarations().empty()) {
        return "";
    }

    ir::AstNode *implementationDecl = nullptr;
    for (auto *decl : group.GetDeclarations()) {
        auto nodeInfo = SetOverloadsInfo(decl);
        if (nodeInfo.HasBody() && nodeInfo.GetBody() != nullptr) {
            implementationDecl = decl;
            break;
        }
    }

    auto *baseDecl = implementationDecl != nullptr ? implementationDecl : group.GetDeclarations().back();
    if (!baseDecl->IsMethodDefinition()) {
        return "";
    }

    std::string fullSignature = baseDecl->DumpEtsSrc();
    std::string newParams = GenerateUnionParameterType(signatures);

    size_t openParen = fullSignature.find('(');
    if (openParen == std::string::npos) {
        return "";
    }

    size_t closeParen = fullSignature.find(')', openParen);
    if (closeParen == std::string::npos) {
        return "";
    }

    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t kStartPosition = 0;
    // NOLINTNEXTLINE(readability-identifier-naming)
    constexpr size_t kCharAfterCloseParen = 1;
    std::string result = fullSignature.substr(kStartPosition, openParen);
    result += newParams;
    result += fullSignature.substr(closeParen + kCharAfterCloseParen);

    return result;
}

std::vector<FileTextChanges> GetEditInfoForConvertOverloadList(const RefactorContext &context,
                                                               const OverloadGroupInfo &group)
{
    if (group.GetDeclarations().empty()) {
        return {};
    }

    std::vector<const checker::Signature *> signatures = ExtractSignatures(group);
    if (signatures.empty()) {
        return {};
    }

    std::string newSignature = GenerateConvertedOverloadSignature(group, signatures);
    if (newSignature.empty()) {
        return {};
    }

    return ChangeTracker::With(*context.textChangesContext, [&](ChangeTracker &tracker) {
        constexpr size_t K_FIRST_DECLARATION_INDEX = 0;
        size_t minStart = group.GetDeclarations()[K_FIRST_DECLARATION_INDEX]->Start().index;
        size_t maxEnd = group.GetDeclarations()[K_FIRST_DECLARATION_INDEX]->End().index;

        for (auto *decl : group.GetDeclarations()) {
            if (decl->Start().index < minStart) {
                minStart = decl->Start().index;
            }
            if (decl->End().index > maxEnd) {
                maxEnd = decl->End().index;
            }
        }

        auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
        const auto sourceFile = ctx->sourceFile;
        TextRange range = {minStart, maxEnd};
        tracker.ReplaceRangeWithText(sourceFile, range, newSignature);
    });
}

// NOLINTNEXTLINE(fuchsia-statically-constructed-objects, cert-err58-cpp)
AutoRefactorRegister<ConvertOverloadListRefactor> g_convertOverloadListRefactorRegister("ConvertOverloadListRefactor");

}  // namespace ark::es2panda::lsp
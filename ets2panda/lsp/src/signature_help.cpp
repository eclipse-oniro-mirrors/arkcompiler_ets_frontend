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

#include "signature_help.h"
#include "internal_api.h"
#include "utils/arena_containers.h"
#include <cstddef>
#include <iostream>
#include <optional>
#include <string>
#include <variant>
#include <vector>
#include "create_type_help_items.h"

namespace ark::es2panda::lsp {

ir::AstNode *FindTokenOnLeftOfPosition(es2panda_Context *context, size_t position)
{
    auto const tokenAtPosition = GetTouchingToken(context, position, false);
    if (tokenAtPosition->Start().index < position && tokenAtPosition->End().index > position) {
        return tokenAtPosition;
    }
    const auto ctx = reinterpret_cast<public_lib::Context *>(context);
    return FindPrecedingToken(position, ctx->parserProgram->Ast(), ctx->allocator);
}

TextSpan CreateTextSpanForNode(const ir::AstNode *node)
{
    TextSpan span {0, 0};
    span.start = node->Start().index;
    span.length = node->End().index - node->Start().index;
    return span;
}
bool IsSyntacticOwner(const ir::AstNode *node)
{
    return node->IsCallExpression() || node->IsNewExpression();
}
checker::Signature *GetResolvedSignatureForSignatureHelp(const ir::AstNode *call, const ir::AstNode *parent,
                                                         std::vector<checker::Signature *> &candidates)
{
    parent->FindChild([&call, &candidates](ir::AstNode *n) {
        switch (n->Type()) {
            case ir::AstNodeType::METHOD_DEFINITION:
                if (call->AsCallExpression()->Callee()->ToString() == n->AsMethodDefinition()->Id()->ToString()) {
                    candidates.push_back(n->AsMethodDefinition()->Function()->Signature());
                }
                break;

            case ir::AstNodeType::CALL_EXPRESSION:
                if (call->AsCallExpression()->Callee()->ToString() == n->AsCallExpression()->Callee()->ToString()) {
                    candidates.push_back(n->AsCallExpression()->Signature());
                }
                break;

            default:
                break;
        }
        return false;
    });
    if (call->IsCallExpression()) {
        auto callExpr = call->AsCallExpression();
        return callExpr->Signature();
    }
    return nullptr;
}

std::optional<InfoType> GetCandidateOrTypeInfo(const std::optional<ArgumentListInfo> info, ir::AstNode *parent,
                                               const bool onlyUseSyntacticOwners)
{
    if (const auto *call = std::get_if<CallInvocation>(&info->GetInvocation());
        call != nullptr && call->callExpressionNode != nullptr) {
        if (onlyUseSyntacticOwners && !IsSyntacticOwner(call->callExpressionNode)) {
            return std::nullopt;
        }
        std::vector<checker::Signature *> candidates;
        checker::Signature *resolvedSignature = nullptr;
        if (call->callExpressionNode != nullptr && call->callExpressionNode->IsCallExpression()) {
            resolvedSignature = GetResolvedSignatureForSignatureHelp(call->callExpressionNode, parent, candidates);
        } else {
            resolvedSignature =
                GetResolvedSignatureForSignatureHelp(call->callExpressionNode->Parent(), parent, candidates);
        }
        if (!candidates.empty()) {
            const auto can = CandidateInfo {CandidateOrTypeKind::CANDIDATE, candidates, resolvedSignature};
            return std::make_optional(can);
        }
    } else if (const auto *typeArgs = std::get_if<TypeArgsInvocation>(&info->GetInvocation())) {
        auto called = typeArgs->identifierNode;
        const auto tp = CandidateOrTypeKind::TYPEENUM;
        TypeInfo val = TypeInfo {tp, called};
        return std::make_optional(val);
    } else if (const auto *context = std::get_if<ContextualInvocation>(&info->GetInvocation()); context != nullptr) {
        auto node = context->node;
        std::vector<checker::Signature *> candidates;
        if (node != nullptr && node->IsMethodDefinition()) {
            auto funcSignature = node->AsMethodDefinition()->Function()->Signature();
            candidates.push_back(funcSignature);
            const auto can = CandidateInfo {CandidateOrTypeKind::CANDIDATE, candidates, funcSignature};
            return std::make_optional(can);
        }
    }
    return std::nullopt;
}

std::string IsReasonCharacterTyped(const SignatureHelpTriggerReason &triggerReason)
{
    return std::visit(
        [](const auto &reason) {
            using T = std::decay_t<decltype(reason)>;
            if constexpr (std::is_same_v<T, SignatureHelpCharacterTypedReason>) {
                return reason.GetKind();
            }
            return "";
        },
        triggerReason);
}

std::string IsManuallyInvoked(const SignatureHelpTriggerReason &triggerReason)
{
    return std::visit(
        [](const auto &reason) {
            using T = std::decay_t<decltype(reason)>;
            if constexpr (std::is_same_v<T, SignatureHelpInvokedReason>) {
                return reason.GetKind();
            }
            return "";
        },
        triggerReason);
}

SignatureHelpItems GetSignatureHelpItems(es2panda_Context *ctx, size_t position,
                                         SignatureHelpTriggerReason triggeredReason,
                                         CancellationToken cancellationToken)
{
    auto const startingToken = FindTokenOnLeftOfPosition(ctx, position);
    if (startingToken == nullptr) {
        return {};
    }

    auto context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto astNode = reinterpret_cast<ark::es2panda::ir::AstNode *>(context->parserProgram->Ast());

    const auto onlyUseSyntacticOwners = IsReasonCharacterTyped(triggeredReason) == "characterTyped";
    if (onlyUseSyntacticOwners) {
        return {};
    }
    const auto isManuallyInvoked = IsManuallyInvoked(triggeredReason) == "invoked";
    const auto argumentInfo = GetContainingArgumentInfo(startingToken, position, isManuallyInvoked);
    if (argumentInfo == std::nullopt) {
        return {};
    }
    if (cancellationToken.IsCancellationRequested()) {
        return {};
    }
    const auto candidateInfoOpt = GetCandidateOrTypeInfo(argumentInfo, astNode, onlyUseSyntacticOwners);
    if (candidateInfoOpt == std::nullopt) {
        return {};
    }

    const auto &candidateInfo = *candidateInfoOpt;

    auto res = SignatureHelpItems();
    if (std::holds_alternative<TypeInfo>(candidateInfo)) {
        const auto &typeInfo = std::get<TypeInfo>(candidateInfo);
        res = CreateTypeHelpItems(typeInfo.GetSymbol(), typeInfo.GetSymbol()->Range(),
                                  CreateTextSpanForNode(typeInfo.GetSymbol()));
    } else if (std::holds_alternative<CandidateInfo>(candidateInfo)) {
        auto candidate = std::get<CandidateInfo>(candidateInfo);
        res = CreateSignatureHelpItems(candidate.GetSignatures(), candidate.GetResolvedSignature(), argumentInfo);
    }
    return res;
}
ir::AstNode *GetHighestBinary(ir::AstNode *node)
{
    return node->Parent()->IsBinaryExpression() ? GetHighestBinary(node->Parent()) : node;
}

size_t CountBinaryExpressionParameters(ir::AstNode *node)
{
    const size_t binaryReturnParam = 2;
    return node->AsBinaryExpression()->Left()->IsBinaryExpression()
               ? CountBinaryExpressionParameters(node->AsBinaryExpression()->Left()) + 1
               : binaryReturnParam;
}

std::optional<ArgumentListInfo> GetImmediatelyContainingArgumentOrContextualParameterInfo(ir::AstNode *node,
                                                                                          size_t position)
{
    return TryGetParameterInfo(node).has_value() ? TryGetParameterInfo(node)
                                                 : (GetImmediatelyContainingArgumentInfo(node, position));
}

std::optional<ArgumentListInfo> GetContainingArgumentInfo(ir::AstNode *node, size_t position, bool isManuallyInvoked)
{
    if (!isManuallyInvoked) {
        return std::nullopt;
    }

    return GetImmediatelyContainingArgumentOrContextualParameterInfo(node, position);
}

ir::AstNode *GetChildListThatStartsWithOpenerToken(ir::AstNode *parent, ir::AstNode *openerToken)
{
    std::vector<ir::AstNode *> children;
    parent->FindChild([&children](ir::AstNode *n) {
        children.push_back(n);
        return false;
    });

    auto const indexOfOpenerToken =
        std::distance(children.begin(), std::find(children.begin(), children.end(), openerToken));
    if (!(indexOfOpenerToken >= 0 && (int)children.size() > indexOfOpenerToken + 1)) {
        return nullptr;
    }
    return children.at(indexOfOpenerToken + 1);
}

size_t GetArgumentCount(ir::AstNode *node, bool ignoreTrailingComma)
{
    int argumentCount = 0;

    node->FindChild([&argumentCount, ignoreTrailingComma](ir::AstNode *child) {
        if (!ignoreTrailingComma && child->IsTSTypeParameter()) {
            argumentCount++;
        } else if (!ignoreTrailingComma && child->IsMemberExpression()) {
            argumentCount++;
        }
        return false;
    });
    return argumentCount;
}
std::vector<ArgumentListInfo> GetArgumentOrParameterListAndIndex(ir::AstNode *node, std::vector<ArgumentListInfo> &list)
{
    if (node->IsMethodDefinition()) {
        const auto params = node->AsMethodDefinition()->Function()->Params();
        for (const auto param : params) {
            auto argum = ArgumentListInfo();
            argum.SetInvocation(Invocation(CallInvocation {InvocationKind::CALL, param}));
            argum.SetApplicableSpan(CreateTextSpanForNode(param));
            argum.SetArgumentIndex(param->Start().index);
            argum.SetArgumentCount(params.size());
            list.push_back(argum);
        }
    }
    if (node->IsCallExpression()) {
        const auto params = node->AsCallExpression()->Arguments();
        for (const auto param : params) {
            auto argum = ArgumentListInfo();
            argum.SetInvocation(Invocation(CallInvocation {InvocationKind::CALL, param}));
            argum.SetApplicableSpan(CreateTextSpanForNode(param));
            argum.SetArgumentIndex(param->Start().index);
            argum.SetArgumentCount(params.size());
            list.push_back(argum);
        }
    }

    return list;
}
ContextualSignatureLocationInfo GetArgumentOrParameterListInfo(ir::AstNode *node)
{
    std::vector<ArgumentListInfo> info;
    info = GetArgumentOrParameterListAndIndex(node, info);
    if (info.empty()) {
        return ContextualSignatureLocationInfo {};
    }

    const auto argumentCount = GetArgumentCount(node, false);
    auto textSpan = CreateTextSpanForNode(node);
    return {info, node->Start().index, argumentCount, textSpan};
}

std::optional<ArgumentListInfo> TryGetParameterInfo(ir::AstNode *node)
{
    auto const info = GetContextualSignatureLocationInfo(node);
    if (!info) {
        return std::nullopt;
    }
    auto const index = info->GetArgumentIndex();
    auto const count = info->GetArgumentCount();
    auto const span = info->GetArgumentsSpan();

    std::optional<ArgumentListInfo> argumentList = ArgumentListInfo();
    if (node->IsCallExpression()) {
        const ContextualInvocation invocation =
            ContextualInvocation {InvocationKind::CONTEXTUAL, node->AsCallExpression()->Signature(), node};
        argumentList->SetInvocation(invocation);
    } else if (node->IsMethodDefinition()) {
        const ContextualInvocation invocation = ContextualInvocation {
            InvocationKind::CONTEXTUAL, node->AsMethodDefinition()->Function()->Signature(), node};
        argumentList->SetInvocation(invocation);
    }
    argumentList->SetApplicableSpan(span);
    argumentList->SetArgumentIndex(index);
    argumentList->SetArgumentCount(count);
    return argumentList;
}

size_t GetArgumentIndexForTemplatePiece(size_t spanIndex, ir::AstNode *node, size_t position)
{
    const size_t spanIndexOne = 1;
    const size_t spanIndexTwo = 2;
    if (node->Type() == ir::AstNodeType::TEMPLATE_LITERAL) {
        if (node->Start().index < position && position < node->End().index) {
            return 0;
        }
        return spanIndex + spanIndexTwo;
    }
    return spanIndex + spanIndexOne;
}

std::optional<ArgumentListInfo> GetImmediatelyContainingArgumentInfo(ir::AstNode *node, size_t position)
{
    if (position == 0) {
        return std::nullopt;
    }
    auto parent = node->Parent();
    if (parent->Type() == ir::AstNodeType::CALL_EXPRESSION || parent->Type() == ir::AstNodeType::NEW_EXPRESSION ||
        parent->Type() == ir::AstNodeType::MEMBER_EXPRESSION) {
        if (parent->IsMemberExpression() && parent->Parent() != nullptr && parent->Parent()->IsCallExpression()) {
            parent = parent->Parent();
        }
        auto const invocation = parent;

        auto const argument = GetArgumentOrParameterListInfo(parent);
        const auto &list = argument.GetList();
        if (!list.empty()) {
            auto const argumentIndex = argument.GetArgumentIndex();
            auto const argumentCount = GetArgumentCount(parent, false);
            auto const span = CreateTextSpanForNode(parent);
            ArgumentListInfo argumentList;
            argumentList.SetInvocation(Invocation(CallInvocation {InvocationKind::CALL, invocation}));
            argumentList.SetApplicableSpan(span);
            argumentList.SetArgumentIndex(argumentIndex);
            argumentList.SetArgumentCount(argumentCount);
            return argumentList;
        }
    } else if (parent->Type() == ir::AstNodeType::METHOD_DEFINITION) {
        auto const info = GetContextualSignatureLocationInfo(parent);
        if (!info) {
            return std::nullopt;
        }
        auto const index = info->GetArgumentIndex();
        auto const count = info->GetArgumentCount();
        auto const span = info->GetArgumentsSpan();
        std::optional<ArgumentListInfo> argumentList = ArgumentListInfo();
        const ContextualInvocation invocation = ContextualInvocation {
            InvocationKind::CONTEXTUAL, parent->AsMethodDefinition()->Function()->Signature(), parent};
        argumentList->SetInvocation(invocation);
        argumentList->SetApplicableSpan(span);
        argumentList->SetArgumentIndex(index);
        argumentList->SetArgumentCount(count);
        return argumentList;
    }
    return std::nullopt;
}

std::optional<ContextualSignatureLocationInfo> GetContextualSignatureLocationInfo(ir::AstNode *node)
{
    ContextualSignatureLocationInfo info;
    switch (node->Type()) {
        case ir::AstNodeType::METHOD_DEFINITION:
        case ir::AstNodeType::FUNCTION_EXPRESSION:
        case ir::AstNodeType::ARROW_FUNCTION_EXPRESSION:
            info = GetArgumentOrParameterListInfo(node);
            return std::make_optional(info);
            break;
        default:
            return std::nullopt;
            break;
    }
}
}  // namespace ark::es2panda::lsp
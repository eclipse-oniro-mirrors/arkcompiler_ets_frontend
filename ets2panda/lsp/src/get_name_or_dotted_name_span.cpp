/**
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

#include "get_name_or_dotted_name_span.h"
#include "get_adjusted_location.h"
#include <cstddef>

namespace ark::es2panda::lsp {
bool IsRightSideOfPropertyAccess(ir::AstNode *node)
{
    ir::AstNode *parent = node->Parent();

    if (parent == nullptr) {
        return false;
    }

    if (parent->Type() == ir::AstNodeType::MEMBER_EXPRESSION) {
        auto *memberExpr = static_cast<ir::MemberExpression *>(parent);
        return memberExpr->Property() == node;
    }

    if (parent->Type() == ir::AstNodeType::CALL_EXPRESSION) {
        auto *callExpr = static_cast<ir::CallExpression *>(parent);
        ir::AstNode *callee = callExpr->Callee();

        if (callee->Type() == ir::AstNodeType::MEMBER_EXPRESSION) {
            auto *memberExpr = static_cast<ir::MemberExpression *>(callee);
            return memberExpr->Property() == node;
        }
    }

    return false;
}

bool IsNameOfModuleDeclaration(ir::AstNode *node)
{
    ir::AstNode *parent = node->Parent();
    if (parent == nullptr || parent->Type() != ir::AstNodeType::TS_MODULE_DECLARATION) {
        return false;
    }

    auto *moduleDecl = static_cast<ir::TSModuleDeclaration *>(parent);
    return moduleDecl->Name() == node;
}

bool IsRightSideOfQualifiedName(ir::AstNode *node)
{
    ir::AstNode *parent = node->Parent();
    if (parent == nullptr || (parent->Type() != ir::AstNodeType::TS_QUALIFIED_NAME &&
                              parent->Type() != ir::AstNodeType::MEMBER_EXPRESSION)) {
        return false;
    }

    if (parent->Type() == ir::AstNodeType::TS_QUALIFIED_NAME) {
        auto *qualifiedName = static_cast<ir::TSQualifiedName *>(parent);
        return qualifiedName->Right() == node;
    }

    if (parent->Type() == ir::AstNodeType::MEMBER_EXPRESSION) {
        auto *memberExpr = static_cast<ir::MemberExpression *>(parent);
        return memberExpr->Property() == node;
    }

    return false;
}

ir::AstNode *AscendToRootName(ir::AstNode *node)
{
    while (node != nullptr) {
        if (IsRightSideOfPropertyAccess(node) || IsRightSideOfQualifiedName(node)) {
            node = node->Parent();
            continue;
        }

        if (!IsNameOfModuleDeclaration(node)) {
            break;
        }

        ir::AstNode *parentDecl = node->Parent();
        if (parentDecl == nullptr || parentDecl->Parent() == nullptr) {
            break;
        }

        if (parentDecl->Parent()->Type() != ir::AstNodeType::TS_MODULE_DECLARATION) {
            break;
        }

        auto *grandParent = static_cast<ir::TSModuleDeclaration *>(parentDecl->Parent());
        if (grandParent->Body() != parentDecl) {
            break;
        }

        node = const_cast<ir::AstNode *>(static_cast<const ir::AstNode *>(grandParent->Name()));
    }

    return node;
}

std::unique_ptr<TextSpan> GetNameOrDottedNameSpanImpl(es2panda_Context *context, int startPos)
{
    ir::AstNode *astNode = ark::es2panda::lsp::GetTouchingPropertyName(context, startPos);
    if (astNode == nullptr) {
        return nullptr;
    }

    switch (astNode->Type()) {
        case ir::AstNodeType::TS_QUALIFIED_NAME:
        case ir::AstNodeType::STRING_LITERAL:
        case ir::AstNodeType::TS_BOOLEAN_KEYWORD:
        case ir::AstNodeType::TS_NULL_KEYWORD:
        case ir::AstNodeType::SUPER_EXPRESSION:
        case ir::AstNodeType::THIS_EXPRESSION:
        case ir::AstNodeType::TS_THIS_TYPE:
        case ir::AstNodeType::IDENTIFIER:
            break;
        default:
            return nullptr;
    }

    auto nodeForStartPos = AscendToRootName(astNode);
    if (nodeForStartPos == nullptr) {
        return nullptr;
    }
    size_t start = nodeForStartPos->Start().index;
    size_t end = nodeForStartPos->End().index;
    if (start >= end) {
        return nullptr;
    }

    return std::make_unique<TextSpan>(start, end - start);
}
}  // namespace ark::es2panda::lsp
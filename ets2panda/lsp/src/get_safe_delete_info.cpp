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

#include <string>
#include <utility>
#include <vector>
#include "compiler/lowering/util.h"
#include "get_safe_delete_info.h"
#include "ir/astNode.h"
#include "internal_api.h"
#include "public/public.h"
#include "references.h"

namespace {
constexpr size_t BUILTIN_TYPE_COUNT = 9;
constexpr std::array<const char *, BUILTIN_TYPE_COUNT> BUILTIN_TYPES = {
    "Number", "String", "Boolean", "void", "BigInt", "Never", "undefined", "null", "Object"};
}  // namespace

namespace ark::es2panda::lsp {

bool IsBuiltinTypeReference(ir::AstNode *node)
{
    if (node == nullptr || node->Type() != ir::AstNodeType::IDENTIFIER) {
        return false;
    }
    auto *parent = node->Parent();
    while (parent != nullptr) {
        if (parent->Type() == ir::AstNodeType::ETS_TYPE_REFERENCE_PART ||
            parent->Type() == ir::AstNodeType::ETS_TYPE_REFERENCE) {
            std::string nameStr(node->AsIdentifier()->Name());
            auto it =
                std::find_if(BUILTIN_TYPES.begin(), BUILTIN_TYPES.end(), [&](const char *s) { return nameStr == s; });
            if (it != BUILTIN_TYPES.end()) {
                return true;
            }
        }
        parent = parent->Parent();
    }
    return false;
}

bool IsDeletableDecl(ir::AstNode *node)
{
    return node->IsFunctionDeclaration() || node->IsVariableDeclarator() || node->IsClassProperty() ||
           node->Type() == ir::AstNodeType::METHOD_DEFINITION || node->Type() == ir::AstNodeType::CLASS_DECLARATION ||
           node->IsTSTypeParameterDeclaration() || node->IsImportDefaultSpecifier() ||
           node->Type() == ir::AstNodeType::ETS_TYPE_REFERENCE_PART || node->IsCallExpression() ||
           node->IsETSImportDeclaration() || node->IsImportSpecifier() || node->IsBinaryExpression() ||
           node->IsTSInterfaceDeclaration() || node->IsETSTypeReferencePart() || node->IsImportNamespaceSpecifier() ||
           node->IsTSTypeAliasDeclaration() || node->IsExpressionStatement() || node->IsMemberExpression() ||
           node->IsTypeofExpression();
}

ir::AstNode *FindNearestDeletableDecl(ir::AstNode *node)
{
    ir::AstNode *cur = node;
    while (cur != nullptr) {
        if (IsDeletableDecl(cur)) {
            return cur;
        }
        cur = cur->Parent();
    }
    return nullptr;
}

std::string NormalizeFilePath(const std::string &filePath)
{
    std::string normPath = filePath;
    std::transform(normPath.begin(), normPath.end(), normPath.begin(), ::tolower);
    std::replace(normPath.begin(), normPath.end(), '\\', '/');
    return normPath;
}

/**
 * Distinguish a namespace from a class by checking whether the class body contains only the $init$ method and has
 * no constructor
 */
bool IsNamespaceClass(ir::AstNode *astNode)
{
    if (astNode->Type() != ir::AstNodeType::CLASS_DECLARATION) {
        return false;
    }
    auto *classDecl = static_cast<ir::ClassDeclaration *>(astNode);
    const auto &body = classDecl->Definition()->Body();
    bool hasConstructor = false;
    bool onlyInit = true;
    for (auto *member : body) {
        if (member->Type() == ir::AstNodeType::METHOD_DEFINITION) {
            auto *method = static_cast<ir::MethodDefinition *>(member);
            util::StringView keyName = method->Key()->AsIdentifier()->Name();
            if (keyName == "constructor") {
                hasConstructor = true;
            }
            if (keyName != "_$init$_") {
                onlyInit = false;
            }
        }
    }
    return !hasConstructor && onlyInit;
}

bool GetSafeDeleteInfoImpl(es2panda_Context *context, size_t position)
{
    auto astNode = GetTouchingToken(context, position, false);
    if (astNode == nullptr) {
        return false;
    }

    auto declInfo = GetDeclInfoImpl(astNode);
    auto fileName = std::get<0>(declInfo);
    std::string normFileName = NormalizeFilePath(fileName);
    if (!normFileName.empty() && normFileName.find("static/build-tools/ets2panda/lib/stdlib") != std::string::npos) {
        return false;
    }
    if (!normFileName.empty() && normFileName.find("/sdk/") != std::string::npos) {
        return false;
    }

    if (IsBuiltinTypeReference(astNode)) {
        return false;
    }

    astNode = FindNearestDeletableDecl(astNode);
    if (astNode == nullptr) {
        return false;
    }

    if (IsNamespaceClass(astNode)) {
        return false;
    }

    if (astNode->IsTSTypeParameterDeclaration()) {
        return false;
    }

    return true;
}
}  // namespace ark::es2panda::lsp

/*
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

#include "node_matchers.h"
#include <string>
#include "public/es2panda_lib.h"
#include "public/public.h"
#include "ir/ets/etsReExportDeclaration.h"

namespace ark::es2panda::lsp {

bool MatchClassDefinition(ir::AstNode *childNode, const NodeInfo *info)
{
    return childNode->IsClassDefinition() && std::string(childNode->AsClassDefinition()->Ident()->Name()) == info->name;
}

bool MatchIdentifier(ir::AstNode *childNode, const NodeInfo *info)
{
    return childNode->IsIdentifier() && std::string(childNode->AsIdentifier()->Name()) == info->name;
}

bool MatchClassProperty(ir::AstNode *childNode, const NodeInfo *info)
{
    return childNode->IsClassProperty() && std::string(childNode->AsClassProperty()->Id()->Name()) == info->name;
}

bool MatchProperty(ir::AstNode *childNode, const NodeInfo *info)
{
    return childNode->IsProperty() && std::string(childNode->AsProperty()->Key()->AsIdentifier()->Name()) == info->name;
}

bool MatchMethodDefinition(ir::AstNode *childNode, const NodeInfo *info)
{
    return childNode->IsMethodDefinition() &&
           std::string(childNode->AsMethodDefinition()->Function()->Id()->Name()) == info->name;
}

bool MatchTSEnumDeclaration(ir::AstNode *childNode, const NodeInfo *info)
{
    return childNode->IsTSEnumDeclaration() &&
           std::string(childNode->AsTSEnumDeclaration()->Key()->Name()) == info->name;
}

bool MatchTSEnumMember(ir::AstNode *childNode, const NodeInfo *info)
{
    return childNode->IsTSEnumMember() && std::string(childNode->AsTSEnumMember()->Name()) == info->name;
}

bool MatchTSInterfaceDeclaration(ir::AstNode *childNode, const NodeInfo *info)
{
    return childNode->IsTSInterfaceDeclaration() &&
           std::string(childNode->AsTSInterfaceDeclaration()->Id()->Name()) == info->name;
}

bool MatchTSTypeAliasDeclaration(ir::AstNode *childNode, const NodeInfo *info)
{
    return childNode->IsTSTypeAliasDeclaration() &&
           std::string(childNode->AsTSTypeAliasDeclaration()->Id()->Name()) == info->name;
}

bool MatchExportSpecifier(ir::AstNode *childNode, const NodeInfo *info)
{
    if (!childNode->IsETSReExportDeclaration()) {
        return false;
    }
    auto specifiers = childNode->AsETSReExportDeclaration()->GetETSImportDeclarations()->Specifiers();
    if (specifiers.empty()) {
        return false;
    }
    for (auto *importSpecifier : specifiers) {
        if (importSpecifier->IsImportSpecifier() &&
            importSpecifier->AsImportSpecifier()->Local()->Name().Mutf8() == info->name) {
            return true;
        }
    }
    return false;
}

bool MatchMemberExpression(ir::AstNode *childNode, const NodeInfo *info)
{
    return childNode->IsMemberExpression() && childNode->AsMemberExpression()->Property()->ToString() == info->name;
}

bool MatchTSClassImplements(ir::AstNode *childNode, const NodeInfo *info)
{
    if (!childNode->IsTSClassImplements()) {
        return false;
    }
    auto dd = childNode->AsTSClassImplements()->Expr()->AsETSTypeReference()->Part();
    return std::string(dd->GetIdent()->Name()) == info->name;
}

ir::AstNode *ExtractExportSpecifierIdentifier(ir::AstNode *node, const NodeInfo *info)
{
    if (!node->IsETSReExportDeclaration()) {
        return node;
    }

    auto specifiers = node->AsETSReExportDeclaration()->GetETSImportDeclarations()->Specifiers();
    if (specifiers.empty()) {
        return node;
    }

    for (auto *importSpecifier : specifiers) {
        if (!importSpecifier->IsImportSpecifier()) {
            continue;
        }

        if (importSpecifier->AsImportSpecifier()->Local()->Name().Mutf8() == info->name) {
            return importSpecifier->AsImportSpecifier()->Local();
        }
    }

    return node;
}

ir::AstNode *ExtractTSClassImplementsIdentifier(ir::AstNode *node, const NodeInfo *info)
{
    if (!node->IsTSClassImplements()) {
        return node;
    }

    auto expr = node->AsTSClassImplements()->Expr();
    if (!expr || !expr->IsETSTypeReference()) {
        return node;
    }

    auto part = expr->AsETSTypeReference()->Part();
    if (!part || !part->GetIdent()) {
        return node;
    }

    if (std::string(part->GetIdent()->Name()) == info->name) {
        return part->GetIdent();
    }

    return node;
}

const std::unordered_map<ir::AstNodeType, NodeExtractor> nodeExtractors = {
    {ir::AstNodeType::CLASS_DEFINITION,
     [](ir::AstNode *node, const NodeInfo *) {
         return node->IsClassDefinition() ? node->AsClassDefinition()->Ident() : node;
     }},
    {ir::AstNodeType::IDENTIFIER,
     [](ir::AstNode *node, const NodeInfo *) { return node->IsIdentifier() ? node->AsIdentifier() : node; }},
    {ir::AstNodeType::CLASS_PROPERTY,
     [](ir::AstNode *node, const NodeInfo *) {
         return node->IsClassProperty() ? node->AsClassProperty()->Id() : node;
     }},
    {ir::AstNodeType::PROPERTY,
     [](ir::AstNode *node, const NodeInfo *) {
         return node->IsProperty() ? node->AsProperty()->Key()->AsIdentifier() : node;
     }},
    {ir::AstNodeType::METHOD_DEFINITION,
     [](ir::AstNode *node, const NodeInfo *) {
         return node->IsMethodDefinition() ? node->AsMethodDefinition()->Function()->Id() : node;
     }},
    {ir::AstNodeType::TS_ENUM_DECLARATION,
     [](ir::AstNode *node, const NodeInfo *) {
         return node->IsTSEnumDeclaration() ? node->AsTSEnumDeclaration()->Key() : node;
     }},
    {ir::AstNodeType::TS_ENUM_MEMBER,
     [](ir::AstNode *node, const NodeInfo *) { return node->IsTSEnumMember() ? node->AsTSEnumMember()->Key() : node; }},
    {ir::AstNodeType::TS_INTERFACE_DECLARATION,
     [](ir::AstNode *node, const NodeInfo *) {
         return node->IsTSInterfaceDeclaration() ? node->AsTSInterfaceDeclaration()->Id() : node;
     }},
    {ir::AstNodeType::TS_TYPE_ALIAS_DECLARATION,
     [](ir::AstNode *node, const NodeInfo *) {
         return node->IsTSTypeAliasDeclaration() ? node->AsTSTypeAliasDeclaration()->Id() : node;
     }},
    {ir::AstNodeType::EXPORT_SPECIFIER,
     [](ir::AstNode *node, const NodeInfo *info) { return ExtractExportSpecifierIdentifier(node, info); }},
    {ir::AstNodeType::MEMBER_EXPRESSION,
     [](ir::AstNode *node, const NodeInfo *) {
         return node->IsMemberExpression() ? node->AsMemberExpression()->Property()->AsIdentifier() : node;
     }},
    {ir::AstNodeType::TS_CLASS_IMPLEMENTS,
     [](ir::AstNode *node, const NodeInfo *info) { return ExtractTSClassImplementsIdentifier(node, info); }}};

ir::AstNode *ExtractIdentifierFromNode(ir::AstNode *node, const NodeInfo *info)
{
    if (!node)
        return node;

    auto it = nodeExtractors.find(info->kind);
    if (it != nodeExtractors.end()) {
        return it->second(node, info);
    }
    return node;
}

const std::unordered_map<ir::AstNodeType, NodeMatcher> nodeMatchers = {
    {ir::AstNodeType::CLASS_DEFINITION, MatchClassDefinition},
    {ir::AstNodeType::IDENTIFIER, MatchIdentifier},
    {ir::AstNodeType::CLASS_PROPERTY, MatchClassProperty},
    {ir::AstNodeType::PROPERTY, MatchProperty},
    {ir::AstNodeType::METHOD_DEFINITION, MatchMethodDefinition},
    {ir::AstNodeType::TS_ENUM_DECLARATION, MatchTSEnumDeclaration},
    {ir::AstNodeType::TS_ENUM_MEMBER, MatchTSEnumMember},
    {ir::AstNodeType::TS_INTERFACE_DECLARATION, MatchTSInterfaceDeclaration},
    {ir::AstNodeType::TS_TYPE_ALIAS_DECLARATION, MatchTSTypeAliasDeclaration},
    {ir::AstNodeType::EXPORT_SPECIFIER, MatchExportSpecifier},
    {ir::AstNodeType::MEMBER_EXPRESSION, MatchMemberExpression},
    {ir::AstNodeType::TS_CLASS_IMPLEMENTS, MatchTSClassImplements}};
}  // namespace ark::es2panda::lsp
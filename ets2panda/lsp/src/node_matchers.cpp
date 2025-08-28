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
#include <cstddef>
#include <string>
#include "public/es2panda_lib.h"
#include "public/public.h"
#include "ir/ets/etsReExportDeclaration.h"
#include "ir/statements/annotationDeclaration.h"
#include "ir/statements/annotationUsage.h"

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
        if (importSpecifier->IsImportSpecifier() &&
            importSpecifier->AsImportSpecifier()->Imported()->Name().Mutf8() == info->name) {
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

bool MatchCallExpression(ir::AstNode *childNode, const NodeInfo *info)
{
    if (childNode->IsCallExpression()) {
        auto callExpr = childNode->AsCallExpression();
        auto callee = callExpr->Callee();
        if (callee->IsIdentifier()) {
            return std::string(callee->AsIdentifier()->Name()) == info->name;
        }
        if (callee->IsSuperExpression() && info->name == "super") {
            return true;
        }
        if (callee->IsMemberExpression()) {
            return callee->AsMemberExpression()->Property()->ToString() == info->name;
        }
    }
    return false;
}

bool MatchTsTypeReference(ir::AstNode *childNode, const NodeInfo *info)
{
    if (childNode->IsETSTypeReference()) {
        auto typeRef = childNode->AsETSTypeReference();
        auto part = typeRef->Part();
        if (part != nullptr && part->Name()->IsIdentifier()) {
            auto identifier = part->Name()->AsIdentifier();
            if (std::string(identifier->Name()) == info->name) {
                return true;
            }
        }
    }
    return false;
}

bool MatchScriptFunction(ir::AstNode *childNode, const NodeInfo *info)
{
    return childNode->IsScriptFunction() && std::string(childNode->AsScriptFunction()->Id()->Name()) == info->name;
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
        if (importSpecifier->AsImportSpecifier()->Imported()->Name().Mutf8() == info->name) {
            return importSpecifier->AsImportSpecifier()->Imported();
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
    if ((expr == nullptr) || !expr->IsETSTypeReference()) {
        return node;
    }

    auto part = expr->AsETSTypeReference()->Part();
    if ((part == nullptr) || (part->GetIdent() == nullptr)) {
        return node;
    }

    if (std::string(part->GetIdent()->Name()) == info->name) {
        return part->GetIdent();
    }

    return node;
}

bool GetNodeNameIsStringLiteralType(ir::AstNode *childNode, const std::string &nodeName)
{
    if (childNode->Parent() == nullptr) {
        return false;
    }
    auto parentNode = reinterpret_cast<ir::AstNode *>(childNode->Parent());
    if (parentNode->IsClassProperty()) {
        return std::string(parentNode->AsClassProperty()->Id()->Name()) == nodeName;
    }
    if (parentNode->IsIdentifier()) {
        return std::string(parentNode->AsIdentifier()->Name()) == nodeName;
    }
    if (parentNode->IsETSUnionType()) {
        auto unionTypeParentAst = reinterpret_cast<ir::AstNode *>(parentNode->Parent());
        if (unionTypeParentAst->IsTSTypeAliasDeclaration()) {
            return std::string(unionTypeParentAst->AsTSTypeAliasDeclaration()->Id()->Name()) == nodeName;
        }
    }
    if (parentNode->IsETSParameterExpression()) {
        return std::string(parentNode->AsETSParameterExpression()->Name()) == nodeName;
    }
    if (parentNode->IsTSTypeAliasDeclaration()) {
        return std::string(parentNode->AsTSTypeAliasDeclaration()->Id()->Name()) == nodeName;
    }
    return false;
}

bool MatchEtsStringLiteralType(ir::AstNode *childNode, const NodeInfo *info)
{
    if (!childNode->IsETSStringLiteralType()) {
        return false;
    }
    return GetNodeNameIsStringLiteralType(childNode, std::string(info->name));
}

ir::AstNode *ExtractETSStringLiteralTypeIdentifier(ir::AstNode *node, const NodeInfo *info)
{
    if (!node->IsETSStringLiteralType()) {
        return nullptr;
    }
    if (GetNodeNameIsStringLiteralType(node, std::string(info->name))) {
        return node->Parent();
    }
    return nullptr;
}

bool MatchEtsTypeReference(ir::AstNode *childNode, const NodeInfo *info)
{
    if (!childNode->IsETSTypeReference()) {
        return false;
    }
    return std::string(childNode->AsETSTypeReference()->Part()->Name()->ToString()) == info->name;
}

bool MatchEtsKeyofType(ir::AstNode *childNode, const NodeInfo *info)
{
    if (!childNode->IsETSKeyofType()) {
        return false;
    }

    auto typeRef = childNode->AsETSKeyofType()->GetTypeRef();
    if (typeRef == nullptr) {
        return false;
    }
    return typeRef->IsETSTypeReference() &&
           std::string(typeRef->AsETSTypeReference()->Part()->Name()->ToString()) == info->name;
}

ir::AstNode *ExtractETSKeyofTypeIdentifier(ir::AstNode *node, const NodeInfo *info)
{
    if (!node->IsETSKeyofType()) {
        return nullptr;
    }
    auto typeRef = node->AsETSKeyofType()->GetTypeRef();
    if (typeRef == nullptr) {
        return nullptr;
    }
    bool result = typeRef->IsETSTypeReference() &&
                  std::string(typeRef->AsETSTypeReference()->Part()->Name()->ToString()) == info->name;
    if (result) {
        return node->Parent();
    }
    return nullptr;
}

bool MatchEtsNewClassInstanceExpression(ir::AstNode *childNode, const NodeInfo *info)
{
    if (!childNode->IsETSNewClassInstanceExpression()) {
        return false;
    }

    auto typeRef = childNode->AsETSNewClassInstanceExpression()->GetTypeRef();
    if (typeRef == nullptr) {
        return false;
    }
    return typeRef->IsETSTypeReference() &&
           std::string(typeRef->AsETSTypeReference()->Part()->Name()->ToString()) == info->name;
}

bool MatchEtsStructDeclaration(ir::AstNode *childNode, const NodeInfo *info)
{
    if (!childNode->IsETSStructDeclaration()) {
        return false;
    }
    return std::string(childNode->AsETSStructDeclaration()->Definition()->Ident()->Name()) == info->name;
}

bool MatchSpreadElement(ir::AstNode *childNode, const NodeInfo *info)
{
    if (!childNode->IsSpreadElement()) {
        return false;
    }
    auto argument = childNode->AsSpreadElement()->Argument();
    if (argument == nullptr) {
        return false;
    }

    if (argument->IsIdentifier()) {
        return std::string(argument->AsIdentifier()->Name()) == info->name;
    }
    if (argument->IsMemberExpression()) {
        return std::string(argument->AsMemberExpression()->Property()->ToString()) == info->name;
    }

    return false;
}

ir::AstNode *ExtractCallExpressionIdentifier(ir::AstNode *node, const NodeInfo *info)
{
    if (node->IsCallExpression()) {
        auto callExpr = node->AsCallExpression();
        auto callee = callExpr->Callee();
        if (callee->IsIdentifier() && std::string(callee->AsIdentifier()->Name()) == info->name) {
            return callee->AsIdentifier();
        }
        if (callee->IsSuperExpression() && info->name == "super") {
            return callee->AsSuperExpression();
        }
        if (callee->IsMemberExpression() && callee->AsMemberExpression()->Property()->ToString() == info->name) {
            return callee->AsMemberExpression()->Property()->AsIdentifier();
        }
    }
    return node;
}

bool MatchVariableDeclarator(ir::AstNode *childNode, const NodeInfo *info)
{
    return childNode->IsVariableDeclarator() &&
           std::string(childNode->AsVariableDeclarator()->Id()->AsIdentifier()->Name()) == info->name;
}

bool MatchVariableDeclaration(ir::AstNode *childNode, const NodeInfo *info)
{
    return childNode->IsVariableDeclaration() &&
           std::string(childNode->AsVariableDeclaration()->Declarators()[0]->Id()->AsIdentifier()->Name()) ==
               info->name;
}

bool MatchClassDeclaration(ir::AstNode *childNode, const NodeInfo *info)
{
    return childNode->IsClassDeclaration() &&
           std::string(childNode->AsClassDeclaration()->Definition()->Ident()->Name()) == info->name;
}

bool MatchAnnotationDeclaration(ir::AstNode *childNode, const NodeInfo *info)
{
    return childNode->IsAnnotationDeclaration() &&
           std::string(childNode->AsAnnotationDeclaration()->GetBaseName()->Name()) == info->name;
}

bool MatchAnnotationUsage(ir::AstNode *childNode, const NodeInfo *info)
{
    return childNode->IsAnnotationUsage() &&
           std::string(childNode->AsAnnotationUsage()->GetBaseName()->Name()) == info->name;
}

bool MatchAwaitExpression(ir::AstNode *childNode, const NodeInfo *info)
{
    if (!childNode->IsAwaitExpression()) {
        return false;
    }
    auto awaitExpr = childNode->AsAwaitExpression();
    if ((awaitExpr != nullptr) && (awaitExpr->Argument() != nullptr) && awaitExpr->Argument()->IsIdentifier()) {
        auto identifier = awaitExpr->Argument()->AsIdentifier();
        return (identifier != nullptr) && std::string(identifier->Name()) == info->name;
    }
    return false;
}

bool MatchBigIntLiteral(ir::AstNode *childNode, const NodeInfo *info)
{
    if (!childNode->IsBigIntLiteral()) {
        return false;
    }
    std::string bigIntLiteral = std::string(childNode->AsBigIntLiteral()->Str());
    if (childNode->Parent()->IsUnaryExpression()) {
        bigIntLiteral.insert(0, lexer::TokenToString(childNode->Parent()->AsUnaryExpression()->OperatorType()));
        return bigIntLiteral == info->name;
    }
    return bigIntLiteral == info->name;
}

ir::AstNode *ExtractAwaitExpressionIdentifier(ir::AstNode *node, [[maybe_unused]] const NodeInfo *info)
{
    if (!node->IsAwaitExpression()) {
        return node;
    }

    if ((node->AsAwaitExpression()->Argument() != nullptr) && node->AsAwaitExpression()->Argument()->IsIdentifier()) {
        return const_cast<ir::Identifier *>(node->AsAwaitExpression()->Argument()->AsIdentifier());
    }
    return node;
}

bool MatchImportSpecifier(ir::AstNode *childNode, const NodeInfo *info)
{
    return childNode->IsImportSpecifier() &&
           std::string(childNode->AsImportSpecifier()->Imported()->Name()) == info->name;
}

bool MatchImportDefaultSpecifier(ir::AstNode *childNode, const NodeInfo *info)
{
    return childNode->IsImportDefaultSpecifier() &&
           std::string(childNode->AsImportDefaultSpecifier()->Local()->Name()) == info->name;
}

bool MatchImportNamespaceSpecifier(ir::AstNode *childNode, const NodeInfo *info)
{
    return childNode->IsImportNamespaceSpecifier() &&
           std::string(childNode->AsImportNamespaceSpecifier()->Local()->Name()) == info->name;
}

bool MatchTSTypeParameter(ir::AstNode *childNode, const NodeInfo *info)
{
    return childNode->IsTSTypeParameter() && std::string(childNode->AsTSTypeParameter()->Name()->Name()) == info->name;
}

bool MatchEtsParameterExpression(ir::AstNode *childNode, const NodeInfo *info)
{
    return childNode->IsETSParameterExpression() &&
           std::string(childNode->AsETSParameterExpression()->Ident()->Name()) == info->name;
}

bool MatchSwitchStatement(ir::AstNode *childNode, const NodeInfo *info)
{
    if (!childNode->IsSwitchStatement()) {
        return false;
    }
    auto discriminant = childNode->AsSwitchStatement()->Discriminant();
    if (discriminant == nullptr) {
        return false;
    }
    if (discriminant->IsIdentifier()) {
        return std::string(discriminant->AsIdentifier()->Name()) == info->name;
    }
    if (discriminant->IsMemberExpression()) {
        return std::string(discriminant->AsMemberExpression()->Object()->AsIdentifier()->Name()) == info->name;
    }
    return false;
}

bool MatchTsNonNullExpression(ir::AstNode *childNode, const NodeInfo *info)
{
    if (!childNode->IsTSNonNullExpression()) {
        return false;
    }
    auto expression = childNode->AsTSNonNullExpression()->Expr();
    if (expression == nullptr) {
        return false;
    }
    if (expression->IsIdentifier()) {
        return std::string(expression->AsIdentifier()->Name()) == info->name;
    }
    if (expression->IsMemberExpression()) {
        return std::string(expression->AsMemberExpression()->Object()->AsIdentifier()->Name()) == info->name ||
               std::string(expression->AsMemberExpression()->Property()->AsIdentifier()->Name()) == info->name;
    }
    return false;
}

bool MatchFunctionDeclaration(ir::AstNode *childNode, const NodeInfo *info)
{
    if (!childNode->IsFunctionDeclaration()) {
        return false;
    }
    return std::string(childNode->AsFunctionDeclaration()->Function()->Id()->Name()) == info->name;
}

ir::AstNode *ExtractIdentifierFromNode(ir::AstNode *node, const NodeInfo *info)
{
    if (node == nullptr) {
        return node;
    }

    const auto &nodeExtractors = GetNodeExtractors();
    auto it = nodeExtractors.find(info->kind);
    if (it != nodeExtractors.end()) {
        return it->second(node, info);
    }
    return node;
}

static std::unordered_map<ir::AstNodeType, NodeExtractor> GetClassAndIdentifierExtractors()
{
    // clang-format off
    return {{ir::AstNodeType::CLASS_DEFINITION,
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
            {ir::AstNodeType::CLASS_DECLARATION,
                    [](ir::AstNode *node, const NodeInfo *) {
                        return node->IsClassDeclaration() ? node->AsClassDeclaration()->Definition()->Ident() : node;
                }}
            };
    // clang-format on
}

static std::unordered_map<ir::AstNodeType, NodeExtractor> GetEnumAndInterfaceExtractors()
{
    // clang-format off
    return {{ir::AstNodeType::TS_ENUM_DECLARATION,
                    [](ir::AstNode *node, const NodeInfo *) {
                        return node->IsTSEnumDeclaration() ? node->AsTSEnumDeclaration()->Key() : node;
                    }},
            {ir::AstNodeType::TS_ENUM_MEMBER,
                    [](ir::AstNode *node, const NodeInfo *) {
                        return node->IsTSEnumMember() ? node->AsTSEnumMember()->Key() : node;
                    }},
            {ir::AstNodeType::TS_INTERFACE_DECLARATION,
                    [](ir::AstNode *node, const NodeInfo *) {
                        return node->IsTSInterfaceDeclaration() ? node->AsTSInterfaceDeclaration()->Id() : node;
                    }},
            {ir::AstNodeType::TS_TYPE_ALIAS_DECLARATION,
                    [](ir::AstNode *node, const NodeInfo *) {
                        return node->IsTSTypeAliasDeclaration() ? node->AsTSTypeAliasDeclaration()->Id() : node;
                }}
            };
    // clang-format on
}

static std::unordered_map<ir::AstNodeType, NodeExtractor> GetExportExtractors()
{
    return {{ir::AstNodeType::EXPORT_SPECIFIER,
             [](ir::AstNode *node, const NodeInfo *info) { return ExtractExportSpecifierIdentifier(node, info); }},
            {ir::AstNodeType::REEXPORT_STATEMENT,
             [](ir::AstNode *node, const NodeInfo *info) { return ExtractExportSpecifierIdentifier(node, info); }}};
}

static std::unordered_map<ir::AstNodeType, NodeExtractor> GetExpressionExtractors()
{
    return {{ir::AstNodeType::MEMBER_EXPRESSION,
             [](ir::AstNode *node, const NodeInfo *) {
                 return node->IsMemberExpression() ? node->AsMemberExpression()->Property()->AsIdentifier() : node;
             }},
            {ir::AstNodeType::CALL_EXPRESSION,
             [](ir::AstNode *node, const NodeInfo *info) { return ExtractCallExpressionIdentifier(node, info); }},
            {ir::AstNodeType::SUPER_EXPRESSION,
             [](ir::AstNode *node, const NodeInfo *info) { return ExtractCallExpressionIdentifier(node, info); }},
            {ir::AstNodeType::AWAIT_EXPRESSION,
             [](ir::AstNode *node, const NodeInfo *info) { return ExtractAwaitExpressionIdentifier(node, info); }}};
}

static std::unordered_map<ir::AstNodeType, NodeExtractor> GetTypeReferenceExtractors()
{
    // clang-format off
    return {{ir::AstNodeType::TS_CLASS_IMPLEMENTS,
                    [](ir::AstNode *node, const NodeInfo *info) {
                        return ExtractTSClassImplementsIdentifier(node, info);
                    }},
            {ir::AstNodeType::ETS_STRING_LITERAL_TYPE,
                    [](ir::AstNode *node, const NodeInfo *info) {
                        return ExtractETSStringLiteralTypeIdentifier(node, info);
                    }},
            {ir::AstNodeType::ETS_KEYOF_TYPE,
                    [](ir::AstNode *node, const NodeInfo *info) {
                        return ExtractETSKeyofTypeIdentifier(node, info);
                    }},
            {ir::AstNodeType::TS_TYPE_REFERENCE,
                    [](ir::AstNode *node, const NodeInfo *) {
                        return node->IsETSTypeReference()
                            ? node->AsETSTypeReference()->Part()->Name()->AsIdentifier() : node;
                    }}
            };
    // clang-format on
}

static std::unordered_map<ir::AstNodeType, NodeExtractor> GetFunctionAndVariableExtractors()
{
    // clang-format off
    return {{ir::AstNodeType::SCRIPT_FUNCTION,
                    [](ir::AstNode *node, const NodeInfo *) {
                        return node->IsScriptFunction() ? node->AsScriptFunction()->Id() : node;
                    }},
            {ir::AstNodeType::VARIABLE_DECLARATOR,
                    [](ir::AstNode *node, const NodeInfo *) {
                        return node->IsVariableDeclarator() ? node->AsVariableDeclarator()->Id()->AsIdentifier() : node;
                    }},
            {ir::AstNodeType::VARIABLE_DECLARATION,
                    [](ir::AstNode *node, const NodeInfo *) {
                        return node->IsVariableDeclaration()
                            ? node->AsVariableDeclaration()->Declarators()[0]->Id()->AsIdentifier()
                            : node;
                }}
            };
    // clang-format on
}

static std::unordered_map<ir::AstNodeType, NodeExtractor> GetAnnotationAndImportExtractors()
{
    // clang-format off
    return {
        {ir::AstNodeType::ANNOTATION_DECLARATION,
                [](ir::AstNode *node, const NodeInfo *) {
                    return node->IsAnnotationDeclaration() ? node->AsAnnotationDeclaration()->GetBaseName() : node;
                }},
        {ir::AstNodeType::ANNOTATION_USAGE,
                [](ir::AstNode *node, const NodeInfo *) {
                    return node->IsAnnotationUsage() ? node->AsAnnotationUsage()->GetBaseName() : node;
                }},
        {ir::AstNodeType::BIGINT_LITERAL,
                [](ir::AstNode *node, const NodeInfo *) {
                    return node->IsBigIntLiteral() ? node->AsBigIntLiteral() : node;
                }},
        {ir::AstNodeType::IMPORT_SPECIFIER,
                [](ir::AstNode *node, const NodeInfo *) {
                    return node->IsImportSpecifier() ? node->AsImportSpecifier()->Imported() : node;
                }},
        {ir::AstNodeType::IMPORT_DEFAULT_SPECIFIER,
                [](ir::AstNode *node, const NodeInfo *) {
                    return node->IsImportDefaultSpecifier() ? node->AsImportDefaultSpecifier()->Local() : node;
                }},
        {ir::AstNodeType::IMPORT_NAMESPACE_SPECIFIER,
                [](ir::AstNode *node, const NodeInfo *) {
                    return node->IsImportNamespaceSpecifier() ? node->AsImportNamespaceSpecifier()->Local() : node;
                }},
        {ir::AstNodeType::TS_TYPE_PARAMETER,
                [](ir::AstNode *node, const NodeInfo *) {
                    return node->IsTSTypeParameter() ? node->AsTSTypeParameter()->Name() : node;
                }}
            };
    // clang-format on
}

const std::unordered_map<ir::AstNodeType, NodeExtractor> &GetNodeExtractors()
{
    static std::unordered_map<ir::AstNodeType, NodeExtractor> nodeExtractors;
    static bool initialized = false;

    if (!initialized) {
        auto classExtractors = GetClassAndIdentifierExtractors();
        auto enumExtractors = GetEnumAndInterfaceExtractors();
        auto exportExtractors = GetExportExtractors();
        auto expressionExtractors = GetExpressionExtractors();
        auto typeRefExtractors = GetTypeReferenceExtractors();
        auto funcVarExtractors = GetFunctionAndVariableExtractors();
        auto annotationImportExtractors = GetAnnotationAndImportExtractors();

        nodeExtractors.insert(classExtractors.begin(), classExtractors.end());
        nodeExtractors.insert(enumExtractors.begin(), enumExtractors.end());
        nodeExtractors.insert(exportExtractors.begin(), exportExtractors.end());
        nodeExtractors.insert(expressionExtractors.begin(), expressionExtractors.end());
        nodeExtractors.insert(typeRefExtractors.begin(), typeRefExtractors.end());
        nodeExtractors.insert(funcVarExtractors.begin(), funcVarExtractors.end());
        nodeExtractors.insert(annotationImportExtractors.begin(), annotationImportExtractors.end());

        initialized = true;
    }

    return nodeExtractors;
}

const std::unordered_map<ir::AstNodeType, NodeMatcher> &GetNodeMatchers()
{
    static const std::unordered_map<ir::AstNodeType, NodeMatcher> NODE_MATCHERS = {
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
        {ir::AstNodeType::TS_CLASS_IMPLEMENTS, MatchTSClassImplements},
        {ir::AstNodeType::ETS_STRING_LITERAL_TYPE, MatchEtsStringLiteralType},
        {ir::AstNodeType::ETS_TYPE_REFERENCE, MatchEtsTypeReference},
        {ir::AstNodeType::ETS_KEYOF_TYPE, MatchEtsKeyofType},
        {ir::AstNodeType::ETS_NEW_CLASS_INSTANCE_EXPRESSION, MatchEtsNewClassInstanceExpression},
        {ir::AstNodeType::STRUCT_DECLARATION, MatchEtsStructDeclaration},
        {ir::AstNodeType::SPREAD_ELEMENT, MatchSpreadElement},
        {ir::AstNodeType::REEXPORT_STATEMENT, MatchExportSpecifier},
        {ir::AstNodeType::CALL_EXPRESSION, MatchCallExpression},
        {ir::AstNodeType::SUPER_EXPRESSION, MatchCallExpression},
        {ir::AstNodeType::TS_TYPE_REFERENCE, MatchTsTypeReference},
        {ir::AstNodeType::SCRIPT_FUNCTION, MatchScriptFunction},
        {ir::AstNodeType::VARIABLE_DECLARATOR, MatchVariableDeclarator},
        {ir::AstNodeType::VARIABLE_DECLARATION, MatchVariableDeclaration},
        {ir::AstNodeType::CLASS_DECLARATION, MatchClassDeclaration},
        {ir::AstNodeType::ANNOTATION_DECLARATION, MatchAnnotationDeclaration},
        {ir::AstNodeType::ANNOTATION_USAGE, MatchAnnotationUsage},
        {ir::AstNodeType::AWAIT_EXPRESSION, MatchAwaitExpression},
        {ir::AstNodeType::BIGINT_LITERAL, MatchBigIntLiteral},
        {ir::AstNodeType::IMPORT_SPECIFIER, MatchImportSpecifier},
        {ir::AstNodeType::IMPORT_DEFAULT_SPECIFIER, MatchImportDefaultSpecifier},
        {ir::AstNodeType::IMPORT_NAMESPACE_SPECIFIER, MatchImportNamespaceSpecifier},
        {ir::AstNodeType::TS_TYPE_PARAMETER, MatchTSTypeParameter},
        {ir::AstNodeType::ETS_PARAMETER_EXPRESSION, MatchEtsParameterExpression},
        {ir::AstNodeType::SWITCH_STATEMENT, MatchSwitchStatement},
        {ir::AstNodeType::TS_NON_NULL_EXPRESSION, MatchTsNonNullExpression},
        {ir::AstNodeType::FUNCTION_DECLARATION, MatchFunctionDeclaration}};
    return NODE_MATCHERS;
}
}  // namespace ark::es2panda::lsp
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

#include "get_adjusted_location.h"
#include "internal_api.h"
#include <cassert>
#include <string>
#include <vector>
#include "public/es2panda_lib.h"
#include "public/public.h"
#include "ir/astNodeFlags.h"
#include "utils/arena_containers.h"
#include "util/options.h"

using ark::es2panda::ir::AstNode;
using ark::es2panda::ir::AstNodeType;
using ark::es2panda::ir::ModifierFlags;

namespace ark::es2panda::lsp {

ArenaVector<AstNode *> GetChildren(AstNode *node, ArenaAllocator *allocator)
{
    ArenaVector<AstNode *> children(allocator->Adapter());

    if (node == nullptr) {
        return children;
    }

    node->IterateRecursively([&children](const AstNode *current) {
        if (current != nullptr) {
            children.push_back(const_cast<AstNode *>(current));
        }
    });

    return children;
}

AstNode *GetTouchingPropertyName(es2panda_Context *context, size_t pos)
{
    AstNode *token = GetTouchingToken(context, pos, false);
    if (token == nullptr) {
        return nullptr;
    }

    if (token->IsCallExpression() && token->AsCallExpression()->Callee()->IsIdentifier()) {
        return token->AsCallExpression()->Callee()->AsIdentifier();
    }

    if (token->IsProperty() || token->IsIdentifier()) {
        return token;
    }

    if (token->IsClassDeclaration() || token->IsFunctionDeclaration() || token->IsTSConstructorType()) {
        return token;
    }

    return nullptr;
}

bool IsDeclarationOrModifier(AstNode *node, AstNode *parent, bool forRename)
{
    return (
        // Modifier check
        (IsModifier(node) && forRename && CanHaveModifiers(*parent) &&
         (parent->Modifiers() & node->Modifiers()) != 0U) ||
        // Class implementation check
        (node->IsTSClassImplements() && (parent->IsClassDeclaration() || node->IsClassExpression())) ||
        // Function declaration check
        (node->IsFunctionDeclaration() && (parent->IsFunctionDeclaration() || node->IsFunctionExpression())) ||
        // Interface declaration check
        (node->IsTSInterfaceDeclaration() && parent->IsTSInterfaceDeclaration()) ||
        // Enum declaration check
        (node->IsTSEnumDeclaration() && parent->IsTSEnumDeclaration()) ||
        // Type alias declaration check
        (node->IsTSTypeAliasDeclaration() && parent->IsTSTypeAliasDeclaration()) ||
        // Module and import namespace check
        ((node->IsImportNamespaceSpecifier() || node->IsTSModuleDeclaration()) && parent->IsTSModuleDeclaration()) ||
        // Import equals declaration check
        (node->IsTSImportEqualsDeclaration() && parent->IsTSImportEqualsDeclaration()));
}

std::optional<AstNode *> HandleTSAsExpression(AstNode *node, AstNode *parent,
                                              const ArenaVector<AstNode *> &parentChildren)
{
    if (!node->IsTSAsExpression()) {
        return std::nullopt;
    }
    // Import/Export specifier check
    if (parent->IsImportSpecifier() || parent->IsExportSpecifier() || parent->IsImportNamespaceSpecifier()) {
        return std::make_optional(FindFirstIdentifier(parent, false, parentChildren));
    }
    // Export all declaration check
    if (parent->IsExportAllDeclaration()) {
        if (auto *exportClause = FindNodeOfType(parent, AstNodeType::EXPORT_SPECIFIER, parentChildren)) {
            if (exportClause->AsExportSpecifier()->Local()->IsIdentifier() &&
                exportClause->AsExportSpecifier()->Local()->AsIdentifier()->Name() == "*") {
                return std::make_optional(FindFirstIdentifier(exportClause, false, parentChildren));
            }
        }
    }
    return std::nullopt;
}

std::optional<AstNode *> HandleTSImportType(AstNode *node, AstNode *parent,
                                            const ArenaVector<AstNode *> &parentChildren, bool forRename,
                                            ArenaAllocator *allocator)
{
    if (!node->IsTSImportType()) {
        return std::nullopt;
    }
    if (auto location = GetAdjustedLocationForDeclaration(parent->Parent(), forRename, parentChildren, allocator)) {
        return location;
    }

    if (parent->IsExportAllDeclaration()) {
        if (auto location = GetAdjustedLocationForExportDeclaration(parent, forRename, parentChildren)) {
            return location;
        }
    }
    return std::nullopt;
}

std::optional<AstNode *> HandleImportDeclaration(AstNode *node, AstNode *parent,
                                                 const ArenaVector<AstNode *> &parentChildren, bool forRename)
{
    if (!node->IsImportDeclaration() || !parent->IsImportDeclaration()) {
        return std::nullopt;
    }
    return GetAdjustedLocationForImportDeclaration(parent, forRename, parentChildren);
}

inline std::optional<AstNode *> HandleExportAllDeclaration(AstNode *node, AstNode *parent, bool forRename,
                                                           const ArenaVector<AstNode *> &parentChildren)
{
    if (!node->IsExportAllDeclaration() || !parent->IsExportAllDeclaration()) {
        return std::nullopt;
    }
    return GetAdjustedLocationForExportDeclaration(parent, forRename, parentChildren);
}

inline std::optional<AstNode *> HandleTSClassImplements(AstNode *node, AstNode *parent)
{
    if (!node->IsTSClassImplements() || !parent->IsTSClassImplements()) {
        return std::nullopt;
    }
    return GetAdjustedLocationForHeritageClause(parent);
}

inline std::optional<AstNode *> HandleTSInferType(AstNode *node, AstNode *parent,
                                                  const ArenaVector<AstNode *> &parentChildren)
{
    if (!node->IsTSInferType() || !parent->IsTSInferType()) {
        return std::nullopt;
    }
    if (auto *typeParam = FindTypeParameter(parent, parentChildren)) {
        return std::make_optional(FindFirstIdentifier(typeParam, false, parentChildren));
    }
    return std::nullopt;
}

inline std::optional<AstNode *> HandleTSTypeParameterDeclaration(AstNode *parent,
                                                                 const ArenaVector<AstNode *> &parentChildren)
{
    if (!parent->IsTSTypeParameterDeclaration() || !parent->Parent()->IsTSTypeParameterDeclaration()) {
        return std::nullopt;
    }
    return std::make_optional(FindFirstIdentifier(parent, false, parentChildren));
}

std::optional<AstNode *> HandleTSTypeOperator(AstNode *parent, const ArenaVector<AstNode *> &parentChildren)
{
    if (!parent->IsTSTypeOperator()) {
        return std::nullopt;
    }
    if (auto *typeRef = FindTypeReference(parent, parentChildren)) {
        return std::make_optional(FindFirstIdentifier(typeRef, false, parentChildren));
    }
    if (auto *arrayType = FindArrayType(parent, parentChildren)) {
        if (auto *elementType = FindTypeReference(arrayType, parentChildren)) {
            return std::make_optional(FindFirstIdentifier(elementType, false, parentChildren));
        }
    }
    return std::nullopt;
}

std::optional<AstNode *> HandleBasicExpressions(AstNode *node, AstNode *parent,
                                                const ArenaVector<AstNode *> &parentChildren)
{
    if ((node->IsNewExpression() && parent->IsNewExpression()) ||
        (node->IsTSVoidKeyword() && parent->IsTSVoidKeyword()) ||
        (node->IsTypeofExpression() && parent->IsTypeofExpression()) ||
        (node->IsAwaitExpression() && parent->IsAwaitExpression()) ||
        (node->IsYieldExpression() && parent->IsYieldExpression())) {
        if (auto *expr = FindFirstExpression(parent, parentChildren)) {
            return std::make_optional(SkipOuterExpressions(expr));
        }
    }
    return std::nullopt;
}

std::optional<AstNode *> HandleBinaryExpressions(AstNode *node, AstNode *parent,
                                                 const ArenaVector<AstNode *> &parentChildren)
{
    if (!parent->IsBinaryExpression()) {
        return std::nullopt;
    }
    if (node->IsTSTypeOperator() ||
        (node->IsForInStatement() && parent->FindChild([node](AstNode *child) { return child == node; }) != nullptr)) {
        auto *firstExpr = FindFirstExpression(parent, parentChildren);
        if (auto *rightExpr = FindFirstExpressionAfter(parent, firstExpr, parentChildren)) {
            return std::make_optional(SkipOuterExpressions(rightExpr));
        }
    }
    return std::nullopt;
}

std::optional<AstNode *> HandleForStatements(AstNode *node, AstNode *parent,
                                             const ArenaVector<AstNode *> &parentChildren)
{
    if ((node->IsForInStatement() && parent->IsForInStatement()) ||
        (node->IsForOfStatement() && parent->IsForOfStatement())) {
        if (auto *expr = FindFirstExpression(parent, parentChildren)) {
            return std::make_optional(SkipOuterExpressions(expr));
        }
    }
    return std::nullopt;
}

std::optional<AstNode *> HandleNonRenameExpressions(AstNode *node, AstNode *parent,
                                                    const ArenaVector<AstNode *> &parentChildren, bool forRename)
{
    if (forRename) {
        return std::nullopt;
    }
    // Try each handler in sequence
    if (auto result = HandleBasicExpressions(node, parent, parentChildren)) {
        return result;
    }

    if (auto result = HandleBinaryExpressions(node, parent, parentChildren)) {
        return result;
    }
    // Handle type assertions
    if (node->IsTSAsExpression() && parent->IsTSAsExpression() && parent->IsTSTypeReference()) {
        return std::make_optional(FindFirstIdentifier(parent, false, parentChildren));
    }
    if (auto result = HandleForStatements(node, parent, parentChildren)) {
        return result;
    }
    return std::nullopt;
}

std::optional<AstNode *> HandleDefaultExport(AstNode *node, ArenaAllocator *allocator)
{
    if ((node->Modifiers() & ModifierFlags::DEFAULT_EXPORT) == 0U) {
        return std::nullopt;
    }
    const std::array<AstNodeType, 21> declarationTypes = {AstNodeType::VARIABLE_DECLARATION,
                                                          AstNodeType::PROPERTY,
                                                          AstNodeType::FUNCTION_DECLARATION,
                                                          AstNodeType::CLASS_DECLARATION,
                                                          AstNodeType::TS_INTERFACE_DECLARATION,
                                                          AstNodeType::TS_ENUM_DECLARATION,
                                                          AstNodeType::TS_TYPE_ALIAS_DECLARATION,
                                                          AstNodeType::TS_TYPE_PARAMETER_DECLARATION,
                                                          AstNodeType::TS_MODULE_DECLARATION,
                                                          AstNodeType::TS_CONSTRUCTOR_TYPE,
                                                          AstNodeType::TS_TYPE_ASSERTION,
                                                          AstNodeType::TS_AS_EXPRESSION,
                                                          AstNodeType::TS_NON_NULL_EXPRESSION,
                                                          AstNodeType::BINARY_EXPRESSION,
                                                          AstNodeType::FOR_IN_STATEMENT,
                                                          AstNodeType::FOR_OF_STATEMENT,
                                                          AstNodeType::NEW_EXPRESSION,
                                                          AstNodeType::TS_VOID_KEYWORD,
                                                          AstNodeType::TYPEOF_EXPRESSION,
                                                          AstNodeType::AWAIT_EXPRESSION,
                                                          AstNodeType::YIELD_EXPRESSION};

    auto children = GetChildren(node, allocator);
    for (const auto type : declarationTypes) {
        if (auto *declaration = FindNodeOfType(node, type, children)) {
            return std::make_optional(declaration);
        }
    }
    return std::nullopt;
}

inline std::optional<AstNode *> HandleVariableDeclaration(AstNode *parent, const ArenaVector<AstNode *> &parentChildren)
{
    if (parent->IsVariableDeclaration()) {
        if (auto *identifier = FindFirstIdentifier(parent, false, parentChildren)) {
            return std::make_optional(identifier);
        }
    }
    return std::nullopt;
}

inline std::optional<AstNode *> HandleExternalModuleReference(AstNode *parent,
                                                              const ArenaVector<AstNode *> &parentChildren)
{
    if (parent->IsTSExternalModuleReference()) {
        if (auto *expr = FindFirstExpression(parent, parentChildren)) {
            return std::make_optional(expr);
        }
    }
    return std::nullopt;
}

inline std::optional<AstNode *> HandleModuleSpecifier(AstNode *parent, const ArenaVector<AstNode *> &parentChildren)
{
    if ((parent->IsImportDeclaration() || parent->IsExportAllDeclaration())) {
        if (auto *moduleSpecifier = FindNodeOfType(parent, AstNodeType::STRING_LITERAL, parentChildren)) {
            return std::make_optional(moduleSpecifier);
        }
    }
    return std::nullopt;
}

std::optional<AstNode *> HandleExpressionAndTypes(AstNode *node, AstNode *parent,
                                                  const ArenaVector<AstNode *> &parentChildren, bool forRename,
                                                  ArenaAllocator *allocator)
{
    // Expression handlers
    if (auto result = HandleTSAsExpression(node, parent, parentChildren)) {
        return result;
    }
    if (auto result = HandleTSImportType(node, parent, parentChildren, forRename, allocator)) {
        return result;
    }

    // Type system handlers
    if (auto result = HandleTSClassImplements(node, parent)) {
        return result;
    }
    if (auto result = HandleTSInferType(node, parent, parentChildren)) {
        return result;
    }
    if (auto result = HandleTSTypeParameterDeclaration(parent, parentChildren)) {
        return result;
    }
    if (auto result = HandleTSTypeOperator(parent, parentChildren)) {
        return result;
    }
    return std::nullopt;
}

std::optional<AstNode *> HandleModulesAndExports(AstNode *node, AstNode *parent,
                                                 const ArenaVector<AstNode *> &parentChildren, bool forRename,
                                                 ArenaAllocator *allocator)
{
    // Import/Export handlers
    if (auto result = HandleImportDeclaration(node, parent, parentChildren, forRename)) {
        return result;
    }
    if (auto result = HandleExportAllDeclaration(node, parent, forRename, parentChildren)) {
        return result;
    }
    if (auto result = HandleExternalModuleReference(parent, parentChildren)) {
        return result;
    }
    if (auto result = HandleModuleSpecifier(parent, parentChildren)) {
        return result;
    }
    if (auto result = HandleDefaultExport(node, allocator)) {
        return result;
    }

    return std::nullopt;
}

std::optional<AstNode *> GetAdjustedLocation(AstNode *node, bool forRename, ArenaAllocator *allocator)
{
    if (node == nullptr) {
        return std::nullopt;
    }
    node = GetOriginalNode(node);
    auto *parent = node->Parent();
    if (parent == nullptr) {
        return std::make_optional(node);
    }

    ArenaVector<AstNode *> parentChildren = GetChildren(parent, allocator);

    // Declaration handlers
    if (IsDeclarationOrModifier(node, parent, forRename)) {
        if (auto location = GetAdjustedLocationForDeclaration(parent, forRename, parentChildren, allocator)) {
            return location;
        }
    }
    // Expression and Type handlers
    if (auto result = HandleExpressionAndTypes(node, parent, parentChildren, forRename, allocator)) {
        return result;
    }
    // Module and Export handlers
    if (auto result = HandleModulesAndExports(node, parent, parentChildren, forRename, allocator)) {
        return result;
    }
    // Variable handlers
    if (auto result = HandleVariableDeclaration(parent, parentChildren)) {
        return result;
    }
    if (auto result = HandleNonRenameExpressions(node, parent, parentChildren, forRename)) {
        return result;
    }
    return std::make_optional(node);
}

std::optional<AstNode *> GetAdjustedLocationForDeclaration(AstNode *node, bool forRename,
                                                           const ArenaVector<AstNode *> &children,
                                                           ArenaAllocator *allocator)
{
    if (!forRename) {
        switch (node->Type()) {
            case AstNodeType::CLASS_DECLARATION:
            case AstNodeType::CLASS_EXPRESSION:
            case AstNodeType::STRUCT_DECLARATION:
                return GetAdjustedLocationForClass(node, allocator);
            case AstNodeType::FUNCTION_DECLARATION:
            case AstNodeType::FUNCTION_EXPRESSION:
                return GetAdjustedLocationForFunction(node, allocator);
            case AstNodeType::TS_CONSTRUCTOR_TYPE:
                return std::make_optional(node);
            default:
                break;
        }
    }
    if (node->IsExportNamedDeclaration()) {
        return std::make_optional(FindFirstIdentifier(node, false, children));
    }
    return std::nullopt;
}

std::optional<AstNode *> GetAdjustedLocationForImportDeclaration(AstNode *node, bool forRename,
                                                                 const ArenaVector<AstNode *> &children)
{
    if (!node->IsImportDeclaration()) {
        return std::nullopt;
    }
    if (forRename) {
        auto *importNode = node->FindChild(
            [](AstNode *child) { return child->IsImportSpecifier() || child->IsImportNamespaceSpecifier(); });
        if (importNode != nullptr) {
            return std::make_optional(FindFirstIdentifier(importNode, false, children));
        }
        return std::nullopt;
    }
    auto *importNode = node->FindChild(
        [](AstNode *child) { return child->IsImportSpecifier() || child->IsImportNamespaceSpecifier(); });
    if (importNode != nullptr) {
        if (auto *identifier = FindFirstIdentifier(importNode, false, children)) {
            return std::make_optional(identifier);
        }
        return std::make_optional(importNode);
    }
    return std::nullopt;
}

std::optional<AstNode *> GetAdjustedLocationForExportDeclaration(AstNode *node, bool forRename,
                                                                 const ArenaVector<AstNode *> &children)
{
    if (!node->IsExportAllDeclaration()) {
        return std::nullopt;
    }
    if (forRename) {
        return std::make_optional(FindFirstIdentifier(node, false, children));
    }
    auto *exportSpecifier = FindNodeOfType(node, AstNodeType::EXPORT_SPECIFIER, children);
    if (exportSpecifier != nullptr) {
        if (auto *identifier = FindFirstIdentifier(exportSpecifier, false, children)) {
            return std::make_optional(identifier);
        }
        return std::make_optional(exportSpecifier);
    }
    if ((node->Modifiers() & ModifierFlags::DEFAULT_EXPORT) != 0U) {
        const std::array<AstNodeType, 21> declarationTypes = {AstNodeType::VARIABLE_DECLARATION,
                                                              AstNodeType::PROPERTY,
                                                              AstNodeType::FUNCTION_DECLARATION,
                                                              AstNodeType::CLASS_DECLARATION,
                                                              AstNodeType::TS_INTERFACE_DECLARATION,
                                                              AstNodeType::TS_ENUM_DECLARATION,
                                                              AstNodeType::TS_TYPE_ALIAS_DECLARATION,
                                                              AstNodeType::TS_TYPE_PARAMETER_DECLARATION,
                                                              AstNodeType::TS_MODULE_DECLARATION,
                                                              AstNodeType::TS_CONSTRUCTOR_TYPE,
                                                              AstNodeType::TS_TYPE_ASSERTION,
                                                              AstNodeType::TS_AS_EXPRESSION,
                                                              AstNodeType::TS_NON_NULL_EXPRESSION,
                                                              AstNodeType::BINARY_EXPRESSION,
                                                              AstNodeType::FOR_IN_STATEMENT,
                                                              AstNodeType::FOR_OF_STATEMENT,
                                                              AstNodeType::NEW_EXPRESSION,
                                                              AstNodeType::TS_VOID_KEYWORD,
                                                              AstNodeType::TYPEOF_EXPRESSION,
                                                              AstNodeType::AWAIT_EXPRESSION,
                                                              AstNodeType::YIELD_EXPRESSION};

        for (const auto type : declarationTypes) {
            if (auto *declaration = FindNodeOfType(node, type, children)) {
                return std::make_optional(declaration);
            }
        }
    }
    return std::nullopt;
}

std::optional<AstNode *> GetAdjustedLocationForClass(AstNode *node, ArenaAllocator *allocator)
{
    if (node == nullptr || (!node->IsClassDeclaration() && !node->IsClassExpression())) {
        return std::nullopt;
    }
    auto children = GetChildren(node, allocator);
    return std::make_optional(FindFirstIdentifier(node, false, children));
}

std::optional<AstNode *> GetAdjustedLocationForFunction(AstNode *node, ArenaAllocator *allocator)
{
    if (node->IsIdentifier()) {
        return std::make_optional(node);
    }
    if (node->IsFunctionDeclaration()) {
        auto children = GetChildren(node, allocator);
        return std::make_optional(FindFirstIdentifier(node, false, children));
    }
    if (auto *parent = node->Parent()) {
        if (parent->IsFunctionDeclaration()) {
            auto children = GetChildren(parent, allocator);
            return std::make_optional(FindFirstIdentifier(parent, false, children));
        }
    }
    return std::nullopt;
}

bool IsModifier(const AstNode *node)
{
    if (node == nullptr) {
        return false;
    }
    switch (node->Modifiers()) {
        case ModifierFlags::ABSTRACT:
        case ModifierFlags::ACCESS:
        case ModifierFlags::ASYNC:
        case ModifierFlags::CONST:
        case ModifierFlags::DECLARE:
        case ModifierFlags::DEFAULT_EXPORT:
        case ModifierFlags::IN:
        case ModifierFlags::EXPORT:
        case ModifierFlags::PUBLIC:
        case ModifierFlags::PRIVATE:
        case ModifierFlags::PROTECTED:
        case ModifierFlags::READONLY:
        case ModifierFlags::STATIC:
        case ModifierFlags::OUT:
        case ModifierFlags::OVERRIDE:
            return true;
        default:
            return false;
    }
}

bool IsOuterExpression(const AstNode *node)
{
    if (node == nullptr) {
        return false;
    }
    switch (node->Type()) {
        case AstNodeType::TS_TYPE_ASSERTION:
        case AstNodeType::TS_AS_EXPRESSION:
        case AstNodeType::TS_NON_NULL_EXPRESSION:
            return true;
        default:
            return false;
    }
}

AstNode *SkipOuterExpressions(AstNode *node)
{
    if (node == nullptr) {
        return node;
    }
    if (node->IsExpression() && IsOuterExpression(node)) {
        return node;
    }
    return node;
}

std::optional<AstNode *> GetAdjustedLocationForHeritageClause(AstNode *node)
{
    if (node == nullptr || node->Type() != AstNodeType::TS_INTERFACE_HERITAGE) {
        return std::nullopt;
    }
    auto *expression = node->FindChild(
        [](AstNode *child) { return child->IsExpression() || child->IsIdentifier() || child->IsTSTypeReference(); });
    return expression != nullptr ? std::make_optional(expression) : std::nullopt;
}

bool CanHaveModifiers(const AstNode &node)
{
    switch (node.Type()) {
        case AstNodeType::CLASS_DECLARATION:
        case AstNodeType::FUNCTION_DECLARATION:
        case AstNodeType::METHOD_DEFINITION:
        case AstNodeType::PROPERTY:
        case AstNodeType::TS_CONSTRUCTOR_TYPE:
        case AstNodeType::TS_INTERFACE_DECLARATION:
        case AstNodeType::TS_TYPE_ALIAS_DECLARATION:
        case AstNodeType::TS_ENUM_DECLARATION:
        case AstNodeType::TS_MODULE_DECLARATION:
        case AstNodeType::VARIABLE_DECLARATION:
            return true;
        default:
            return false;
    }
}

AstNode *FindFirstIdentifier(AstNode *node, bool skipModifiers, const ArenaVector<AstNode *> &children)
{
    if (node->IsIdentifier() && (!skipModifiers || !IsModifier(node))) {
        return node;
    }
    for (auto *child : children) {
        if (child->IsIdentifier() && (!skipModifiers || !IsModifier(child))) {
            return child;
        }
    }
    return nullptr;
}

AstNode *FindFirstExpression(AstNode *node, const ArenaVector<AstNode *> &children)
{
    if (node->IsExpression()) {
        return node;
    }
    for (auto *child : children) {
        if (child->IsExpression()) {
            return child;
        }
    }
    return nullptr;
}

AstNode *FindFirstExpressionAfter(AstNode *node, AstNode *after, const ArenaVector<AstNode *> &children)
{
    if (node->IsExpression() && node != after) {
        return node;
    }
    bool foundAfter = false;
    for (auto *child : children) {
        if (child == after) {
            foundAfter = true;
            continue;
        }
        if (foundAfter && child->IsExpression()) {
            return child;
        }
    }
    return nullptr;
}

AstNode *FindNodeOfType(AstNode *node, AstNodeType type, const ArenaVector<AstNode *> &children)
{
    if (node->Type() == type) {
        return node;
    }
    for (auto *child : children) {
        if (child->Type() == type) {
            return child;
        }
    }
    return nullptr;
}

AstNode *FindTypeReference(AstNode *node, const ArenaVector<AstNode *> &children)
{
    if (node != nullptr && node->IsTSTypeReference()) {
        return node;
    }
    for (auto *child : children) {
        if (child->IsTSTypeReference()) {
            return child;
        }
    }
    return nullptr;
}

AstNode *FindTypeParameter(AstNode *node, const ArenaVector<AstNode *> &children)
{
    if (node->IsTSTypeParameterDeclaration()) {
        return node;
    }

    for (auto *child : children) {
        if (child->IsTSTypeParameterDeclaration()) {
            return child;
        }
        if (child->IsIdentifier() && child->AsIdentifier()->Name() == "T") {
            return child;
        }
    }

    return nullptr;
}

AstNode *FindArrayType(AstNode *node, const ArenaVector<AstNode *> &children)
{
    if (node->IsTSArrayType()) {
        return node;
    }
    for (auto *child : children) {
        if (child->IsTSArrayType()) {
            return child;
        }
    }
    return nullptr;
}

}  // namespace ark::es2panda::lsp
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

#include <vector>
#include <cassert>
#include "internal_api.h"
#include "ir/astNodeFlags.h"
#include "public/es2panda_lib.h"
#include "get_adjusted_location.h"
#include "libarkbase/utils/arena_containers.h"

using ark::es2panda::ir::AstNode;
using ark::es2panda::ir::AstNodeType;
using ark::es2panda::ir::ModifierFlags;

namespace ark::es2panda::lsp {
static inline bool IsOuterExpressionKind(AstNodeType t)
{
    return t == AstNodeType::TS_TYPE_ASSERTION || t == AstNodeType::TS_AS_EXPRESSION ||
           t == AstNodeType::TS_NON_NULL_EXPRESSION;
}

static inline bool IsMirroredSimpleKind(AstNodeType t)
{
    switch (t) {
        case AstNodeType::NEW_EXPRESSION:
        case AstNodeType::TS_VOID_KEYWORD:
        case AstNodeType::TYPEOF_EXPRESSION:
        case AstNodeType::AWAIT_EXPRESSION:
        case AstNodeType::YIELD_EXPRESSION:
            return true;
        default:
            return false;
    }
}

ArenaVector<AstNode *> GetChildren(AstNode *node, ArenaAllocator *allocator)
{
    ArenaVector<AstNode *> children(allocator->Adapter());
    if (node == nullptr) {
        return children;
    }
    node->IterateRecursively([&children](const AstNode *cur) {
        if (cur != nullptr) {
            children.push_back(const_cast<AstNode *>(cur));
        }
    });
    return children;
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

bool IsOuterExpression(const AstNode *node)
{
    return (node != nullptr) && IsOuterExpressionKind(node->Type());
}

AstNode *SkipOuterExpressions(AstNode *node)
{
    while (node != nullptr && node->IsExpression() && IsOuterExpressionKind(node->Type())) {
        AstNode *inner = node->FindChild(
            [](AstNode *c) { return c->IsExpression() || c->IsIdentifier() || c->IsTSTypeReference(); });
        if (inner == nullptr) {
            break;
        }
        node = inner;
    }
    return node;
}

AstNode *FindFirstIdentifier(AstNode *node, bool skipModifiers, const ArenaVector<AstNode *> & /*children*/)
{
    if (node == nullptr) {
        return nullptr;
    }
    if (node->IsIdentifier() && (!skipModifiers || !IsModifier(node))) {
        return node;
    }
    return node->FindChild([&](AstNode *n) { return n->IsIdentifier() && (!skipModifiers || !IsModifier(n)); });
}

AstNode *FindFirstExpression(AstNode *node, const ArenaVector<AstNode *> & /*children*/)
{
    if (node == nullptr) {
        return nullptr;
    }
    if (node->IsExpression()) {
        return node;
    }
    return node->FindChild([](AstNode *n) { return n->IsExpression(); });
}

AstNode *FindFirstExpressionAfter(AstNode *node, AstNode *after, const ArenaVector<AstNode *> & /*children*/)
{
    if (node == nullptr) {
        return nullptr;
    }
    bool seen = false;
    AstNode *found = nullptr;
    node->IterateRecursivelyPreorder([&](AstNode *n) {
        if (found != nullptr) {
            return;
        }
        if (!seen) {
            if (n == after) {
                seen = true;
            }
            return;
        }
        if (n->IsExpression()) {
            found = n;
        }
    });
    return found;
}

AstNode *FindNodeOfType(AstNode *node, AstNodeType type, const ArenaVector<AstNode *> & /*children*/)
{
    if (node == nullptr) {
        return nullptr;
    }
    if (node->Type() == type) {
        return node;
    }
    return node->FindChild([&](AstNode *n) { return n->Type() == type; });
}

AstNode *FindTypeReference(AstNode *node, const ArenaVector<AstNode *> & /*children*/)
{
    if (node == nullptr) {
        return nullptr;
    }
    if (node->IsTSTypeReference()) {
        return node;
    }
    return node->FindChild([](AstNode *n) { return n->IsTSTypeReference(); });
}

AstNode *FindTypeParameter(AstNode *node, const ArenaVector<AstNode *> & /*children*/)
{
    if (node == nullptr) {
        return nullptr;
    }
    if (node->IsTSTypeParameterDeclaration()) {
        return node;
    }
    return node->FindChild([](AstNode *n) { return n->IsTSTypeParameterDeclaration(); });
}

AstNode *FindArrayType(AstNode *node, const ArenaVector<AstNode *> & /*children*/)
{
    if (node == nullptr) {
        return nullptr;
    }
    if (node->IsTSArrayType()) {
        return node;
    }
    return node->FindChild([](AstNode *n) { return n->IsTSArrayType(); });
}

bool IsDeclarationOrModifier(AstNode *node, AstNode *parent)
{
    if (node == nullptr || parent == nullptr) {
        return false;
    }
    return (
        (IsModifier(node) && CanHaveModifiers(*parent) && ((parent->Modifiers() & node->Modifiers()) != 0U)) ||
        (node->IsTSClassImplements() && (parent->IsClassDeclaration() || parent->IsClassExpression())) ||
        (node->IsFunctionDeclaration() && (parent->IsFunctionDeclaration() || parent->IsFunctionExpression())) ||
        (node->IsTSInterfaceDeclaration() && parent->IsTSInterfaceDeclaration()) ||
        (node->IsTSEnumDeclaration() && parent->IsTSEnumDeclaration()) ||
        (node->IsTSTypeAliasDeclaration() && parent->IsTSTypeAliasDeclaration()) ||
        ((node->IsImportNamespaceSpecifier() || node->IsTSModuleDeclaration()) && parent->IsTSModuleDeclaration()) ||
        (node->IsTSImportEqualsDeclaration() && parent->IsTSImportEqualsDeclaration()));
}

std::optional<AstNode *> GetAdjustedLocationForClass(AstNode *node, ArenaAllocator *allocator)
{
    if (node == nullptr || allocator == nullptr) {
        return std::nullopt;
    }
    if (!node->IsClassDeclaration() && !node->IsClassExpression()) {
        return std::nullopt;
    }
    ArenaVector<AstNode *> dummy(allocator->Adapter());
    AstNode *id = FindFirstIdentifier(node, false, dummy);
    if (id != nullptr) {
        return id;
    }
    return std::nullopt;
}

static AstNode *FunctionDeclSelfOrParent(AstNode *n)
{
    if (n == nullptr) {
        return nullptr;
    }
    if (n->IsFunctionDeclaration()) {
        return n;
    }
    AstNode *p = n->Parent();
    if (p != nullptr && p->IsFunctionDeclaration()) {
        return p;
    }
    return nullptr;
}

std::optional<AstNode *> GetAdjustedLocationForFunction(AstNode *node, ArenaAllocator *allocator)
{
    if (node == nullptr || allocator == nullptr) {
        return std::nullopt;
    }

    if (node->IsIdentifier()) {
        return node;
    }

    AstNode *fn = FunctionDeclSelfOrParent(node);
    if (fn == nullptr) {
        return std::nullopt;
    }
    // CC-OFFNXT(G.NAM.03-CPP) project code style
    constexpr bool K_SKIP_MODIFIERS = false;
    ArenaVector<AstNode *> dummy(allocator->Adapter());
    AstNode *id = FindFirstIdentifier(fn, K_SKIP_MODIFIERS, dummy);
    return (id != nullptr) ? std::optional<AstNode *> {id} : std::nullopt;
}

std::optional<AstNode *> GetAdjustedLocationForDeclaration(AstNode *node, const ArenaVector<AstNode *> &children,
                                                           ArenaAllocator *allocator)
{
    switch (node->Type()) {
        case AstNodeType::CLASS_DECLARATION:
        case AstNodeType::CLASS_EXPRESSION:
        case AstNodeType::STRUCT_DECLARATION:
            return GetAdjustedLocationForClass(node, allocator);
        case AstNodeType::FUNCTION_DECLARATION:
        case AstNodeType::FUNCTION_EXPRESSION:
            return GetAdjustedLocationForFunction(node, allocator);
        case AstNodeType::TS_CONSTRUCTOR_TYPE:
            return node;
        default:
            break;
    }
    if (node->IsExportNamedDeclaration()) {
        AstNode *id = FindFirstIdentifier(node, false, children);
        if (id != nullptr) {
            return id;
        }
    }
    return std::nullopt;
}

std::optional<AstNode *> GetAdjustedLocationForImportDeclaration(AstNode *node, const ArenaVector<AstNode *> &children)
{
    if (!node->IsImportDeclaration()) {
        return std::nullopt;
    }
    AstNode *spec =
        node->FindChild([](AstNode *c) { return c->IsImportSpecifier() || c->IsImportNamespaceSpecifier(); });
    if (spec == nullptr) {
        return std::nullopt;
    }
    AstNode *id = FindFirstIdentifier(spec, false, children);
    if (id != nullptr) {
        return id;
    }
    return std::make_optional(spec);
}

std::optional<AstNode *> GetAdjustedLocationForExportDeclaration(AstNode *node, const ArenaVector<AstNode *> &children)
{
    if (!node->IsExportAllDeclaration()) {
        return std::nullopt;
    }
    AstNode *spec = FindNodeOfType(node, AstNodeType::EXPORT_SPECIFIER, children);
    if (spec != nullptr) {
        AstNode *id = FindFirstIdentifier(spec, false, children);
        if (id != nullptr) {
            return id;
        }
        return spec;
    }
    return std::nullopt;
}

std::optional<AstNode *> GetAdjustedLocationForHeritageClause(AstNode *node)
{
    if (node == nullptr || node->Type() != AstNodeType::TS_INTERFACE_HERITAGE) {
        return std::nullopt;
    }
    AstNode *expr =
        node->FindChild([](AstNode *c) { return c->IsExpression() || c->IsIdentifier() || c->IsTSTypeReference(); });
    if (expr != nullptr) {
        return expr;
    }
    return std::nullopt;
}

static std::optional<AstNode *> TryTSAsExpression(AstNode *node, AstNode *parent,
                                                  const ArenaVector<AstNode *> &parentChildren)
{
    if (node == nullptr || parent == nullptr) {
        return std::nullopt;
    }
    if (!node->IsTSAsExpression()) {
        return std::nullopt;
    }
    if (parent->IsImportSpecifier() || parent->IsExportSpecifier() || parent->IsImportNamespaceSpecifier()) {
        if (AstNode *id = FindFirstIdentifier(parent, false, parentChildren)) {
            return id;
        }
        return std::nullopt;
    }
    if (parent->IsExportAllDeclaration()) {
        if (AstNode *spec = FindNodeOfType(parent, AstNodeType::EXPORT_SPECIFIER, parentChildren)) {
            if (AstNode *id = FindFirstIdentifier(spec, false, parentChildren)) {
                return id;
            }
        }
    }
    return std::nullopt;
}

static std::optional<AstNode *> TryTSImportType(AstNode *node, AstNode *parent,
                                                const ArenaVector<AstNode *> &parentChildren, ArenaAllocator *allocator)
{
    if (node == nullptr || parent == nullptr) {
        return std::nullopt;
    }
    if (!node->IsTSImportType()) {
        return std::nullopt;
    }
    if (AstNode *pp = parent->Parent()) {
        if (auto loc = GetAdjustedLocationForDeclaration(pp, parentChildren, allocator)) {
            return loc;
        }
    }
    if (parent->IsExportAllDeclaration()) {
        if (auto loc = GetAdjustedLocationForExportDeclaration(parent, parentChildren)) {
            return loc;
        }
    }
    return std::nullopt;
}

static std::optional<AstNode *> TryTSHeritageAndInfer(AstNode *node, AstNode *parent,
                                                      const ArenaVector<AstNode *> &parentChildren)
{
    if (node == nullptr || parent == nullptr) {
        return std::nullopt;
    }
    if (node->IsTSClassImplements() && parent->IsTSClassImplements()) {
        if (auto loc = GetAdjustedLocationForHeritageClause(parent)) {
            return loc;
        }
    }
    if (node->IsTSInferType() && parent->IsTSInferType()) {
        if (AstNode *tp = FindTypeParameter(parent, parentChildren)) {
            if (AstNode *id = FindFirstIdentifier(tp, false, parentChildren)) {
                return id;
            }
        }
    }
    if (parent->IsTSTypeParameterDeclaration()) {
        if (AstNode *id = FindFirstIdentifier(parent, false, parentChildren)) {
            return id;
        }
    }
    return std::nullopt;
}

static std::optional<AstNode *> TryTypeOperatorFamily(AstNode *parent, const ArenaVector<AstNode *> &parentChildren)
{
    if (parent == nullptr) {
        return std::nullopt;
    }
    if (!parent->IsTSTypeOperator()) {
        return std::nullopt;
    }
    if (AstNode *ref = FindTypeReference(parent, parentChildren)) {
        if (AstNode *id = FindFirstIdentifier(ref, false, parentChildren)) {
            return id;
        }
    }
    if (AstNode *arr = FindArrayType(parent, parentChildren)) {
        if (AstNode *elem = FindTypeReference(arr, parentChildren)) {
            if (AstNode *id = FindFirstIdentifier(elem, false, parentChildren)) {
                return id;
            }
        }
    }
    return std::nullopt;
}

static std::optional<AstNode *> TryDeclaration(AstNode *node, AstNode *parent,
                                               const ArenaVector<AstNode *> &parentChildren, ArenaAllocator *allocator)
{
    if (!IsDeclarationOrModifier(node, parent)) {
        return std::nullopt;
    }
    return GetAdjustedLocationForDeclaration(parent, parentChildren, allocator);
}

static std::optional<AstNode *> TryExpressions(AstNode *node, AstNode *parent,
                                               const ArenaVector<AstNode *> &parentChildren, ArenaAllocator *allocator)
{
    if (auto asExpr = TryTSAsExpression(node, parent, parentChildren)) {
        return asExpr;
    }
    if (auto importType = TryTSImportType(node, parent, parentChildren, allocator)) {
        return importType;
    }
    if (auto heritageOrInfer = TryTSHeritageAndInfer(node, parent, parentChildren)) {
        return heritageOrInfer;
    }
    if (auto typeOperator = TryTypeOperatorFamily(parent, parentChildren)) {
        return typeOperator;
    }
    return std::nullopt;
}

static std::optional<AstNode *> TryImportsAndExports(AstNode *parent, const ArenaVector<AstNode *> &parentChildren)
{
    if (auto importDecl = GetAdjustedLocationForImportDeclaration(parent, parentChildren)) {
        return importDecl;
    }
    if (parent->IsExportAllDeclaration()) {
        if (auto exportDecl = GetAdjustedLocationForExportDeclaration(parent, parentChildren)) {
            return exportDecl;
        }
    }
    return std::nullopt;
}

static std::optional<AstNode *> TryModuleOrVariable(AstNode *parent, const ArenaVector<AstNode *> &parentChildren)
{
    if (parent->IsTSExternalModuleReference()) {
        if (AstNode *expr = FindFirstExpression(parent, parentChildren)) {
            return expr;
        }
    }
    if (parent->IsImportDeclaration() || parent->IsExportAllDeclaration()) {
        if (AstNode *lit = FindNodeOfType(parent, AstNodeType::STRING_LITERAL, parentChildren)) {
            return lit;
        }
    }
    if (parent->IsVariableDeclaration()) {
        if (AstNode *id = FindFirstIdentifier(parent, false, parentChildren)) {
            return id;
        }
    }
    return std::nullopt;
}

static std::optional<AstNode *> TryConvenienceExpressions(AstNode *node, AstNode *parent,
                                                          const ArenaVector<AstNode *> &parentChildren)
{
    if (node == nullptr || parent == nullptr) {
        return std::nullopt;
    }

    const AstNodeType nt = node->Type();
    const AstNodeType pt = parent->Type();
    if (nt == pt && IsMirroredSimpleKind(nt)) {
        if (AstNode *expr = FindFirstExpression(parent, parentChildren)) {
            return SkipOuterExpressions(expr);
        }
        return std::nullopt;
    }

    if (parent->IsBinaryExpression() && node->IsTSTypeOperator()) {
        AstNode *lhs = FindFirstExpression(parent, parentChildren);
        if (lhs == nullptr) {
            return std::nullopt;
        }
        AstNode *rhs = FindFirstExpressionAfter(parent, lhs, parentChildren);
        if (rhs == nullptr) {
            return std::nullopt;
        }
        return SkipOuterExpressions(rhs);
    }

    const bool isForPair = (nt == AstNodeType::FOR_IN_STATEMENT && pt == AstNodeType::FOR_IN_STATEMENT) ||
                           (nt == AstNodeType::FOR_OF_STATEMENT && pt == AstNodeType::FOR_OF_STATEMENT);
    if (isForPair) {
        if (AstNode *expr = FindFirstExpression(parent, parentChildren)) {
            return SkipOuterExpressions(expr);
        }
        return std::nullopt;
    }

    return std::nullopt;
}

std::optional<AstNode *> GetAdjustedLocation(AstNode *node, ArenaAllocator *allocator)
{
    if (node == nullptr) {
        return std::nullopt;
    }

    node = GetOriginalNode(node);

    AstNode *parent = node->Parent();
    if (parent == nullptr) {
        return node;
    }

    ArenaVector<AstNode *> parentChildren = GetChildren(parent, allocator);
    if (auto r = TryDeclaration(node, parent, parentChildren, allocator)) {
        return r;
    }
    if (auto r = TryExpressions(node, parent, parentChildren, allocator)) {
        return r;
    }
    if (auto r = TryImportsAndExports(parent, parentChildren)) {
        return r;
    }
    if (auto r = TryModuleOrVariable(parent, parentChildren)) {
        return r;
    }
    if (auto r = TryConvenienceExpressions(node, parent, parentChildren)) {
        return r;
    }

    return node;
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
}  // namespace ark::es2panda::lsp
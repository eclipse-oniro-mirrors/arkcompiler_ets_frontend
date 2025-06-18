/**
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

#include "varbinder/variableFlags.h"
#include "checker/types/ets/etsTupleType.h"
#include "checker/ETSchecker.h"

namespace ark::es2panda::checker {
void ETSChecker::ValidatePropertyAccess(varbinder::Variable *var, ETSObjectType *obj, const lexer::SourcePosition &pos)
{
    if ((Context().Status() & CheckerStatus::IGNORE_VISIBILITY) != 0U) {
        return;
    }
    if (var->HasFlag(varbinder::VariableFlags::METHOD)) {
        return;
    }

    if (var->HasFlag(varbinder::VariableFlags::PRIVATE) || var->HasFlag(varbinder::VariableFlags::PROTECTED)) {
        if ((Context().ContainingClass() == obj ||
             Context().ContainingClass()->GetOriginalBaseType() == obj->GetOriginalBaseType()) &&
            obj->IsPropertyInherited(var)) {
            return;
        }

        if (var->HasFlag(varbinder::VariableFlags::PROTECTED) && Context().ContainingClass()->IsDescendantOf(obj) &&
            obj->IsPropertyInherited(var)) {
            return;
        }

        auto *currentOutermost = Context().ContainingClass()->OutermostClass();
        auto *objOutermost = obj->OutermostClass();

        if (currentOutermost != nullptr && objOutermost != nullptr && currentOutermost == objOutermost &&
            obj->IsPropertyInherited(var)) {
            return;
        }

        std::ignore = TypeError(var, diagnostic::PROP_INVISIBLE, {var->Name()}, pos);
    }
}

bool ETSChecker::IsStaticInvoke(ir::MemberExpression *const expr)
{
    ir::Identifier *ident = nullptr;
    if (expr->Object()->IsIdentifier()) {
        ident = expr->Object()->AsIdentifier();
    } else if (expr->Object()->IsMemberExpression()) {
        auto object = expr->Object();

        while (object->IsMemberExpression()) {
            object = object->AsMemberExpression()->Object();
        }
        if (object->IsIdentifier()) {
            ident = object->AsIdentifier();
        }
    }

    return (ident != nullptr &&
            (ident->Variable()->Declaration()->IsClassDecl() || ident->Variable()->Declaration()->IsImportDecl()));
}

void ETSChecker::ValidateCallExpressionIdentifier(ir::Identifier *const ident, Type *const type)
{
    ir::CallExpression *callExpr = nullptr;
    if (ident->Parent()->IsMemberExpression() && IsStaticInvoke(ident->Parent()->AsMemberExpression())) {
        callExpr = ident->Parent()->Parent()->AsCallExpression();
    } else if (ident->Parent()->IsCallExpression()) {
        callExpr = ident->Parent()->AsCallExpression();
    } else {
        return;
    }

    if (ident->Variable()->HasFlag(varbinder::VariableFlags::CLASS_OR_INTERFACE)) {
        if (callExpr->Callee()->Variable() != nullptr && callExpr->Callee()->Variable()->Name().Is("with")) {
            LogError(diagnostic::ERROR_ARKTS_NO_WITH, callExpr->Start());
        }
    }

    if (ident->Variable()->HasFlag(varbinder::VariableFlags::CLASS_OR_INTERFACE) && callExpr->Callee() != ident &&
        callExpr->Callee() != ident->Parent()) {
        std::ignore =
            TypeError(ident->Variable(), diagnostic::CLASS_OR_IFACE_AS_OBJ, {ident->ToString()}, ident->Start());
    }

    if (callExpr->Callee() != ident && callExpr->Callee() != ident->Parent()) {
        return;
    }

    ES2PANDA_ASSERT(ident->Variable() != nullptr);
    if (ident->Variable()->Declaration()->Node() != nullptr &&
        ident->Variable()->Declaration()->Node()->IsOverloadDeclaration()) {
        return;
    }
    if (ident->Variable()->Declaration()->Node() != nullptr &&
        ident->Variable()->Declaration()->Node()->IsImportNamespaceSpecifier()) {
        std::ignore = TypeError(ident->Variable(), diagnostic::NAMESPACE_CALL, {ident->ToString()}, ident->Start());
    }
    if (type->IsETSFunctionType()) {
        return;
    }
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    if (TryTransformingToStaticInvoke(ident, type)) {
        return;
    }

    std::ignore = TypeError(ident->Variable(), diagnostic::EXPR_NOT_CALLABLE, {}, ident->Start());
}

void ETSChecker::ValidateNewClassInstanceIdentifier(ir::Identifier *const ident)
{
    auto const resolved = ident->Variable();
    ES2PANDA_ASSERT(resolved != nullptr);

    if (ident->Parent()->AsETSNewClassInstanceExpression()->GetTypeRef() == ident &&
        !resolved->HasFlag(varbinder::VariableFlags::CLASS_OR_INTERFACE) && !resolved->TsType()->IsTypeError()) {
        LogError(diagnostic::REF_INVALID, {ident->Name()}, ident->Start());
    }
}

void ETSChecker::ValidateMemberIdentifier(ir::Identifier *const ident)
{
    auto const resolved = ident->Variable();
    // Handles Enum[enumVar] MemberExpression
    if (resolved->Declaration()->Node()->IsClassDefinition() &&
        resolved->Declaration()->Node()->AsClassDefinition()->TsType()->IsETSEnumType() &&
        ident->Parent()->AsMemberExpression()->HasMemberKind(ir::MemberExpressionKind::ELEMENT_ACCESS)) {
        return;
    }
    if (ident->Parent()->AsMemberExpression()->IsComputed()) {
        if ((resolved != nullptr) && !resolved->Declaration()->PossibleTDZ()) {
            WrongContextErrorClassifyByType(ident);
        }
        return;
    }
}

void ETSChecker::ValidateAssignmentIdentifier(ir::Identifier *const ident, Type *const type)
{
    auto const resolved = ident->Variable();
    const auto *const assignmentExpr = ident->Parent()->AsAssignmentExpression();
    if (assignmentExpr->Left() == ident && (resolved != nullptr) && !resolved->Declaration()->PossibleTDZ()) {
        WrongContextErrorClassifyByType(ident);
        return;
    }

    if (assignmentExpr->Right() == ident && (resolved != nullptr) &&
        (!resolved->Declaration()->PossibleTDZ() && !type->IsETSFunctionType())) {
        WrongContextErrorClassifyByType(ident);
        return;
    }
}

bool ETSChecker::ValidateBinaryExpressionIdentifier(ir::Identifier *const ident, Type *const type)
{
    auto const resolved = ident->Variable();
    const auto *const binaryExpr = ident->Parent()->AsBinaryExpression();
    bool isFinished = false;

    bool isInstanceOfKeyword = binaryExpr->OperatorType() == lexer::TokenType::KEYW_INSTANCEOF;
    if (isInstanceOfKeyword && binaryExpr->Left() == ident) {
        if (!IsReferenceType(type)) {
            std::ignore =
                TypeError(ident->Variable(), diagnostic::INSTANCEOF_NONOBJECT, {ident->Name()}, ident->Start());
        }
        isFinished = true;
    }

    if (binaryExpr->OperatorType() == lexer::TokenType::PUNCTUATOR_NULLISH_COALESCING ||
        binaryExpr->OperatorType() == lexer::TokenType::PUNCTUATOR_LOGICAL_OR ||
        binaryExpr->OperatorType() == lexer::TokenType::PUNCTUATOR_LOGICAL_AND) {
        if ((resolved != nullptr) && (!resolved->Declaration()->PossibleTDZ() && !type->IsETSFunctionType())) {
            WrongContextErrorClassifyByType(ident);
        }
        isFinished = true;
    }
    if (isInstanceOfKeyword && ident->Variable()->HasFlag(varbinder::VariableFlags::CLASS_OR_INTERFACE |
                                                          varbinder::VariableFlags::TYPE_ALIAS)) {
        LogError(diagnostic::WRONG_LEFT_OF_INSTANCEOF, {}, ident->Start());
    }
    return isFinished;
}

void ETSChecker::ValidateResolvedIdentifier(ir::Identifier *const ident)
{
    varbinder::Variable *const resolved = ident->Variable();
    if (resolved->Declaration()->IsAnnotationDecl() && !ident->IsAnnotationUsage()) {
        LogError(diagnostic::ANNOT_WITHOUT_AT, {}, ident->Start());
    }

    auto *smartType = Context().GetSmartCast(resolved);
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto *const resolvedType = GetApparentType(smartType != nullptr ? smartType : GetTypeOfVariable(resolved));
    bool isValidResolved =
        resolved != nullptr && !resolved->Declaration()->PossibleTDZ() &&
        !(resolvedType->IsETSFunctionType() && !resolved->Declaration()->Node()->IsTSTypeAliasDeclaration());
    switch (ident->Parent()->Type()) {
        case ir::AstNodeType::CALL_EXPRESSION:
            // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
            ValidateCallExpressionIdentifier(ident, resolvedType);
            break;
        case ir::AstNodeType::ETS_NEW_CLASS_INSTANCE_EXPRESSION:
            ValidateNewClassInstanceIdentifier(ident);
            break;
        case ir::AstNodeType::MEMBER_EXPRESSION:
            ValidateMemberIdentifier(ident);
            break;
        case ir::AstNodeType::BINARY_EXPRESSION:
            if (ValidateBinaryExpressionIdentifier(ident, resolvedType)) {
                return;
            }
            if (isValidResolved) {
                WrongContextErrorClassifyByType(ident);
            }
            break;
        case ir::AstNodeType::UPDATE_EXPRESSION:
        case ir::AstNodeType::UNARY_EXPRESSION:
            if (resolved != nullptr && !resolved->Declaration()->PossibleTDZ()) {
                WrongContextErrorClassifyByType(ident);
            }
            break;
        case ir::AstNodeType::ASSIGNMENT_EXPRESSION:
            ValidateAssignmentIdentifier(ident, resolvedType);
            break;
        default:
            if (isValidResolved) {
                WrongContextErrorClassifyByType(ident);
            }
    }
}

void ETSChecker::ValidateUnaryOperatorOperand(varbinder::Variable *variable, ir::Expression *expr)
{
    if (IsVariableGetterSetter(variable)) {
        return;
    }

    if (variable->Declaration()->IsConstDecl() || variable->Declaration()->IsReadonlyDecl()) {
        std::string_view fieldType = variable->Declaration()->IsConstDecl() ? "constant" : "readonly";
        if ((HasStatus(CheckerStatus::IN_CONSTRUCTOR | CheckerStatus::IN_STATIC_BLOCK) &&
             !variable->HasFlag(varbinder::VariableFlags::EXPLICIT_INIT_REQUIRED)) ||
            (variable->HasFlag(varbinder::VariableFlags::INIT_IN_STATIC_BLOCK) &&
             variable->HasFlag(varbinder::VariableFlags::INITIALIZED))) {
            std::ignore = TypeError(variable, diagnostic::FIELD_REASSIGNMENT, {fieldType, variable->Name()},
                                    variable->Declaration()->Node()->Start());
            return;
        }
        if (!HasStatus(CheckerStatus::IN_CONSTRUCTOR | CheckerStatus::IN_STATIC_BLOCK)) {
            const auto &diagKind = variable->Declaration()->IsConstDecl() ? diagnostic::FIELD_ASSIGN_TO_CONST
                                                                          : diagnostic::FIELD_ASSIGN_TO_READONLY;
            std::ignore = TypeError(variable, diagKind, {variable->Name()}, expr->Start());
        }

        if (variable->HasFlag(varbinder::VariableFlags::INIT_IN_STATIC_BLOCK)) {
            variable->AddFlag(varbinder::VariableFlags::INITIALIZED);
        }
    }
}

void ETSChecker::ValidateGenericTypeAliasForClonedNode(ir::TSTypeAliasDeclaration *const typeAliasNode,
                                                       const ir::TSTypeParameterInstantiation *const exactTypeParams)
{
    static std::string_view const TRANSFORMATION_NAME = __func__;

    RecursionPreserver<ir::TSTypeAliasDeclaration> recursionPreserver(elementStack_, typeAliasNode);

    if (*recursionPreserver) {
        // NOTE(SM): We in recursion for type alias. So isn't make sense check the same type alias twice;
        return;
    }

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto *const clonedNode = typeAliasNode->TypeAnnotation()->Clone(Allocator(), typeAliasNode);

    // Basic check, we really don't want to change the original type nodes, more precise checking should be made
    ES2PANDA_ASSERT(clonedNode != typeAliasNode->TypeAnnotation());

    // Currently only reference types are checked. This should be extended for other types in a follow up patch, but for
    // complete usability, if the type isn't a simple reference type, then doN't check type alias declaration at all.
    bool checkTypealias = true;

    // Only transforming a temporary cloned node, so no modification is made in the AST
    clonedNode->TransformChildrenRecursively(
        // CC-OFFNXT(G.FMT.14-CPP) project code style
        [this, &checkTypealias, &exactTypeParams, typeAliasNode](ir::AstNode *const node) -> ir::AstNode * {
            if (!node->IsETSTypeReference()) {
                return node;
            }

            const auto *const nodeIdent = node->AsETSTypeReference()->Part()->GetIdent();

            size_t typeParamIdx = 0;
            for (const auto *const typeParam : typeAliasNode->TypeParams()->Params()) {
                if (typeParam->Name()->AsIdentifier()->Variable() == nodeIdent->Variable()) {
                    break;
                }
                typeParamIdx++;
            }

            if (typeParamIdx == typeAliasNode->TypeParams()->Params().size()) {
                return node;
            }

            ir::TypeNode *typeParamType = nullptr;

            if (exactTypeParams != nullptr && exactTypeParams->Params().size() > typeParamIdx) {
                typeParamType = exactTypeParams->Params().at(typeParamIdx);
            } else {
                typeParamType = typeAliasNode->TypeParams()->Params().at(typeParamIdx)->DefaultType();
            }

            if (!typeParamType->IsETSTypeReference()) {
                checkTypealias = false;
                return node;
            }

            // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
            return typeParamType->Clone(Allocator(), nullptr);
        },
        TRANSFORMATION_NAME);

    if (checkTypealias) {
        clonedNode->Check(this);
    }
}

bool ETSChecker::IsArrayExprSizeValidForTuple(const ir::ArrayExpression *const arrayExpr,
                                              const ETSTupleType *const tuple)
{
    std::size_t size = 0;

    for (ir::Expression *const element : arrayExpr->Elements()) {
        if (element->IsSpreadElement()) {
            const Type *const argType = element->AsSpreadElement()->Argument()->Check(this);
            if (argType->IsETSTupleType()) {
                size += argType->AsETSTupleType()->GetTupleSize();
                continue;
            }
            LogError(diagnostic::INVALID_SPREAD_IN_TUPLE, {argType}, element->Start());
        }
        ++size;
    }

    if (size != tuple->GetTupleSize()) {
        LogError(diagnostic::TUPLE_WRONG_NUMBER_OF_ELEMS, {size, tuple->GetTupleSize()}, arrayExpr->Start());
        return false;
    }

    return true;
}
}  // namespace ark::es2panda::checker

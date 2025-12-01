/*
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

#include "TSAnalyzer.h"

#include "checker/TSchecker.h"
#include "checker/ts/destructuringContext.h"

namespace ark::es2panda::checker {

TSChecker *TSAnalyzer::GetTSChecker() const
{
    return static_cast<TSChecker *>(GetChecker());
}

// from base folder
checker::Type *TSAnalyzer::Check(ir::CatchClause *st) const
{
    TSChecker *checker = GetTSChecker();
    ir::Expression *typeAnnotation = st->Param()->AsAnnotatedExpression()->TypeAnnotation();

    if (typeAnnotation != nullptr) {
        checker::Type *catchParamType = typeAnnotation->Check(checker);

        if (!catchParamType->HasTypeFlag(checker::TypeFlag::ANY_OR_UNKNOWN)) {
            checker->ThrowTypeError("Catch clause variable type annotation must be 'any' or 'unknown' if specified",
                                    st->Start());
        }
    }

    st->Body()->Check(checker);

    return nullptr;
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::ClassDefinition *node) const
{
    TSChecker *checker = GetTSChecker();
    // NOTE: aszilagyi.
    return checker->GlobalAnyType();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::MetaProperty *expr) const
{
    TSChecker *checker = GetTSChecker();
    // NOTE: aszilagyi.
    return checker->GlobalAnyType();
}

checker::Type *TSAnalyzer::Check(ir::TSIndexSignature *node) const
{
    TSChecker *checker = GetTSChecker();
    if (node->TsType() != nullptr) {
        return node->TsType();
    }

    const util::StringView &paramName = node->Param()->AsIdentifier()->Name();
    node->typeAnnotation_->Check(checker);
    checker::Type *indexType = node->typeAnnotation_->GetType(checker);
    checker::IndexInfo *info =
        checker->Allocator()->New<checker::IndexInfo>(indexType, paramName, node->Readonly(), node->Start());
    checker::ObjectDescriptor *desc = checker->Allocator()->New<checker::ObjectDescriptor>(checker->Allocator());
    checker::ObjectType *placeholder = checker->Allocator()->New<checker::ObjectLiteralType>(desc);

    ES2PANDA_ASSERT(placeholder != nullptr);
    if (node->Kind() == ir::TSIndexSignature::TSIndexSignatureKind::NUMBER) {
        placeholder->Desc()->numberIndexInfo = info;
    } else {
        placeholder->Desc()->stringIndexInfo = info;
    }

    node->SetTsType(placeholder);
    return placeholder;
}

checker::Type *TSAnalyzer::Check(ir::TSMethodSignature *node) const
{
    TSChecker *checker = GetTSChecker();
    if (node->Computed()) {
        checker->CheckComputedPropertyName(node->Key());
    }

    checker::ScopeContext scopeCtx(checker, node->Scope());

    auto *signatureInfo = checker->Allocator()->New<checker::SignatureInfo>(checker->Allocator());
    checker->CheckFunctionParameterDeclarations(node->Params(), signatureInfo);

    auto *callSignature = checker->Allocator()->New<checker::Signature>(signatureInfo, checker->GlobalAnyType());
    node->Variable()->SetTsType(checker->CreateFunctionTypeWithSignature(callSignature));

    auto returnType = node->ReturnTypeAnnotation();
    if (returnType == nullptr) {
        checker->ThrowTypeError(
            "Method signature, which lacks return-type annotation, implicitly has an 'any' return type.",
            node->Start());
    }

    returnType->Check(checker);
    ES2PANDA_ASSERT(callSignature != nullptr);
    callSignature->SetReturnType(returnType->GetType(checker));

    return nullptr;
}

checker::Type *TSAnalyzer::Check(ir::TSPropertySignature *node) const
{
    TSChecker *checker = GetTSChecker();
    if (node->TypeAnnotation() != nullptr) {
        node->TypeAnnotation()->Check(checker);
    }

    if (node->Computed()) {
        checker->CheckComputedPropertyName(node->Key());
    }

    if (node->TypeAnnotation() != nullptr) {
        node->Variable()->SetTsType(node->TypeAnnotation()->GetType(checker));
        return nullptr;
    }

    checker->ThrowTypeError("Property implicitly has an 'any' type.", node->Start());
    return nullptr;
}

checker::Type *TSAnalyzer::Check(ir::TSSignatureDeclaration *node) const
{
    TSChecker *checker = GetTSChecker();
    if (node->TsType() != nullptr) {
        return node->TsType();
    }

    checker::ScopeContext scopeCtx(checker, node->Scope());

    auto *signatureInfo = checker->Allocator()->New<checker::SignatureInfo>(checker->Allocator());
    checker->CheckFunctionParameterDeclarations(node->Params(), signatureInfo);

    bool isCallSignature = (node->Kind() == ir::TSSignatureDeclaration::TSSignatureDeclarationKind::CALL_SIGNATURE);

    if (node->ReturnTypeAnnotation() == nullptr) {
        if (isCallSignature) {
            checker->ThrowTypeError(
                "Call signature, which lacks return-type annotation, implicitly has an 'any' return type.",
                node->Start());
        }

        checker->ThrowTypeError(
            "Construct signature, which lacks return-type annotation, implicitly has an 'any' return type.",
            node->Start());
    }

    node->ReturnTypeAnnotation()->Check(checker);
    checker::Type *returnType = node->ReturnTypeAnnotation()->GetType(checker);

    auto *signature = checker->Allocator()->New<checker::Signature>(signatureInfo, returnType);

    checker::Type *placeholderObj = nullptr;

    if (isCallSignature) {
        placeholderObj = checker->CreateObjectTypeWithCallSignature(signature);
    } else {
        placeholderObj = checker->CreateObjectTypeWithConstructSignature(signature);
    }

    node->SetTsType(placeholderObj);
    return placeholderObj;
}

static void GetSpreadElementType(checker::TSChecker *checker, checker::Type *spreadType,
                                 ArenaVector<checker::Type *> &elementTypes, const lexer::SourcePosition &loc)
{
    bool inConstContext = checker->HasStatus(checker::CheckerStatus::IN_CONST_CONTEXT);

    if (spreadType->IsObjectType() && spreadType->AsObjectType()->IsTupleType()) {
        ArenaVector<checker::Type *> tupleElementTypes(checker->Allocator()->Adapter());
        checker::TupleType *spreadTuple = spreadType->AsObjectType()->AsTupleType();

        for (auto *it : spreadTuple->Properties()) {
            if (inConstContext) {
                elementTypes.push_back(it->TsType());
                continue;
            }

            tupleElementTypes.push_back(it->TsType());
        }

        if (inConstContext) {
            return;
        }

        elementTypes.push_back(checker->CreateUnionType(std::move(tupleElementTypes)));
        return;
    }

    if (!spreadType->IsUnionType()) {
        checker->ThrowTypeError(
            {"Type '", spreadType, "' must have a '[Symbol.iterator]()' method that returns an iterator."}, loc);
        return;
    }

    ArenaVector<checker::Type *> spreadTypes(checker->Allocator()->Adapter());
    bool throwError = false;

    for (auto *type : spreadType->AsUnionType()->ConstituentTypes()) {
        if (type->IsArrayType()) {
            spreadTypes.push_back(type->AsArrayType()->ElementType());
            continue;
        }

        if (type->IsObjectType() && type->AsObjectType()->IsTupleType()) {
            checker::TupleType *tuple = type->AsObjectType()->AsTupleType();

            for (auto *it : tuple->Properties()) {
                spreadTypes.push_back(it->TsType());
            }

            continue;
        }

        throwError = true;
        break;
    }

    if (!throwError) {
        elementTypes.push_back(checker->CreateUnionType(std::move(spreadTypes)));
        return;
    }

    checker->ThrowTypeError(
        {"Type '", spreadType, "' must have a '[Symbol.iterator]()' method that returns an iterator."}, loc);
}

checker::Type *TSAnalyzer::Check(ir::ArrayExpression *expr) const
{
    TSChecker *checker = GetTSChecker();
    ArenaVector<checker::Type *> elementTypes(checker->Allocator()->Adapter());
    ArenaVector<checker::ElementFlags> elementFlags(checker->Allocator()->Adapter());
    bool inConstContext = checker->HasStatus(checker::CheckerStatus::IN_CONST_CONTEXT);
    bool createTuple = checker->HasStatus(checker::CheckerStatus::FORCE_TUPLE);

    for (auto *it : expr->Elements()) {
        if (it->IsSpreadElement()) {
            checker::Type *spreadType = it->AsSpreadElement()->Argument()->Check(checker);

            if (spreadType->IsArrayType()) {
                elementTypes.push_back(inConstContext ? spreadType : spreadType->AsArrayType()->ElementType());
                elementFlags.push_back(checker::ElementFlags::VARIADIC);
                continue;
            }

            GetSpreadElementType(checker, spreadType, elementTypes, it->Start());
            elementFlags.push_back(checker::ElementFlags::REST);
            continue;
        }

        checker::Type *elementType = it->Check(checker);

        if (!inConstContext) {
            elementType = checker->GetBaseTypeOfLiteralType(elementType);
        }

        elementFlags.push_back(checker::ElementFlags::REQUIRED);
        elementTypes.push_back(elementType);
    }

    if (inConstContext || createTuple) {
        checker::ObjectDescriptor *desc = checker->Allocator()->New<checker::ObjectDescriptor>(checker->Allocator());
        uint32_t index = 0;

        for (auto it = elementTypes.begin(); it != elementTypes.end(); it++, index++) {
            util::StringView memberIndex = util::Helpers::ToStringView(checker->Allocator(), index);
            varbinder::LocalVariable *tupleMember = varbinder::Scope::CreateVar(
                checker->Allocator(), memberIndex, varbinder::VariableFlags::PROPERTY, nullptr);
            ES2PANDA_ASSERT(tupleMember != nullptr);
            if (inConstContext) {
                tupleMember->AddFlag(varbinder::VariableFlags::READONLY);
            }

            tupleMember->SetTsType(*it);
            ES2PANDA_ASSERT(desc != nullptr);
            desc->properties.push_back(tupleMember);
        }

        const checker::TupleTypeInfo tupleTypeInfo = {ElementFlags::REQUIRED, index, index, inConstContext};
        return checker->CreateTupleType(desc, std::move(elementFlags), tupleTypeInfo);
    }

    checker::Type *arrayElementType = nullptr;
    if (elementTypes.empty()) {
        arrayElementType = checker->GlobalAnyType();
    } else {
        arrayElementType = checker->CreateUnionType(std::move(elementTypes));
    }

    return checker->Allocator()->New<checker::ArrayType>(arrayElementType);
}

checker::Type *TSAnalyzer::Check(ir::ArrowFunctionExpression *expr) const
{
    TSChecker *checker = GetTSChecker();
    varbinder::Variable *funcVar = nullptr;

    if (expr->Function()->Parent()->Parent() != nullptr &&
        expr->Function()->Parent()->Parent()->IsVariableDeclarator() &&
        expr->Function()->Parent()->Parent()->AsVariableDeclarator()->Id()->IsIdentifier()) {
        funcVar = expr->Function()->Parent()->Parent()->AsVariableDeclarator()->Id()->AsIdentifier()->Variable();
    }

    checker::ScopeContext scopeCtx(checker, expr->Function()->Scope());

    auto *signatureInfo = checker->Allocator()->New<checker::SignatureInfo>(checker->Allocator());
    checker->CheckFunctionParameterDeclarations(expr->Function()->Params(), signatureInfo);

    auto *signature = checker->Allocator()->New<checker::Signature>(signatureInfo, checker->GlobalResolvingReturnType(),
                                                                    expr->Function());
    checker::Type *funcType = checker->CreateFunctionTypeWithSignature(signature);

    if (funcVar != nullptr && funcVar->TsType() == nullptr) {
        funcVar->SetTsType(funcType);
    }
    ES2PANDA_ASSERT(signature != nullptr);
    signature->SetReturnType(checker->HandleFunctionReturn(expr->Function()));

    if (!expr->Function()->Body()->IsExpression()) {
        expr->Function()->Body()->Check(checker);
    }

    return funcType;
}

checker::Type *TSAnalyzer::CheckAssignmentExprOperatorType(ir::AssignmentExpression *expr, checker::Type *leftType,
                                                           checker::Type *rightType) const
{
    TSChecker *checker = GetTSChecker();
    ExpressionTypeInfo leftRightType {};
    leftRightType.leftType = leftType;
    leftRightType.rightType = rightType;
    switch (expr->OperatorType()) {
        case lexer::TokenType::PUNCTUATOR_MULTIPLY_EQUAL:
        case lexer::TokenType::PUNCTUATOR_EXPONENTIATION_EQUAL:
        case lexer::TokenType::PUNCTUATOR_DIVIDE_EQUAL:
        case lexer::TokenType::PUNCTUATOR_MOD_EQUAL:
        case lexer::TokenType::PUNCTUATOR_MINUS_EQUAL:
        case lexer::TokenType::PUNCTUATOR_LEFT_SHIFT_EQUAL:
        case lexer::TokenType::PUNCTUATOR_RIGHT_SHIFT_EQUAL:
        case lexer::TokenType::PUNCTUATOR_UNSIGNED_RIGHT_SHIFT_EQUAL:
        case lexer::TokenType::PUNCTUATOR_BITWISE_AND_EQUAL:
        case lexer::TokenType::PUNCTUATOR_BITWISE_XOR_EQUAL:
        case lexer::TokenType::PUNCTUATOR_BITWISE_OR_EQUAL: {
            return checker->CheckBinaryOperator(&leftRightType, expr->Left(), expr->Right(), expr,
                                                expr->OperatorType());
        }
        case lexer::TokenType::PUNCTUATOR_PLUS_EQUAL: {
            return checker->CheckPlusOperator(&leftRightType, expr->Left(), expr->Right(), expr, expr->OperatorType());
        }
        case lexer::TokenType::PUNCTUATOR_SUBSTITUTION: {
            checker->CheckAssignmentOperator(expr->OperatorType(), expr->Left(), leftType, rightType);
            return rightType;
        }
        default: {
            ES2PANDA_UNREACHABLE();
            break;
        }
    }

    return nullptr;
}

checker::Type *TSAnalyzer::Check(ir::AssignmentExpression *expr) const
{
    TSChecker *checker = GetTSChecker();
    if (expr->Left()->IsArrayPattern()) {
        auto savedContext = checker::SavedCheckerContext(checker, checker::CheckerStatus::FORCE_TUPLE);
        auto destructuringContext =
            checker::ArrayDestructuringContext({checker, expr->Left(), true, true, nullptr, expr->Right()});
        destructuringContext.Start();
        return destructuringContext.InferredType();
    }

    if (expr->Left()->IsObjectPattern()) {
        auto savedContext = checker::SavedCheckerContext(checker, checker::CheckerStatus::FORCE_TUPLE);
        auto destructuringContext =
            checker::ObjectDestructuringContext({checker, expr->Left(), true, true, nullptr, expr->Right()});
        destructuringContext.Start();
        return destructuringContext.InferredType();
    }

    if (expr->Left()->IsIdentifier() && expr->Left()->AsIdentifier()->Variable() != nullptr &&
        expr->Left()->AsIdentifier()->Variable()->Declaration()->IsConstDecl()) {
        checker->ThrowTypeError(
            {"Cannot assign to ", expr->Left()->AsIdentifier()->Name(), " because it is a constant."},
            expr->Left()->Start());
    }

    auto *leftType = expr->Left()->Check(checker);

    if (leftType->HasTypeFlag(checker::TypeFlag::READONLY)) {
        checker->ThrowTypeError("Cannot assign to this property because it is readonly.", expr->Left()->Start());
    }

    if (expr->OperatorType() == lexer::TokenType::PUNCTUATOR_SUBSTITUTION) {
        checker->ElaborateElementwise(leftType, expr->Right(), expr->Left()->Start());
        return checker->CheckTypeCached(expr->Right());
    }

    auto *rightType = expr->Right()->Check(checker);

    return CheckAssignmentExprOperatorType(expr, leftType, rightType);
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::AwaitExpression *expr) const
{
    TSChecker *checker = GetTSChecker();
    // NOTE(aszilagyi)
    return checker->GlobalAnyType();
}

checker::Type *TSAnalyzer::CheckBinaryExprArithmLogical(ir::BinaryExpression *expr, ExpressionTypeInfo *leftRightType,
                                                        TSChecker *checker) const
{
    switch (expr->OperatorType()) {
        case lexer::TokenType::PUNCTUATOR_MULTIPLY:
        case lexer::TokenType::PUNCTUATOR_EXPONENTIATION:
        case lexer::TokenType::PUNCTUATOR_DIVIDE:
        case lexer::TokenType::PUNCTUATOR_MOD:
        case lexer::TokenType::PUNCTUATOR_MINUS:
        case lexer::TokenType::PUNCTUATOR_LEFT_SHIFT:
        case lexer::TokenType::PUNCTUATOR_RIGHT_SHIFT:
        case lexer::TokenType::PUNCTUATOR_UNSIGNED_RIGHT_SHIFT:
        case lexer::TokenType::PUNCTUATOR_BITWISE_AND:
        case lexer::TokenType::PUNCTUATOR_BITWISE_XOR:
        case lexer::TokenType::PUNCTUATOR_BITWISE_OR: {
            return checker->CheckBinaryOperator(leftRightType, expr->Left(), expr->Right(), expr, expr->OperatorType());
        }
        case lexer::TokenType::PUNCTUATOR_PLUS: {
            return checker->CheckPlusOperator(leftRightType, expr->Left(), expr->Right(), expr, expr->OperatorType());
        }
        case lexer::TokenType::PUNCTUATOR_LOGICAL_AND: {
            return checker->CheckAndOperator(leftRightType->leftType, leftRightType->rightType, expr->Left());
        }
        case lexer::TokenType::PUNCTUATOR_LOGICAL_OR: {
            return checker->CheckOrOperator(leftRightType->leftType, leftRightType->rightType, expr->Left());
        }
        default: {
            return nullptr;
        }
    }
}

checker::Type *TSAnalyzer::Check(ir::BinaryExpression *expr) const
{
    TSChecker *checker = GetTSChecker();
    ExpressionTypeInfo leftRightType {};
    leftRightType.leftType = expr->Left()->Check(checker);
    leftRightType.rightType = expr->Right()->Check(checker);

    auto *checkBinaryExprPunctuator = CheckBinaryExprArithmLogical(expr, &leftRightType, checker);
    if (checkBinaryExprPunctuator != nullptr) {
        return checkBinaryExprPunctuator;
    }

    switch (expr->OperatorType()) {
        case lexer::TokenType::PUNCTUATOR_LESS_THAN:
        case lexer::TokenType::PUNCTUATOR_GREATER_THAN: {
            return checker->CheckCompareOperator(&leftRightType, expr->Left(), expr->Right(), expr,
                                                 expr->OperatorType());
        }
        case lexer::TokenType::PUNCTUATOR_EQUAL:
        case lexer::TokenType::PUNCTUATOR_NOT_EQUAL:
        case lexer::TokenType::PUNCTUATOR_STRICT_EQUAL:
        case lexer::TokenType::PUNCTUATOR_NOT_STRICT_EQUAL: {
            if (checker->IsTypeEqualityComparableTo(leftRightType.leftType, leftRightType.rightType) ||
                checker->IsTypeEqualityComparableTo(leftRightType.rightType, leftRightType.leftType)) {
                return checker->GlobalBooleanType();
            }

            checker->ThrowBinaryLikeError(expr->OperatorType(), leftRightType.leftType, leftRightType.rightType,
                                          expr->Start());
        }
        case lexer::TokenType::PUNCTUATOR_NULLISH_COALESCING: {
            // NOTE: Csaba Repasi. Implement checker for nullish coalescing
            return checker->GlobalAnyType();
        }
        case lexer::TokenType::PUNCTUATOR_SUBSTITUTION: {
            checker->CheckAssignmentOperator(expr->OperatorType(), expr->Left(), leftRightType.leftType,
                                             leftRightType.rightType);
            return leftRightType.rightType;
        }
        case lexer::TokenType::KEYW_INSTANCEOF: {
            return checker->CheckInstanceofExpression(leftRightType.leftType, leftRightType.rightType, expr->Right(),
                                                      expr);
        }
        case lexer::TokenType::KEYW_IN: {
            return checker->CheckInExpression(leftRightType.leftType, leftRightType.rightType, expr->Left(),
                                              expr->Right(), expr);
        }
        default: {
            ES2PANDA_UNREACHABLE();
            break;
        }
    }

    return nullptr;
}

checker::Type *TSAnalyzer::Check(ir::CallExpression *expr) const
{
    TSChecker *checker = GetTSChecker();
    checker::Type *calleeType = expr->callee_->Check(checker);

    // NOTE: aszilagyi. handle optional chain
    if (calleeType->IsObjectType()) {
        checker::ObjectType *calleeObj = calleeType->AsObjectType();
        return checker->ResolveCallOrNewExpression(calleeObj->CallSignatures(), expr->Arguments(), expr->Start());
    }

    checker->ThrowTypeError("This expression is not callable.", expr->Start());
    return nullptr;
}

checker::Type *TSAnalyzer::Check(ir::ChainExpression *expr) const
{
    TSChecker *checker = GetTSChecker();
    return expr->expression_->Check(checker);
}

checker::Type *TSAnalyzer::Check(ir::ConditionalExpression *expr) const
{
    TSChecker *checker = GetTSChecker();
    checker::Type *testType = expr->Test()->Check(checker);

    checker->CheckTruthinessOfType(testType, expr->Test()->Start());
    checker->CheckTestingKnownTruthyCallableOrAwaitableType(expr->Test(), testType, expr->Consequent());

    checker::Type *consequentType = expr->Consequent()->Check(checker);
    checker::Type *alternateType = expr->Alternate()->Check(checker);

    return checker->CreateUnionType({consequentType, alternateType});
}

checker::Type *TSAnalyzer::Check(ir::FunctionExpression *expr) const
{
    TSChecker *checker = GetTSChecker();
    varbinder::Variable *funcVar = nullptr;

    if (expr->Function()->Parent()->Parent() != nullptr &&
        expr->Function()->Parent()->Parent()->IsVariableDeclarator() &&
        expr->Function()->Parent()->Parent()->AsVariableDeclarator()->Id()->IsIdentifier()) {
        funcVar = expr->Function()->Parent()->Parent()->AsVariableDeclarator()->Id()->AsIdentifier()->Variable();
    }

    checker::ScopeContext scopeCtx(checker, expr->Function()->Scope());

    auto *signatureInfo = checker->Allocator()->New<checker::SignatureInfo>(checker->Allocator());
    checker->CheckFunctionParameterDeclarations(expr->Function()->Params(), signatureInfo);

    auto *signature = checker->Allocator()->New<checker::Signature>(signatureInfo, checker->GlobalResolvingReturnType(),
                                                                    expr->Function());
    checker::Type *funcType = checker->CreateFunctionTypeWithSignature(signature);

    if (funcVar != nullptr && funcVar->TsType() == nullptr) {
        funcVar->SetTsType(funcType);
    }
    ES2PANDA_ASSERT(signature != nullptr);
    signature->SetReturnType(checker->HandleFunctionReturn(expr->Function()));

    expr->Function()->Body()->Check(checker);

    return funcType;
}

checker::Type *TSAnalyzer::Check(ir::Identifier *expr) const
{
    TSChecker *checker = GetTSChecker();
    if (expr->Variable() == nullptr) {
        if (expr->Name().Is("undefined")) {
            return checker->GlobalUndefinedType();
        }

        checker->ThrowTypeError({"Cannot find name ", expr->Name()}, expr->Start());
    }

    const varbinder::Decl *decl = expr->Variable()->Declaration();

    if (decl->IsTypeAliasDecl() || decl->IsInterfaceDecl()) {
        checker->ThrowTypeError({expr->Name(), " only refers to a type, but is being used as a value here."},
                                expr->Start());
    }

    expr->SetTsType(checker->GetTypeOfVariable(expr->Variable()));
    return expr->TsType();
}

void TSAnalyzer::CheckComputed(ir::MemberExpression *expr, checker::Type *indexType) const
{
    TSChecker *checker = GetTSChecker();
    if (!indexType->HasTypeFlag(checker::TypeFlag::STRING_LIKE | checker::TypeFlag::NUMBER_LIKE)) {
        checker->ThrowTypeError({"Type ", indexType, " cannot be used as index type"}, expr->Property()->Start());
    }

    if (indexType->IsNumberType()) {
        checker->ThrowTypeError("No index signature with a parameter of type 'string' was found on type this type",
                                expr->Start());
    }

    if (indexType->IsStringType()) {
        checker->ThrowTypeError("No index signature with a parameter of type 'number' was found on type this type",
                                expr->Start());
    }

    switch (expr->Property()->Type()) {
        case ir::AstNodeType::IDENTIFIER: {
            checker->ThrowTypeError(
                {"Property ", expr->Property()->AsIdentifier()->Name(), " does not exist on this type."},
                expr->Property()->Start());
        }
        case ir::AstNodeType::NUMBER_LITERAL: {
            checker->ThrowTypeError(
                {"Property ", expr->Property()->AsNumberLiteral()->Str(), " does not exist on this type."},
                expr->Property()->Start());
        }
        case ir::AstNodeType::STRING_LITERAL: {
            checker->ThrowTypeError(
                {"Property ", expr->Property()->AsStringLiteral()->Str(), " does not exist on this type."},
                expr->Property()->Start());
        }
        default: {
            ES2PANDA_UNREACHABLE();
        }
    }
}

checker::Type *TSAnalyzer::Check(ir::MemberExpression *expr) const
{
    TSChecker *checker = GetTSChecker();
    checker::Type *baseType = checker->CheckNonNullType(expr->Object()->Check(checker), expr->Object()->Start());

    if (expr->IsComputed()) {
        checker::Type *indexType = expr->Property()->Check(checker);
        checker::Type *indexedAccessType = checker->GetPropertyTypeForIndexType(baseType, indexType);

        if (indexedAccessType != nullptr) {
            return indexedAccessType;
        }
        CheckComputed(expr, indexType);
    }

    varbinder::Variable *prop = checker->GetPropertyOfType(baseType, expr->Property()->AsIdentifier()->Name());

    if (prop != nullptr) {
        checker::Type *propType = checker->GetTypeOfVariable(prop);
        if (prop->HasFlag(varbinder::VariableFlags::READONLY)) {
            propType->AddTypeFlag(checker::TypeFlag::READONLY);
        }

        return propType;
    }

    if (baseType->IsObjectType()) {
        checker::ObjectType *objType = baseType->AsObjectType();

        if (objType->StringIndexInfo() != nullptr) {
            checker::Type *indexType = objType->StringIndexInfo()->GetType();
            if (objType->StringIndexInfo()->Readonly()) {
                indexType->AddTypeFlag(checker::TypeFlag::READONLY);
            }

            return indexType;
        }
    }

    checker->ThrowTypeError({"Property ", expr->Property()->AsIdentifier()->Name(), " does not exist on this type."},
                            expr->Property()->Start());
    return nullptr;
}

checker::Type *TSAnalyzer::Check(ir::NewExpression *expr) const
{
    TSChecker *checker = GetTSChecker();
    checker::Type *calleeType = expr->callee_->Check(checker);

    if (calleeType->IsObjectType()) {
        checker::ObjectType *calleeObj = calleeType->AsObjectType();
        return checker->ResolveCallOrNewExpression(calleeObj->ConstructSignatures(), expr->Arguments(), expr->Start());
    }

    checker->ThrowTypeError("This expression is not callable.", expr->Start());
    return nullptr;
}
static util::StringView GetPropertyName(const ir::Expression *key)
{
    if (key->IsIdentifier()) {
        return key->AsIdentifier()->Name();
    }

    if (key->IsStringLiteral()) {
        return key->AsStringLiteral()->Str();
    }

    ES2PANDA_ASSERT(key->IsNumberLiteral());
    return key->AsNumberLiteral()->Str();
}

static varbinder::VariableFlags GetFlagsForProperty(const ir::Property *prop)
{
    if (!prop->IsMethod()) {
        return varbinder::VariableFlags::PROPERTY;
    }

    varbinder::VariableFlags propFlags = varbinder::VariableFlags::METHOD;

    if (prop->IsAccessor() && prop->Kind() == ir::PropertyKind::GET) {
        propFlags |= varbinder::VariableFlags::READONLY;
    }

    return propFlags;
}

static checker::Type *GetTypeForProperty(ir::Property *prop, checker::TSChecker *checker)
{
    if (prop->IsAccessor()) {
        checker::Type *funcType = prop->Value()->Check(checker);

        if (prop->Kind() == ir::PropertyKind::SET) {
            return checker->GlobalAnyType();
        }

        ES2PANDA_ASSERT(funcType->IsObjectType() && funcType->AsObjectType()->IsFunctionType());
        return funcType->AsObjectType()->CallSignatures()[0]->ReturnType();
    }

    if (prop->IsShorthand()) {
        return prop->Key()->Check(checker);
    }

    return prop->Value()->Check(checker);
}

void TSAnalyzer::CheckSpread(std::unordered_map<util::StringView, lexer::SourcePosition> &allPropertiesMap,
                             checker::ObjectDescriptor *desc, ir::Expression *it) const
{
    TSChecker *checker = GetTSChecker();
    ES2PANDA_ASSERT(it->IsSpreadElement());

    checker::Type *const spreadType = it->AsSpreadElement()->Argument()->Check(checker);

    // NOTE: aszilagyi. handle union of object types
    if (!spreadType->IsObjectType()) {
        checker->ThrowTypeError("Spread types may only be created from object types.", it->Start());
    }

    for (auto *spreadProp : spreadType->AsObjectType()->Properties()) {
        auto found = allPropertiesMap.find(spreadProp->Name());
        if (found != allPropertiesMap.end()) {
            checker->ThrowTypeError({found->first, " is specified more than once, so this usage will be overwritten."},
                                    found->second);
        }

        varbinder::LocalVariable *foundMember = desc->FindProperty(spreadProp->Name());

        if (foundMember != nullptr) {
            foundMember->SetTsType(spreadProp->TsType());
            continue;
        }

        desc->properties.push_back(spreadProp);
    }
}

void TSAnalyzer::CheckNonComputed(checker::ObjectDescriptor *desc, ir::Expression *it,
                                  std::unordered_map<util::StringView, lexer::SourcePosition> &allPropertiesMap,
                                  bool inConstContext) const
{
    TSChecker *checker = GetTSChecker();
    auto *prop = it->AsProperty();
    checker::Type *propType = GetTypeForProperty(prop, checker);
    varbinder::VariableFlags flags = GetFlagsForProperty(prop);
    util::StringView propName = GetPropertyName(prop->Key());

    auto *memberVar = varbinder::Scope::CreateVar(checker->Allocator(), propName, flags, it);

    ES2PANDA_ASSERT(memberVar != nullptr);
    if (inConstContext) {
        memberVar->AddFlag(varbinder::VariableFlags::READONLY);
    } else {
        propType = checker->GetBaseTypeOfLiteralType(propType);
    }

    memberVar->SetTsType(propType);

    if (prop->Key()->IsNumberLiteral()) {
        memberVar->AddFlag(varbinder::VariableFlags::NUMERIC_NAME);
    }
    ES2PANDA_ASSERT(desc != nullptr);
    varbinder::LocalVariable *foundMember = desc->FindProperty(propName);
    allPropertiesMap.insert({propName, it->Start()});

    if (foundMember != nullptr) {
        foundMember->SetTsType(propType);
        return;
    }

    desc->properties.push_back(memberVar);
}

checker::IndexInfo *TSAnalyzer::CreateUnionTypeHelper(ArenaVector<checker::Type *> &computedPropTypes,
                                                      bool inConstContext) const
{
    TSChecker *checker = GetTSChecker();

    return checker->Allocator()->New<checker::IndexInfo>(checker->CreateUnionType(std::move(computedPropTypes)), "x",
                                                         inConstContext);
}

checker::Type *TSAnalyzer::Check(ir::ObjectExpression *expr) const
{
    TSChecker *checker = GetTSChecker();

    checker::ObjectDescriptor *desc = checker->Allocator()->New<checker::ObjectDescriptor>(checker->Allocator());
    std::unordered_map<util::StringView, lexer::SourcePosition> allPropertiesMap;
    bool inConstContext = checker->HasStatus(checker::CheckerStatus::IN_CONST_CONTEXT);
    ArenaVector<checker::Type *> computedNumberPropTypes(checker->Allocator()->Adapter());
    ArenaVector<checker::Type *> computedStringPropTypes(checker->Allocator()->Adapter());
    bool hasComputedNumberProperty = false;
    bool hasComputedStringProperty = false;
    bool seenSpread = false;

    for (auto *it : expr->Properties()) {
        if (it->IsProperty()) {
            auto *prop = it->AsProperty();

            if (prop->IsComputed() && checker->CheckComputedPropertyName(prop->Key())->IsNumberType()) {
                hasComputedNumberProperty = true;
                computedNumberPropTypes.push_back(prop->Value()->Check(checker));
                continue;
            }

            if (prop->IsComputed() && checker->CheckComputedPropertyName(prop->Key())->IsStringType()) {
                hasComputedStringProperty = true;
                computedStringPropTypes.push_back(prop->Value()->Check(checker));
                continue;
            }

            CheckNonComputed(desc, it, allPropertiesMap, inConstContext);
        }

        if (it->IsSpreadElement()) {
            CheckSpread(allPropertiesMap, desc, it);
            seenSpread = true;
        }
    }

    if (!seenSpread && (hasComputedNumberProperty || hasComputedStringProperty)) {
        for (auto *it : desc->properties) {
            computedStringPropTypes.push_back(it->TsType());

            if (hasComputedNumberProperty && it->HasFlag(varbinder::VariableFlags::NUMERIC_NAME)) {
                computedNumberPropTypes.push_back(it->TsType());
            }
        }

        if (hasComputedNumberProperty) {
            desc->numberIndexInfo = CreateUnionTypeHelper(computedNumberPropTypes, inConstContext);
        }

        if (hasComputedStringProperty) {
            desc->stringIndexInfo = CreateUnionTypeHelper(computedStringPropTypes, inConstContext);
        }
    }

    checker::Type *returnType = checker->Allocator()->New<checker::ObjectLiteralType>(desc);
    ES2PANDA_ASSERT(returnType != nullptr);
    returnType->AsObjectType()->AddObjectFlag(checker::ObjectFlags::RESOLVED_MEMBERS |
                                              checker::ObjectFlags::CHECK_EXCESS_PROPS);
    return returnType;
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::OmittedExpression *expr) const
{
    TSChecker *checker = GetTSChecker();
    return checker->GlobalUndefinedType();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::OpaqueTypeNode *expr) const
{
    return expr->TsType();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::BrokenTypeNode *expr) const
{
    return nullptr;
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::SequenceExpression *expr) const
{
    TSChecker *checker = GetTSChecker();
    // NOTE: aszilagyi.
    return checker->GlobalAnyType();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::SuperExpression *expr) const
{
    TSChecker *checker = GetTSChecker();
    // NOTE: aszilagyi.
    return checker->GlobalAnyType();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::TaggedTemplateExpression *expr) const
{
    TSChecker *checker = GetTSChecker();
    // NOTE: aszilagyi.
    return checker->GlobalAnyType();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::TemplateLiteral *expr) const
{
    TSChecker *checker = GetTSChecker();
    // NOTE(aszilagyi)
    return checker->GlobalAnyType();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::ThisExpression *expr) const
{
    TSChecker *checker = GetTSChecker();
    // NOTE: aszilagyi
    return checker->GlobalAnyType();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::TypeofExpression *expr) const
{
    TSChecker *checker = GetTSChecker();
    return checker->GlobalStringType();
}

checker::Type *TSAnalyzer::CheckDeleteKeyword([[maybe_unused]] checker::TSChecker *checker,
                                              ir::UnaryExpression *expr) const
{
    checker::Type *propType = expr->argument_->Check(checker);
    if (!expr->Argument()->IsMemberExpression()) {
        checker->ThrowTypeError("The operand of a delete operator must be a property reference.",
                                expr->Argument()->Start());
    }
    if (propType->Variable()->HasFlag(varbinder::VariableFlags::READONLY)) {
        checker->ThrowTypeError("The operand of a delete operator cannot be a readonly property.",
                                expr->Argument()->Start());
    }
    if (!propType->Variable()->HasFlag(varbinder::VariableFlags::OPTIONAL)) {
        checker->ThrowTypeError("The operand of a delete operator must be a optional.", expr->Argument()->Start());
    }
    return checker->GlobalBooleanType();
}

checker::Type *TSAnalyzer::CheckLiteral([[maybe_unused]] checker::TSChecker *checker, ir::UnaryExpression *expr) const
{
    if (!expr->Argument()->IsLiteral()) {
        return nullptr;
    }

    const ir::Literal *lit = expr->Argument()->AsLiteral();
    if (lit->IsNumberLiteral()) {
        auto numberValue = lit->AsNumberLiteral()->Number().GetDouble();
        if (expr->OperatorType() == lexer::TokenType::PUNCTUATOR_PLUS) {
            return checker->CreateNumberLiteralType(numberValue);
        }
        if (expr->OperatorType() == lexer::TokenType::PUNCTUATOR_MINUS) {
            return checker->CreateNumberLiteralType(-numberValue);
        }
    } else if (lit->IsBigIntLiteral() && expr->OperatorType() == lexer::TokenType::PUNCTUATOR_MINUS) {
        return checker->CreateBigintLiteralType(lit->AsBigIntLiteral()->Str(), true);
    }

    return nullptr;
}

checker::Type *TSAnalyzer::Check(ir::UnaryExpression *expr) const
{
    TSChecker *checker = GetTSChecker();
    checker::Type *operandType = expr->argument_->Check(checker);

    if (expr->operator_ == lexer::TokenType::KEYW_TYPEOF) {
        return operandType;
    }

    if (expr->operator_ == lexer::TokenType::KEYW_DELETE) {
        return CheckDeleteKeyword(checker, expr);
    }

    auto *res = CheckLiteral(checker, expr);
    if (res != nullptr) {
        return res;
    }

    switch (expr->operator_) {
        case lexer::TokenType::PUNCTUATOR_PLUS:
        case lexer::TokenType::PUNCTUATOR_MINUS:
        case lexer::TokenType::PUNCTUATOR_TILDE: {
            checker->CheckNonNullType(operandType, expr->Start());
            // NOTE: aszilagyi. check Symbol like types

            if (expr->operator_ == lexer::TokenType::PUNCTUATOR_PLUS) {
                if (checker::TSChecker::MaybeTypeOfKind(operandType, checker::TypeFlag::BIGINT_LIKE)) {
                    checker->ThrowTypeError({"Operator '+' cannot be applied to type '", operandType, "'"},
                                            expr->Start());
                }

                return checker->GlobalNumberType();
            }

            return checker->GetUnaryResultType(operandType);
        }
        case lexer::TokenType::PUNCTUATOR_EXCLAMATION_MARK: {
            checker->CheckTruthinessOfType(operandType, expr->Start());
            auto facts = operandType->GetTypeFacts();
            if ((facts & checker::TypeFacts::TRUTHY) != 0) {
                return checker->GlobalFalseType();
            }

            if ((facts & checker::TypeFacts::FALSY) != 0) {
                return checker->GlobalTrueType();
            }

            return checker->GlobalBooleanType();
        }
        default: {
            ES2PANDA_UNREACHABLE();
        }
    }

    return nullptr;
}

checker::Type *TSAnalyzer::Check(ir::UpdateExpression *expr) const
{
    TSChecker *checker = GetTSChecker();
    checker::Type *operandType = expr->argument_->Check(checker);
    checker->CheckNonNullType(operandType, expr->Start());

    if (!operandType->HasTypeFlag(checker::TypeFlag::VALID_ARITHMETIC_TYPE)) {
        checker->ThrowTypeError("An arithmetic operand must be of type 'any', 'number', 'bigint' or an enum type.",
                                expr->Start());
    }

    checker->CheckReferenceExpression(
        expr->argument_, "The operand of an increment or decrement operator must be a variable or a property access",
        "The operand of an increment or decrement operator may not be an optional property access");

    return checker->GetUnaryResultType(operandType);
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::YieldExpression *expr) const
{
    TSChecker *checker = GetTSChecker();
    // NOTE: aszilagyi.
    return checker->GlobalAnyType();
}
// compile methods for LITERAL EXPRESSIONS in alphabetical order
checker::Type *TSAnalyzer::Check(ir::BigIntLiteral *expr) const
{
    TSChecker *checker = GetTSChecker();
    auto search = checker->BigintLiteralMap().find(expr->Str());
    if (search != checker->BigintLiteralMap().end()) {
        return search->second;
    }

    auto *newBigintLiteralType = checker->Allocator()->New<checker::BigintLiteralType>(expr->Str(), false);
    checker->BigintLiteralMap().insert({expr->Str(), newBigintLiteralType});
    return newBigintLiteralType;
}

checker::Type *TSAnalyzer::Check(ir::BooleanLiteral *expr) const
{
    TSChecker *checker = GetTSChecker();
    return expr->Value() ? checker->GlobalTrueType() : checker->GlobalFalseType();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::NullLiteral *expr) const
{
    TSChecker *checker = GetTSChecker();
    return checker->GlobalNullType();
}

checker::Type *TSAnalyzer::Check(ir::NumberLiteral *expr) const
{
    TSChecker *checker = GetTSChecker();
    auto search = checker->NumberLiteralMap().find(expr->Number().GetDouble());
    if (search != checker->NumberLiteralMap().end()) {
        return search->second;
    }

    auto *newNumLiteralType = checker->Allocator()->New<checker::NumberLiteralType>(expr->Number().GetDouble());
    checker->NumberLiteralMap().insert({expr->Number().GetDouble(), newNumLiteralType});
    return newNumLiteralType;
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::RegExpLiteral *expr) const
{
    TSChecker *checker = GetTSChecker();
    // NOTE: aszilagyi
    return checker->GlobalAnyType();
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::AnnotationDeclaration *expr) const
{
    return nullptr;
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::AnnotationUsage *expr) const
{
    return nullptr;
}

checker::Type *TSAnalyzer::Check(ir::StringLiteral *expr) const
{
    TSChecker *checker = GetTSChecker();
    auto search = checker->StringLiteralMap().find(expr->Str());
    if (search != checker->StringLiteralMap().end()) {
        return search->second;
    }

    auto *newStrLiteralType = checker->Allocator()->New<checker::StringLiteralType>(expr->Str());
    checker->StringLiteralMap().insert({expr->Str(), newStrLiteralType});

    return newStrLiteralType;
}

checker::Type *TSAnalyzer::Check(ir::BlockStatement *st) const
{
    TSChecker *checker = GetTSChecker();
    checker::ScopeContext scopeCtx(checker, st->Scope());

    for (auto *it : st->Statements()) {
        it->Check(checker);
    }

    return nullptr;
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::BreakStatement *st) const
{
    return nullptr;
}

checker::Type *TSAnalyzer::Check(ir::DoWhileStatement *st) const
{
    TSChecker *checker = GetTSChecker();
    checker::ScopeContext scopeCtx(checker, st->Scope());

    checker::Type *testType = st->Test()->Check(checker);
    checker->CheckTruthinessOfType(testType, st->Test()->Start());
    st->Body()->Check(checker);

    return nullptr;
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::EmptyStatement *st) const
{
    return nullptr;
}

checker::Type *TSAnalyzer::Check(ir::ExpressionStatement *st) const
{
    TSChecker *checker = GetTSChecker();
    return st->GetExpression()->Check(checker);
}

checker::Type *TSAnalyzer::Check(ir::ForUpdateStatement *st) const
{
    TSChecker *checker = GetTSChecker();
    checker::ScopeContext scopeCtx(checker, st->Scope());

    if (st->Init() != nullptr) {
        st->Init()->Check(checker);
    }

    if (st->Test() != nullptr) {
        checker::Type *testType = st->Test()->Check(checker);
        checker->CheckTruthinessOfType(testType, st->Start());
    }

    if (st->Update() != nullptr) {
        st->Update()->Check(checker);
    }

    st->Body()->Check(checker);

    return nullptr;
}

checker::Type *TSAnalyzer::Check(ir::FunctionDeclaration *st) const
{
    TSChecker *checker = GetTSChecker();
    if (st->Function()->IsOverload()) {
        return nullptr;
    }

    const util::StringView &funcName = st->Function()->Id()->Name();
    auto result = checker->Scope()->Find(funcName);
    ES2PANDA_ASSERT(result.variable);

    checker::ScopeContext scopeCtx(checker, st->Function()->Scope());

    if (result.variable->TsType() == nullptr) {
        checker->InferFunctionDeclarationType(result.variable->Declaration()->AsFunctionDecl(), result.variable);
    }

    st->Function()->Body()->Check(checker);

    return nullptr;
}

checker::Type *TSAnalyzer::Check(ir::IfStatement *st) const
{
    TSChecker *checker = GetTSChecker();
    checker::Type *testType = st->Test()->Check(checker);
    checker->CheckTruthinessOfType(testType, st->Start());
    checker->CheckTestingKnownTruthyCallableOrAwaitableType(st->Test(), testType, st->Consequent());

    st->Consequent()->Check(checker);

    if (st->Alternate() != nullptr) {
        st->Alternate()->Check(checker);
    }

    return nullptr;
}

checker::Type *TSAnalyzer::Check(ir::ReturnStatement *st) const
{
    TSChecker *checker = GetTSChecker();
    ir::AstNode *ancestor = util::Helpers::FindAncestorGivenByType(st, ir::AstNodeType::SCRIPT_FUNCTION);
    ES2PANDA_ASSERT(ancestor && ancestor->IsScriptFunction());
    auto *containingFunc = ancestor->AsScriptFunction();

    if (containingFunc->Parent()->Parent()->IsMethodDefinition()) {
        const ir::MethodDefinition *containingClassMethod = containingFunc->Parent()->Parent()->AsMethodDefinition();
        if (containingClassMethod->Kind() == ir::MethodDefinitionKind::SET) {
            checker->ThrowTypeError("Setters cannot return a value", st->Start());
        }
    }

    if (containingFunc->ReturnTypeAnnotation() != nullptr) {
        checker::Type *returnType = checker->GlobalUndefinedType();
        checker::Type *funcReturnType = containingFunc->ReturnTypeAnnotation()->GetType(checker);

        if (st->Argument() != nullptr) {
            checker->ElaborateElementwise(funcReturnType, st->Argument(), st->Start());
            returnType = checker->CheckTypeCached(st->Argument());
        }

        checker->IsTypeAssignableTo(returnType, funcReturnType, diagnostic::INVALID_ASSIGNMNENT_2,
                                    {returnType, funcReturnType}, st->Start());
    }

    return nullptr;
}

checker::Type *TSAnalyzer::Check(ir::SwitchStatement *st) const
{
    TSChecker *checker = GetTSChecker();
    checker::ScopeContext scopeCtx(checker, st->Scope());

    checker::Type *exprType = st->Discriminant()->Check(checker);
    bool exprIsLiteral = checker::TSChecker::IsLiteralType(exprType);

    for (auto *it : st->Cases()) {
        if (it->Test() != nullptr) {
            checker::Type *caseType = it->Test()->Check(checker);
            bool caseIsLiteral = checker::TSChecker::IsLiteralType(caseType);
            checker::Type *comparedExprType = exprType;

            if (!caseIsLiteral || !exprIsLiteral) {
                caseType = caseIsLiteral ? checker->GetBaseTypeOfLiteralType(caseType) : caseType;
                comparedExprType = checker->GetBaseTypeOfLiteralType(exprType);
            }

            if (!checker->IsTypeEqualityComparableTo(comparedExprType, caseType) &&
                !checker->IsTypeComparableTo(caseType, comparedExprType)) {
                checker->ThrowTypeError({"Type ", caseType, " is not comparable to type ", comparedExprType},
                                        it->Test()->Start());
            }
        }

        for (auto *caseStmt : it->Consequent()) {
            caseStmt->Check(checker);
        }
    }

    return nullptr;
}

checker::Type *TSAnalyzer::Check(ir::TryStatement *st) const
{
    TSChecker *checker = GetTSChecker();
    st->Block()->Check(checker);

    for (auto *catchClause : st->CatchClauses()) {
        if (catchClause != nullptr) {
            catchClause->Check(checker);
        }
    }

    if (st->HasFinalizer()) {
        st->finalizer_->Check(checker);
    }

    return nullptr;
}

static void CheckSimpleVariableDeclaration(checker::TSChecker *checker, ir::VariableDeclarator *declarator)
{
    varbinder::Variable *const bindingVar = declarator->Id()->AsIdentifier()->Variable();
    checker::Type *previousType = bindingVar->TsType();
    auto *const typeAnnotation = declarator->Id()->AsIdentifier()->TypeAnnotation();
    auto *const initializer = declarator->Init();
    const bool isConst = declarator->Parent()->AsVariableDeclaration()->Kind() ==
                         ir::VariableDeclaration::VariableDeclarationKind::CONST;

    if (isConst) {
        checker->AddStatus(checker::CheckerStatus::IN_CONST_CONTEXT);
    }

    if (typeAnnotation != nullptr) {
        typeAnnotation->Check(checker);
    }

    if (typeAnnotation != nullptr && initializer != nullptr) {
        checker::Type *const annotationType = typeAnnotation->GetType(checker);
        checker->ElaborateElementwise(annotationType, initializer, declarator->Id()->Start());
        bindingVar->SetTsType(annotationType);
    } else if (typeAnnotation != nullptr) {
        bindingVar->SetTsType(typeAnnotation->GetType(checker));
    } else if (initializer != nullptr) {
        checker::Type *initializerType = checker->CheckTypeCached(initializer);

        if (!isConst) {
            initializerType = checker->GetBaseTypeOfLiteralType(initializerType);
        }

        ES2PANDA_ASSERT(initializerType != nullptr);
        if (initializerType->IsNullType()) {
            checker->ThrowTypeError(
                {"Cannot infer type for variable '", declarator->Id()->AsIdentifier()->Name(), "'."},
                declarator->Id()->Start());
        }

        bindingVar->SetTsType(initializerType);
    } else {
        checker->ThrowTypeError({"Variable ", declarator->Id()->AsIdentifier()->Name(), " implicitly has an any type."},
                                declarator->Id()->Start());
    }

    if (previousType != nullptr) {
        checker->IsTypeIdenticalTo(bindingVar->TsType(), previousType, diagnostic::DIFFERENT_SUBSEQ_DECL,
                                   {bindingVar->Name(), previousType, bindingVar->TsType()}, declarator->Id()->Start());
    }

    checker->RemoveStatus(checker::CheckerStatus::IN_CONST_CONTEXT);
}

checker::Type *TSAnalyzer::Check(ir::VariableDeclarator *st) const
{
    TSChecker *checker = GetTSChecker();

    if (st->TsType() == checker->GetMagicCheckedType()) {
        return nullptr;
    }

    if (st->Id()->IsIdentifier()) {
        CheckSimpleVariableDeclaration(checker, st);
        st->SetTsType(checker->GetMagicCheckedType());
        return nullptr;
    }

    if (st->Id()->IsArrayPattern()) {
        auto context = checker::SavedCheckerContext(checker, checker::CheckerStatus::FORCE_TUPLE);
        checker::ArrayDestructuringContext({checker, st->Id(), false,
                                            st->Id()->AsArrayPattern()->TypeAnnotation() == nullptr,
                                            st->Id()->AsArrayPattern()->TypeAnnotation(), st->Init()})
            .Start();

        st->SetTsType(checker->GetMagicCheckedType());
        return nullptr;
    }

    ES2PANDA_ASSERT(st->Id()->IsObjectPattern());
    auto context = checker::SavedCheckerContext(checker, checker::CheckerStatus::FORCE_TUPLE);
    checker::ObjectDestructuringContext({checker, st->Id(), false,
                                         st->Id()->AsObjectPattern()->TypeAnnotation() == nullptr,
                                         st->Id()->AsObjectPattern()->TypeAnnotation(), st->Init()})
        .Start();

    st->SetTsType(checker->GetMagicCheckedType());
    return nullptr;
}

checker::Type *TSAnalyzer::Check(ir::VariableDeclaration *st) const
{
    TSChecker *checker = GetTSChecker();
    for (auto *it : st->Declarators()) {
        it->Check(checker);
    }

    return nullptr;
}

checker::Type *TSAnalyzer::Check(ir::WhileStatement *st) const
{
    TSChecker *checker = GetTSChecker();
    checker::ScopeContext scopeCtx(checker, st->Scope());

    checker::Type *testType = st->Test()->Check(checker);
    checker->CheckTruthinessOfType(testType, st->Test()->Start());

    st->Body()->Check(checker);
    return nullptr;
}
// from ts folder
checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::TSAnyKeyword *node) const
{
    return nullptr;
}

checker::Type *TSAnalyzer::Check(ir::TSArrayType *node) const
{
    TSChecker *checker = GetTSChecker();
    node->elementType_->Check(checker);
    return nullptr;
}

static bool IsValidConstAssertionArgument(checker::Checker *checker, const ir::AstNode *arg)
{
    switch (arg->Type()) {
        case ir::AstNodeType::NUMBER_LITERAL:
        case ir::AstNodeType::STRING_LITERAL:
        case ir::AstNodeType::BIGINT_LITERAL:
        case ir::AstNodeType::BOOLEAN_LITERAL:
        case ir::AstNodeType::ARRAY_EXPRESSION:
        case ir::AstNodeType::OBJECT_EXPRESSION:
        case ir::AstNodeType::TEMPLATE_LITERAL: {
            return true;
        }
        case ir::AstNodeType::UNARY_EXPRESSION: {
            const ir::UnaryExpression *unaryExpr = arg->AsUnaryExpression();
            lexer::TokenType op = unaryExpr->OperatorType();
            const ir::Expression *unaryArg = unaryExpr->Argument();
            return (op == lexer::TokenType::PUNCTUATOR_MINUS && unaryArg->IsLiteral() &&
                    (unaryArg->AsLiteral()->IsNumberLiteral() || unaryArg->AsLiteral()->IsBigIntLiteral())) ||
                   (op == lexer::TokenType::PUNCTUATOR_PLUS && unaryArg->IsLiteral() &&
                    unaryArg->AsLiteral()->IsNumberLiteral());
        }
        case ir::AstNodeType::MEMBER_EXPRESSION: {
            const ir::MemberExpression *memberExpr = arg->AsMemberExpression();
            if (memberExpr->Object()->IsIdentifier()) {
                auto result = checker->Scope()->Find(memberExpr->Object()->AsIdentifier()->Name());
                constexpr auto ENUM_LITERAL_TYPE = checker::EnumLiteralType::EnumLiteralTypeKind::LITERAL;
                if (result.variable != nullptr &&
                    result.variable->TsType()->HasTypeFlag(checker::TypeFlag::ENUM_LITERAL) &&
                    result.variable->TsType()->AsEnumLiteralType()->Kind() == ENUM_LITERAL_TYPE) {
                    return true;
                }
            }
            return false;
        }
        default:
            return false;
    }
}

checker::Type *TSAnalyzer::Check(ir::TSAsExpression *expr) const
{
    TSChecker *checker = GetTSChecker();
    if (expr->IsConst()) {
        auto context = checker::SavedCheckerContext(checker, checker::CheckerStatus::IN_CONST_CONTEXT);
        checker::Type *exprType = expr->Expr()->Check(checker);

        if (!IsValidConstAssertionArgument(checker, expr->Expr())) {
            checker->ThrowTypeError(
                "A 'const' assertions can only be applied to references to enum members, or string, number, "
                "boolean, array, or object literals.",
                expr->Expr()->Start());
        }

        return exprType;
    }

    auto context = checker::SavedCheckerContext(checker, checker::CheckerStatus::NO_OPTS);

    expr->TypeAnnotation()->Check(checker);
    checker::Type *exprType = checker->GetBaseTypeOfLiteralType(expr->Expr()->Check(checker));
    checker::Type *targetType = expr->TypeAnnotation()->GetType(checker);

    checker->IsTypeComparableTo(targetType, exprType, diagnostic::DISJOINT_CONVERSION, {exprType, targetType},
                                expr->Start());

    return targetType;
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::TSBigintKeyword *node) const
{
    return nullptr;
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::TSBooleanKeyword *node) const
{
    return nullptr;
}

checker::Type *TSAnalyzer::Check(ir::TSConstructorType *node) const
{
    TSChecker *checker = GetTSChecker();
    checker::ScopeContext scopeCtx(checker, node->Scope());

    auto *signatureInfo = checker->Allocator()->New<checker::SignatureInfo>(checker->Allocator());
    checker->CheckFunctionParameterDeclarations(node->Params(), signatureInfo);
    node->ReturnType()->Check(checker);
    auto *constructSignature =
        checker->Allocator()->New<checker::Signature>(signatureInfo, node->ReturnType()->GetType(checker));

    return checker->CreateConstructorTypeWithSignature(constructSignature);
}

static varbinder::EnumMemberResult EvaluateIdentifier(checker::TSChecker *checker, varbinder::EnumVariable *enumVar,
                                                      const ir::Identifier *expr)
{
    if (expr->Name() == "NaN") {
        return std::nan("");
    }
    if (expr->Name() == "Infinity") {
        return std::numeric_limits<double>::infinity();
    }

    varbinder::Variable *enumMember = expr->AsIdentifier()->Variable();

    if (enumMember == nullptr) {
        checker->ThrowTypeError({"Cannot find name ", expr->AsIdentifier()->Name()},
                                enumVar->Declaration()->Node()->Start());
    }

    if (enumMember->IsEnumVariable()) {
        varbinder::EnumVariable *exprEnumVar = enumMember->AsEnumVariable();
        if (std::holds_alternative<bool>(exprEnumVar->Value())) {
            checker->ThrowTypeError(
                "A member initializer in a enum declaration cannot reference members declared after it, "
                "including "
                "members defined in other enums.",
                enumVar->Declaration()->Node()->Start());
        }

        return exprEnumVar->Value();
    }

    return false;
}

static int32_t ToInt(double num)
{
    if (num >= std::numeric_limits<int32_t>::min() && num <= std::numeric_limits<int32_t>::max()) {
        return static_cast<int32_t>(num);
    }

    // NOTE (aszilagyi): Perform ECMA defined toInt conversion

    return 0;
}

static uint32_t ToUInt(double num)
{
    if (num >= std::numeric_limits<uint32_t>::min() && num <= std::numeric_limits<uint32_t>::max()) {
        return static_cast<int32_t>(num);
    }

    // NOTE (aszilagyi): Perform ECMA defined toInt conversion

    return 0;
}

varbinder::EnumMemberResult GetOperationResulForDouble(lexer::TokenType type, varbinder::EnumMemberResult left,
                                                       varbinder::EnumMemberResult right)
{
    switch (type) {
        case lexer::TokenType::PUNCTUATOR_BITWISE_OR: {
            return static_cast<double>(ToUInt(std::get<double>(left)) | ToUInt(std::get<double>(right)));
        }
        case lexer::TokenType::PUNCTUATOR_BITWISE_AND: {
            return static_cast<double>(ToUInt(std::get<double>(left)) & ToUInt(std::get<double>(right)));
        }
        case lexer::TokenType::PUNCTUATOR_BITWISE_XOR: {
            return static_cast<double>(ToUInt(std::get<double>(left)) ^ ToUInt(std::get<double>(right)));
        }
        case lexer::TokenType::PUNCTUATOR_LEFT_SHIFT: {  // NOLINTNEXTLINE(hicpp-signed-bitwise)
            return static_cast<double>(ToInt(std::get<double>(left)) << ToUInt(std::get<double>(right)));
        }
        case lexer::TokenType::PUNCTUATOR_RIGHT_SHIFT: {  // NOLINTNEXTLINE(hicpp-signed-bitwise)
            return static_cast<double>(ToInt(std::get<double>(left)) >> ToUInt(std::get<double>(right)));
        }
        case lexer::TokenType::PUNCTUATOR_UNSIGNED_RIGHT_SHIFT: {
            return static_cast<double>(ToUInt(std::get<double>(left)) >> ToUInt(std::get<double>(right)));
        }
        case lexer::TokenType::PUNCTUATOR_PLUS: {
            return std::get<double>(left) + std::get<double>(right);
        }
        case lexer::TokenType::PUNCTUATOR_MINUS: {
            return std::get<double>(left) - std::get<double>(right);
        }
        case lexer::TokenType::PUNCTUATOR_MULTIPLY: {
            return std::get<double>(left) * std::get<double>(right);
        }
        case lexer::TokenType::PUNCTUATOR_DIVIDE: {
            return std::get<double>(left) / std::get<double>(right);
        }
        case lexer::TokenType::PUNCTUATOR_MOD: {
            return std::fmod(std::get<double>(left), std::get<double>(right));
        }
        case lexer::TokenType::PUNCTUATOR_EXPONENTIATION: {
            return std::pow(std::get<double>(left), std::get<double>(right));
        }
        default: {
            return false;
        }
    }
}

varbinder::EnumMemberResult TSAnalyzer::EvaluateBinaryExpression(checker::TSChecker *checker,
                                                                 varbinder::EnumVariable *enumVar,
                                                                 const ir::BinaryExpression *expr) const
{
    varbinder::EnumMemberResult left = EvaluateEnumMember(checker, enumVar, expr->AsBinaryExpression()->Left());
    varbinder::EnumMemberResult right = EvaluateEnumMember(checker, enumVar, expr->AsBinaryExpression()->Right());
    if (std::holds_alternative<double>(left) && std::holds_alternative<double>(right)) {
        GetOperationResulForDouble(expr->AsBinaryExpression()->OperatorType(), left, right);
    }

    if (std::holds_alternative<util::StringView>(left) && std::holds_alternative<util::StringView>(right) &&
        expr->AsBinaryExpression()->OperatorType() == lexer::TokenType::PUNCTUATOR_PLUS) {
        std::stringstream ss;
        ss << std::get<util::StringView>(left) << std::get<util::StringView>(right);

        util::UString res(ss.str(), checker->Allocator());
        return res.View();
    }

    return false;
}

varbinder::EnumMemberResult TSAnalyzer::EvaluateUnaryExpression(checker::TSChecker *checker,
                                                                varbinder::EnumVariable *enumVar,
                                                                const ir::UnaryExpression *expr) const
{
    varbinder::EnumMemberResult value = EvaluateEnumMember(checker, enumVar, expr->Argument());
    if (!std::holds_alternative<double>(value)) {
        return false;
    }

    switch (expr->OperatorType()) {
        case lexer::TokenType::PUNCTUATOR_PLUS: {
            return std::get<double>(value);
        }
        case lexer::TokenType::PUNCTUATOR_MINUS: {
            return -std::get<double>(value);
        }
        case lexer::TokenType::PUNCTUATOR_TILDE: {
            return static_cast<double>(~ToInt(std::get<double>(value)));  // NOLINT(hicpp-signed-bitwise)
        }
        default: {
            break;
        }
    }

    return false;
}

varbinder::EnumMemberResult TSAnalyzer::EvaluateEnumMember(checker::TSChecker *checker,
                                                           varbinder::EnumVariable *enumVar,
                                                           const ir::AstNode *expr) const
{
    switch (expr->Type()) {
        case ir::AstNodeType::UNARY_EXPRESSION: {
            return EvaluateUnaryExpression(checker, enumVar, expr->AsUnaryExpression());
        }
        case ir::AstNodeType::BINARY_EXPRESSION: {
            return EvaluateBinaryExpression(checker, enumVar, expr->AsBinaryExpression());
        }
        case ir::AstNodeType::NUMBER_LITERAL: {
            return expr->AsNumberLiteral()->Number().GetDouble();
        }
        case ir::AstNodeType::STRING_LITERAL: {
            return expr->AsStringLiteral()->Str();
        }
        case ir::AstNodeType::IDENTIFIER: {
            return EvaluateIdentifier(checker, enumVar, expr->AsIdentifier());
        }
        case ir::AstNodeType::MEMBER_EXPRESSION: {
            return EvaluateEnumMember(checker, enumVar, expr->AsMemberExpression());
        }
        default:
            break;
    }

    return false;
}

static bool IsComputedEnumMember(const ir::Expression *init)
{
    if (init->IsLiteral()) {
        return !init->AsLiteral()->IsStringLiteral() && !init->AsLiteral()->IsNumberLiteral();
    }

    if (init->IsTemplateLiteral()) {
        return !init->AsTemplateLiteral()->Quasis().empty();
    }

    return true;
}

static void AddEnumValueDeclaration(checker::TSChecker *checker, double number, varbinder::EnumVariable *variable)
{
    variable->SetTsType(checker->GlobalNumberType());

    util::StringView memberStr = util::Helpers::ToStringView(checker->Allocator(), number);

    varbinder::LocalScope *enumScope = checker->Scope()->AsLocalScope();
    varbinder::Variable *res = enumScope->FindLocal(memberStr, varbinder::ResolveBindingOptions::BINDINGS);
    varbinder::EnumVariable *enumVar = nullptr;

    if (res == nullptr) {
        auto *decl = checker->Allocator()->New<varbinder::EnumDecl>(memberStr);
        ES2PANDA_ASSERT(decl != nullptr);
        decl->BindNode(variable->Declaration()->Node());
        enumScope->AddDecl(checker->Allocator(), decl, ScriptExtension::TS);
        res = enumScope->FindLocal(memberStr, varbinder::ResolveBindingOptions::BINDINGS);
        ES2PANDA_ASSERT(res && res->IsEnumVariable());
        ES2PANDA_ASSERT(enumVar != nullptr);
        enumVar = res->AsEnumVariable();
        enumVar->AsEnumVariable()->SetBackReference();
        enumVar->SetTsType(checker->GlobalStringType());
    } else {
        ES2PANDA_ASSERT(res->IsEnumVariable());
        enumVar = res->AsEnumVariable();
        auto *decl = checker->Allocator()->New<varbinder::EnumDecl>(memberStr);
        ES2PANDA_ASSERT(decl != nullptr);
        decl->BindNode(variable->Declaration()->Node());
        enumVar->ResetDecl(decl);
    }

    enumVar->SetValue(variable->Declaration()->Name());
}

// NOLINTBEGIN(modernize-avoid-c-arrays)
static constexpr char const INVALID_COMPUTED_WITH_STRING[] =
    "Computed values are not permitted in an enum with string valued members.";
static constexpr char const INVALID_CONST_MEMBER[] =
    "'const' enum member initializers can only contain literal values and other computed enum values.";
static constexpr char const INVALID_CONST_NAN[] =
    "'const' enum member initializer was evaluated to disallowed value 'NaN'.";
static constexpr char const INVALID_CONST_INF[] =
    "'const' enum member initializer was evaluated to a non-finite value.";
// NOLINTEND(modernize-avoid-c-arrays)

void TSAnalyzer::InferEnumVariableType(varbinder::EnumVariable *variable, double *value, bool *initNext,
                                       bool *isLiteralEnum, bool isConstEnum) const
{
    TSChecker *checker = GetTSChecker();
    const ir::Expression *init = variable->Declaration()->Node()->AsTSEnumMember()->Init();

    if (init == nullptr && *initNext) {
        checker->ThrowTypeError("Enum member must have initializer.", variable->Declaration()->Node()->Start());
    }

    if (init == nullptr && !*initNext) {
        variable->SetValue(++(*value));
        AddEnumValueDeclaration(checker, *value, variable);
        return;
    }

    ES2PANDA_ASSERT(init);
    if (IsComputedEnumMember(init) && *isLiteralEnum) {
        checker->ThrowTypeError(INVALID_COMPUTED_WITH_STRING, init->Start());
    }

    varbinder::EnumMemberResult res = EvaluateEnumMember(checker, variable, init);
    if (std::holds_alternative<util::StringView>(res)) {
        *isLiteralEnum = true;
        variable->SetTsType(checker->GlobalStringType());
        *initNext = true;
        return;
    }

    if (std::holds_alternative<bool>(res)) {
        if (isConstEnum) {
            checker->ThrowTypeError(INVALID_CONST_MEMBER, init->Start());
        }

        *initNext = true;
        return;
    }

    ES2PANDA_ASSERT(std::holds_alternative<double>(res));
    variable->SetValue(res);

    *value = std::get<double>(res);
    if (isConstEnum && std::isnan(*value)) {
        checker->ThrowTypeError(INVALID_CONST_NAN, init->Start());
    }

    if (isConstEnum && std::isinf(*value)) {
        checker->ThrowTypeError(INVALID_CONST_INF, init->Start());
    }

    *initNext = false;
    AddEnumValueDeclaration(checker, *value, variable);
}

checker::Type *TSAnalyzer::InferType(checker::TSChecker *checker, bool isConst, ir::TSEnumDeclaration *st) const
{
    double value = -1.0;

    varbinder::LocalScope *enumScope = checker->Scope()->AsLocalScope();

    bool initNext = false;
    bool isLiteralEnum = false;
    size_t localsSize = enumScope->Decls().size();

    for (size_t i = 0; i < localsSize; i++) {
        const util::StringView &currentName = enumScope->Decls()[i]->Name();
        varbinder::Variable *currentVar = enumScope->FindLocal(currentName, varbinder::ResolveBindingOptions::BINDINGS);
        ES2PANDA_ASSERT(currentVar && currentVar->IsEnumVariable());
        InferEnumVariableType(currentVar->AsEnumVariable(), &value, &initNext, &isLiteralEnum, isConst);
    }

    checker::Type *enumType = checker->Allocator()->New<checker::EnumLiteralType>(
        st->Key()->Name(), checker->Scope(),
        isLiteralEnum ? checker::EnumLiteralType::EnumLiteralTypeKind::LITERAL
                      : checker::EnumLiteralType::EnumLiteralTypeKind::NUMERIC);

    return enumType;
}

checker::Type *TSAnalyzer::Check(ir::TSEnumDeclaration *st) const
{
    TSChecker *checker = GetTSChecker();
    varbinder::Variable *enumVar = st->Key()->Variable();
    ES2PANDA_ASSERT(enumVar);

    if (enumVar->TsType() == nullptr) {
        checker::ScopeContext scopeCtx(checker, st->Scope());
        checker::Type *enumType = InferType(checker, st->IsConst(), st);
        ES2PANDA_ASSERT(enumType != nullptr);
        enumType->SetVariable(enumVar);
        enumVar->SetTsType(enumType);
    }

    return nullptr;
}

checker::Type *TSAnalyzer::Check(ir::TSFunctionType *node) const
{
    TSChecker *checker = GetTSChecker();
    checker::ScopeContext scopeCtx(checker, node->Scope());

    auto *signatureInfo = checker->Allocator()->New<checker::SignatureInfo>(checker->Allocator());
    checker->CheckFunctionParameterDeclarations(node->Params(), signatureInfo);
    node->ReturnType()->Check(checker);
    auto *callSignature =
        checker->Allocator()->New<checker::Signature>(signatureInfo, node->ReturnType()->GetType(checker));

    return checker->CreateFunctionTypeWithSignature(callSignature);
}

checker::Type *TSAnalyzer::Check(ir::TSIndexedAccessType *node) const
{
    TSChecker *checker = GetTSChecker();
    node->objectType_->Check(checker);
    node->indexType_->Check(checker);
    checker::Type *resolved = node->GetType(checker);

    if (resolved != nullptr) {
        return nullptr;
    }

    checker::Type *indexType = checker->CheckTypeCached(node->indexType_);

    if (!indexType->HasTypeFlag(checker::TypeFlag::STRING_LIKE | checker::TypeFlag::NUMBER_LIKE)) {
        checker->ThrowTypeError({"Type ", indexType, " cannot be used as index type"}, node->IndexType()->Start());
    }

    if (indexType->IsNumberType()) {
        checker->ThrowTypeError("Type has no matching signature for type 'number'", node->Start());
    }

    checker->ThrowTypeError("Type has no matching signature for type 'string'", node->Start());
    return nullptr;
}

checker::Type *TSAnalyzer::Check(ir::TSInterfaceBody *expr) const
{
    TSChecker *checker = GetTSChecker();
    for (auto *it : expr->Body()) {
        it->Check(checker);
    }

    return nullptr;
}

static void CheckInheritedPropertiesAreIdentical(checker::TSChecker *checker, checker::InterfaceType *type,
                                                 const lexer::SourcePosition &locInfo)
{
    checker->GetBaseTypes(type);

    size_t constexpr BASE_SIZE_LIMIT = 2;
    if (type->Bases().size() < BASE_SIZE_LIMIT) {
        return;
    }

    checker->ResolveDeclaredMembers(type);

    checker::InterfacePropertyMap properties;

    for (auto *it : type->Properties()) {
        properties.insert({it->Name(), {it, type}});
    }

    for (auto *base : type->Bases()) {
        checker->ResolveStructuredTypeMembers(base);
        ArenaVector<varbinder::LocalVariable *> inheritedProperties(checker->Allocator()->Adapter());
        base->AsInterfaceType()->CollectProperties(&inheritedProperties);

        for (auto *inheritedProp : inheritedProperties) {
            auto res = properties.find(inheritedProp->Name());
            if (res == properties.end()) {
                properties.insert({inheritedProp->Name(), {inheritedProp, base->AsInterfaceType()}});
            } else if (res->second.second != type) {
                checker::Type *sourceType = checker->GetTypeOfVariable(inheritedProp);
                checker::Type *targetType = checker->GetTypeOfVariable(res->second.first);
                checker->IsTypeIdenticalTo(sourceType, targetType, diagnostic::IFACE_MULTIPLE_EXTENSION,
                                           {type, res->second.second, base->AsInterfaceType()}, locInfo);
            }
        }
    }
}

checker::Type *TSAnalyzer::Check(ir::TSInterfaceDeclaration *st) const
{
    TSChecker *checker = GetTSChecker();
    varbinder::Variable *var = st->Id()->Variable();
    ES2PANDA_ASSERT(var->Declaration()->Node() && var->Declaration()->Node()->IsTSInterfaceDeclaration());

    if (st == var->Declaration()->Node()) {
        checker::Type *resolvedType = var->TsType();

        if (resolvedType == nullptr) {
            checker::ObjectDescriptor *desc =
                checker->Allocator()->New<checker::ObjectDescriptor>(checker->Allocator());
            resolvedType =
                checker->Allocator()->New<checker::InterfaceType>(checker->Allocator(), st->Id()->Name(), desc);
            ES2PANDA_ASSERT(resolvedType != nullptr);
            resolvedType->SetVariable(var);
            var->SetTsType(resolvedType);
        }

        checker::InterfaceType *resolvedInterface = resolvedType->AsObjectType()->AsInterfaceType();
        CheckInheritedPropertiesAreIdentical(checker, resolvedInterface, st->Id()->Start());

        for (auto *base : resolvedInterface->Bases()) {
            checker->IsTypeAssignableTo(resolvedInterface, base, diagnostic::IFACE_INVALID_EXTENDS,
                                        {st->Id()->Name(), base}, st->Id()->Start());
        }

        checker->CheckIndexConstraints(resolvedInterface);
    }

    st->Body()->Check(checker);

    return nullptr;
}

checker::Type *TSAnalyzer::Check(ir::TSLiteralType *node) const
{
    TSChecker *checker = GetTSChecker();
    node->GetType(checker);
    return nullptr;
}

checker::Type *TSAnalyzer::Check(ir::TSNamedTupleMember *node) const
{
    TSChecker *checker = GetTSChecker();
    node->ElementType()->Check(checker);
    return nullptr;
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::TSNeverKeyword *node) const
{
    return nullptr;
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::TSNullKeyword *node) const
{
    return nullptr;
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::TSNumberKeyword *node) const
{
    return nullptr;
}

checker::Type *TSAnalyzer::Check(ir::TSParenthesizedType *node) const
{
    TSChecker *checker = GetTSChecker();
    node->type_->Check(checker);
    return nullptr;
}

checker::Type *TSAnalyzer::Check(ir::TSQualifiedName *expr) const
{
    TSChecker *checker = GetTSChecker();
    checker::Type *baseType = checker->CheckNonNullType(expr->Left()->Check(checker), expr->Left()->Start());
    varbinder::Variable *prop = checker->GetPropertyOfType(baseType, expr->Right()->Name());

    if (prop != nullptr) {
        return checker->GetTypeOfVariable(prop);
    }

    if (baseType->IsObjectType()) {
        checker::ObjectType *objType = baseType->AsObjectType();

        if (objType->StringIndexInfo() != nullptr) {
            return objType->StringIndexInfo()->GetType();
        }
    }

    checker->ThrowTypeError({"Property ", expr->Right()->Name(), " does not exist on this type."},
                            expr->Right()->Start());
    return nullptr;
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::TSStringKeyword *node) const
{
    return nullptr;
}

checker::Type *TSAnalyzer::Check(ir::TSTupleType *node) const
{
    TSChecker *checker = GetTSChecker();
    for (auto *it : node->ElementType()) {
        it->Check(checker);
    }

    node->GetType(checker);
    return nullptr;
}

checker::Type *TSAnalyzer::Check(ir::TSTypeAliasDeclaration *st) const
{
    TSChecker *checker = GetTSChecker();
    st->TypeAnnotation()->Check(checker);
    return nullptr;
}

checker::Type *TSAnalyzer::Check(ir::TSTypeLiteral *node) const
{
    TSChecker *checker = GetTSChecker();

    for (auto *it : node->Members()) {
        it->Check(checker);
    }

    checker::Type *type = node->GetType(checker);
    checker->CheckIndexConstraints(type);

    return nullptr;
}

checker::Type *TSAnalyzer::Check(ir::TSTypeQuery *node) const
{
    TSChecker *checker = GetTSChecker();
    if (node->TsType() != nullptr) {
        return node->TsType();
    }

    node->SetTsType(node->exprName_->Check(checker));
    return node->TsType();
}

checker::Type *TSAnalyzer::Check(ir::TSTypeReference *node) const
{
    TSChecker *checker = GetTSChecker();
    node->GetType(checker);
    return nullptr;
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::TSUndefinedKeyword *node) const
{
    return nullptr;
}

checker::Type *TSAnalyzer::Check(ir::TSUnionType *node) const
{
    TSChecker *checker = GetTSChecker();
    for (auto *it : node->Types()) {
        it->Check(checker);
    }

    node->GetType(checker);
    return nullptr;
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::TSUnknownKeyword *node) const
{
    return nullptr;
}

checker::Type *TSAnalyzer::Check([[maybe_unused]] ir::TSVoidKeyword *node) const
{
    return nullptr;
}
}  // namespace ark::es2panda::checker

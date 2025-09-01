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

#include "lreference.h"

#include "varbinder/declaration.h"
#include "varbinder/variableFlags.h"
#include "compiler/base/destructuring.h"
#include "compiler/core/function.h"
#include "compiler/core/pandagen.h"
#include "compiler/core/ETSGen.h"
#include "checker/types/ets/etsTupleType.h"
#include "ir/base/spreadElement.h"
#include "ir/base/classProperty.h"
#include "ir/base/classDefinition.h"
#include "ir/base/scriptFunction.h"
#include "ir/expressions/assignmentExpression.h"
#include "ir/expressions/memberExpression.h"
#include "ir/statements/variableDeclaration.h"
#include "util/helpers.h"

namespace ark::es2panda::compiler {

LReference::LReferenceBase LReference::CreateBase(CodeGen *cg, const ir::AstNode *node, bool isDeclaration)
{
    switch (node->Type()) {
        // NOTE: This case is never reached in case of ETS
        case ir::AstNodeType::IDENTIFIER: {
            const util::StringView &name = node->AsIdentifier()->Name();
            auto res = cg->Scope()->Find(name, varbinder::ResolveBindingOptions::ALL);
            if (res.variable == nullptr) {
                res.variable = node->AsIdentifier()->Variable();
            }

            return {cg, node, ReferenceKind::VAR_OR_GLOBAL, res, isDeclaration};
        }
        case ir::AstNodeType::MEMBER_EXPRESSION: {
            return {cg, node, ReferenceKind::MEMBER, {}, false};
        }
        case ir::AstNodeType::VARIABLE_DECLARATION: {
            ES2PANDA_ASSERT(node->AsVariableDeclaration()->Declarators().size() == 1);
            return CreateBase(cg, node->AsVariableDeclaration()->Declarators()[0]->Id(), true);
        }
        case ir::AstNodeType::VARIABLE_DECLARATOR: {
            return CreateBase(cg, node->AsVariableDeclarator()->Id(), true);
        }
        case ir::AstNodeType::ARRAY_PATTERN:
        case ir::AstNodeType::OBJECT_PATTERN: {
            return {cg, node, ReferenceKind::DESTRUCTURING, {}, isDeclaration};
        }
        case ir::AstNodeType::ASSIGNMENT_PATTERN: {
            return CreateBase(cg, node->AsAssignmentPattern()->Left(), true);
        }
        case ir::AstNodeType::REST_ELEMENT: {
            return CreateBase(cg, node->AsRestElement()->Argument(), true);
        }
        case ir::AstNodeType::TS_NON_NULL_EXPRESSION: {
            return CreateBase(cg, node->AsTSNonNullExpression()->Expr(), isDeclaration);
        }
        case ir::AstNodeType::ETS_NEW_CLASS_INSTANCE_EXPRESSION: {
            ES2PANDA_ASSERT(node->AsETSNewClassInstanceExpression()->GetArguments().size() == 1);
            return CreateBase(cg, node->AsETSNewClassInstanceExpression()->GetArguments()[0], isDeclaration);
        }
        default: {
            ES2PANDA_UNREACHABLE();
        }
    }
}

JSLReference::JSLReference(CodeGen *cg, const ir::AstNode *node, ReferenceKind refKind,
                           varbinder::ConstScopeFindResult res, bool isDeclaration)
    : LReference(node, refKind, res, isDeclaration), pg_(static_cast<PandaGen *>(cg))
{
    if (Kind() != ReferenceKind::MEMBER) {
        return;
    }

    const auto *memberExpr = Node()->AsMemberExpression();

    if (memberExpr->Object()->IsSuperExpression()) {
        SetKind(ReferenceKind::SUPER);
    } else if (memberExpr->IsPrivateReference()) {
        SetKind(ReferenceKind::PRIVATE);
        privateCtor_ = pg_->AllocReg();
        Function::LoadClassContexts(Node(), pg_, privateCtor_, memberExpr->Property()->AsIdentifier()->Name());
    }

    obj_ = pg_->AllocReg();
    memberExpr->Object()->Compile(pg_);
    pg_->StoreAccumulator(Node(), obj_);

    prop_ = pg_->ToNamedPropertyKey(memberExpr->Property(), memberExpr->IsComputed());
    if (std::holds_alternative<util::StringView>(prop_)) {
        return;
    }

    if (std::holds_alternative<int64_t>(prop_) && Kind() != ReferenceKind::SUPER) {
        return;
    }

    memberExpr->Property()->Compile(pg_);

    VReg propReg = pg_->AllocReg();
    pg_->StoreAccumulator(Node(), propReg);
    prop_ = propReg;
}

void JSLReference::GetValue() const
{
    switch (Kind()) {
        case ReferenceKind::VAR_OR_GLOBAL: {
            pg_->LoadVar(Node()->AsIdentifier(), Result());
            break;
        }
        case ReferenceKind::MEMBER: {
            if (std::holds_alternative<VReg>(prop_)) {
                pg_->LoadObjProperty(Node(), obj_);
                break;
            }
            [[fallthrough]];
        }
        case ReferenceKind::SUPER: {
            pg_->LoadObjProperty(Node(), prop_);
            break;
        }
        case ReferenceKind::PRIVATE: {
            pg_->ClassPrivateFieldGet(Node(), privateCtor_, obj_, std::get<util::StringView>(prop_));
            break;
        }
        default: {
            ES2PANDA_UNREACHABLE();
        }
    }
}

void JSLReference::SetValue() const
{
    switch (Kind()) {
        case ReferenceKind::VAR_OR_GLOBAL: {
            pg_->StoreVar(Node(), Result(), IsDeclaration());
            break;
        }
        case ReferenceKind::SUPER: {
            pg_->StoreSuperProperty(Node(), obj_, prop_);

            break;
        }
        case ReferenceKind::MEMBER: {
            pg_->StoreObjProperty(Node(), obj_, prop_);

            break;
        }
        case ReferenceKind::PRIVATE: {
            pg_->ClassPrivateFieldSet(Node(), privateCtor_, obj_, std::get<util::StringView>(prop_));
            break;
        }
        case ReferenceKind::DESTRUCTURING: {
            Destructuring::Compile(pg_, Node()->AsExpression());
            break;
        }
        default: {
            ES2PANDA_UNREACHABLE();
        }
    }
}

ETSLReference::ETSLReference(CodeGen *cg, const ir::AstNode *node, ReferenceKind refKind,
                             varbinder::ConstScopeFindResult res, bool isDeclaration)
    : LReference(node, refKind, res, isDeclaration), etsg_(static_cast<ETSGen *>(cg))
{
    if (Kind() != ReferenceKind::MEMBER) {
        SetKind(ResolveReferenceKind(res.variable));
        return;
    }

    const auto *memberExpr = Node()->AsMemberExpression();
    staticObjRef_ = memberExpr->Object()->TsType();
    if (!memberExpr->IsComputed() && memberExpr->PropVar() != nullptr &&
        etsg_->Checker()->IsVariableStatic(memberExpr->PropVar())) {
        return;
    }

    TargetTypeContext ttctx(etsg_, memberExpr->Object()->TsType());
    memberExpr->Object()->Compile(etsg_);
    baseReg_ = etsg_->AllocReg();
    etsg_->StoreAccumulator(node, baseReg_);

    if (memberExpr->IsComputed()) {
        TargetTypeContext pttctx(etsg_, memberExpr->Property()->TsType());
        memberExpr->Property()->Compile(etsg_);
        etsg_->ApplyConversion(memberExpr->Property());
        ES2PANDA_ASSERT(memberExpr->Object()->TsType()->IsETSAnyType() ||
                        etsg_->GetAccumulatorType()->HasTypeFlag(checker::TypeFlag::ETS_INTEGRAL));
        propReg_ = etsg_->AllocReg();
        etsg_->StoreAccumulator(node, propReg_);
    }
}

ETSLReference ETSLReference::Create(CodeGen *const cg, const ir::AstNode *const node, const bool isDeclaration)
{
    if (node->Type() == ir::AstNodeType::IDENTIFIER) {
        auto *var = node->Variable();
        ES2PANDA_ASSERT(var != nullptr);
        varbinder::ConstScopeFindResult res;
        res.name = var->Name();
        res.variable = var;
        res.scope = var->GetScope();
        auto refKind = ReferenceKind::VAR_OR_GLOBAL;
        if (var->HasFlag(varbinder::VariableFlags::PROPERTY)) {
            refKind = ReferenceKind::FIELD;
        }
        return {cg, node, refKind, res, isDeclaration};
    }
    return std::make_from_tuple<ETSLReference>(CreateBase(cg, node, isDeclaration));
}

ReferenceKind ETSLReference::ResolveReferenceKind(const varbinder::Variable *variable)
{
    if (variable->HasFlag(varbinder::VariableFlags::SYNTHETIC)) {
        return ReferenceKind::METHOD;
    }
    if (variable->HasFlag(varbinder::VariableFlags::LOCAL)) {
        return ReferenceKind::LOCAL;
    }

    auto *declNode = variable->Declaration()->Node();

    switch (declNode->Type()) {
        case ir::AstNodeType::CLASS_PROPERTY: {
            auto *classField = declNode->AsClassProperty();
            return classField->IsStatic() ? ReferenceKind::STATIC_FIELD : ReferenceKind::FIELD;
        }
        case ir::AstNodeType::CLASS_DEFINITION: {
            auto *classDef = declNode->AsClassDefinition();
            return classDef->IsStatic() ? ReferenceKind::STATIC_CLASS : ReferenceKind::CLASS;
        }
        case ir::AstNodeType::METHOD_DEFINITION: {
            return ReferenceKind::METHOD;
        }
        case ir::AstNodeType::TS_INTERFACE_DECLARATION: {
            return ReferenceKind::CLASS;
        }
        default: {
            break;
        }
    }

    return ReferenceKind::LOCAL;
}

void ETSLReference::GetValue() const
{
    switch (Kind()) {
        case ReferenceKind::MEMBER: {
            Node()->AsMemberExpression()->Compile(etsg_);
            break;
        }
        default: {
            ES2PANDA_ASSERT(Node()->IsIdentifier());
            auto const *const ident = Node()->AsIdentifier();
            auto const *const variable = Variable();
            etsg_->LoadVar(ident, variable);
            //  Process possible smart type of identifier.
            if (auto const *const smartType = ident->TsType();
                !etsg_->Checker()->Relation()->IsIdenticalTo(smartType, variable->TsType())) {
                etsg_->SetAccumulatorType(smartType);
            }
            break;
        }
    }
}

void ETSLReference::SetValueComputed(const ir::MemberExpression *memberExpr) const
{
    const auto *const objectType = memberExpr->Object()->TsType();

    if (objectType->IsETSAnyType()) {
        if (memberExpr->Property()->TsType()->HasTypeFlag(checker::TypeFlag::ETS_NUMERIC)) {
            etsg_->StoreByIndexAny(memberExpr, baseReg_, propReg_);
        } else {
            etsg_->StoreByValueAny(memberExpr, baseReg_, propReg_);
        }
        return;
    }

    if (objectType->IsETSTupleType()) {
        ES2PANDA_ASSERT(memberExpr->GetTupleIndexValue().has_value());

        std::size_t indexValue = *memberExpr->GetTupleIndexValue();
        etsg_->StoreTupleElement(Node(), baseReg_, objectType->AsETSTupleType()->GetTypeAtIndex(indexValue),
                                 indexValue);
        return;
    }

    if (objectType->IsETSArrayType() || objectType->IsETSResizableArrayType()) {
        auto vRegtype = etsg_->GetVRegType(baseReg_);
        ES2PANDA_ASSERT(vRegtype != nullptr);
        auto *elementType = vRegtype->IsETSArrayType() ? vRegtype->AsETSArrayType()->ElementType()
                                                       : vRegtype->AsETSResizableArrayType()->ElementType();
        etsg_->StoreArrayElement(Node(), baseReg_, propReg_, elementType);
        return;
    }

    ES2PANDA_ASSERT(objectType->IsETSNeverType());  // nothing to do, we're in dead code anyway
}

void ETSLReference::SetValueGetterSetter(const ir::MemberExpression *memberExpr) const
{
    ES2PANDA_ASSERT(memberExpr->PropVar() != nullptr);
    const auto *sig = memberExpr->PropVar()->TsType()->AsETSFunctionType()->FindSetter();
    ES2PANDA_ASSERT(sig->Function() != nullptr);

    auto argReg = etsg_->AllocReg();
    etsg_->StoreAccumulator(Node(), argReg);
    if (sig->Function()->IsStatic()) {
        etsg_->CallExact(Node(), sig->InternalName(), argReg);
    } else if (memberExpr->Object()->IsSuperExpression()) {
        etsg_->CallExact(Node(), sig->InternalName(), baseReg_, argReg);
    } else {
        etsg_->CallVirtual(Node(), sig, baseReg_, argReg);
    }
}

void ETSLReference::SetValue() const
{
    if (Kind() != ReferenceKind::MEMBER) {
        etsg_->StoreVar(Node()->AsIdentifier(), Result());
        return;
    }

    const auto *const memberExpr = Node()->AsMemberExpression();
    const auto *const memberExprTsType = memberExpr->TsType();
    auto const *objectType = memberExpr->Object()->TsType();

    if (!memberExpr->IsIgnoreBox()) {
        etsg_->ApplyConversion(Node(), memberExprTsType);
    }

    if (memberExpr->IsComputed()) {
        SetValueComputed(memberExpr);
        return;
    }

    if (objectType->IsETSAnyType()) {
        etsg_->StorePropertyByNameAny(memberExpr, baseReg_, memberExpr->Property()->AsIdentifier()->Name());
        return;
    }

    ES2PANDA_ASSERT(memberExpr->PropVar() != nullptr);
    if (memberExpr->PropVar()->TsType()->HasTypeFlag(checker::TypeFlag::GETTER_SETTER)) {
        SetValueGetterSetter(memberExpr);
        return;
    }

    const util::StringView fullName = etsg_->FormClassPropReference(memberExpr->Property()->Variable());

    if (memberExpr->PropVar()->HasFlag(varbinder::VariableFlags::STATIC)) {
        etsg_->StoreStaticProperty(Node(), memberExprTsType, fullName);
        return;
    }

    if (objectType->IsETSUnionType()) {
        etsg_->StorePropertyByName(Node(), baseReg_,
                                   checker::ETSChecker::FormNamedAccessMetadata(memberExpr->PropVar()));
        return;
    }

    const auto *type = memberExpr->PropVar()->TsType();

    etsg_->StoreProperty(Node(), type, baseReg_, fullName);
}

}  // namespace ark::es2panda::compiler

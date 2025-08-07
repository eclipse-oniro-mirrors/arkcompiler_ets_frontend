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

#include "memberExpression.h"

#include "checker/TSchecker.h"
#include "checker/ets/castingContext.h"
#include "checker/types/ets/etsTupleType.h"
#include "compiler/core/ETSGen.h"
#include "compiler/core/pandagen.h"
#include "util/diagnostic.h"
#include "util/es2pandaMacros.h"

namespace ark::es2panda::ir {
MemberExpression::MemberExpression([[maybe_unused]] Tag const tag, MemberExpression const &other,
                                   ArenaAllocator *allocator)
    : MemberExpression(other)
{
    object_ = other.object_ != nullptr ? other.object_->Clone(allocator, this)->AsExpression() : nullptr;
    property_ = other.property_ != nullptr ? other.property_->Clone(allocator, this)->AsExpression() : nullptr;
}

bool MemberExpression::IsPrivateReference() const noexcept
{
    return property_->IsIdentifier() && property_->AsIdentifier()->IsPrivateIdent();
}

void MemberExpression::TransformChildren(const NodeTransformer &cb, std::string_view const transformationName)
{
    if (auto *transformedNode = cb(object_); object_ != transformedNode) {
        object_->SetTransformedNode(transformationName, transformedNode);
        object_ = transformedNode->AsExpression();
    }

    if (auto *transformedNode = cb(property_); property_ != transformedNode) {
        property_->SetTransformedNode(transformationName, transformedNode);
        property_ = transformedNode->AsExpression();
    }
}

void MemberExpression::Iterate(const NodeTraverser &cb) const
{
    cb(object_);
    cb(property_);
}

void MemberExpression::Dump(ir::AstDumper *dumper) const
{
    dumper->Add({{"type", "MemberExpression"},
                 {"object", object_},
                 {"property", property_},
                 {"computed", computed_},
                 {"optional", IsOptional()}});
}

void MemberExpression::Dump(ir::SrcDumper *dumper) const
{
    ES2PANDA_ASSERT(object_ != nullptr);
    ES2PANDA_ASSERT(property_ != nullptr);

    object_->Dump(dumper);
    if (IsOptional()) {
        dumper->Add("?");
        if ((MemberExpressionKind::ELEMENT_ACCESS & kind_) != 0U) {
            dumper->Add(".");
        }
    }
    if ((MemberExpressionKind::ELEMENT_ACCESS & kind_) != 0U) {
        dumper->Add("[");
        property_->Dump(dumper);
        dumper->Add("]");
    } else {
        dumper->Add(".");
        property_->Dump(dumper);
    }
    if ((parent_ != nullptr) && (parent_->IsBlockStatement() || parent_->IsBlockExpression())) {
        dumper->Add(";");
        dumper->Endl();
    }
}

void MemberExpression::LoadRhs(compiler::PandaGen *pg) const
{
    compiler::RegScope rs(pg);
    bool isSuper = object_->IsSuperExpression();
    compiler::Operand prop = pg->ToPropertyKey(property_, computed_, isSuper);

    if (isSuper) {
        pg->LoadSuperProperty(this, prop);
    } else if (IsPrivateReference()) {
        const auto &name = property_->AsIdentifier()->Name();
        compiler::VReg objReg = pg->AllocReg();
        pg->StoreAccumulator(this, objReg);
        compiler::VReg ctor = pg->AllocReg();
        compiler::Function::LoadClassContexts(this, pg, ctor, name);
        pg->ClassPrivateFieldGet(this, ctor, objReg, name);
    } else {
        pg->LoadObjProperty(this, prop);
    }
}

void MemberExpression::CompileToRegs(compiler::PandaGen *pg, compiler::VReg object, compiler::VReg property) const
{
    object_->Compile(pg);
    pg->StoreAccumulator(this, object);

    pg->OptionalChainCheck(IsOptional(), object);

    if (!computed_) {
        pg->LoadAccumulatorString(this, property_->AsIdentifier()->Name());
    } else {
        property_->Compile(pg);
    }

    pg->StoreAccumulator(this, property);
}

void MemberExpression::Compile(compiler::PandaGen *pg) const
{
    pg->GetAstCompiler()->Compile(this);
}

void MemberExpression::CompileToReg(compiler::PandaGen *pg, compiler::VReg objReg) const
{
    object_->Compile(pg);
    pg->StoreAccumulator(this, objReg);
    pg->OptionalChainCheck(IsOptional(), objReg);
    LoadRhs(pg);
}

void MemberExpression::Compile(compiler::ETSGen *etsg) const
{
    etsg->GetAstCompiler()->Compile(this);
}

checker::Type *MemberExpression::Check(checker::TSChecker *checker)
{
    return checker->GetAnalyzer()->Check(this);
}

std::pair<checker::Type *, varbinder::LocalVariable *> MemberExpression::ResolveObjectMember(
    checker::ETSChecker *checker) const
{
    auto resolveRes = checker->ResolveMemberReference(this, objType_);
    switch (resolveRes.size()) {
        case 0U: {
            /* resolution failed, error already reported */
            return {nullptr, nullptr};
        }
        case 1U: {
            if (resolveRes[0]->Kind() == checker::ResolvedKind::PROPERTY) {
                auto var = resolveRes[0]->Variable()->AsLocalVariable();
                checker->ValidatePropertyAccess(var, objType_, property_->Start());
                return {checker->GetTypeOfVariable(var), var};
            }

            if (resolveRes[0]->Kind() == checker::ResolvedKind::EXTENSION_ACCESSOR) {
                auto *callee = const_cast<ir::Expression *>(this->AsExpression());
                callee->AsMemberExpression()->AddMemberKind(ir::MemberExpressionKind::EXTENSION_ACCESSOR);
                return {resolveRes[0]->Variable()->TsType(), nullptr};
            }

            return {checker->GetTypeOfVariable(resolveRes[0]->Variable()), nullptr};
        }
        case 2U: {
            auto classMethodType = checker->GetTypeOfVariable(resolveRes[1]->Variable());
            auto extensionMethodType = checker->GetTypeOfVariable(resolveRes[0]->Variable());
            auto *resolvedType = extensionMethodType;
            if (classMethodType->IsETSArrowType()) {
                auto *callee = const_cast<ir::Expression *>(this->AsExpression());
                callee->AsMemberExpression()->AddMemberKind(ir::MemberExpressionKind::EXTENSION_ACCESSOR);
                return {resolveRes[0]->Variable()->TsType(), nullptr};
            }

            if (classMethodType->IsETSFunctionType()) {
                ES2PANDA_ASSERT(extensionMethodType->IsETSFunctionType());
                resolvedType = checker->CreateETSExtensionFuncHelperType(classMethodType->AsETSFunctionType(),
                                                                         extensionMethodType->AsETSFunctionType());
            }
            return {resolvedType, nullptr};
        }
        default: {
            ES2PANDA_UNREACHABLE();
        }
    }
}

checker::Type *MemberExpression::TraverseUnionMember(checker::ETSChecker *checker, checker::ETSUnionType *unionType)

{
    checker::Type *commonPropType = nullptr;

    auto const addPropType = [this, checker, &commonPropType](checker::Type *memberType) {
        if (commonPropType == nullptr) {
            commonPropType = memberType;
            return;
        }

        if (memberType == nullptr) {
            checker->LogError(diagnostic::MEMBER_TYPE_MISMATCH_ACROSS_UNION, {}, Start());
            return;
        }

        if (memberType->IsETSMethodType() && memberType->Variable()->HasFlag(varbinder::VariableFlags::OVERLOAD)) {
            checker->LogError(diagnostic::OVERLOADED_UNION_CALL, {}, Start());
            return;
        }

        if (memberType->IsETSMethodType() && memberType->AsETSFunctionType()->CallSignatures().size() > 1U) {
            if (!Parent()->IsCallExpression() || Parent()->AsCallExpression()->Callee() != this) {
                commonPropType = checker->GlobalTypeError();
                checker->LogError(diagnostic::OVERLOADED_METHOD_AS_VALUE, Start());
                return;
            }
        }

        if (!commonPropType->IsETSMethodType() && !memberType->IsETSMethodType()) {
            if (!checker->IsTypeIdenticalTo(commonPropType, memberType)) {
                checker->LogError(diagnostic::MEMBER_TYPE_MISMATCH_ACROSS_UNION, {}, Start());
            }
            return;
        }

        if (!commonPropType->IsETSFunctionType() || !memberType->IsETSFunctionType()) {
            checker->LogError(diagnostic::MEMBER_TYPE_MISMATCH_ACROSS_UNION, {}, Start());
            commonPropType = checker->GlobalTypeError();
            return;
        }

        auto newType =
            checker->IntersectSignatureSets(commonPropType->AsETSFunctionType(), memberType->AsETSFunctionType());
        if (newType->AsETSFunctionType()->CallSignatures().empty()) {
            checker->LogError(diagnostic::MEMBER_TYPE_MISMATCH_ACROSS_UNION, {}, Start());
        }

        commonPropType = newType;
    };

    for (auto *const type : unionType->ConstituentTypes()) {
        auto *const apparent = checker->GetApparentType(type);
        ES2PANDA_ASSERT(apparent != nullptr);
        if (apparent->IsETSObjectType()) {
            SetObjectType(apparent->AsETSObjectType());
            auto *memberType = ResolveObjectMember(checker).first;
            if (memberType != nullptr && memberType->IsTypeError()) {
                return checker->GlobalTypeError();
            }

            addPropType(memberType);
        } else {
            checker->LogError(diagnostic::UNION_MEMBER_ILLEGAL_TYPE, {unionType}, Start());
        }
    }
    return commonPropType;
}

checker::Type *MemberExpression::CheckUnionMember(checker::ETSChecker *checker, checker::Type *baseType)
{
    auto *const unionType = baseType->AsETSUnionType();
    if (object_->Variable() != nullptr && object_->Variable()->Declaration() != nullptr &&
        object_->Variable()->Declaration()->IsTypeAliasDecl()) {
        checker->LogError(diagnostic::STATIC_UNION, {}, Start());
        return checker->GlobalTypeError();
    }
    auto *const commonPropType = TraverseUnionMember(checker, unionType);
    SetObjectType(checker->GlobalETSObjectType());
    return commonPropType;
}

static checker::Type *AdjustRecordReturnType(checker::Type *type, checker::Type *objType)
{
    auto *recordKeyType = objType->AsETSObjectType()->TypeArguments()[0];
    auto *recordValueType = objType->AsETSObjectType()->TypeArguments()[1];

    auto const isStringLiteralOrConstantUnion = [](checker::Type *recordKey) {
        if (recordKey->IsETSStringType() && recordKey->IsConstantType()) {
            return true;
        }
        if (!recordKey->IsETSUnionType()) {
            return false;
        }
        auto constituentTypes = recordKey->AsETSUnionType()->ConstituentTypes();
        return std::all_of(constituentTypes.begin(), constituentTypes.end(),
                           [](auto *it) { return it->IsETSStringType() && it->IsConstantType(); });
    };
    if (isStringLiteralOrConstantUnion(recordKeyType)) {
        if (type->IsETSUnionType()) {
            return recordValueType;
        }

        if (type->IsETSFunctionType() && type->AsETSFunctionType()->Name().Is(compiler::Signatures::GET_INDEX_METHOD)) {
            type->AsETSFunctionType()->CallSignatures()[0]->SetReturnType(recordValueType);
        }
    }

    return type;
}

checker::Type *MemberExpression::AdjustType(checker::ETSChecker *checker, checker::Type *type)
{
    auto *const objType = checker->GetApparentType(Object()->TsType());
    ES2PANDA_ASSERT(objType != nullptr);
    if (type != nullptr && objType->IsETSObjectType() &&
        objType->ToAssemblerName().str() == compiler::Signatures::BUILTIN_RECORD) {
        type = AdjustRecordReturnType(type, objType);
    }

    if (PropVar() != nullptr) {  // access erased property type
        uncheckedType_ = checker->GuaranteedTypeForUncheckedPropertyAccess(PropVar());
    } else if (IsComputed() && objType->IsETSArrayType()) {  // access erased array or tuple type
        uncheckedType_ = checker->GuaranteedTypeForUncheckedCast(objType->AsETSArrayType()->ElementType(), type);
    } else if (IsComputed() && objType->IsETSTupleType()) {
        if (!checker->ValidateTupleIndex(objType->AsETSTupleType(), this, false)) {
            // error recovery
            return checker->InvalidateType(this);
        }
        uncheckedType_ = checker->GetApparentType(checker->MaybeBoxType(GetTypeOfTupleElement(checker, objType)));
    } else if (objType->IsETSUnionType()) {
        uncheckedType_ = checker->GuaranteedTypeForUnionFieldAccess(this, objType->AsETSUnionType());
    } else if (checker->IsExtensionAccessorFunctionType(type)) {
        SetTsType(type);
        checker::Type *accessorReturnType = checker->GetExtensionAccessorReturnType(this);
        SetTsType(accessorReturnType == nullptr ? checker->GlobalTypeError() : accessorReturnType);
        return TsType();
    }
    SetTsType(type == nullptr ? checker->GlobalTypeError() : type);
    return TsType();
}

checker::Type *MemberExpression::SetAndAdjustType(checker::ETSChecker *checker, checker::ETSObjectType *objectType)
{
    SetObjectType(objectType);
    auto [resType, resVar] = ResolveObjectMember(checker);
    if (resType == nullptr) {
        return checker->InvalidateType(this);
    }
    SetPropVar(resVar);
    return AdjustType(checker, resType);
}

std::optional<std::size_t> MemberExpression::GetTupleIndexValue() const
{
    if (object_->TsType() == nullptr || !object_->TsType()->IsETSTupleType() || !property_->IsNumberLiteral()) {
        return std::nullopt;
    }
    return property_->AsNumberLiteral()->Number().GetValueAndCastTo<std::size_t>();
}

bool MemberExpression::CheckArrayIndexValue(checker::ETSChecker *checker) const
{
    std::size_t index;

    auto const &number = property_->AsNumberLiteral()->Number();

    if (number.IsInteger()) {
        auto const value = number.GetValueAndCastTo<int64_t>();
        if (value < 0) {
            checker->LogError(diagnostic::NEGATIVE_INDEX, {}, property_->Start());
            return false;
        }
        index = static_cast<std::size_t>(value);
    } else {
        ES2PANDA_ASSERT(number.IsReal());
        auto value = number.GetValueAndCastTo<double>();
        double fraction = std::modf(value, &value);
        if (value < 0.0 || fraction >= std::numeric_limits<double>::epsilon()) {
            checker->LogError(diagnostic::INDEX_NEGATIVE_OR_FRACTIONAL, {}, property_->Start());
            return false;
        }
        index = static_cast<std::size_t>(value);
    }

    if (object_->IsArrayExpression() && object_->AsArrayExpression()->Elements().size() <= index) {
        checker->LogError(diagnostic::INDEX_OOB, {}, property_->Start());
        return false;
    }

    return true;
}

checker::Type *MemberExpression::ResolveReturnTypeFromSignature(checker::ETSChecker *checker, bool isSetter,
                                                                ArenaVector<ir::Expression *> &arguments,
                                                                ArenaVector<checker::Signature *> &signatures,
                                                                std::string_view const methodName)
{
    auto flags = checker::TypeRelationFlag::NONE;
    checker::Signature *signature =
        checker->ValidateSignatures(signatures, nullptr, arguments, Start(), "indexing", flags);
    if (signature == nullptr) {
        if (isSetter) {
            Parent()->AsAssignmentExpression()->Right()->SetParent(Parent());
        }
        checker->LogError(diagnostic::MISSING_INDEX_ACCESSOR_WITH_SIG, {}, Property()->Start());
        return nullptr;
    }
    checker->ValidateSignatureAccessibility(objType_, signature, Start());

    ES2PANDA_ASSERT(signature->Function() != nullptr);

    if (isSetter) {
        if (checker->IsClassStaticMethod(objType_, signature)) {
            checker->LogError(diagnostic::PROP_IS_STATIC, {methodName, objType_->Name()}, Property()->Start());
        }
        // Restore the right assignment node's parent to keep AST invariant valid.
        Parent()->AsAssignmentExpression()->Right()->SetParent(Parent());
        return signature->Params()[1]->TsType();
    }

    // #24301: requires enum refactoring
    if (!signature->Owner()->IsETSEnumType() && checker->IsClassStaticMethod(objType_, signature)) {
        checker->LogError(diagnostic::PROP_IS_STATIC, {methodName, objType_->Name()}, Property()->Start());
        return nullptr;
    }
    return signature->ReturnType();
}

checker::Type *MemberExpression::CheckIndexAccessMethod(checker::ETSChecker *checker)
{
    checker::PropertySearchFlags searchFlag = checker::PropertySearchFlags::SEARCH_METHOD;
    searchFlag |= checker::PropertySearchFlags::SEARCH_IN_BASE | checker::PropertySearchFlags::SEARCH_IN_INTERFACES;
    // NOTE(DZ) maybe we need to exclude static methods: search_flag &= ~(checker::PropertySearchFlags::SEARCH_STATIC);

    if (objType_->HasTypeFlag(checker::TypeFlag::GENERIC)) {
        searchFlag |= checker::PropertySearchFlags::SEARCH_ALL;
    }

    bool const isSetter = Parent()->IsAssignmentExpression() && Parent()->AsAssignmentExpression()->Left() == this;
    std::string_view const methodName =
        isSetter ? compiler::Signatures::SET_INDEX_METHOD : compiler::Signatures::GET_INDEX_METHOD;
    auto *const method = objType_->GetProperty(methodName, searchFlag);
    if (method == nullptr || !method->HasFlag(varbinder::VariableFlags::METHOD)) {
        checker->LogError(diagnostic::NO_INDEX_ACCESS_METHOD, {}, Start());
        return nullptr;
    }

    if (objType_->IsETSResizableArrayType() && property_->IsNumberLiteral()) {
        CheckArrayIndexValue(checker);
    }

    ArenaVector<Expression *> arguments {checker->ProgramAllocator()->Adapter()};
    arguments.emplace_back(property_);
    if (isSetter) {
        //  Temporary change the parent of right assignment node to check if correct "$_set" function presents.
        //  later on in lowering the entire assignment expression will be replace top the call to that method.
        auto *value = Parent()->AsAssignmentExpression()->Right();
        value->SetParent(this);
        arguments.emplace_back(value);
    }
    auto &signatures = checker->GetTypeOfVariable(method)->AsETSFunctionType()->CallSignatures();

    return ResolveReturnTypeFromSignature(checker, isSetter, arguments, signatures, methodName);
}

checker::Type *MemberExpression::GetTypeOfTupleElement(checker::ETSChecker *checker, checker::Type *baseType)
{
    ES2PANDA_ASSERT(baseType->IsETSTupleType());
    auto const idxIfAny = checker->GetTupleElementAccessValue(Property());
    return idxIfAny.has_value() ? baseType->AsETSTupleType()->GetTypeAtIndex(*idxIfAny) : nullptr;
}

static void CastTupleElementFromClassMemberType(checker::ETSChecker *checker,
                                                ir::MemberExpression *tupleElementAccessor, checker::Type *baseType)
{
    auto *typeOfTuple = tupleElementAccessor->GetTypeOfTupleElement(checker, baseType);
    auto *storedTupleType = checker->MaybeBoxType(typeOfTuple);

    const checker::CastingContext tupleCast(
        checker->Relation(), diagnostic::CAST_FAIL_UNREACHABLE, {},
        checker::CastingContext::ConstructorData {tupleElementAccessor, storedTupleType, typeOfTuple,
                                                  tupleElementAccessor->Start(), checker::TypeRelationFlag::NO_THROW});
}

checker::Type *MemberExpression::HandleComputedInGradualType(checker::ETSChecker *checker, checker::Type *baseType)
{
    property_->Check(checker);
    if (baseType->IsETSObjectType()) {
        util::StringView searchName;
        if (property_->IsLiteral()) {
            searchName = util::StringView {property_->AsLiteral()->ToString()};
        }
        auto found = baseType->AsETSObjectType()->GetProperty(searchName, checker::PropertySearchFlags::SEARCH_ALL);
        if (found == nullptr) {
            // Try to find indexer method
            checker::Type *indexType = CheckIndexAccessMethod(checker);
            if (indexType != nullptr) {
                return indexType;
            }
            checker->LogError(diagnostic::PROPERTY_NONEXISTENT, {searchName, baseType->AsETSObjectType()->Name()},
                              property_->Start());
            return nullptr;
        }
        return found->TsType();
    }
    ES2PANDA_UNREACHABLE();
    return nullptr;
}

checker::Type *MemberExpression::CheckComputed(checker::ETSChecker *checker, checker::Type *baseType)
{
    if (baseType->IsETSObjectType() && baseType->AsETSObjectType()->GetDeclNode() != nullptr &&
        baseType->AsETSObjectType()->GetDeclNode()->AsTyped()->TsType() != nullptr &&
        baseType->AsETSObjectType()->GetDeclNode()->AsTyped()->TsType()->IsGradualType()) {
        SetObjectType(baseType->AsETSObjectType());
        return HandleComputedInGradualType(checker, baseType);
    }
    if (baseType->IsETSArrayType()) {
        auto *dflt = baseType->AsETSArrayType()->ElementType();
        if (!checker->ValidateArrayIndex(property_)) {
            // error already reported to log
            return dflt;
        }

        // Check if the index value is inside array bounds if it is defined explicitly
        if (property_->IsNumberLiteral() && !CheckArrayIndexValue(checker)) {
            // error reported to log
            return dflt;
        }

        return dflt;
    }

    if (baseType->IsETSTupleType()) {
        if (!checker->ValidateTupleIndex(baseType->AsETSTupleType(), this)) {
            // error reported to log
            return baseType;
        }

        if (Parent()->IsAssignmentExpression() && Parent()->AsAssignmentExpression()->Left() == this) {
            // NOTE (smartin): When an assignment to a tuple type happens, see it as the boxed (stored) type of the
            // element. This can be removed, when generics without type erasure is possible
            auto *typeOfTuple = GetTypeOfTupleElement(checker, baseType);
            auto *storedTupleType = checker->MaybeBoxType(typeOfTuple);
            return storedTupleType;
        }

        CastTupleElementFromClassMemberType(checker, this, baseType);

        // NOTE: apply capture conversion on this type
        auto *res = GetTypeOfTupleElement(checker, baseType);
        return (res == nullptr) ? baseType : res;
    }

    if (baseType->IsETSObjectType()) {
        SetObjectType(baseType->AsETSObjectType());
        return CheckIndexAccessMethod(checker);
    }
    checker->LogError(diagnostic::INDEX_ON_INVALID_TYPE, {}, Object()->Start());
    return nullptr;
}

checker::VerifiedType MemberExpression::Check(checker::ETSChecker *checker)
{
    return {this, checker->GetAnalyzer()->Check(this)};
}

MemberExpression *MemberExpression::Clone(ArenaAllocator *const allocator, AstNode *const parent)
{
    auto *const clone = allocator->New<MemberExpression>(Tag {}, *this, allocator);
    ES2PANDA_ASSERT(clone != nullptr);
    if (parent != nullptr) {
        clone->SetParent(parent);
    }

    clone->SetRange(Range());
    return clone;
}

std::string MemberExpression::ToString() const
{
    auto str1 = object_ != nullptr ? object_->ToString() : std::string {INVALID_EXPRESSION};
    if (str1 == INVALID_EXPRESSION) {
        return str1;
    }

    auto str2 = property_ != nullptr ? property_->ToString() : std::string {INVALID_EXPRESSION};

    if (kind_ == MemberExpressionKind::ELEMENT_ACCESS) {
        return str1 + '[' + str2 + ']';
    }

    if (str2 == INVALID_EXPRESSION) {
        return str1 + str2;
    }

    return str1 + '.' + str2;
}
}  // namespace ark::es2panda::ir

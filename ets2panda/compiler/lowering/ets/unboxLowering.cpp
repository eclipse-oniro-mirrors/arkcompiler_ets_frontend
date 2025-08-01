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

#include "compiler/lowering/ets/unboxLowering.h"

#include "checker/types/type.h"
#include "generated/tokenType.h"
#include "ir/visitor/IterateAstVisitor.h"
#include "checker/ETSchecker.h"
#include "checker/types/ets/etsTupleType.h"
#include "checker/types/typeFlag.h"
#include "checker/types/globalTypesHolder.h"
#include "compiler/lowering/util.h"
#include "util/es2pandaMacros.h"
#include "generated/signatures.h"

namespace ark::es2panda::compiler {

namespace {

struct UnboxContext {
    // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes,readability-identifier-naming)
    explicit UnboxContext(public_lib::Context *ctx)
        : parser(ctx->parser->AsETSParser()),
          varbinder(ctx->GetChecker()->VarBinder()->AsETSBinder()),
          checker(ctx->GetChecker()->AsETSChecker()),
          allocator(ctx->Allocator()),
          handled(ctx->Allocator()->Adapter())
    {
    }

    // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
    parser::ETSParser *parser;
    varbinder::ETSBinder *varbinder;
    checker::ETSChecker *checker;
    ArenaAllocator *allocator;
    ArenaSet<ir::AstNode *> handled;
    // NOLINTEND(misc-non-private-member-variables-in-classes)
};

bool AnyOfElementTypes(checker::Type *type, const std::function<bool(checker::Type *)> &isFunc)
{
    auto const anyOf = [&isFunc](ArenaVector<checker::Type *> const &list) {
        auto const pred = [&isFunc](checker::Type *t) { return AnyOfElementTypes(t, isFunc); };
        return std::any_of(list.begin(), list.end(), pred);
    };
    return (type->IsETSTupleType() && anyOf(type->AsETSTupleType()->GetTupleTypesList())) ||
           (type->IsETSArrayType() && isFunc(type->AsETSArrayType()->ElementType())) ||
           (type->IsETSUnionType() && anyOf(type->AsETSUnionType()->ConstituentTypes())) ||
           (type->IsETSObjectType() && anyOf(type->AsETSObjectType()->TypeArguments()));
}

bool IsRecursivelyUnboxed(checker::Type *t)
{
    return t->IsETSPrimitiveType() || AnyOfElementTypes(t, IsRecursivelyUnboxed);
}

bool TypeIsBoxedPrimitive(checker::Type *tp)
{
    return tp->IsETSObjectType() && tp->AsETSObjectType()->IsBoxedPrimitive();
}

bool IsUnboxingApplicable(checker::Type *t)
{
    return TypeIsBoxedPrimitive(t) || AnyOfElementTypes(t, IsUnboxingApplicable);
}

}  // namespace

using TypeIdStorage = std::vector<std::uint64_t>;  // Long recursion chains are unlikely, use vector
static checker::Type *NormalizeType(UnboxContext *uctx, checker::Type *t, TypeIdStorage *alreadySeen = nullptr);
static checker::Type *MaybeRecursivelyUnboxReferenceType(UnboxContext *uctx, checker::Type *t,
                                                         TypeIdStorage *alreadySeen);

static checker::Type *MaybeRecursivelyUnboxType(UnboxContext *uctx, checker::Type *t,
                                                TypeIdStorage *alreadySeen = nullptr)
{
    if (TypeIsBoxedPrimitive(t)) {
        return uctx->checker->MaybeUnboxType(t);
    }
    return NormalizeType(uctx, t, alreadySeen);
}

static checker::Type *MaybeRecursivelyUnboxTypeParameter(UnboxContext *uctx, checker::Type *t,
                                                         TypeIdStorage *alreadySeen)
{
    /* Any recursion involves type parameters */
    if (std::find(alreadySeen->begin(), alreadySeen->end(), t->Id()) != alreadySeen->end()) {
        return t;
    }
    alreadySeen->push_back(t->Id());

    auto typeParameter = t->AsETSTypeParameter();
    auto constraintType = typeParameter->GetConstraintType();
    typeParameter->SetConstraintType(MaybeRecursivelyUnboxReferenceType(uctx, constraintType, alreadySeen));
    return t;
}

static checker::Type *MaybeRecursivelyUnboxTupleType(UnboxContext *uctx, checker::Type *t, TypeIdStorage *alreadySeen)
{
    bool anyChange = false;
    auto *srcTup = t->AsETSTupleType();

    ArenaVector<checker::Type *> newTps {uctx->allocator->Adapter()};
    for (auto *e : srcTup->GetTupleTypesList()) {
        auto *newE = MaybeRecursivelyUnboxReferenceType(uctx, e, alreadySeen);
        newTps.push_back(newE);
        anyChange |= (newE != e);
    }

    return anyChange ? uctx->allocator->New<checker::ETSTupleType>(uctx->checker, newTps) : t;
}

static checker::Type *MaybeRecursivelyUnboxUnionType(UnboxContext *uctx, checker::Type *t, TypeIdStorage *alreadySeen)
{
    bool anyChange = false;
    auto *srcUnion = t->AsETSUnionType();
    ArenaVector<checker::Type *> newTps {uctx->allocator->Adapter()};
    for (auto *e : srcUnion->ConstituentTypes()) {
        auto *newE = MaybeRecursivelyUnboxReferenceType(uctx, e, alreadySeen);
        newTps.push_back(newE);
        anyChange |= (newE != e);
    }
    return anyChange ? uctx->checker->CreateETSUnionType(std::move(newTps)) : t;
}

static checker::Type *MaybeRecursivelyUnboxObjectType(UnboxContext *uctx, checker::Type *t, TypeIdStorage *alreadySeen)
{
    bool anyChange = false;

    auto *objTp = t->AsETSObjectType();
    ArenaVector<checker::Type *> newTps {uctx->allocator->Adapter()};
    for (auto *e : objTp->TypeArguments()) {
        auto *newE = MaybeRecursivelyUnboxReferenceType(uctx, e, alreadySeen);
        newTps.push_back(newE);
        anyChange |= (newE != e);
    }
    return anyChange ? objTp->GetOriginalBaseType()->SubstituteArguments(uctx->checker->Relation(), newTps) : t;
}

static checker::Type *MaybeRecursivelyUnboxReferenceType(UnboxContext *uctx, checker::Type *t,
                                                         TypeIdStorage *alreadySeen)
{
    if (t == nullptr) {
        return t;
    }

    if (t->IsETSTypeParameter()) {
        return MaybeRecursivelyUnboxTypeParameter(uctx, t, alreadySeen);
    }

    if (t->IsETSTupleType()) {
        return MaybeRecursivelyUnboxTupleType(uctx, t, alreadySeen);
    }

    if (t->IsETSArrayType()) {
        auto *srcArr = t->AsETSArrayType();
        auto *newE = MaybeRecursivelyUnboxType(uctx, srcArr->ElementType(), alreadySeen);
        return (newE == srcArr->ElementType()) ? t : uctx->checker->CreateETSArrayType(newE);
    }

    if (t->IsETSResizableArrayType()) {
        auto *srcArr = t->AsETSResizableArrayType();
        auto *newE = MaybeRecursivelyUnboxReferenceType(uctx, srcArr->ElementType(), alreadySeen);
        return (newE == srcArr->ElementType()) ? t : uctx->checker->CreateETSResizableArrayType(newE);
    }

    if (t->IsETSUnionType()) {
        return MaybeRecursivelyUnboxUnionType(uctx, t, alreadySeen);
    }

    if (t->IsETSObjectType()) {
        return MaybeRecursivelyUnboxObjectType(uctx, t, alreadySeen);
    }

    return t;
}

// We should never see an array of boxed primitives, even as a component of some bigger type construction
static checker::Type *NormalizeType(UnboxContext *uctx, checker::Type *tp, TypeIdStorage *alreadySeen)
{
    if (alreadySeen == nullptr) {
        TypeIdStorage newAlreadySeen {};
        return MaybeRecursivelyUnboxReferenceType(uctx, tp, &newAlreadySeen);
    }

    return MaybeRecursivelyUnboxReferenceType(uctx, tp, alreadySeen);
}

static void NormalizeAllTypes(UnboxContext *uctx, ir::AstNode *ast)
{
    // Use preorder to avoid dealing with inner structure of type nodes: they are quickly replaced
    // by opaque nodes that have no children.
    ast->TransformChildrenRecursivelyPreorder(
        // clang-format off
        // CC-OFFNXT(G.FMT.14-CPP) project code style
        [uctx](ir::AstNode *child) -> ir::AstNode* {
            if (child->IsExpression() && child->AsExpression()->IsTypeNode()) {
                // Avoid dealing with annotation usages.
                // ETSTypeReferenceParts only appear within ETSTypeReference, so the only way to get one is
                // again through AnnotationUsage.
                if (child->Parent()->IsAnnotationUsage() || child->IsETSTypeReferencePart()) {
                    return child;
                }
                auto typeNodeType = child->AsExpression()->AsTypeNode()->GetType(uctx->checker);
                if (typeNodeType == nullptr || typeNodeType->IsETSAnyType()) {
                    return child;
                }
                auto r = uctx->allocator->New<ir::OpaqueTypeNode>(NormalizeType(uctx, typeNodeType),
                                                                             uctx->allocator);
                r->SetRange(child->Range());
                r->SetParent(child->Parent());
                return r;
            }
            if (child->IsTyped()) {
                child->AsTyped()->SetTsType(NormalizeType(uctx, child->AsTyped()->TsType()));
                if (child->Variable() != nullptr && child->Variable()->TsType() != nullptr) {
                    child->Variable()->SetTsType(NormalizeType(uctx, child->Variable()->TsType()));
                }
            }
            return child;
        },
        // clang-format on
        "unbox-normalize-types");
}

static void HandleScriptFunctionHeader(UnboxContext *uctx, ir::ScriptFunction *func)
{
    // dynamic function does not need to unbox
    if (func->Language() == Language::Id::JS) {
        return;
    }

    auto *sig = func->Signature();
    if (sig == nullptr) {
        return;
    }

    // Special case for primitive `valueOf` functions -- should still return boxed values (used in codegen)
    if (func->Parent()->Parent()->IsMethodDefinition() &&
        func->Parent()->Parent()->AsMethodDefinition()->Id()->Name() == "valueOf" &&
        ContainingClass(func)->AsETSObjectType()->IsBoxedPrimitive() && sig->Params().size() == 1 &&
        !sig->Params()[0]->TsType()->IsETSEnumType()) {
        auto *boxed = func->Parent()->Parent()->Parent()->AsTyped()->TsType();
        auto *unboxed = MaybeRecursivelyUnboxType(uctx, boxed);

        ES2PANDA_ASSERT(sig->ReturnType() == boxed);

        sig->Params()[0]->SetTsType(unboxed);
        uctx->varbinder->BuildFunctionName(func);
        return;
    }

    for (size_t i = 0; i < func->Signature()->Params().size(); i++) {
        auto *sigParam = func->Signature()->Params()[i];
        auto *funcParam = func->Params()[i]->AsETSParameterExpression();
        if (IsUnboxingApplicable(sigParam->TsType())) {
            auto *unboxedType = MaybeRecursivelyUnboxType(uctx, sigParam->TsType());
            sigParam->SetTsType(unboxedType);
            funcParam->SetTsType(unboxedType);
            funcParam->Ident()->SetTsType(unboxedType);
            funcParam->Variable()->SetTsType(unboxedType);
        }
    }
    if (sig->RestVar() != nullptr) {
        auto *funcRestParam = func->Params()[func->Params().size() - 1]->AsETSParameterExpression();
        ES2PANDA_ASSERT(funcRestParam != nullptr && funcRestParam->IsRestParameter());

        auto *unboxedType = MaybeRecursivelyUnboxType(uctx, sig->RestVar()->TsType());
        sig->RestVar()->SetTsType(unboxedType);
        funcRestParam->Ident()->SetTsType(unboxedType);
        funcRestParam->Ident()->Variable()->SetTsType(unboxedType);
    }
    if (IsUnboxingApplicable(sig->ReturnType())) {
        sig->SetReturnType(MaybeRecursivelyUnboxType(uctx, sig->ReturnType()));
    }

    // Signature may have changed, so need to change internal name.
    uctx->varbinder->BuildFunctionName(func);
}

static void HandleClassProperty(UnboxContext *uctx, ir::ClassProperty *prop, bool forceUnbox = false)
{
    auto *propType = prop->TsType();
    if (propType == nullptr) {
        propType = prop->Key()->Variable()->TsType();
    }
    // Primitive Types from JS should be Boxed, but in case of annotation, it should be unboxed.
    ir::AstNode *node = prop;
    while (node != nullptr && !node->IsETSModule()) {
        node = node->Parent();
    }
    if (node != nullptr && node->AsETSModule()->Program()->IsDeclForDynamicStaticInterop() && !forceUnbox) {
        return;
    }
    ES2PANDA_ASSERT(propType != nullptr);
    if (IsUnboxingApplicable(propType) && prop->Key()->IsIdentifier()) {
        auto *unboxedType = MaybeRecursivelyUnboxType(uctx, propType);
        prop->SetTsType(unboxedType);
        prop->Key()->Variable()->SetTsType(unboxedType);
    }
}

static void HandleVariableDeclarator(UnboxContext *uctx, ir::VariableDeclarator *vdecl)
{
    if (IsUnboxingApplicable(vdecl->Id()->Variable()->TsType())) {
        auto *unboxedType = MaybeRecursivelyUnboxType(uctx, vdecl->Id()->Variable()->TsType());
        vdecl->SetTsType(unboxedType);
        vdecl->Id()->SetTsType(unboxedType);
        vdecl->Id()->Variable()->SetTsType(unboxedType);
    }
}

static void HandleDeclarationNode(UnboxContext *uctx, ir::AstNode *ast)  ///
{
    if (uctx->handled.count(ast) > 0) {
        return;
    }
    if (ast->IsScriptFunction()) {
        HandleScriptFunctionHeader(uctx, ast->AsScriptFunction());
    } else if (ast->IsMethodDefinition()) {
        HandleScriptFunctionHeader(uctx, ast->AsMethodDefinition()->Function());
    } else if (ast->IsClassProperty()) {
        HandleClassProperty(uctx, ast->AsClassProperty());
    } else if (ast->IsVariableDeclarator()) {
        HandleVariableDeclarator(uctx, ast->AsVariableDeclarator());
    }
    uctx->handled.insert(ast);
}

std::string_view GetUnboxerMethodName(const checker::Type *unboxedType)
{
    if (unboxedType->IsETSBooleanType()) {
        return Signatures::BOOLEAN_CAST;
    }
    if (unboxedType->IsByteType()) {
        return Signatures::BYTE_CAST;
    }
    // NOTE(pronaip): #29054 Remove IsCharType once stdlib stops using legacy JS type
    if (unboxedType->IsETSCharType() || unboxedType->IsCharType()) {
        return Signatures::CHAR_CAST;
    }
    if (unboxedType->IsDoubleType()) {
        return Signatures::DOUBLE_CAST;
    }
    if (unboxedType->IsFloatType()) {
        return Signatures::FLOAT_CAST;
    }
    if (unboxedType->IsIntType()) {
        return Signatures::INT_CAST;
    }
    if (unboxedType->IsLongType()) {
        return Signatures::LONG_CAST;
    }
    if (unboxedType->IsShortType()) {
        return Signatures::SHORT_CAST;
    }
    ES2PANDA_UNREACHABLE();
}

static ir::Expression *InsertUnboxing(UnboxContext *uctx, ir::Expression *expr)
{
    auto *boxedType = expr->TsType();
    if (boxedType->IsETSTypeParameter()) {
        boxedType = boxedType->AsETSTypeParameter()->GetConstraintType();
    }
    auto *unboxedType = MaybeRecursivelyUnboxType(uctx, boxedType);
    auto *parent = expr->Parent();

    auto *allocator = uctx->allocator;

    // Avoid unboxing application right on top of boxing.
    if (expr->IsETSNewClassInstanceExpression() &&
        expr->AsETSNewClassInstanceExpression()->GetArguments().size() == 1 &&
        uctx->checker->Relation()->IsIdenticalTo(expr->AsETSNewClassInstanceExpression()->GetArguments()[0]->TsType(),
                                                 unboxedType)) {
        auto *ret = expr->AsETSNewClassInstanceExpression()->GetArguments()[0];
        ret->SetParent(parent);
        return ret;
    }

    const std::string_view unboxerName = GetUnboxerMethodName(unboxedType);

    auto *methodId = allocator->New<ir::Identifier>(unboxerName, allocator);
    auto *mexpr = util::NodeAllocator::ForceSetParent<ir::MemberExpression>(
        allocator, expr, methodId, ir::MemberExpressionKind::PROPERTY_ACCESS, false, false);
    auto *call = util::NodeAllocator::ForceSetParent<ir::CallExpression>(
        allocator, mexpr, ArenaVector<ir::Expression *>(allocator->Adapter()), nullptr, false);
    call->SetParent(parent);

    BindLoweredNode(uctx->varbinder, call);

    auto *methodVar = boxedType->AsETSObjectType()->InstanceMethods()[methodId->Name()];
    methodId->SetVariable(methodVar);

    /* Ensure that calleeMethod's signature is updated to return an unboxed value */
    auto *calleeMethod = methodVar->Declaration()->Node();
    HandleDeclarationNode(uctx, calleeMethod);

    mexpr->SetTsType(methodVar->TsType());
    mexpr->SetObjectType(boxedType->AsETSObjectType());
    call->SetTsType(unboxedType);
    call->SetSignature(methodVar->TsType()->AsETSFunctionType()->CallSignatures()[0]);

    return call;
}

static ir::Expression *CreateToIntrinsicCallExpression(UnboxContext *uctx, checker::Type *toType,
                                                       checker::Type *exprType, ir::Expression *expr)
{
    auto *allocator = uctx->allocator;

    auto *parent = expr->Parent();
    auto *boxedToType = uctx->checker->MaybeBoxType(toType)->AsETSObjectType();
    auto *boxedExprType = uctx->checker->MaybeBoxType(exprType)->AsETSObjectType();
    auto args = ArenaVector<ir::Expression *>(allocator->Adapter());
    auto name = util::UString("to" + boxedToType->ToStringAsSrc(), allocator).View();

    auto *memberExpr = util::NodeAllocator::ForceSetParent<ir::MemberExpression>(
        allocator, allocator->New<ir::OpaqueTypeNode>(boxedExprType, allocator),
        allocator->New<ir::Identifier>(name, allocator), ir::MemberExpressionKind::PROPERTY_ACCESS, false, false);
    args.push_back(expr);

    auto *call =
        util::NodeAllocator::ForceSetParent<ir::CallExpression>(allocator, memberExpr, std::move(args), nullptr, false);
    call->SetParent(parent);

    BindLoweredNode(uctx->varbinder, call);

    auto *methodVar = boxedExprType->StaticMethods()[name];
    memberExpr->Property()->SetVariable(methodVar);

    /* Ensure that calleeMethod's signature is updated to accept an unboxed value */
    auto *calleeMethod = methodVar->Declaration()->Node();
    HandleDeclarationNode(uctx, calleeMethod);

    memberExpr->SetTsType(methodVar->TsType());
    memberExpr->SetObjectType(boxedExprType);
    call->SetTsType(toType);
    call->SetSignature(methodVar->TsType()->AsETSFunctionType()->CallSignatures()[0]);
    return call;
}

static bool CheckIfOnTopOfUnboxing(UnboxContext *uctx, ir::Expression *expr, checker::Type *boxedType)
{
    constexpr std::array<std::string_view, 8> UNBOXER_METHOD_NAMES {
        Signatures::BOOLEAN_CAST, Signatures::BYTE_CAST, Signatures::CHAR_CAST, Signatures::DOUBLE_CAST,
        Signatures::FLOAT_CAST,   Signatures::INT_CAST,  Signatures::LONG_CAST, Signatures::SHORT_CAST,
    };
    if (expr->IsCallExpression() && expr->AsCallExpression()->Arguments().empty() &&
        expr->AsCallExpression()->Callee()->IsMemberExpression() &&
        expr->AsCallExpression()->Callee()->AsMemberExpression()->Property()->IsIdentifier()) {
        const util::StringView &name =
            expr->AsCallExpression()->Callee()->AsMemberExpression()->Property()->AsIdentifier()->Name();
        return UNBOXER_METHOD_NAMES.cend() != std::find_if(UNBOXER_METHOD_NAMES.cbegin(), UNBOXER_METHOD_NAMES.cend(),
                                                           [&name](const auto &hay) { return name.Is(hay); }) &&
               uctx->checker->Relation()->IsIdenticalTo(
                   expr->AsCallExpression()->Callee()->AsMemberExpression()->Object()->TsType(), boxedType);
    }
    return false;
}

static ir::Expression *LinkUnboxingExpr(ir::Expression *expr, ir::AstNode *parent)
{
    auto *ret = expr->AsCallExpression()->Callee()->AsMemberExpression()->Object();
    ret->SetParent(parent);
    return ret;
}

// CC-OFFNXT(huge_method[C++], G.FUN.01-CPP, G.FUD.05) solid logic
static ir::Expression *InsertBoxing(UnboxContext *uctx, ir::Expression *expr)
{
    auto *unboxedType = expr->TsType();
    auto *boxedType = uctx->checker->MaybeBoxType(unboxedType);
    auto *parent = expr->Parent();

    // Avoid boxing application right on top of unboxing.
    if (CheckIfOnTopOfUnboxing(uctx, expr, boxedType)) {
        return LinkUnboxingExpr(expr, parent);
    }

    auto *allocator = uctx->allocator;

    auto args = ArenaVector<ir::Expression *>(allocator->Adapter());

    args.push_back(expr);
    auto *constrCall = util::NodeAllocator::ForceSetParent<ir::ETSNewClassInstanceExpression>(
        allocator, allocator->New<ir::OpaqueTypeNode>(boxedType, allocator), std::move(args));
    constrCall->SetParent(parent);

    auto &constructSignatures = boxedType->AsETSObjectType()->ConstructSignatures();
    checker::Signature *signature = nullptr;
    for (auto *sig : constructSignatures) {
        if (sig->Params().size() == 1 && sig->Params()[0]->TsType() == unboxedType) {
            signature = sig;
            break;
        }
    }
    ES2PANDA_ASSERT(signature != nullptr);

    /* Ensure that the constructor signature is updated to accept an unboxed value */
    auto *constructor = signature->Function();
    HandleDeclarationNode(uctx, constructor);
    constrCall->SetTsType(boxedType);
    constrCall->SetSignature(signature);

    return constrCall;
}

static checker::Type *SelectTypeToConvert(
    std::tuple<UnboxContext *, checker::TypeRelation *, checker::ETSChecker *> ctx, checker::Type *toConvert,
    checker::Type *expectedType, checker::Type *actualType)
{
    auto [uctx, relation, checker] = ctx;
    if (toConvert == nullptr && actualType->IsCharType() &&
        relation->IsSupertypeOf(expectedType, checker->GlobalBuiltinETSStringType())) {
        return uctx->checker->GlobalBuiltinETSStringType();
    }
    if (toConvert == nullptr && actualType->IsByteType() &&
        relation->IsSupertypeOf(expectedType, checker->GlobalCharBuiltinType())) {
        return uctx->checker->GlobalCharBuiltinType();
    }

    // Appears in "~b" if "b" is of type Float
    if (toConvert == nullptr && actualType->IsFloatType() &&
        relation->IsSupertypeOf(expectedType, checker->GlobalIntBuiltinType())) {
        return checker->GlobalIntBuiltinType();
    }

    // Appears in "~b" if "b" is of type Double
    if (toConvert == nullptr && actualType->IsDoubleType() &&
        relation->IsSupertypeOf(expectedType, checker->GlobalLongBuiltinType())) {
        return checker->GlobalLongBuiltinType();
    }
    return toConvert;
}

/* NOTE(gogabr): conversions should be inserted at the checker stage. This function is temporary. */
// CC-OFFNXT(huge_method[C++], G.FUN.01-CPP, huge_cyclomatic_complexity) solid logic
static ir::Expression *InsertPrimitiveConversionIfNeeded(UnboxContext *uctx, ir::Expression *expr,
                                                         checker::Type *expectedType)
{
    auto *checker = uctx->checker;
    auto *relation = checker->Relation();
    auto *actualType = expr->TsType();

    ES2PANDA_ASSERT(IsRecursivelyUnboxed(actualType));

    if (relation->IsSupertypeOf(expectedType, uctx->checker->MaybeBoxType(actualType))) {
        return expr;
    }

    checker::Type *toConvert = nullptr;
    auto checkSubtyping = [expectedType, checker, &toConvert](checker::Type *tp) {
        if (toConvert != nullptr) {
            return;
        }
        if (checker->Relation()->IsSupertypeOf(expectedType, checker->MaybeBoxType(tp))) {
            toConvert = tp;
        }
    };

    switch (checker->ETSType(MaybeRecursivelyUnboxType(uctx, actualType))) {
        case checker::TypeFlag::BYTE:
            checkSubtyping(checker->GlobalByteBuiltinType());
            [[fallthrough]];
        case checker::TypeFlag::SHORT:
            checkSubtyping(checker->GlobalShortBuiltinType());
            [[fallthrough]];
        case checker::TypeFlag::CHAR:
        case checker::TypeFlag::INT:
            checkSubtyping(checker->GlobalIntBuiltinType());
            [[fallthrough]];
        case checker::TypeFlag::LONG:
            checkSubtyping(checker->GlobalLongBuiltinType());
            [[fallthrough]];
        case checker::TypeFlag::FLOAT:
            checkSubtyping(checker->GlobalFloatBuiltinType());
            [[fallthrough]];
        case checker::TypeFlag::DOUBLE:
            checkSubtyping(checker->GlobalDoubleBuiltinType());
            [[fallthrough]];
        default:
            break;
    }

    toConvert = SelectTypeToConvert(std::make_tuple(uctx, relation, checker), toConvert, expectedType, actualType);
    ES2PANDA_ASSERT(toConvert != nullptr);
    auto *toConvertUnboxed = checker->MaybeUnboxType(toConvert);

    auto *res = CreateToIntrinsicCallExpression(uctx, toConvertUnboxed, actualType, expr);
    auto range = expr->Range();
    SetSourceRangesRecursively(res, range);
    res->SetRange(range);

    return res;
}

// CC-OFFNXT(huge_cyclomatic_complexity, huge_cca_cyclomatic_complexity[C++], G.FUN.01-CPP) solid logic
static ir::Expression *PerformLiteralConversion(UnboxContext *uctx, lexer::Number const &n, checker::Type *expectedType)
{
    auto *allocator = uctx->allocator;
    bool isInt = false;
    int64_t longValue = 0;
    double doubleValue = 0.0;
    if (n.IsByte()) {
        longValue = n.GetByte();
        isInt = true;
    } else if (n.IsShort()) {
        longValue = n.GetShort();
        isInt = true;
    } else if (n.IsInt()) {
        longValue = n.GetInt();
        isInt = true;
    } else if (n.IsLong()) {
        longValue = n.GetLong();
        isInt = true;
    } else if (n.IsFloat()) {
        doubleValue = n.GetFloat();
        isInt = false;
    } else if (n.IsDouble()) {
        doubleValue = n.GetDouble();
        isInt = false;
    } else {
        ES2PANDA_UNREACHABLE();
    }

    lexer::Number num {};
    if (expectedType->IsByteType()) {
        num = lexer::Number {isInt ? (int8_t)longValue : (int8_t)doubleValue};
    } else if (expectedType->IsShortType()) {
        num = lexer::Number {isInt ? (int16_t)longValue : (int16_t)doubleValue};
    } else if (expectedType->IsIntType()) {
        num = lexer::Number {isInt ? (int32_t)longValue : (int32_t)doubleValue};
    } else if (expectedType->IsLongType()) {
        num = lexer::Number {isInt ? longValue : (int64_t)doubleValue};
    } else if (expectedType->IsFloatType()) {
        num = lexer::Number {isInt ? (float)longValue : (float)doubleValue};
    } else if (expectedType->IsDoubleType()) {
        num = lexer::Number {isInt ? (double)longValue : doubleValue};
    } else {
        ES2PANDA_UNREACHABLE();
    }

    auto *res = allocator->New<ir::NumberLiteral>(num);
    res->SetTsType(expectedType);
    return res;
}

static ir::Expression *InsertConversionBetweenPrimitivesIfNeeded(UnboxContext *uctx, ir::Expression *expr,
                                                                 checker::Type *expectedType)
{
    auto *oldType = expr->TsType();
    if (uctx->checker->Relation()->IsIdenticalTo(oldType, expectedType)) {
        return expr;
    }

    auto *parent = expr->Parent();
    ir::Expression *res;

    auto range = expr->Range();

    if (expr->IsNumberLiteral() && expectedType->HasTypeFlag(checker::TypeFlag::ETS_NUMERIC)) {
        /* Some contexts (namely, annotations) expect literals, so provide them if possible */
        res = PerformLiteralConversion(uctx, expr->AsNumberLiteral()->Number(), expectedType);
        res->SetRange(range);
    } else if (expr->IsCharLiteral() && expectedType->HasTypeFlag(checker::TypeFlag::ETS_NUMERIC)) {
        res = PerformLiteralConversion(uctx, lexer::Number {expr->AsCharLiteral()->Char()}, expectedType);
        res->SetRange(range);
    } else {
        res = CreateToIntrinsicCallExpression(uctx, expectedType, oldType, expr);
        SetSourceRangesRecursively(res, range);
    }

    res->SetParent(parent);
    res->SetTsType(expectedType);
    return res;
}

static ir::Expression *AdjustType(UnboxContext *uctx, ir::Expression *expr, checker::Type *expectedType)
{
    if (expr == nullptr) {
        return nullptr;
    }
    expectedType = uctx->checker->GetApparentType(expectedType);
    checker::Type *actualType = expr->Check(uctx->checker);

    if (expectedType->HasTypeFlag(checker::TypeFlag::ETS_NEVER | checker::TypeFlag::ETS_VOID)) {
        return expr;
    }

    if (actualType->IsETSPrimitiveType() && checker::ETSChecker::IsReferenceType(expectedType)) {
        expr = InsertPrimitiveConversionIfNeeded(uctx, expr, expectedType);
        ES2PANDA_ASSERT(
            uctx->checker->Relation()->IsSupertypeOf(expectedType, uctx->checker->MaybeBoxType(expr->TsType())) ||
            (expr->TsType()->IsCharType() && expectedType->IsETSStringType()));
        return InsertBoxing(uctx, expr);
    }
    if ((TypeIsBoxedPrimitive(actualType) ||
         (actualType->IsETSTypeParameter() &&
          TypeIsBoxedPrimitive(actualType->AsETSTypeParameter()->GetConstraintType()))) &&
        expectedType->IsETSPrimitiveType()) {
        return InsertConversionBetweenPrimitivesIfNeeded(uctx, InsertUnboxing(uctx, expr), expectedType);
    }
    if (TypeIsBoxedPrimitive(actualType) && checker::ETSChecker::IsReferenceType(expectedType) &&
        !uctx->checker->Relation()->IsSupertypeOf(expectedType, actualType)) {
        return AdjustType(uctx, InsertUnboxing(uctx, expr), expectedType);
    }
    if (actualType->IsETSPrimitiveType() && expectedType->IsETSPrimitiveType()) {
        return InsertConversionBetweenPrimitivesIfNeeded(uctx, expr, expectedType);
    }
    return expr;
}

static void HandleForOfStatement(UnboxContext *uctx, ir::ForOfStatement *forOf)
{
    auto *left = forOf->Left();

    ir::Identifier *id = nullptr;
    if (left->IsIdentifier()) {
        id = left->AsIdentifier();
    } else if (left->IsVariableDeclaration()) {
        ES2PANDA_ASSERT(left->AsVariableDeclaration()->Declarators().size() == 1);
        id = left->AsVariableDeclaration()->Declarators()[0]->Id()->AsIdentifier();
    }
    ES2PANDA_ASSERT(id != nullptr);

    // NOTE(gogabr): we need to recompute the right side type instead of just unboxing;
    // this may be, for example, a generic call that returns a boxed array.
    auto *tp = MaybeRecursivelyUnboxType(uctx, forOf->Right()->TsType());
    if (tp->IsETSAnyType()) {
        return;
    }

    checker::Type *elemTp = nullptr;
    if (tp->IsETSArrayType()) {
        elemTp = tp->AsETSArrayType()->ElementType();
    } else if (tp->IsETSStringType()) {
        elemTp = uctx->checker->GlobalCharType();
    } else {
        ES2PANDA_ASSERT(tp->IsETSUnionType());
        ES2PANDA_ASSERT(id->Variable()->TsType()->IsETSUnionType());
        // NOLINTNEXTLINE(clang-analyzer-core.CallAndMessage)
        elemTp = id->Variable()->TsType();  // always a union type, no need to change
    }

    /* This type assignment beats other assignment that could be produced during normal handling of id's declaration
     */
    // NOLINTNEXTLINE(clang-analyzer-core.CallAndMessage)
    id->SetTsType(elemTp);
    id->Variable()->SetTsType(elemTp);
    id->Variable()->Declaration()->Node()->AsTyped()->SetTsType(elemTp);
}

// Borrowed from arithmetic.cpp, didn't want to make it public -- gogabr
static checker::Type *EffectiveTypeOfNumericOrEqualsOp(checker::ETSChecker *checker, checker::Type *left,
                                                       checker::Type *right)
{
    if (left->IsDoubleType() || right->IsDoubleType()) {
        return checker->GlobalDoubleType();
    }
    if (left->IsFloatType() || right->IsFloatType()) {
        return checker->GlobalFloatType();
    }
    if (left->IsLongType() || right->IsLongType()) {
        return checker->GlobalLongType();
    }
    if (left->IsCharType() && right->IsCharType()) {
        return checker->GlobalCharType();
    }
    if (left->IsETSBooleanType() && right->IsETSBooleanType()) {
        return checker->GlobalETSBooleanType();
    }
    return checker->GlobalIntType();
}

static void ReplaceInParent(ir::AstNode *from, ir::AstNode *to)
{
    // clang-format off
    // CC-OFFNXT(G.FMT.14-CPP) project code style
    auto const replaceNode = [=](ir::AstNode *child) -> ir::AstNode* {
        if (child == from) {
            to->SetParent(from->Parent());
            return to;
        }
        return child;
    };
    // clang-format on
    from->Parent()->TransformChildren(replaceNode, "UnboxLoweringReplaceInParent");
}

namespace {
struct UnboxVisitor : public ir::visitor::EmptyAstVisitor {
    explicit UnboxVisitor(UnboxContext *uctx) : uctx_(uctx) {}

    inline checker::Type *GetArrayElementType(checker::Type *arrType)
    {
        return uctx_->checker->GetElementTypeOfArray(arrType);
    }

    void VisitReturnStatement(ir::ReturnStatement *retStmt) override
    {
        ir::ScriptFunction *nearestScriptFunction = nullptr;
        for (ir::AstNode *curr = retStmt; curr != nullptr; curr = curr->Parent()) {
            if (curr->IsScriptFunction()) {
                nearestScriptFunction = curr->AsScriptFunction();
                break;
            }
        }

        if (nearestScriptFunction != nullptr && retStmt != nullptr) {
            retStmt->SetArgument(
                AdjustType(uctx_, retStmt->Argument(), nearestScriptFunction->Signature()->ReturnType()));
        }
    }

    void VisitIfStatement(ir::IfStatement *ifStmt) override
    {
        if (TypeIsBoxedPrimitive(ifStmt->Test()->TsType())) {
            ifStmt->SetTest(InsertUnboxing(uctx_, ifStmt->Test()));
        }
    }

    void VisitWhileStatement(ir::WhileStatement *whStmt) override
    {
        if (TypeIsBoxedPrimitive(whStmt->Test()->TsType())) {
            whStmt->SetTest(InsertUnboxing(uctx_, whStmt->Test()));
        }
    }

    void VisitSwitchStatement(ir::SwitchStatement *swtch) override
    {
        auto *discType = uctx_->checker->MaybeUnboxType(swtch->Discriminant()->TsType());
        if (!discType->IsETSPrimitiveType()) {  // should be string
            return;
        }
        swtch->SetDiscriminant(AdjustType(uctx_, swtch->Discriminant(), discType));
        for (auto *scase : swtch->Cases()) {
            scase->SetTest(AdjustType(uctx_, scase->Test(), discType));
        }
    }

    // CC-OFFNXT(huge_method[C++], G.FUN.01-CPP, G.FUD.05) solid logic
    void VisitCallExpression(ir::CallExpression *call) override
    {
        if (!call->Signature()->HasFunction() || call->Signature()->Function()->Language() == Language::Id::JS) {
            // some lambda call and dynamic call to js, all arguments and return type need to be boxed
            // NOLINTNEXTLINE(modernize-loop-convert)
            for (size_t i = 0; i < call->Arguments().size(); i++) {
                auto *arg = call->Arguments()[i];
                call->Arguments()[i] = AdjustType(uctx_, arg, uctx_->checker->MaybeBoxType(arg->TsType()));
            }
            return;
        }

        auto *func = call->Signature()->Function();

        HandleDeclarationNode(uctx_, func);
        // NOLINTNEXTLINE(modernize-loop-convert)
        for (size_t i = 0; i < call->Arguments().size(); i++) {
            auto *arg = call->Arguments()[i];

            if (i >= func->Signature()->Params().size()) {
                auto *restVar = call->Signature()->RestVar();
                if (restVar != nullptr &&
                    !arg->IsSpreadElement()) {  // NOTE(gogabr) should we try to unbox spread elements?
                    auto *restElemType = GetArrayElementType(restVar->TsType());
                    call->Arguments()[i] = AdjustType(uctx_, arg, restElemType);
                }
            } else {
                auto *origSigType = func->Signature()->Params()[i]->TsType();
                if (origSigType->IsETSPrimitiveType()) {
                    call->Signature()->Params()[i]->SetTsType(origSigType);
                    call->Arguments()[i] = AdjustType(uctx_, arg, origSigType);
                } else {
                    call->Arguments()[i] = AdjustType(uctx_, arg, call->Signature()->Params()[i]->TsType());
                }
            }
        }

        if (func->Signature()->ReturnType()->IsETSPrimitiveType()) {
            call->Signature()->SetReturnType(func->Signature()->ReturnType());
        } else {
            call->Signature()->SetReturnType(NormalizeType(uctx_, call->Signature()->ReturnType()));
        }

        if (call->Signature()->HasSignatureFlag(checker::SignatureFlags::THIS_RETURN_TYPE)) {
            auto *callee = call->Callee();
            auto isFuncRefCall = [&callee]() {
                if (!callee->IsMemberExpression()) {
                    return false;
                };
                auto *calleeObject = callee->AsMemberExpression()->Object();
                return (calleeObject)
                           ->TsType()
                           ->IsETSFunctionType() ||  // NOTE(gogabr): How can this happen after lambdaLowering?
                       (calleeObject->TsType()->IsETSObjectType() &&
                        calleeObject->TsType()->AsETSObjectType()->HasObjectFlag(checker::ETSObjectFlags::FUNCTIONAL));
            }();
            if (callee->IsMemberExpression() && !isFuncRefCall) {
                call->SetTsType(callee->AsMemberExpression()->Object()->TsType());
            } else {
                // Either a functional reference call, or
                // function with receiver called in a "normal", "function-like" way:
                // function f(x: this) : this { return this }
                // f(new A)
                ES2PANDA_ASSERT(!call->Arguments().empty());
                call->SetTsType(call->Arguments()[0]->TsType());
            }
        } else if (auto *returnType = call->Signature()->ReturnType(); returnType->IsETSPrimitiveType()) {
            call->SetTsType(returnType);
        }
    }

    void VisitETSNewClassInstanceExpression(ir::ETSNewClassInstanceExpression *call) override
    {
        auto *func = call->GetSignature()->Function();
        if (func == nullptr || func->Language() == Language::Id::JS) {
            // For dynamic call to js, all arguments and return type need to be boxed
            // NOLINTNEXTLINE(modernize-loop-convert)
            for (size_t i = 0; i < call->GetArguments().size(); i++) {
                auto *arg = call->GetArguments()[i];
                call->GetArguments()[i] = AdjustType(uctx_, arg, uctx_->checker->MaybeBoxType(arg->TsType()));
            }
            return;
        }

        HandleDeclarationNode(uctx_, func);

        for (size_t i = 0; i < call->GetArguments().size(); i++) {
            auto *arg = call->GetArguments()[i];

            if (i >= func->Signature()->Params().size()) {
                auto *restVar = call->GetSignature()->RestVar();
                if (restVar != nullptr &&
                    !arg->IsSpreadElement()) {  // NOTE(gogabr) should we try to unbox spread elements?
                    auto *restElemType = GetArrayElementType(restVar->TsType());
                    call->GetArguments()[i] = AdjustType(uctx_, arg, restElemType);
                }
            } else {
                auto *origSigType = func->Signature()->Params()[i]->TsType();
                if (origSigType->IsETSPrimitiveType()) {
                    call->GetSignature()->Params()[i]->SetTsType(origSigType);
                    call->GetArguments()[i] = AdjustType(uctx_, arg, origSigType);
                } else {
                    call->GetArguments()[i] = AdjustType(uctx_, arg, call->GetSignature()->Params()[i]->TsType());
                }
            }
        }

        call->SetTsType(call->GetTypeRef()->TsType());
    }

    void VisitSpreadElement(ir::SpreadElement *spread) override
    {
        spread->SetTsType(spread->Argument()->TsType());
    }

    void VisitArrayExpression(ir::ArrayExpression *aexpr) override
    {
        auto *unboxedType = MaybeRecursivelyUnboxType(uctx_, aexpr->TsType());
        aexpr->SetTsType(unboxedType);

        for (size_t i = 0; i < aexpr->Elements().size(); i++) {
            checker::Type *expectedType;
            if (aexpr->TsType()->IsETSTupleType()) {
                expectedType = aexpr->TsType()->AsETSTupleType()->GetTypeAtIndex(i);
            } else if (aexpr->TsType()->IsETSArrayType() || aexpr->TsType()->IsETSResizableArrayType()) {
                expectedType = GetArrayElementType(aexpr->TsType());
            } else {
                ES2PANDA_UNREACHABLE();
            }
            aexpr->Elements()[i] = AdjustType(uctx_, aexpr->Elements()[i], expectedType);
        }
    }

    void HandleArithmeticLike(ir::BinaryExpression *bexpr)
    {
        bexpr->SetTsType(uctx_->checker->MaybeUnboxType(bexpr->TsType()));
        bexpr->SetOperationType(uctx_->checker->MaybeUnboxType(bexpr->OperationType()));
        if (TypeIsBoxedPrimitive(bexpr->Left()->TsType())) {
            bexpr->SetLeft(InsertUnboxing(uctx_, bexpr->Left()));
        }
        if (TypeIsBoxedPrimitive(bexpr->Right()->TsType())) {
            bexpr->SetRight(InsertUnboxing(uctx_, bexpr->Right()));
        }
    }

    void HandleEqualityOrInequality(ir::BinaryExpression *bexpr)
    {
        auto *leftTp = bexpr->Left()->TsType();
        auto *rightTp = bexpr->Right()->TsType();

        checker::Type *opType = nullptr;
        if ((leftTp->IsETSPrimitiveType() || TypeIsBoxedPrimitive(leftTp)) &&
            (rightTp->IsETSPrimitiveType() || TypeIsBoxedPrimitive(rightTp))) {
            auto *newLeftTp = uctx_->checker->MaybeUnboxType(leftTp);
            auto *newRightTp = uctx_->checker->MaybeUnboxType(rightTp);
            bexpr->SetLeft(AdjustType(uctx_, bexpr->Left(), newLeftTp));
            bexpr->SetRight(AdjustType(uctx_, bexpr->Right(), newRightTp));

            opType = EffectiveTypeOfNumericOrEqualsOp(uctx_->checker, newLeftTp, newRightTp);
            bexpr->SetLeft(InsertConversionBetweenPrimitivesIfNeeded(uctx_, bexpr->Left(), opType));
            bexpr->SetRight(InsertConversionBetweenPrimitivesIfNeeded(uctx_, bexpr->Right(), opType));
        } else {
            bexpr->SetLeft(AdjustType(uctx_, bexpr->Left(), uctx_->checker->MaybeBoxType(leftTp)));
            bexpr->SetRight(AdjustType(uctx_, bexpr->Right(), uctx_->checker->MaybeBoxType(rightTp)));
            opType = bexpr->OperationType();
        }

        bexpr->SetOperationType(opType);
        bexpr->SetTsType(uctx_->checker->GlobalETSBooleanType());
    }

    void HandleLogical(ir::BinaryExpression *bexpr)
    {
        auto *leftType = bexpr->Left()->TsType();
        auto *rightType = bexpr->Right()->TsType();
        if (uctx_->checker->Relation()->IsIdenticalTo(leftType, rightType)) {
            bexpr->SetTsType(leftType);
            bexpr->SetOperationType(leftType);
        } else {
            // NOTE(gogabr): simplify codegen here. Lower logical operators.
            auto *oldLeft = bexpr->Left();
            auto *oldRight = bexpr->Right();
            auto *leftBoxed = uctx_->checker->MaybeBoxType(leftType);
            auto *rightBoxed = uctx_->checker->MaybeBoxType(rightType);
            checker::Type *resType;

            if (oldRight->IsNumberLiteral() && !oldLeft->IsNumberLiteral() && leftBoxed->IsBuiltinNumeric()) {
                resType = leftBoxed;
            } else if (oldLeft->IsNumberLiteral() && !oldRight->IsNumberLiteral() && rightBoxed->IsBuiltinNumeric()) {
                resType = rightBoxed;
            } else {
                resType = uctx_->checker->MaybeUnboxType(uctx_->checker->CreateETSUnionType({leftBoxed, rightBoxed}));
            }

            bexpr->SetLeft(AdjustType(uctx_, oldLeft, resType));
            bexpr->SetRight(AdjustType(uctx_, oldRight, resType));
            if (bexpr->Result() == oldLeft) {
                bexpr->SetResult(bexpr->Left());
            } else if (bexpr->Result() == oldRight) {
                bexpr->SetResult(bexpr->Right());
            }
            bexpr->SetTsType(resType);
            bexpr->SetOperationType(bexpr->TsType());
        }
    }

    void VisitBinaryExpression(ir::BinaryExpression *bexpr) override
    {
        if (bexpr->IsArithmetic() || bexpr->IsBitwise() ||
            bexpr->OperatorType() == lexer::TokenType::PUNCTUATOR_LESS_THAN ||
            bexpr->OperatorType() == lexer::TokenType::PUNCTUATOR_LESS_THAN_EQUAL ||
            bexpr->OperatorType() == lexer::TokenType::PUNCTUATOR_GREATER_THAN ||
            bexpr->OperatorType() == lexer::TokenType::PUNCTUATOR_GREATER_THAN_EQUAL ||
            bexpr->OperatorType() == lexer::TokenType::PUNCTUATOR_LEFT_SHIFT ||
            bexpr->OperatorType() == lexer::TokenType::PUNCTUATOR_RIGHT_SHIFT ||
            bexpr->OperatorType() == lexer::TokenType::PUNCTUATOR_UNSIGNED_RIGHT_SHIFT) {
            HandleArithmeticLike(bexpr);
            return;
        }
        if (bexpr->OperatorType() == lexer::TokenType::PUNCTUATOR_STRICT_EQUAL ||
            bexpr->OperatorType() == lexer::TokenType::PUNCTUATOR_NOT_STRICT_EQUAL ||
            bexpr->OperatorType() == lexer::TokenType::PUNCTUATOR_EQUAL ||
            bexpr->OperatorType() == lexer::TokenType::PUNCTUATOR_NOT_EQUAL) {
            HandleEqualityOrInequality(bexpr);
            return;
        }
        if (bexpr->OperatorType() == lexer::TokenType::PUNCTUATOR_NULLISH_COALESCING) {
            bexpr->SetLeft(
                AdjustType(uctx_, bexpr->Left(),
                           uctx_->checker->CreateETSUnionType({bexpr->TsType(), uctx_->checker->GlobalETSNullType(),
                                                               uctx_->checker->GlobalETSUndefinedType()})));
            bexpr->SetRight(AdjustType(uctx_, bexpr->Right(), bexpr->TsType()));
            return;
        }
        if (bexpr->OperatorType() == lexer::TokenType::PUNCTUATOR_LOGICAL_AND ||
            bexpr->OperatorType() == lexer::TokenType::PUNCTUATOR_LOGICAL_OR) {
            HandleLogical(bexpr);
            return;
        }

        if (bexpr->OperatorType() == lexer::TokenType::KEYW_INSTANCEOF) {
            bexpr->SetLeft(AdjustType(uctx_, bexpr->Left(), uctx_->checker->MaybeBoxType(bexpr->Left()->TsType())));
            bexpr->SetTsType(uctx_->checker->GlobalETSBooleanType());
            return;
        }
    }

    void VisitUnaryExpression(ir::UnaryExpression *uexpr) override
    {
        if (uexpr->OperatorType() == lexer::TokenType::PUNCTUATOR_TILDE) {
            uexpr->SetArgument(AdjustType(uctx_, uexpr->Argument(), uexpr->TsType()));
        }

        uexpr->SetTsType(uctx_->checker->MaybeUnboxType(uexpr->TsType()));
        if (TypeIsBoxedPrimitive(uexpr->Argument()->TsType())) {
            uexpr->SetArgument(InsertUnboxing(uctx_, uexpr->Argument()));
        }
    }

    static bool IsStaticMemberExpression(ir::MemberExpression *mexpr)
    {
        ES2PANDA_ASSERT(mexpr->Kind() == ir::MemberExpressionKind::PROPERTY_ACCESS);

        auto *propDeclNode = mexpr->Property()->Variable()->Declaration()->Node();
        if (propDeclNode->IsMethodDefinition()) {
            return propDeclNode->AsMethodDefinition()->IsStatic();
        }
        if (propDeclNode->IsClassProperty()) {
            return propDeclNode->AsClassProperty()->IsStatic();
        }
        return propDeclNode->IsTSEnumMember();
    }

    static int GetNumberLiteral(ir::Expression *expr)  // NOTE(gogabr): should use code from ConstantExpressionLowering
    {
        if (expr->IsNumberLiteral()) {
            return static_cast<int>(expr->AsNumberLiteral()->Number().GetDouble());
        }
        // References to temp variables can appear in lowerings
        if (expr->IsIdentifier()) {
            auto *declNode = expr->Variable()->Declaration()->Node()->Parent()->AsVariableDeclarator();
            auto *initVal = declNode->Init();
            while (initVal->IsTSAsExpression()) {
                initVal = initVal->AsTSAsExpression()->Expr();
            }
            ES2PANDA_ASSERT(initVal->IsNumberLiteral());
            return initVal->AsNumberLiteral()->Number().GetInt();
        }
        ES2PANDA_UNREACHABLE();
    }

    checker::Type *GetHandledGetterSetterType(ir::MemberExpression *mexpr, checker::Type *propType)
    {
        if (propType->IsETSMethodType()) {
            bool needSetter =
                mexpr->Parent()->IsAssignmentExpression() && mexpr == mexpr->Parent()->AsAssignmentExpression()->Left();
            if (needSetter) {  // CC-OFF(G.FUN.01-CPP, C_RULE_ID_FUNCTION_NESTING_LEVEL) solid logic
                if (auto *setterSig = propType->AsETSFunctionType()->FindSetter(); setterSig != nullptr) {
                    HandleDeclarationNode(uctx_, setterSig->Function());
                    propType = setterSig->Params()[0]->TsType();
                }
            } else if (auto *getterSig = propType->AsETSFunctionType()->FindGetter(); getterSig != nullptr) {
                HandleDeclarationNode(uctx_, getterSig->Function());
                propType = getterSig->ReturnType();
            }
        } else if (mexpr->Property()->Variable() != nullptr) {
            /* Adjustment needed for Readonly<T> types and possibly some other cases */
            mexpr->Property()->Variable()->SetTsType(propType);
        }
        return propType;
    }

    // CC-OFFNXT(C_RULE_ID_FUNCTION_NESTING_LEVEL, huge_method[C++], huge_cca_cyclomatic_complexity[C++]) solid logic
    // CC-OFFNXT(huge_cyclomatic_complexity, huge_depth[C++], huge_depth, huge_method, G.FUN.01-CPP, G.FUN.05) solid
    void VisitMemberExpression(ir::MemberExpression *mexpr) override
    {
        if (mexpr->Kind() == ir::MemberExpressionKind::PROPERTY_ACCESS ||
            /* Workaround for memo plugin */
            mexpr->Kind() == ir::MemberExpressionKind::NONE || mexpr->Kind() == ir::MemberExpressionKind::GETTER ||
            mexpr->Kind() == ir::MemberExpressionKind::SETTER) {
            if (mexpr->Property()->Variable() != nullptr) {
                checker::Type *propType = nullptr;
                if (mexpr->Property()->Variable()->Declaration() != nullptr &&
                    mexpr->Property()->Variable()->Declaration()->Node() != nullptr &&
                    mexpr->Property()->Variable()->Declaration()->Node()->IsTyped() &&
                    !mexpr->Object()->TsType()->IsETSAnyType()) {
                    HandleDeclarationNode(uctx_, mexpr->Property()->Variable()->Declaration()->Node());
                    propType = mexpr->Property()->Variable()->Declaration()->Node()->AsTyped()->TsType();
                } else if (mexpr->Property()->Variable()->TsType() != nullptr) {
                    propType = mexpr->Property()->Variable()->TsType();
                } else {
                    propType = mexpr->Property()->TsType();
                }
                ES2PANDA_ASSERT(propType != nullptr);

                /* Special handling for getters/setters. */
                if (propType->IsETSMethodType()) {
                    propType = GetHandledGetterSetterType(mexpr, propType);
                } else if (mexpr->Property()->Variable() != nullptr) {
                    /* Adjustment needed for Readonly<T> types and possibly some other cases */
                    mexpr->Property()->Variable()->SetTsType(propType);
                }

                if (IsRecursivelyUnboxed(propType)) {
                    mexpr->Property()->SetTsType(propType);
                    mexpr->SetTsType(propType);
                }
            } else if (mexpr->Property()->Variable() == nullptr && mexpr->Object()->TsType()->IsETSArrayType() &&
                       mexpr->Property()->AsIdentifier()->Name() == "length") {
                mexpr->SetTsType(uctx_->checker->GlobalIntType());
            }
            if (mexpr->Object()->TsType()->IsETSPrimitiveType() && !IsStaticMemberExpression(mexpr)) {
                // NOTE(gogabr): need to handle some elementary method calls as intrinsics
                mexpr->SetObject(InsertBoxing(uctx_, mexpr->Object()));
            }
        } else if (mexpr->Kind() == ir::MemberExpressionKind::ELEMENT_ACCESS) {
            /* Getters are already handled in a lowering, we need a primtive as an index */
            if (TypeIsBoxedPrimitive(mexpr->Property()->TsType())) {
                mexpr->SetProperty(InsertUnboxing(uctx_, mexpr->Property()));
            }

            if (mexpr->Object()->TsType()->IsETSTupleType()) {
                auto tupType = mexpr->Object()->TsType()->AsETSTupleType();
                auto index = GetNumberLiteral(mexpr->Property());
                ES2PANDA_ASSERT(index >= 0 && (size_t)index < tupType->GetTupleSize());
                mexpr->SetTsType(tupType->GetTupleTypesList()[index]);
            } else if (mexpr->Object()->TsType()->IsETSArrayType()) {
                mexpr->SetTsType(GetArrayElementType(mexpr->Object()->TsType()));
            }
            /* mexpr->Object() may also have never type; nothing needs to be done in that case */
        } else {
            ES2PANDA_UNREACHABLE();
        }
    }

    void VisitTSAsExpression(ir::TSAsExpression *asExpr) override
    {
        auto *exprType = asExpr->Expr()->TsType();
        auto *targetType = asExpr->TypeAnnotation()->TsType();
        if (targetType->IsETSPrimitiveType() || TypeIsBoxedPrimitive(targetType)) {
            if (exprType->IsETSPrimitiveType() || TypeIsBoxedPrimitive(exprType)) {
                auto *primTargetType = MaybeRecursivelyUnboxType(uctx_, targetType);
                asExpr->TypeAnnotation()->SetTsType(primTargetType);
                asExpr->SetExpr(AdjustType(uctx_, asExpr->Expr(), MaybeRecursivelyUnboxType(uctx_, exprType)));
                asExpr->SetTsType(primTargetType);
            } else {
                auto *boxedTargetType = uctx_->checker->MaybeBoxType(targetType);
                asExpr->TypeAnnotation()->SetTsType(boxedTargetType);
                asExpr->SetTsType(boxedTargetType);
            }
        } else if (exprType->IsETSPrimitiveType()) {
            asExpr->SetExpr(AdjustType(uctx_, asExpr->Expr(), targetType));
        }
        asExpr->SetTsType(asExpr->TypeAnnotation()->TsType());
    }

    void VisitConditionalExpression(ir::ConditionalExpression *cexpr) override
    {
        if (TypeIsBoxedPrimitive(cexpr->Test()->TsType())) {
            cexpr->SetTest(InsertUnboxing(uctx_, cexpr->Test()));
        }

        auto *tp = cexpr->TsType();
        if (!tp->IsETSPrimitiveType() && !TypeIsBoxedPrimitive(tp)) {
            // Box if needed
            cexpr->SetConsequent(AdjustType(uctx_, cexpr->Consequent(), tp));
            cexpr->SetAlternate(AdjustType(uctx_, cexpr->Alternate(), tp));
        } else {
            // Unbox if needed
            auto *primTp = uctx_->checker->MaybeUnboxType(tp);
            cexpr->SetConsequent(AdjustType(uctx_, cexpr->Consequent(), primTp));
            cexpr->SetAlternate(AdjustType(uctx_, cexpr->Alternate(), primTp));
            cexpr->SetTsType(primTp);
        }
    }

    void VisitETSNewArrayInstanceExpression(ir::ETSNewArrayInstanceExpression *nexpr) override
    {
        auto unboxedType = MaybeRecursivelyUnboxType(uctx_, nexpr->TsType());
        nexpr->SetTsType(unboxedType);
        nexpr->TypeReference()->SetTsType(GetArrayElementType(unboxedType));

        nexpr->SetDimension(
            AdjustType(uctx_, nexpr->Dimension(), uctx_->checker->MaybeUnboxType(nexpr->Dimension()->TsType())));
    }

    void VisitETSNewMultiDimArrayInstanceExpression(ir::ETSNewMultiDimArrayInstanceExpression *nexpr) override
    {
        auto *unboxedType = MaybeRecursivelyUnboxType(uctx_, nexpr->TsType());
        nexpr->SetTsType(unboxedType);

        auto toUnbox = unboxedType;
        for (auto &dim : nexpr->Dimensions()) {
            dim = AdjustType(uctx_, dim, uctx_->checker->MaybeUnboxType(dim->TsType()));
            toUnbox = GetArrayElementType(toUnbox);
        }

        nexpr->TypeReference()->SetTsType(toUnbox);
        nexpr->SetSignature(
            uctx_->checker->CreateBuiltinArraySignature(unboxedType->AsETSArrayType(), nexpr->Dimensions().size()));
    }

    void VisitBlockExpression(ir::BlockExpression *bexpr) override
    {
        auto &stmts = bexpr->Statements();
        auto *lastStmt = stmts[stmts.size() - 1];
        ES2PANDA_ASSERT(lastStmt->IsExpressionStatement());

        bexpr->SetTsType(lastStmt->AsExpressionStatement()->GetExpression()->TsType());
    }

    void VisitSequenceExpression(ir::SequenceExpression *sexpr) override
    {
        sexpr->SetTsType(sexpr->Sequence().back()->TsType());
    }

    void HandleLiteral(ir::Literal *lit)
    {
        if (lit->TsType() == nullptr) {
            return;
        }
        lit->SetTsType(uctx_->checker->MaybeUnboxType(lit->TsType()));
    }

    void VisitBooleanLiteral(ir::BooleanLiteral *blit) override
    {
        HandleLiteral(blit);
    }
    void VisitCharLiteral(ir::CharLiteral *clit) override
    {
        HandleLiteral(clit);
    }
    void VisitNumberLiteral(ir::NumberLiteral *nlit) override
    {
        HandleLiteral(nlit);
    }

    void HandleVariableRef(ir::Expression *expr)
    {
        auto *var = expr->Variable();
        if (var == nullptr || var->TsType() == nullptr || expr->TsType() == nullptr ||
            var->Declaration() == nullptr) {  // lambda invoke function
            return;
        }
        auto *declNode = var->Declaration()->Node();
        if (declNode->IsClassProperty()) {
            HandleDeclarationNode(uctx_, declNode);
        }
        if (declNode->IsClassDeclaration() || declNode->IsTSEnumDeclaration() || declNode->IsTSInterfaceDeclaration()) {
            return;
        }
        if (expr->Variable()->TsType()->IsETSPrimitiveType()) {
            expr->SetTsType(expr->Variable()->TsType());
        } else if (expr->TsType()->IsETSPrimitiveType()) {
            expr->SetTsType(uctx_->checker->MaybeBoxType(expr->TsType()));
        } else {
            expr->SetTsType(NormalizeType(uctx_, expr->TsType()));
        }
    }

    void VisitIdentifier(ir::Identifier *id) override
    {
        HandleVariableRef(id);
    }

    void VisitTSQualifiedName(ir::TSQualifiedName *qname) override
    {
        HandleVariableRef(qname);
    }

    void VisitAssignmentExpression(ir::AssignmentExpression *aexpr) override
    {
        aexpr->SetRight(AdjustType(uctx_, aexpr->Right(), aexpr->Left()->TsType()));
        aexpr->SetTsType(aexpr->Left()->TsType());
    }

    void VisitClassProperty(ir::ClassProperty *prop) override
    {
        prop->SetValue(AdjustType(uctx_, prop->Value(), prop->Key()->Variable()->TsType()));
    }

    void VisitETSParameterExpression(ir::ETSParameterExpression *pexpr) override
    {
        pexpr->AsETSParameterExpression()->SetInitializer(
            AdjustType(uctx_, pexpr->Initializer(), pexpr->Ident()->TsType()));
    }

    void VisitVariableDeclarator(ir::VariableDeclarator *vdecl) override
    {
        if (vdecl->Init() != nullptr) {
            vdecl->SetInit(AdjustType(uctx_, vdecl->Init(), vdecl->Id()->Variable()->TsType()));
        }
    }

    void VisitTSNonNullExpression(ir::TSNonNullExpression *nnexpr) override
    {
        if (nnexpr->Expr()->TsType()->IsETSPrimitiveType()) {
            ReplaceInParent(nnexpr, nnexpr->Expr());
            return;
        }
        nnexpr->SetTsType(uctx_->checker->GetNonNullishType(nnexpr->Expr()->TsType()));
        nnexpr->SetOriginalType(nnexpr->TsType());
    }

    // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes, readability-identifier-naming, G.NAM.03-CPP)
    UnboxContext *uctx_;
};
}  // namespace

//  Extracted just to avoid large depth of method 'SetUpBuiltinConstructorsAndMethods(UnboxContext *uctx)'.
static void HandleInstanceMethodsDeclaration(checker::Type *tp, UnboxContext *uctx)
{
    for (auto [_, var] : tp->AsETSObjectType()->InstanceMethods()) {
        auto *nd = var->Declaration()->Node();
        HandleDeclarationNode(uctx, nd);
        if (nd->IsMethodDefinition()) {
            for (auto overload : nd->AsMethodDefinition()->Overloads()) {
                HandleDeclarationNode(uctx, overload);
            }
        }
    }
}

//  Extracted just to avoid large depth of method 'SetUpBuiltinConstructorsAndMethods(UnboxContext *uctx)'.
static void HandleStaticMethodDeclaration(checker::Type *tp, UnboxContext *uctx)
{
    for (auto [_, var] : tp->AsETSObjectType()->StaticMethods()) {
        auto *nd = var->Declaration()->Node();
        HandleDeclarationNode(uctx, nd);
        if (nd->IsMethodDefinition()) {
            for (auto overload : nd->AsMethodDefinition()->Overloads()) {
                HandleDeclarationNode(uctx, overload);
            }
        }
    }
}

// We need to convert function declarations that can be referenced even without explicit mention
// in the source code.
void SetUpBuiltinConstructorsAndMethods(UnboxContext *uctx)
{
    auto *checker = uctx->checker;
    auto setUpType = [&uctx](checker::Type *tp) {
        if (tp == nullptr || !tp->IsETSObjectType()) {
            return;
        }
        for (auto *sig : tp->AsETSObjectType()->ConstructSignatures()) {
            HandleDeclarationNode(uctx, sig->Function());
        }
        HandleInstanceMethodsDeclaration(tp, uctx);
        HandleStaticMethodDeclaration(tp, uctx);
    };

    for (auto tpix = (size_t)checker::GlobalTypeId::ETS_BOOLEAN; tpix < (size_t)checker::GlobalTypeId::ETS_BIG_INT;
         tpix++) {
        setUpType(checker->GetGlobalTypesHolder()->GlobalTypes().at(tpix));
    }
}

template <bool PROG_IS_EXTERNAL = false>
static void VisitExternalPrograms(UnboxVisitor *visitor, parser::Program *program)
{
    for (auto &[_, extPrograms] : program->ExternalSources()) {
        (void)_;
        for (auto *extProg : extPrograms) {
            VisitExternalPrograms<true>(visitor, extProg);
        }
    }

    if constexpr (!PROG_IS_EXTERNAL) {
        return;
    }

    auto annotationIterator = [visitor](auto *child) {
        if (child->IsClassProperty()) {
            auto prop = child->AsClassProperty();
            HandleClassProperty(visitor->uctx_, prop, true);
            if (prop->Value() != nullptr) {
                ES2PANDA_ASSERT(prop->Value()->IsLiteral() || prop->Value()->IsArrayExpression() ||
                                (prop->Value()->IsTyped() && prop->Value()->AsTyped()->TsType()->IsETSEnumType()));
                prop->Value()->Accept(visitor);
            }
            visitor->VisitClassProperty(child->AsClassProperty());
        };
    };

    program->Ast()->IterateRecursivelyPostorder([&annotationIterator](ir::AstNode *ast) {
        if (ast->IsAnnotationDeclaration() || ast->IsAnnotationUsage()) {
            ast->Iterate(annotationIterator);
        }
    });
}

bool UnboxPhase::PerformForModule(public_lib::Context *ctx, parser::Program *program)
{
    auto uctx = UnboxContext(ctx);

    SetUpBuiltinConstructorsAndMethods(&uctx);

    NormalizeAllTypes(&uctx, program->Ast());

    program->Ast()->IterateRecursivelyPostorder([&uctx](ir::AstNode *ast) {
        if (ast->IsClassProperty() || ast->IsScriptFunction() || ast->IsVariableDeclarator()) {
            HandleDeclarationNode(&uctx, ast);
        } else if (ast->IsForOfStatement()) {
            HandleForOfStatement(&uctx, ast->AsForOfStatement());
        }
    });

    UnboxVisitor visitor(&uctx);
    program->Ast()->IterateRecursivelyPostorder([&visitor](ir::AstNode *ast) { ast->Accept(&visitor); });
    VisitExternalPrograms(&visitor, program);

    for (auto *stmt : program->Ast()->Statements()) {
        RefineSourceRanges(stmt);
    }
    uctx.checker->ClearApparentTypes();

    return true;
}

}  // namespace ark::es2panda::compiler

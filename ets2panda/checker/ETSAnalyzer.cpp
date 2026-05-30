/**
 * Copyright (c) 2021-2026 Huawei Device Co., Ltd.
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

#include "ETSAnalyzer.h"
#include "checker/ETSchecker.h"
#include "checker/types/ets/etsObjectTypeConstants.h"
#include "checker/types/ets/etsTupleType.h"
#include "checker/types/globalTypesHolder.h"
#include "checker/types/typeError.h"
#include "checker/types/typeRelation.h"
#include "compiler/lowering/checkerPhase.h"
#include "compiler/lowering/ets/setJumpTarget.h"
#include "compiler/lowering/util.h"
#include "evaluate/scopedDebugInfoPlugin.h"
#include "ir/base/classProperty.h"
#include "ir/base/methodDefinition.h"
#include "ir/base/property.h"
#include "ir/ets/etsDestructuring.h"

namespace ark::es2panda::checker {

static Type *GetAppropriatePreferredType(Type *originalType, std::function<bool(Type *)> const &predicate);
ETSChecker *ETSAnalyzer::GetETSChecker() const
{
    return static_cast<ETSChecker *>(GetChecker());
}

static bool IsInsideObjectLiteralMethod(const ir::AstNode *ast)
{
    bool foundMethod = false;
    for (const ir::AstNode *curr = ast->Parent(); curr != nullptr; curr = curr->Parent()) {
        if (curr->IsProperty() && curr->AsProperty()->IsMethod()) {
            foundMethod = true;
        }
        if (foundMethod && curr->IsObjectExpression()) {
            return true;
        }
    }
    return false;
}

static ETSObjectType *GetObjectLiteralMethodThisType(const ir::AstNode *ast)
{
    if (!IsInsideObjectLiteralMethod(ast)) {
        return nullptr;
    }

    auto *ancestor = util::Helpers::FindAncestorGivenByType(ast, ir::AstNodeType::OBJECT_EXPRESSION);
    if (ancestor == nullptr) {
        return nullptr;
    }

    auto *preferredType = ancestor->AsObjectExpression()->PreferredType();
    if (preferredType == nullptr) {
        return nullptr;
    }

    if (preferredType->IsETSTypeAliasType()) {
        preferredType = preferredType->AsETSTypeAliasType()->GetTargetType();
    }

    if (preferredType->IsETSUnionType()) {
        preferredType = GetAppropriatePreferredType(preferredType, [](Type *type) { return type->IsETSObjectType(); });
    }

    return preferredType != nullptr && preferredType->IsETSObjectType() ? preferredType->AsETSObjectType() : nullptr;
}

static bool IsNotInStructFieldInitializer(const ir::AstNode *node)
{
    for (const ir::AstNode *current = node; current != nullptr; current = current->Parent()) {
        if (current->IsClassDefinition() && current->AsClassDefinition()->IsFromStruct()) {
            return false;
        }
    }
    return true;
}

static bool IsExpressionInClassPropertyInitializer(const ir::Expression *expr)
{
    for (const ir::AstNode *node = expr; node != nullptr && !node->IsClassDefinition(); node = node->Parent()) {
        if (node->IsClassProperty() && IsNotInStructFieldInitializer(node)) {
            return true;
        }
    }
    return false;
}

static void LogNonExportedTypeError(ETSChecker *checker, checker::Type const *type, ir::AstNode const *decl)
{
    if (!util::Helpers::IsExported(decl) && !util::Helpers::IsStdLib(decl->Program())) {
        checker->LogError(diagnostic::USED_TYPE_IS_NOT_EXPORTED, {type->ToString()}, decl->Start());
    }
}

static void CheckPartialTypeExport(ETSChecker *checker, checker::ETSObjectType const *partialType)
{
    auto *baseType = partialType->GetBaseType();
    if (baseType == nullptr || baseType->HasObjectFlag(ETSObjectFlags::BUILTIN_TYPE)) {
        return;
    }
    LogNonExportedTypeError(checker, static_cast<Type const *>(baseType), baseType->GetDeclNode());
}

static void CheckExport(ETSChecker *checker, checker::Type const *type)
{
    if (type == nullptr || type->IsTypeError()) {
        return;
    }

    auto const checkExported = [checker](Type const *testType) {
        if (testType->IsETSObjectType()) {
            if (testType->AsETSObjectType()->IsPartial()) {
                CheckPartialTypeExport(checker, testType->AsETSObjectType());
                return;
            }
            if (testType->AsETSObjectType()->HasObjectFlag(ETSObjectFlags::BUILTIN_TYPE)) {
                return;
            }
            LogNonExportedTypeError(checker, testType, testType->AsETSObjectType()->GetDeclNode());
            return;
        }
        if (testType->IsETSTypeAliasType()) {
            LogNonExportedTypeError(checker, testType, testType->AsETSTypeAliasType()->GetDeclNode());
        }
    };

    // class AtomicInt {} need to write "export" qualifier (source
    // ./runtime_core/static_core/plugins/ets/stdlib/std/containers/ConcurrencyHelpers.ets) class AtomicInt {} need
    // to write "export" qualifier (source
    // ./runtime_core/static_core/plugins/ets/stdlib/std/concurrency/ConcurrencyHelpers.ets) class AtomicInt {} has
    // the same code in both sources after that need to remove util::Helpers::IsStdLib() condition

    type->IterateRecursively(checkExported);
}

//  Helper: checks that type parameter default types and constraints were declared exported
static void CheckExportForTypeParams(ETSChecker *checker, ir::TSTypeParameterDeclaration *typeParams)
{
    if (typeParams == nullptr) {
        return;
    }

    for (auto *typeParam : typeParams->Params()) {
        auto *constraintType = typeParam->Constraint();
        if (constraintType != nullptr) {
            CheckExport(checker, constraintType->GetType(checker));
        }
        auto *defaultType = typeParam->DefaultType();
        if (defaultType != nullptr) {
            CheckExport(checker, defaultType->GetType(checker));
        }
    }
}

//  Helper: checks that type alias was declared exported
static void CheckExportForTypeAlias(checker::ETSChecker *checker, const ir::AstNode *node)
{
    if (!node->IsETSTypeReferencePart()) {
        return;
    }
    auto *typeRef = node->AsETSTypeReferencePart();
    auto *baseName = typeRef->Name();
    if (baseName == nullptr || baseName->Variable() == nullptr) {
        return;
    }
    auto *declNode = baseName->Variable()->Declaration()->Node();
    if (declNode->IsTSTypeAliasDeclaration() && !util::Helpers::IsExported(declNode)) {
        checker->LogError(diagnostic::USED_TYPE_IS_NOT_EXPORTED, {baseName->AsIdentifier()->Name().Utf8()},
                          baseName->Start());
    }
}

//  from base folder
checker::Type *ETSAnalyzer::Check(ir::CatchClause *st) const
{
    ETSChecker *checker = GetETSChecker();
    checker::Type *exceptionType = checker->GlobalTypeError();

    if (st->Param() != nullptr) {
        ES2PANDA_ASSERT(st->Param()->IsIdentifier());

        ir::Identifier *paramIdent = st->Param()->AsIdentifier();
        if (!paramIdent->IsErrorPlaceHolder()) {
            if (paramIdent->TypeAnnotation() != nullptr) {
                checker::Type *catchParamAnnotationType = paramIdent->TypeAnnotation()->GetType(checker);
                exceptionType = checker->CheckExceptionOrErrorType(catchParamAnnotationType, st->Param()->Start());
            } else {
                exceptionType = checker->GlobalBuiltinErrorType();
            }
            paramIdent->Variable()->SetTsType(exceptionType);
        }
        paramIdent->SetTsType(exceptionType);
    } else {
        ES2PANDA_ASSERT(checker->IsAnyError());
    }

    const varbinder::Variable *catchVar = nullptr;
    if (st->Param() != nullptr && st->Param()->IsIdentifier()) {
        catchVar = st->Param()->AsIdentifier()->Variable();
        ES2PANDA_ASSERT(catchVar != nullptr);
        catchParamStack_.push_back(catchVar);
    }

    st->Body()->Check(checker);

    if (catchVar != nullptr) {
        catchParamStack_.pop_back();
    }

    return st->SetTsType(exceptionType);
}

checker::Type *ETSAnalyzer::Check(ir::ClassDefinition *node) const
{
    ETSChecker *checker = GetETSChecker();

    if (node->TsType() == nullptr) {
        checker->BuildBasicClassProperties(node);
    }

    // NOTE(vpukhov): #31391
    if (!node->IsClassDefinitionChecked()) {
        checker->CheckClassDefinition(node);
    }

    if ((node->IsExported() || node->IsDefaultExported()) && node->TsType() != nullptr) {
        const auto *classType = node->TsType();
        if (classType->IsETSObjectType() && classType->AsETSObjectType()->SuperType() != nullptr) {
            CheckExport(checker, classType->AsETSObjectType()->SuperType());
        }
        CheckExportForTypeParams(checker, node->TypeParams());
    }
    return node->TsType();
}

static void CheckOverridenFieldImpl(ir::ClassProperty *st, ETSChecker *checker, ir::ClassDefinition *classDef,
                                    varbinder::LocalVariable *propVar, util::StringView superTypeName)
{
    auto *propNode = propVar->Declaration()->Node();
    propNode->Check(checker);
    if (propNode->IsPrivate()) {
        if (st->IsOverride()) {
            checker->LogError(diagnostic::OVERRIDE_NOT_PRIVATE, {propVar->Declaration()->Name(), superTypeName},
                              st->Start());
        }
        return;
    }

    if (st->HasAnnotations()) {
        checker->LogError(diagnostic::CANNOT_ANNOTATE, {propVar->Declaration()->Name(), superTypeName}, st->Start());
    }

    size_t baseAccessLevel = propNode->IsPrivate() ? 0 : propNode->IsProtected() ? 1 : 2;
    size_t derivedAccessLevel = st->IsPrivate() ? 0 : st->IsProtected() ? 1 : 2;
    if (baseAccessLevel != derivedAccessLevel) {
        checker->LogError(diagnostic::ACCESS_MODIFIER_MISMATCH,
                          {st->Id()->Name(), classDef->Ident()->Name(), superTypeName}, st->Start());
    }

    if (!st->TsType()->IsETSTypeParameter() && propVar->Declaration()->Node()->IsClassProperty()) {
        auto *baseProp = propVar->Declaration()->Node()->AsClassProperty();
        if (baseProp->TsType() != nullptr && baseProp->TsType()->IsETSTypeParameter()) {
            return;
        }
    }

    if (!checker->Relation()->IsIdenticalTo(propVar->TsType(), st->TsType())) {
        checker->LogError(diagnostic::INCOMPATIBLE_TYPE_FOR_OVERRIDE,
                          {st->Id()->Name(), "", classDef->Ident()->Name(), propVar->Name(), "", superTypeName},
                          st->Start());
    }

    st->SetOverride();
    st->SetBasePropertyVar(propVar);
    if (propNode->IsDefinite()) {
        st->AddModifier(ir::ModifierFlags::DEFINITE);
    } else {
        st->ClearModifier(ir::ModifierFlags::DEFINITE);
    }
}

static bool CheckFieldInitializationInBody(ir::AstNode *body, ir::ClassProperty *st)
{
    bool isInitialized = false;

    body->IterateRecursively([&](ir::AstNode *node) {
        if (isInitialized) {
            return;
        }
        if (!node->IsAssignmentExpression()) {
            return;
        }

        auto *assign = node->AsAssignmentExpression();
        auto *left = assign->Left();

        if (left->IsMemberExpression()) {
            auto *member = left->AsMemberExpression();
            if (member->Object() && member->Object()->IsThisExpression() && member->Property() &&
                member->Property()->IsIdentifier() && member->Property()->AsIdentifier()->Name() == st->Id()->Name()) {
                isInitialized = true;
            }
        } else if (left->IsIdentifier() && left->AsIdentifier()->Name() == st->Id()->Name()) {
            isInitialized = true;
        }
    });

    return isInitialized;
}

static bool CheckOverridenFieldInitialization(ir::ClassProperty *st, ir::ClassDefinition *classDef)
{
    auto modifiers = st->Modifiers();
    const bool isDefinite = (modifiers & ir::ModifierFlags::DEFINITE) != 0;
    if (isDefinite) {
        return true;
    }
    if (st->Value() != nullptr) {
        return true;
    }

    for (const auto it : classDef->Body()) {
        if (!it->IsMethodDefinition()) {
            continue;
        }

        auto *methodDef = it->AsMethodDefinition();
        if (!methodDef->IsConstructor() || !methodDef->IsDefaultAccessModifier()) {
            continue;
        }

        auto *body = methodDef->Function()->Body();
        if (body == nullptr) {
            continue;
        }

        bool isInitialized = CheckFieldInitializationInBody(body, st);
        if (isInitialized) {
            return true;
        }
    }

    return false;
}

static void CheckOverridenField(ir::ClassProperty *st, ETSChecker *checker, ir::ClassDefinition *classDef)
{
    auto *superType = classDef->Super()->TsType()->AsETSObjectType();
    ES2PANDA_ASSERT(classDef->Super() != nullptr && superType != nullptr && !superType->IsGradual());
    varbinder::LocalVariable *propVar = nullptr;
    while (superType != nullptr) {
        propVar = superType->GetProperty(st->Id()->Name(), PropertySearchFlags::SEARCH_INSTANCE_FIELD);
        if (propVar != nullptr) {
            break;
        }
        superType = superType->SuperType();
    }

    if (superType == nullptr) {
        if (st->IsOverride()) {
            checker->LogError(diagnostic::OVERRIDE_NOT_IN_BASE,
                              {classDef->Super()->TsType()->AsETSObjectType()->Name()}, st->Start());
        }
        ES2PANDA_ASSERT(st->BasePropertyVar() == nullptr);
        return;
    }

    bool isInitialized = CheckOverridenFieldInitialization(st, classDef);
    if (!isInitialized) {
        checker->LogError(diagnostic::OVERRIDE_FIELD_MUST_HAVE_INITIALIZER, {st->Id()->Name()}, st->Start());
        return;
    }

    ES2PANDA_ASSERT(propVar != nullptr);
    CheckOverridenFieldImpl(st, checker, classDef, propVar, superType->AsETSObjectType()->Name());
}

static void CheckFieldOverride(ir::ClassProperty *st, ETSChecker *checker)
{
    if (st->IsStatic()) {
        if (st->IsOverride()) {
            checker->LogError(diagnostic::STATIC_OVERRIDE, {st->Id()->Name()}, st->Start());
        }
        return;
    }

    auto *parent = st->Parent();
    if (parent == nullptr || !parent->IsClassDefinition()) {
        return;
    }

    ir::ClassDefinition *classDef = parent->AsClassDefinition();
    util::StringView subClassName = classDef->Ident()->Name();
    if (classDef->Super() == nullptr) {
        if (st->IsOverride()) {
            checker->LogError(diagnostic::OVERRIDE_NOT_EXTENDS, {subClassName}, st->Start());
        }
        return;
    }
    if (classDef->Super() != nullptr && classDef->Super()->TsType() != nullptr &&
        classDef->Super()->TsType()->IsETSObjectType()) {
        CheckOverridenField(st, checker, classDef);
    }
}

static void TypeAnnoCheckForExportedMethod(checker::ETSChecker *checker, ir::MethodDefinition *methodDef)
{
    if (!methodDef->IsConstructor() && !methodDef->Function()->IsSetter() &&
        methodDef->Function()->ReturnTypeAnnotation() == nullptr) {
        checker->LogError(diagnostic::EXPORTED_ENTITIES_DOESNOT_HAS_TYPEANNO, {"function", methodDef->Id()->Name()},
                          methodDef->Start());
    }
}

static void TypeAnnoCheckForExportedClassProp(checker::ETSChecker *checker, ir::ClassProperty *classProp)
{
    if (classProp->TypeAnnotation() == nullptr && !classProp->Parent()->IsAnnotationUsage() &&
        classProp->Id()->Name() != compiler::Signatures::REEXPORT_DEFAULT_ANONYMOUSLY) {
        const std::string entityType = util::Helpers::IsGlobalClass(checker->Context().ContainingClass()->GetDeclNode())
                                           ? "variable"
                                           : "class property";
        checker->LogError(diagnostic::EXPORTED_ENTITIES_DOESNOT_HAS_TYPEANNO, {entityType, classProp->Id()->Name()},
                          classProp->Start());
    }
}

checker::Type *ETSAnalyzer::Check(ir::ClassProperty *st) const
{
    if (st->TsType() != nullptr) {
        return st->TsType();
    }

    ES2PANDA_ASSERT(st->Id() != nullptr);

    ETSChecker *checker = GetETSChecker();

    if (st->Id()->Variable() == nullptr) {
        // Now invalid or dummy nodes obtaining after parsing don't have associated variables at all, that leads to
        // incorrect AST and multiple reported errors in AST verifier. Need to create and bind [special]? variables for
        // them with default TypeError set[?]. Why can't we directly check the 'Id'? During the process of
        // resolveIdentifier, we might obtain the wrong variable, which breaks the consistency between the variable and
        // its tsType. see wrong_variable_binding.ets for more details.
        auto ident = st->Id();
        auto [decl, var] = checker->VarBinder()->NewVarDecl<varbinder::LetDecl>(
            ident->Start(), compiler::GenName(checker->ProgramAllocator()).View());
        var->SetScope(checker->VarBinder()->GetScope());
        ident->SetVariable(var);
        decl->BindNode(ident);
        ident->SetTsType(var->SetTsType(checker->GlobalTypeError()));
    }

    ES2PANDA_ASSERT(st->Id()->Variable() != nullptr);

    checker->CheckAnnotations(st);
    if (st->TypeAnnotation() != nullptr) {
        st->TypeAnnotation()->Check(checker);
    }

    checker::SavedCheckerContext savedContext(checker, checker->Context().Status(),
                                              checker->Context().ContainingClass(),
                                              checker->Context().ContainingSignature());

    if (st->IsStatic()) {
        checker->AddStatus(checker::CheckerStatus::IN_STATIC_CONTEXT);
    }

    checker::Type *propertyType =
        checker->CheckVariableDeclaration(st->Id(), st->TypeAnnotation(), st->Value(), st->Modifiers());

    propertyType = propertyType != nullptr ? propertyType : checker->GlobalTypeError();
    st->SetTsType(propertyType);

    if (st->IsDefinite() && propertyType->PossiblyETSNullish()) {
        checker->LogError(diagnostic::LATE_INITIALIZATION_FIELD_HAS_INVALID_TYPE, st->TypeAnnotation()->Start());
    }
    CheckFieldOverride(st, checker);

    if (!st->IsPrivate() && util::Helpers::IsExported(st)) {
        CheckExport(checker, propertyType);
        TypeAnnoCheckForExportedClassProp(checker, st);
    }

    return propertyType;
}

checker::Type *ETSAnalyzer::Check(ir::ClassStaticBlock *st) const
{
    ETSChecker *checker = GetETSChecker();

    if (checker->HasStatus(checker::CheckerStatus::INNER_CLASS)) {
        checker->LogError(diagnostic::STATIC_INIT_IN_NESTED_CLASS, {}, st->Start());
        st->SetTsType(checker->GlobalTypeError());
        return st->TsType();
    }

    auto *func = st->Function();
    checker->BuildFunctionSignature(func);

    if (func->Signature() == nullptr) {
        st->SetTsType(checker->GlobalTypeError());
    } else {
        st->SetTsType(checker->BuildMethodType(func));
    }

    if (!func->HasBody() || (func->IsExternal() && !func->IsExternalOverload())) {
        return st->TsType();
    }

    checker::ScopeContext scopeCtx(checker, func->Scope());
    checker::SavedCheckerContext savedContext(checker, checker->Context().Status(),
                                              checker->Context().ContainingClass());
    checker->AddStatus(checker::CheckerStatus::IN_STATIC_BLOCK | checker::CheckerStatus::IN_STATIC_CONTEXT);
    func->Body()->Check(checker);
    return st->TsType();
}

static void CheckAsyncFunctionReturnType(ETSChecker *checker, ir::ScriptFunction *scriptFunc)
{
    ES2PANDA_ASSERT(scriptFunc);

    if (!scriptFunc->IsAsyncFunc() || scriptFunc->IsProxy() || scriptFunc->Signature() == nullptr) {
        return;
    }

    /*
     * NOTE(knazarov): To not break compatibility with existing behaviour,
     * we keep return type of the AsyncImpl methods as Object, so here we only check
     * AsyncFunc itself.
     */
    auto *asyncFuncReturnType = scriptFunc->Signature()->ReturnType();
    ES2PANDA_ASSERT(asyncFuncReturnType);

    if (!asyncFuncReturnType->IsETSObjectType() || !checker->IsPromiseType(asyncFuncReturnType->AsETSObjectType())) {
        checker->LogError(diagnostic::ASYNC_FUNCTION_RETURN_TYPE, {}, scriptFunc->Start());
        scriptFunc->Signature()->SetReturnType(checker->GlobalTypeError());
        return;
    }
}

//  Extracted from 'ETSAnalyzer::Check(ir::MethodDefinition *node)' to reduce its size
static checker::Type *CheckMethodDefinitionHelper(ETSChecker *checker, ir::MethodDefinition *method) noexcept
{
    auto *const methodType = method->TsType();
    // NOTE(gogabr): temporary, until we have proper bridges, see #16485
    // Don't check overriding for synthetic functional classes.
    if ((method->Parent()->Modifiers() & ir::ModifierFlags::FUNCTIONAL) == 0) {
        checker->CheckOverride(methodType->AsETSFunctionType()->FindSignature(method->Function()));
    }

    for (auto *overload : method->Overloads()) {
        overload->Check(checker);
    }

    if (!method->IsPrivate() && method->Function() != nullptr && util::Helpers::IsExported(method) &&
        method->Id()->Name().Utf8().find("lambda_invoke-") == std::string_view::npos) {
        CheckExport(checker, methodType);
        CheckExportForTypeParams(checker, method->Function()->TypeParams());
        TypeAnnoCheckForExportedMethod(checker, method);
    }

    return methodType;
}

static bool IsInitializerBlockTransfer(std::string_view str)
{
    auto prefix = compiler::Signatures::INITIALIZER_BLOCK_INIT;
    return str.size() >= prefix.size() && str.compare(0, prefix.size(), prefix) == 0;
}

// CC-OFFNXT(G.NAM.03-CPP) project code style
static bool IsSignatureUnreachable(ETSChecker *checker, Signature *currSig, Signature *prevSig,
                                   std::optional<lexer::SourcePosition> start_pos)
{
    SavedTypeRelationFlagsContext savedFlagsCtx(checker->Relation(), TypeRelationFlag::NO_RETURN_TYPE_CHECK);
    if (checker->Relation()->SignatureIsCoveredBy(currSig, prevSig)) {
        auto start = start_pos.has_value() ? start_pos.value() : currSig->Function()->Id()->Start();
        checker->LogError(diagnostic::OVERLOAD_UNREACHABLE_WARNING, {currSig->ToString(), prevSig->ToString()}, start);
        return true;
    }
    return false;
}

static Type *CheckUnreachableSignatureInFunctionType(
    ETSChecker *checker, Type *type,
    std::optional<lexer::SourcePosition> start_pos = std::optional<lexer::SourcePosition>())
{
    if (type == nullptr || !type->IsETSFunctionType()) {
        return type;
    }

    auto &signatures = type->AsETSFunctionType()->CallSignatures();
    for (size_t j = 1; j < signatures.size(); ++j) {
        for (size_t i = 0; i < j; ++i) {
            auto *currSig = signatures[j];
            auto *prevSig = signatures[i];

            if (IsSignatureUnreachable(checker, currSig, prevSig, start_pos)) {
                break;
            }
        }
    }

    return type;
}

checker::Type *ETSAnalyzer::Check(ir::MethodDefinition *node) const
{
    ETSChecker *checker = GetETSChecker();
    auto *scriptFunc = node->Function();

    // CC-OFFNXT(G.FMT.14-CPP) project code style
    auto const returnErrorType = [checker, node]() -> checker::Type * {
        node->SetTsType(checker->GlobalTypeError());
        return node->TsType();
    };

    checker->CheckAnnotations(scriptFunc);
    checker->CheckFunctionSignatureAnnotations(scriptFunc->Params(), scriptFunc->TypeParams(),
                                               scriptFunc->ReturnTypeAnnotation());

    if (scriptFunc->IsProxy()) {
        return ReturnTypeForStatement(node);
    }

    ES2PANDA_ASSERT(!(scriptFunc->IsGetter() && scriptFunc->IsSetter()));
    if (scriptFunc->IsGetter() || scriptFunc->IsSetter()) {
        auto status = scriptFunc->IsGetter() ? CheckerStatus::IN_GETTER : CheckerStatus::IN_SETTER;
        checker->AddStatus(status);
    }

    // NOTE: aszilagyi. make it correctly check for open function not have body
    if (!scriptFunc->HasBody() && !(node->IsAbstract() || node->IsNative() || node->IsDeclare() ||
                                    checker->HasStatus(checker::CheckerStatus::IN_INTERFACE))) {
        checker->LogError(diagnostic::FUNCTION_WITHOUT_BODY, {}, scriptFunc->Start());
        return returnErrorType();
    }

    if (CheckReturnTypeNecessity(node) && scriptFunc->ReturnTypeAnnotation() == nullptr) {
        checker->LogError(diagnostic::MISSING_RETURN_TYPE, {}, scriptFunc->Start());
        return returnErrorType();
    }

    if (node->TsType() == nullptr) {
        node->SetTsType(checker->BuildMethodSignature(node));
    }

    if (IsInitializerBlockTransfer(scriptFunc->Id()->Name().Utf8())) {
        checker->AddStatus(CheckerStatus::IN_STATIC_BLOCK);
    }

    if (node->TsType() != nullptr && node->TsType()->IsTypeError()) {
        return node->TsType();
    }

    this->CheckMethodModifiers(node);
    CheckAsyncFunctionReturnType(checker, scriptFunc);
    DoBodyTypeChecking(checker, node, scriptFunc);
    CheckPredefinedMethodReturnType(checker, scriptFunc);
    if (node->TsType()->IsTypeError()) {
        return node->TsType();
    }

    auto *type = CheckMethodDefinitionHelper(checker, node);
    return CheckUnreachableSignatureInFunctionType(checker, type);
}

void ETSAnalyzer::CheckMethodModifiers(ir::MethodDefinition *node) const
{
    ETSChecker *checker = GetETSChecker();
    auto const notValidInAbstract = ir::ModifierFlags::NATIVE | ir::ModifierFlags::PRIVATE |
                                    ir::ModifierFlags::OVERRIDE | ir::ModifierFlags::FINAL | ir::ModifierFlags::STATIC;

    if (node->IsAbstract() && (node->Modifiers() & notValidInAbstract) != 0U) {
        checker->LogError(diagnostic::ABSTRACT_METHOD_INVALID_MODIFIER, {}, node->Start());
        node->SetTsType(checker->GlobalTypeError());
        return;
    }

    if ((node->IsAbstract() || (!node->Function()->HasBody() && !node->IsNative() && !node->IsDeclare())) &&
        !(checker->HasStatus(checker::CheckerStatus::IN_ABSTRACT) ||
          checker->HasStatus(checker::CheckerStatus::IN_INTERFACE))) {
        checker->LogError(diagnostic::ABSTRACT_IN_CONCRETE, {}, node->Start());
        node->SetTsType(checker->GlobalTypeError());
    }

    auto const notValidInFinal = ir::ModifierFlags::ABSTRACT | ir::ModifierFlags::STATIC;

    if (node->IsFinal() && (node->Modifiers() & notValidInFinal) != 0U) {
        checker->LogError(diagnostic::FINAL_METHOD_INVALID_MODIFIER, {}, node->Start());
        node->SetTsType(checker->GlobalTypeError());
    }

    auto const notValidInStatic = ir::ModifierFlags::ABSTRACT | ir::ModifierFlags::FINAL | ir::ModifierFlags::OVERRIDE;

    if (node->IsStatic() && (node->Modifiers() & notValidInStatic) != 0U) {
        checker->LogError(diagnostic::STATIC_METHOD_INVALID_MODIFIER, {}, node->Start());
        node->SetTsType(checker->GlobalTypeError());
    }
}

static void CheckDuplicationInOverloadDeclaration(ETSChecker *const checker, ir::OverloadDeclaration *const node)
{
    auto overloadedNameSet = ArenaSet<std::string>(checker->ProgramAllocator()->Adapter());
    for (ir::Expression *const overloadedName : node->OverloadedList()) {
        bool isQualifiedName = true;
        std::function<std::string(ir::Expression *const)> getFullOverloadedName =
            [&isQualifiedName, &getFullOverloadedName](ir::Expression *const expr) -> std::string {
            if (!isQualifiedName) {
                return "";
            }
            if (expr->IsIdentifier()) {
                return expr->AsIdentifier()->Name().Mutf8();
            }
            if (expr->IsMemberExpression()) {
                return getFullOverloadedName(expr->AsMemberExpression()->Object()) + "." +
                       getFullOverloadedName(expr->AsMemberExpression()->Property());
            }
            isQualifiedName = false;
            return "";
        };
        std::string fullOverloadedName = getFullOverloadedName(overloadedName);
        if (!isQualifiedName) {
            continue;
        }
        if (overloadedNameSet.find(fullOverloadedName) != overloadedNameSet.end()) {
            checker->LogError(diagnostic::DUPLICATE_OVERLOADED_NAME, overloadedName->Start());
            continue;
        }
        overloadedNameSet.insert(fullOverloadedName);
    }
}

static void CheckOverloadSameNameMethod(ETSChecker *const checker, ir::OverloadDeclaration *const overloadDecl)
{
    Type *objectType = overloadDecl->Parent()->IsClassDefinition()
                           ? overloadDecl->Parent()->AsClassDefinition()->Check(checker)
                           : overloadDecl->Parent()->Parent()->AsTSInterfaceDeclaration()->Check(checker);
    ES2PANDA_ASSERT(objectType->IsETSObjectType());

    if (!overloadDecl->IsClassMethodOverloadDeclaration() && !overloadDecl->IsFunctionOverloadDeclaration() &&
        !overloadDecl->IsInterfaceMethodOverloadDeclaration()) {
        return;
    }

    PropertySearchFlags searchFlags = PropertySearchFlags::DISALLOW_SYNTHETIC_METHOD_CREATION |
                                      (overloadDecl->IsStatic() || overloadDecl->IsFunctionOverloadDeclaration()
                                           ? PropertySearchFlags::SEARCH_STATIC_METHOD
                                           : PropertySearchFlags::SEARCH_INSTANCE_METHOD);
    if (!overloadDecl->IsStatic() && overloadDecl->IsClassMethodOverloadDeclaration()) {
        searchFlags |= PropertySearchFlags::SEARCH_IN_INTERFACES | PropertySearchFlags::SEARCH_IN_BASE;
    }
    auto *sameNameMethod = objectType->AsETSObjectType()->GetProperty(overloadDecl->Id()->Name(), searchFlags);
    if (sameNameMethod == nullptr) {
        return;
    }

    auto *methodOwner = static_cast<checker::ETSObjectType *>(nullptr);
    if (sameNameMethod->TsType() != nullptr && sameNameMethod->TsType()->IsETSFunctionType() &&
        !sameNameMethod->TsType()->AsETSFunctionType()->CallSignatures().empty()) {
        methodOwner = sameNameMethod->TsType()->AsETSFunctionType()->CallSignatures().front()->Owner();
        if (sameNameMethod->HasFlag(varbinder::VariableFlags::PRIVATE) &&
            methodOwner != objectType->AsETSObjectType()) {
            return;
        }
    }

    auto serachName = overloadDecl->Id()->Name().Mutf8();
    auto hasSameNameMethod =
        std::find_if(overloadDecl->OverloadedList().begin(), overloadDecl->OverloadedList().end(),
                     [serachName](ir::Expression *overloadId) {
                         return overloadId->IsIdentifier() && overloadId->AsIdentifier()->Name().Is(serachName);
                     });
    if (hasSameNameMethod == overloadDecl->OverloadedList().end()) {
        checker->LogError(diagnostic::OVERLOAD_SAME_NAME_METHOD, {serachName}, overloadDecl->Start());
    }
}

checker::Type *ETSAnalyzer::Check(ir::OverloadDeclaration *node) const
{
    ETSChecker *checker = GetETSChecker();
    ES2PANDA_ASSERT(node != nullptr);
    ES2PANDA_ASSERT(node->Key());

    CheckDuplicationInOverloadDeclaration(checker, node);
    CheckOverloadSameNameMethod(checker, node);

    if (node->IsConstructorOverloadDeclaration()) {
        ES2PANDA_ASSERT(node->Parent()->IsClassDefinition());
        checker->CheckConstructorOverloadDeclaration(checker, node);
    } else if (node->IsFunctionOverloadDeclaration()) {
        ES2PANDA_ASSERT(
            node->Parent()->IsClassDefinition() &&
            (compiler::HasGlobalClassParent(node) || node->Parent()->AsClassDefinition()->IsNamespaceTransformed()));
        checker->CheckFunctionOverloadDeclaration(checker, node);
    } else if (node->IsClassMethodOverloadDeclaration()) {
        ES2PANDA_ASSERT(node->Parent()->IsClassDefinition());
        checker->CheckClassMethodOverloadDeclaration(checker, node);
    } else if (node->IsInterfaceMethodOverloadDeclaration()) {
        ES2PANDA_ASSERT(node->Parent()->Parent()->IsTSInterfaceDeclaration());
        checker->CheckInterfaceMethodOverloadDeclaration(checker, node);
    }

    auto *type = checker->CreateSyntheticTypeFromOverload(node->Id()->Variable());
    return CheckUnreachableSignatureInFunctionType(checker, type, node->Id()->Start());
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::Property *expr) const
{
    ETSChecker *checker = GetETSChecker();
    return checker->GlobalTypeError();
}

static ir::SpreadElement::ResolvedSpreadKind GetResolvedSpreadKind(ETSChecker *checker, Type *type)
{
    auto *const normalizedType = checker->NormalizeSpreadType(type);
    if (normalizedType->IsTypeError()) {
        return ir::SpreadElement::ResolvedSpreadKind::INVALID;
    }

    if (normalizedType->IsETSTupleType()) {
        return ir::SpreadElement::ResolvedSpreadKind::TUPLE;
    }

    if (normalizedType->IsETSUnionType()) {
        return ir::SpreadElement::ResolvedSpreadKind::ITERABLE;
    }

    if (normalizedType->IsETSStringType() || util::Helpers::IsArrayType(normalizedType)) {
        return ir::SpreadElement::ResolvedSpreadKind::INDEXABLE;
    }

    return ir::SpreadElement::ResolvedSpreadKind::ITERABLE;
}

static void SetResolvedSpread(ETSChecker *checker, ir::SpreadElement *expr, Type *sourceType)
{
    auto const kind = GetResolvedSpreadKind(checker, sourceType);
    auto *const elementType = kind == ir::SpreadElement::ResolvedSpreadKind::INVALID
                                  ? checker->GlobalTypeError()
                                  : checker->GetElementTypeOfSpreadType(sourceType);
    expr->SetResolvedSpread(kind, sourceType, elementType);
}

checker::Type *ETSAnalyzer::Check(ir::SpreadElement *expr) const
{
    ETSChecker *checker = GetETSChecker();
    if (expr->TsType() != nullptr) {
        if (expr->GetResolvedSpreadKind() == ir::SpreadElement::ResolvedSpreadKind::INVALID &&
            checker->IsValidSpreadType(expr->TsType())) {
            SetResolvedSpread(checker, expr, expr->TsType());
        }
        return expr->TsType();
    }

    if (expr->PreferredType() != nullptr) {
        expr->Argument()->SetPreferredType(expr->PreferredType());
    }

    auto const exprType = expr->Argument()->Check(checker);
    if (!checker->IsValidSpreadType(exprType)) {
        if (!exprType->IsTypeError()) {
            // Don't duplicate error messages for the same error
            checker->LogError(diagnostic::SPREAD_OF_INVALID_TYPE, {exprType}, expr->Start());
        }
        expr->SetResolvedSpread(ir::SpreadElement::ResolvedSpreadKind::INVALID, checker->GlobalTypeError(),
                                checker->GlobalTypeError());
        return checker->InvalidateType(expr);
    }

    SetResolvedSpread(checker, expr, exprType);
    return expr->SetTsType(exprType);
}

checker::Type *ETSAnalyzer::Check(ir::TemplateElement *expr) const
{
    ETSChecker *checker = GetETSChecker();
    expr->SetTsType(checker->CreateETSStringLiteralType(expr->Raw()));
    return expr->TsType();
}

checker::Type *ETSAnalyzer::Check(ir::ETSClassLiteral *expr) const
{
    ETSChecker *checker = GetETSChecker();
    auto *const literal = expr->Expr();

    checker->LogError(diagnostic::UNSUPPORTED_CLASS_LITERAL, {}, literal->Start());
    expr->SetTsType(checker->GlobalTypeError());
    return expr->TsType();

    auto exprType = literal->Check(checker);

    ArenaVector<checker::Type *> typeArgTypes(checker->ProgramAllocator()->Adapter());
    typeArgTypes.push_back(exprType);  // NOTE: Box it if it's a primitive type

    checker::InstantiationContext ctx(checker, checker->GlobalBuiltinTypeType(), std::move(typeArgTypes),
                                      expr->Range().start);
    expr->SetTsType(ctx.Result());

    return expr->TsType();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::ETSIntrinsicNode *node) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check(ir::ETSFunctionType *node) const
{
    if (node->TsType() != nullptr) {
        return node->TsType();
    }
    ETSChecker *checker = GetETSChecker();
    checker->CheckAnnotations(node);
    checker->CheckFunctionSignatureAnnotations(node->Params(), node->TypeParams(), node->ReturnType());

    auto *signatureInfo = checker->ComposeSignatureInfo(node->TypeParams(), node->Params());
    if (signatureInfo == nullptr) {
        ES2PANDA_ASSERT(GetChecker()->IsAnyError());
        return node->SetTsType(checker->GlobalTypeError());
    }
    auto *returnType = node->IsExtensionFunction() && node->ReturnType()->IsTSThisType()
                           ? signatureInfo->params.front()->TsType()
                           : checker->ComposeReturnType(node->ReturnType(), node->IsAsync());

    auto *const signature =
        checker->CreateSignature(signatureInfo, returnType, node->Flags(), node->IsExtensionFunction());
    if (signature == nullptr) {  // #23134
        ES2PANDA_ASSERT(GetChecker()->IsAnyError());
        return node->SetTsType(checker->GlobalTypeError());
    }

    signature->SetOwner(checker->Context().ContainingClass());

    return node->SetTsType(checker->CreateETSArrowType(signature));
}

static Signature *ValidateParameterlessConstructor(ETSChecker *checker, Signature *signature,
                                                   const lexer::SourcePosition &pos, bool throwError)
{
    if (signature->MinArgCount() != 0) {
        if (throwError) {
            checker->LogError(diagnostic::NO_SUCH_PARAMLESS_CTOR_2, {signature->MinArgCount()}, pos);
        }
        return nullptr;
    }
    return signature;
}

static Signature *CollectParameterlessConstructor(ETSChecker *checker, ArenaVector<Signature *> &signatures,
                                                  const lexer::SourcePosition &pos)
{
    // We are able to provide more specific error messages.
    bool throwError = signatures.size() == 1;
    for (auto *sig : signatures) {
        if (auto *concreteSig = ValidateParameterlessConstructor(checker, sig, pos, throwError);
            concreteSig != nullptr) {
            return concreteSig;
        }
    }
    checker->LogError(diagnostic::NO_SUCH_PARAMLESS_CTOR, {}, pos);
    return nullptr;
}

static bool HasBareTypeParameter(checker::Type const *elementType)
{
    if (elementType->IsETSTypeParameter()) {
        return true;
    }
    if (elementType->IsETSUnionType()) {
        return false;
    }
    return elementType->TypeExpressionContains([](checker::Type const *tp) { return tp->IsETSTypeParameter(); });
}

static bool IsTypeNotPreservedByErasure(checker::Type const *elementType, bool isFirstCall = true);

// Generic helper: returns true if any child type satisfies the erasure check.
template <typename Container>
static bool AnyChildNotPreserved(const Container &children, bool isFirstCall)
{
    for (auto *child : children) {
        if (IsTypeNotPreservedByErasure(child, isFirstCall)) {
            return true;
        }
    }
    return false;
}

// Top-level union: reject only when type params are mixed with concrete types.
static bool CheckTopLevelUnion(checker::Type const *elementType)
{
    bool hasTypeParam = false;
    bool hasConcrete = false;
    for (auto *ct : elementType->AsETSUnionType()->ConstituentTypes()) {
        if (ct->IsETSUndefinedType() || ct->IsETSVoidType()) {
            continue;
        }
        if (ct->IsETSTypeParameter()) {
            hasTypeParam = true;
        } else {
            hasConcrete = true;
            if (IsTypeNotPreservedByErasure(ct)) {
                hasTypeParam = true;
            }
        }
    }
    return hasTypeParam && hasConcrete;
}

// Returns true if the element type is not preserved by type erasure.
// `isFirstCall` is true when checking the FixedArray element type itself;
// it is false when recursing into an erased position inside an erasing structure
// (where a concrete leaf type means the parent structure is not preserved).
static bool IsTypeNotPreservedByErasure(checker::Type const *elementType, bool isFirstCall)
{
    // Bare type parameters (T, U) are preserved by FixedArray — the Effective
    // Type of T is its constraint, but FixedArray stores that effective type
    // directly.  Unlike generic type instantiations (G<T>) where the erasure
    // loses the type argument structure, a bare T has no structure to lose.
    if (elementType->IsETSTypeParameter()) {
        return false;
    }
    if (elementType->IsETSArrayType() || elementType->IsETSReadonlyArrayType()) {
        return false;
    }
    if (elementType->IsETSFunctionType()) {
        return true;
    }
    if (elementType->IsETSUnionType()) {
        return isFirstCall ? CheckTopLevelUnion(elementType)
                           : AnyChildNotPreserved(elementType->AsETSUnionType()->ConstituentTypes(), false);
    }
    if (elementType->IsETSTupleType()) {
        return AnyChildNotPreserved(elementType->AsETSTupleType()->GetTupleTypesList(), false);
    }
    if (elementType->IsETSObjectType() && !elementType->AsETSObjectType()->TypeArguments().empty()) {
        return AnyChildNotPreserved(elementType->AsETSObjectType()->TypeArguments(), false);
    }
    // Leaf type: preserved on the first call; concrete at an erased position.
    return !isFirstCall;
}

// Validates the element type for FixedArray / ValueArray construction.
template <typename T, typename = typename std::enable_if_t<std::is_base_of_v<ir::Expression, T>>>
static bool CheckArrayElementType(ETSChecker *checker, T *newArrayInstanceExpr, checker::Type *elementType)
{
    ES2PANDA_ASSERT(checker != nullptr);
    ES2PANDA_ASSERT(newArrayInstanceExpr != nullptr);
    ES2PANDA_ASSERT(elementType != nullptr);
    ES2PANDA_ASSERT(!elementType->IsETSPrimitiveType());

    if (IsTypeNotPreservedByErasure(elementType)) {
        checker->LogError(diagnostic::TYPE_NOT_PRESERVED_BY_ERASURE_FOR_FIXED_ARRAY, {elementType},
                          newArrayInstanceExpr->Start());
        return false;
    }
    if (elementType->IsETSObjectType()) {
        auto *calleeObj = elementType->AsETSObjectType();
        const auto flags = checker::ETSObjectFlags::ABSTRACT | checker::ETSObjectFlags::INTERFACE;
        if (!calleeObj->HasObjectFlag(flags)) {
            // A workaround check for new Interface[...] in test cases
            newArrayInstanceExpr->SetSignature(CollectParameterlessConstructor(
                checker, calleeObj->ConstructSignatures(), newArrayInstanceExpr->Start()));
            checker->ValidateSignatureAccessibility(calleeObj, newArrayInstanceExpr->Signature(),
                                                    newArrayInstanceExpr->Start());
        } else {
            checker->LogError(diagnostic::ABSTRACT_CLASS_AS_ARRAY_ELEMENT_TYPE, {}, newArrayInstanceExpr->Start());
            return false;
        }
    } else {
        if (HasBareTypeParameter(elementType)) {
            checker->LogError(diagnostic::TYPE_PARAMETER_AS_ARRAY_ELEMENT_TYPE, {}, newArrayInstanceExpr->Start());
            return false;
        }
        if (!checker->Relation()->IsSupertypeOf(elementType, checker->GlobalETSUndefinedType()) &&
            !checker->Relation()->IsIdenticalTo(checker->GetApparentType(elementType), elementType)) {
            checker->LogError(diagnostic::TYPE_PARAMETER_AS_ARRAY_ELEMENT_TYPE, {}, newArrayInstanceExpr->Start());
            return false;
        }
        if (elementType->IsETSFunctionType() && newArrayInstanceExpr->IsETSNewClassInstanceExpression()) {
            return true;
        }
        if (!checker->Relation()->IsSupertypeOf(elementType, checker->GlobalETSUndefinedType())) {
            checker->LogError(diagnostic::NON_SUPERTYPE_OF_UNDEFINED_AS_ARRAY_ELEMENT_TYPE, {},
                              newArrayInstanceExpr->Start());
            return false;
        }
    }
    return true;
}

static checker::Type *CheckInstantiatedNewType(ETSChecker *checker, ir::ETSNewClassInstanceExpression *expr)
{
    auto calleeType = expr->GetTypeRef()->Check(checker);
    FORWARD_TYPE_ERROR(checker, calleeType, expr->GetTypeRef());

    if (calleeType->IsETSUnionType()) {
        return checker->TypeError(expr->GetTypeRef(), diagnostic::UNION_NONCONSTRUCTIBLE, expr->Start());
    }
    if (!ir::ETSNewClassInstanceExpression::TypeIsAllowedForInstantiation(calleeType)) {
        return checker->TypeError(expr->GetTypeRef(), diagnostic::CALLEE_NONCONSTRUCTIBLE, {calleeType}, expr->Start());
    }
    if (!calleeType->IsETSObjectType()) {
        return checker->TypeError(expr->GetTypeRef(), diagnostic::EXPR_NONCONSTRUCTIBLE, {}, expr->Start());
    }

    auto calleeObj = calleeType->AsETSObjectType();
    if (calleeObj->HasObjectFlag(checker::ETSObjectFlags::ABSTRACT)) {
        checker->LogError(diagnostic::ABSTRACT_INSTANTIATION, {calleeObj->Name()}, expr->Start());
        return checker->GlobalTypeError();
    }

    if (calleeObj->HasObjectFlag(checker::ETSObjectFlags::INTERFACE)) {
        checker->LogError(diagnostic::INTERFACE_INSTANTIATION, {calleeObj->Name()}, expr->Start());
        return checker->GlobalTypeError();
    }

    if (calleeObj->HasObjectFlag(ETSObjectFlags::REQUIRED) &&
        !expr->HasAstNodeFlags(ir::AstNodeFlags::ALLOW_REQUIRED_INSTANTIATION)) {
        checker->LogError(diagnostic::NONLITERAL_INSTANTIATION, {}, expr->GetTypeRef()->Start());
        return checker->GlobalTypeError();
    }

    return calleeType;
}

/*
 * Object literals do not get checked in the process of call resolution; we need to check them separately
 * afterwards.
 */
static void CheckObjectLiteralArguments(ETSChecker *checker, Signature *signature,
                                        ArenaVector<ir::Expression *> const &arguments)
{
    for (uint32_t index = 0; index < arguments.size(); index++) {
        if (!arguments[index]->IsObjectExpression()) {
            continue;
        }

        Type *tp;
        if (index >= signature->Params().size()) {
            ES2PANDA_ASSERT(signature->RestVar());
            // Use element type as rest object literal type
            tp = checker->GetElementTypeOfArray(signature->RestVar()->TsType());
        } else {
            // #22952: infer optional parameter heuristics
            tp = checker->GetNonNullishType(signature->Params()[index]->TsType());
        }

        arguments[index]->SetPreferredType(tp);
        arguments[index]->Check(checker);
    }
}

checker::Type *ETSAnalyzer::Check(ir::ETSNewClassInstanceExpression *expr) const
{
    if (expr->TsType() != nullptr) {
        return expr->TsType();
    }

    ETSChecker *checker = GetETSChecker();

    expr->GetTypeRef()->Check(checker);
    auto *type = expr->GetTypeRef()->TsType();

    if (type != nullptr && type->IsETSArrayType()) {
        if (expr->GetArguments().empty()) {
            checker->LogError(diagnostic::MISSING_ARRAY_SIZE, {type->ToString()}, expr->Start());
            return expr->SetTsType(checker->GlobalTypeError());
        }
        if (expr->GetArguments().size() > 1) {
            auto *arg = expr->GetArguments()[1];
            if (!arg->IsArrowFunctionExpression()) {
                arg->SetPreferredType(type->AsETSArrayType()->ElementType());
            }
        }
        for (auto arg : expr->GetArguments()) {
            arg->Check(checker);
        }
        checker->ValidateArrayIndex(expr->GetArguments()[0], true);
        CheckArrayElementType(checker, expr->AsETSNewClassInstanceExpression(), type->AsETSArrayType()->ElementType());
        expr->SetTsType(type);
        checker->CreateBuiltinArraySignature(expr->TsType()->AsETSArrayType(), 1);
        return type;
    }
    auto *calleeType = CheckInstantiatedNewType(checker, expr);
    FORWARD_TYPE_ERROR(checker, calleeType, expr);

    auto calleeObj = calleeType->AsETSObjectType();
    expr->SetTsType(calleeType);

    if (calleeType->IsETSResizableArrayType() && expr->GetArguments().size() == 2U &&
        expr->GetArguments()[1]->IsArrowFunctionExpression()) {
        expr->GetArguments()[1]->SetPreferredType(calleeType->AsETSResizableArrayType()->ElementType());
        if (!checker->ValidateResizableArrayDimension(expr->GetArguments()[0])) {
            return checker->InvalidateType(expr);
        }
    }

    auto *signature = checker->ResolveConstructExpression(calleeObj, expr);

    if (signature == nullptr) {
        return checker->InvalidateType(expr);
    }

    CheckObjectLiteralArguments(checker, signature, expr->GetArguments());

    checker->ValidateSignatureAccessibility(calleeObj, signature, expr->Start());

    expr->SetSignature(signature);

    return expr->TsType();
}

checker::Type *ETSAnalyzer::Check(ir::ETSNewMultiDimArrayInstanceExpression *expr) const
{
    if (expr->TsType() != nullptr) {
        return expr->TsType();
    }
    ETSChecker *checker = GetETSChecker();

    auto *elementType = expr->TypeReference()->GetType(checker);
    CheckArrayElementType(checker, expr->AsETSNewMultiDimArrayInstanceExpression(), elementType);

    auto *fixedArrayType = elementType;
    std::size_t dimensionIndex = 1U;
    for (auto *dim : expr->Dimensions()) {
        checker->ValidateResizableArrayDimension(dim, dimensionIndex);
        ++dimensionIndex;
        fixedArrayType = checker->CreateETSArrayType(fixedArrayType, false);
    }
    expr->SetTsType(checker->CreateETSMultiDimResizableArrayType(elementType, expr->Dimensions().size()));
    if (expr->TsType()->IsETSArrayType()) {
        expr->SetSignature(
            checker->CreateBuiltinArraySignature(expr->TsType()->AsETSArrayType(), expr->Dimensions().size()));
    }

    return expr->TsType();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::ETSPackageDeclaration *st) const
{
    return ReturnTypeForStatement(st);
}

checker::Type *ETSAnalyzer::Check(ir::ETSParameterExpression *expr) const
{
    ETSChecker *checker = GetETSChecker();
    if (expr->TsType() != nullptr) {
        return expr->TsType();
    }
    ASSERT_PRINT(expr->Initializer() == nullptr, "default parameter was not lowered");

    if (expr->Ident()->TsType() != nullptr) {
        expr->SetTsType(expr->Ident()->TsType());
    } else if (expr->IsRestParameter()) {
        expr->SetTsType(expr->RestParameter()->Check(checker));
    } else {
        expr->SetTsType(expr->Ident()->Check(checker));
    }
    ES2PANDA_ASSERT(!expr->IsOptional() ||
                    checker->Relation()->IsSupertypeOf(expr->TsType(), checker->GlobalETSUndefinedType()));
    return expr->TsType();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::ETSPrimitiveType *node) const
{
    ETSChecker *checker = GetETSChecker();
    return node->GetType(checker);
}

checker::Type *ETSAnalyzer::Check(ir::ETSStructDeclaration *node) const
{
    ETSChecker *checker = GetETSChecker();
    node->Definition()->Check(checker);
    return ReturnTypeForStatement(node);
}

checker::Type *ETSAnalyzer::Check(ir::ETSTypeReference *node) const
{
    ETSChecker *checker = GetETSChecker();
    checker->CheckAnnotations(node);
    return node->GetType(checker);
}

checker::Type *ETSAnalyzer::Check(ir::ETSTypeReferencePart *node) const
{
    ETSChecker *checker = GetETSChecker();
    return node->GetType(checker);
}

checker::Type *ETSAnalyzer::Check(ir::ETSNonNullishTypeNode *node) const
{
    if (node->TsType() != nullptr) {
        return node->TsType();
    }
    ETSChecker *checker = GetETSChecker();
    return node->SetTsType(checker->GetNonNullishType(node->GetTypeNode()->Check(checker)));
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::ETSNullType *node) const
{
    ETSChecker *checker = GetETSChecker();
    checker->CheckAnnotations(node);
    return node->SetTsType(checker->GlobalETSNullType());
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::ETSUndefinedType *node) const
{
    ETSChecker *checker = GetETSChecker();
    checker->CheckAnnotations(node);
    return node->SetTsType(checker->GlobalETSUndefinedType());
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::ETSNeverType *node) const
{
    ETSChecker *checker = GetETSChecker();
    return checker->GlobalETSNeverType();
}

checker::Type *ETSAnalyzer::Check(ir::ETSStringLiteralType *node) const
{
    ETSChecker *checker = GetETSChecker();
    checker->CheckAnnotations(node);
    return node->GetType(checker);
}

checker::Type *ETSAnalyzer::Check(ir::ETSKeyofType *node) const
{
    ETSChecker *checker = GetETSChecker();
    return node->GetType(checker);
}

// compile methods for EXPRESSIONS in alphabetical order

static void AddSpreadElementTypes(ETSChecker *checker, ir::SpreadElement *const element,
                                  std::vector<std::pair<Type *, ir::Expression *>> &elementTypes)
{
    Type *const spreadType = element->Check(checker);

    if (spreadType->IsTypeError()) {
        // error recovery
        return;
    }

    Type *const spreadArgumentType = element->Argument()->TsType();

    if (spreadArgumentType->IsETSTupleType()) {
        for (Type *type : spreadArgumentType->AsETSTupleType()->GetTupleTypesList()) {
            elementTypes.emplace_back(type, element);
        }
    } else {
        elementTypes.emplace_back(checker->GetElementTypeOfSpreadType(spreadArgumentType), element);
    }
}

static bool ValidArrayExprSizeForTupleSize(ETSChecker *checker, Type *possibleTupleType,
                                           ir::Expression *possibleArrayExpr)
{
    if (!possibleArrayExpr->IsArrayExpression() || !possibleTupleType->IsETSTupleType()) {
        return true;
    }

    return checker->IsArrayExprSizeValidForTuple(possibleArrayExpr->AsArrayExpression(),
                                                 possibleTupleType->AsETSTupleType());
}

static std::vector<std::pair<Type *, ir::Expression *>> GetElementTypes(ETSChecker *checker, ir::ArrayExpression *expr)
{
    std::vector<std::pair<Type *, ir::Expression *>> elementTypes {};

    auto *const exprPreferredType = expr->PreferredType();
    auto *const exprTupleType = exprPreferredType->IsETSTupleType() ? exprPreferredType->AsETSTupleType() : nullptr;
    checker::Type *elemPreferredType =
        exprPreferredType->IsETSTupleType() ? nullptr : checker->GetElementTypeOfArray(exprPreferredType);

    for (std::size_t idx = 0U; idx < expr->Elements().size(); ++idx) {
        ir::Expression *const element = expr->Elements()[idx];

        if (element->IsSpreadElement()) {
            element->SetPreferredType(exprPreferredType);
            AddSpreadElementTypes(checker, element->AsSpreadElement(), elementTypes);
            continue;
        }

        if (exprTupleType != nullptr) {
            if (idx >= exprTupleType->GetTupleSize() ||
                !ValidArrayExprSizeForTupleSize(checker, exprTupleType->GetTypeAtIndex(idx), element)) {
                elementTypes.emplace_back(element->SetTsType(checker->GlobalTypeError()), element);
                continue;
            }
            elemPreferredType = exprTupleType->GetTypeAtIndex(idx);
        }

        element->SetPreferredType(elemPreferredType);
        elementTypes.emplace_back(element->Check(checker), element);
    }

    return elementTypes;
}

static Type *GetArrayElementType(ETSChecker *checker, Type *preferredType)
{
    if (preferredType->IsETSArrayType()) {
        return checker->GetNonConstantType(checker->GetElementTypeOfArray(preferredType));
    }
    ES2PANDA_ASSERT(preferredType->IsETSResizableArrayType());
    return preferredType->AsETSResizableArrayType()->ElementType();
}

static bool CheckElement(ETSChecker *checker, Type *const preferredType,
                         std::vector<std::pair<Type *, ir::Expression *>> arrayExprElementTypes, std::size_t idx)
{
    auto [elementType, currentElement] = arrayExprElementTypes[idx];

    if (elementType->IsTypeError()) {
        return true;
    }

    Type *targetType = nullptr;

    if (preferredType->IsETSTupleType()) {
        const auto *const tupleType = preferredType->AsETSTupleType();
        if (tupleType->GetTupleSize() != arrayExprElementTypes.size()) {
            return false;
        }

        auto *const compareType = tupleType->GetTypeAtIndex(idx);
        if (compareType == nullptr) {
            checker->LogError(diagnostic::TUPLE_SIZE_MISMATCH, {tupleType->GetTupleSize()}, currentElement->Start());
            return false;
        }

        auto ctx = AssignmentContext(checker->Relation(), currentElement, elementType, compareType,
                                     currentElement->Start(), {{diagnostic::TUPLE_UNASSIGNABLE_ARRAY, {idx}}});
        if (!ctx.IsAssignable()) {
            return false;
        }

        const CastingContext castCtx(checker->Relation(), diagnostic::CAST_FAIL_UNREACHABLE, {},
                                     CastingContext::ConstructorData {currentElement, compareType, compareType,
                                                                      currentElement->Start(), TypeRelationFlag::NONE});

        targetType = compareType;
    } else {
        targetType = GetArrayElementType(checker, preferredType);
    }

    auto ctx = AssignmentContext(checker->Relation(), currentElement, elementType, targetType, currentElement->Start(),
                                 {{diagnostic::ARRAY_ELEMENT_INIT_TYPE_INCOMPAT, {idx, elementType, targetType}}});
    return ctx.IsAssignable();
}

static Type *InferPreferredTypeFromElements(ETSChecker *checker, ir::ArrayExpression *arrayExpr)
{
    std::vector<Type *> arrayExpressionElementTypes;
    for (auto *const element : arrayExpr->Elements()) {
        element->RemoveAstNodeFlags(ir::AstNodeFlags::GENERATE_VALUE_OF);
        auto *elementType = *element->Check(checker);
        if (element->IsSpreadElement() && elementType->IsETSTupleType()) {
            for (auto *typeFromTuple : elementType->AsETSTupleType()->GetTupleTypesList()) {
                arrayExpressionElementTypes.emplace_back(typeFromTuple);
            }

            continue;
        }

        if (element->IsSpreadElement()) {
            elementType = checker->GetElementTypeOfSpreadType(elementType);
        }

        arrayExpressionElementTypes.emplace_back(elementType);
    }

    // NOTE (smartin): optimize element access on constant array expressions (note is here, because the constant value
    // will be present on the type)
    auto *un = checker->CreateETSUnionType(std::move(arrayExpressionElementTypes));
    return checker->CreateETSResizableArrayType(
        checker->GetNonConstantType(un->IsETSUnionType() ? un->AsETSUnionType()->NormalizedType() : un));
}

static bool CheckArrayExpressionElements(ETSChecker *checker, ir::ArrayExpression *arrayExpr)
{
    const std::vector<std::pair<Type *, ir::Expression *>> arrayExprElementTypes = GetElementTypes(checker, arrayExpr);

    bool allElementsAssignable = !std::any_of(arrayExprElementTypes.begin(), arrayExprElementTypes.end(),
                                              [](auto &pair) { return pair.first->IsTypeError(); });

    for (std::size_t idx = 0; idx < arrayExprElementTypes.size(); ++idx) {
        allElementsAssignable &= CheckElement(checker, arrayExpr->PreferredType(), arrayExprElementTypes, idx);
    }

    return allElementsAssignable;
}

static Type *GetAppropriatePreferredType(Type *originalType, std::function<bool(Type *)> const &predicate)
{
    if (originalType == nullptr) {
        return nullptr;
    }

    while (originalType->IsETSTypeAliasType()) {
        if (predicate(originalType)) {
            return originalType;
        }
        auto *targetType = originalType->AsETSTypeAliasType()->GetTargetType();
        if (targetType == nullptr) {
            return nullptr;
        }
        originalType = targetType;
    }

    if (originalType->IsETSTypeParameter()) {
        return GetAppropriatePreferredType(originalType->AsETSTypeParameter()->GetConstraintType(), predicate);
    }

    if (predicate(originalType)) {
        return originalType;
    }

    if (!originalType->IsETSUnionType()) {
        return nullptr;
    }

    Type *preferredType = nullptr;
    for (Type *type : originalType->AsETSUnionType()->ConstituentTypes()) {
        while (type->IsETSTypeAliasType()) {
            auto *targetType = type->AsETSTypeAliasType()->GetTargetType();
            if (targetType == nullptr) {
                type = nullptr;
                break;
            }
            type = targetType;
        }
        if (type != nullptr && predicate(type)) {
            if (preferredType != nullptr) {
                return nullptr;  // ambiguity
            }
            preferredType = type;
        }
    }
    return preferredType;
}

static inline checker::Type *CheckElemUnder(checker::ETSChecker *checker, ir::Expression *node,
                                            checker::Type *preferElem)
{
    auto *oldPref = node->PreferredType();
    node->SetPreferredType(preferElem);
    checker::Type *t = node->Check(checker);
    node->SetPreferredType(oldPref);
    return t;
}

static bool CheckCandidateCompatibility(ETSChecker *checker, ir::ArrayExpression *arrayLiteral, Type *candElem)
{
    return std::all_of(arrayLiteral->Elements().begin(), arrayLiteral->Elements().end(), [=](auto *el) {
        Type *elTy = CheckElemUnder(checker, el, candElem);
        if (elTy == nullptr || elTy->IsTypeError()) {
            return false;
        }
        AssignmentContext ctx(checker->Relation(), el, elTy, candElem, arrayLiteral->Start(), std::nullopt,
                              TypeRelationFlag::NONE);
        return ctx.IsAssignable();
    });
}

static bool CheckElementTypeAssignabilityToTuple(ETSChecker *checker, ETSTupleType *tupleType,
                                                 ir::ArrayExpression *arrayExpr)
{
    if (!ValidArrayExprSizeForTupleSize(checker, tupleType, arrayExpr)) {
        return false;
    }

    for (size_t i = 0; i < arrayExpr->Elements().size(); ++i) {
        ir::Expression *element = arrayExpr->Elements()[i];
        element->RemoveAstNodeFlags(ir::AstNodeFlags::GENERATE_VALUE_OF);
        auto *elementType = *element->Check(checker);
        auto *targetType = tupleType->GetTypeAtIndex(i);
        if (const auto ctx = AssignmentContext(checker->Relation(), element, elementType, targetType,
                                               arrayExpr->Start(), std::nullopt, TypeRelationFlag::NONE);
            !ctx.IsAssignable()) {
            return false;
        }
    }

    return true;
}

static Type *ValidatePreferredTypeForArrayLiteral(ETSChecker *checker, ir::ArrayExpression *arrayLiteral,
                                                  Type *candidate)
{
    ES2PANDA_ASSERT(candidate->IsETSArrayType() || candidate->IsETSResizableArrayType() || candidate->IsETSTupleType());
    InferMatchContext specificTypeMatchCtx(checker, util::DiagnosticType::SEMANTIC, arrayLiteral->Range(), false);
    bool valid = true;
    if (candidate->IsETSArrayType() || candidate->IsETSResizableArrayType()) {
        Type *candidateElem = checker->GetElementTypeOfArray(candidate);
        valid = CheckCandidateCompatibility(checker, arrayLiteral, candidateElem);
    }

    if (candidate->IsETSTupleType()) {
        valid = CheckElementTypeAssignabilityToTuple(checker, candidate->AsETSTupleType(), arrayLiteral);
    }
    valid &= specificTypeMatchCtx.ValidMatchStatus();
    arrayLiteral->CleanCheckInformation();
    return valid ? candidate : nullptr;
}

static Type *SelectPreferredTypeForLiteral(ETSChecker *checker, ir::ArrayExpression *arrayLiteral,
                                           ETSUnionType *contextualType)
{
    for (auto *el : arrayLiteral->Elements()) {
        if (el->IsSpreadElement() || el->IsBrokenExpression()) {
            return nullptr;
        }
    }

    auto &alts = contextualType->ConstituentTypes();
    std::vector<Type *> matchedCandidates;
    checker->AddStatus(checker::CheckerStatus::IN_TYPE_INFER);
    for (Type *candidate : alts) {
        if (!candidate->IsETSArrayType() && !candidate->IsETSResizableArrayType() && !candidate->IsETSTupleType()) {
            continue;
        }

        if (auto select = ValidatePreferredTypeForArrayLiteral(checker, arrayLiteral, candidate); select != nullptr) {
            matchedCandidates.emplace_back(select);
        }
    }
    checker->RemoveStatus(checker::CheckerStatus::IN_TYPE_INFER);
    if (matchedCandidates.empty()) {
        return nullptr;
    }

    if (matchedCandidates.size() != 1) {
        checker->LogError(diagnostic::AMBIGUOUS_ARRAY_LITERAL_TYPE, {matchedCandidates[0], matchedCandidates[1]},
                          arrayLiteral->Start());
        return nullptr;
    }

    // get result and do final check.
    auto res = matchedCandidates.front();
    if (res->IsETSTupleType()) {
        CheckElementTypeAssignabilityToTuple(checker, res->AsETSTupleType(), arrayLiteral);
        return res;
    }

    Type *resElem = checker->GetElementTypeOfArray(res);
    CheckCandidateCompatibility(checker, arrayLiteral, resElem);
    return res;
}

static Type *GetPreferredTypeFromArraySupertypes(ETSChecker *checker, Type *originalType)
{
    if (originalType == nullptr || !originalType->IsETSObjectType() ||
        originalType->AsETSObjectType()->TypeArguments().empty()) {
        return nullptr;
    }
    auto *arrayType = checker->CreateETSResizableArrayType(originalType->AsETSObjectType()->TypeArguments().front());
    if (checker->Relation()->IsSupertypeOf(originalType->AsETSObjectType(), arrayType)) {
        return arrayType;
    }
    return nullptr;
}

static bool IsInitForSyntheticVariable(const ir::ArrayExpression *expr)
{
    bool hasSyntheticId = false;
    if (expr->Parent() != nullptr && expr->Parent()->IsVariableDeclarator()) {
        const auto *identifier = expr->Parent()->AsVariableDeclarator()->Id();
        if (identifier != nullptr) {
            hasSyntheticId = compiler::IsSyntheticIdentifier(identifier);
        }
    }
    return hasSyntheticId;
}

static Type *ExtractArrayPreferredType(ETSChecker *checker, ir::ArrayExpression *expr)
{
    Type *preferredType = GetPreferredTypeFromArraySupertypes(checker, expr->PreferredType());

    if (preferredType == nullptr) {
        preferredType = GetAppropriatePreferredType(expr->PreferredType(), &Type::IsAnyETSArrayOrTupleType);
    }

    if (expr->PreferredType() != nullptr && expr->PreferredType()->IsETSUnionType()) {
        if (auto *picked = SelectPreferredTypeForLiteral(checker, expr, expr->PreferredType()->AsETSUnionType())) {
            preferredType = picked;
            expr->SetPreferredType(preferredType);
        }
    }

    if (preferredType != nullptr && preferredType->IsETSReadonlyArrayType()) {
        const auto elementType = preferredType->AsETSObjectType()->TypeArguments().front();
        preferredType = checker->CreateETSResizableArrayType(elementType);
    }

    return preferredType;
}

static Type *ResolvePreferredTypeForArrayLiteral(ETSChecker *checker, ir::ArrayExpression *expr)
{
    Type *preferredType = ExtractArrayPreferredType(checker, expr);

    if (!IsArrayExpressionValidInitializerForType(checker, preferredType)) {
        checker->LogError(diagnostic::UNEXPECTED_ARRAY, {expr->PreferredType()}, expr->Start());
        return checker->InvalidateType(expr);
    }

    if (!expr->Elements().empty()) {
        if (preferredType == nullptr ||
            checker->Relation()->IsSupertypeOf(preferredType, checker->GlobalETSObjectType()) ||
            util::Helpers::TypeContainsParameterUnderInference(preferredType)) {
            preferredType = InferPreferredTypeFromElements(checker, expr);
        }

        expr->SetPreferredType(preferredType);
    }

    if (preferredType == nullptr) {
        return checker->TypeError(expr, diagnostic::UNRESOLVABLE_ARRAY, expr->Start());
    }

    if (preferredType->IsETSArrayType()) {
        auto *elementType = preferredType->AsETSArrayType()->ElementType();
        if (HasBareTypeParameter(elementType) && !IsInitForSyntheticVariable(expr)) {
            return checker->TypeError(expr, diagnostic::TYPE_PARAMETER_AS_ARRAY_ELEMENT_TYPE, expr->Start());
        }
        // Skip if the array literal is inside a call argument chain, where the
        // preferred type is cascaded from the parameter type rather than being
        // an explicit user annotation.  Stop at function boundaries.
        bool isInsideCallArg = false;
        for (auto *p = expr->Parent(); p != nullptr && !p->IsScriptFunction() && !p->IsArrowFunctionExpression();
             p = p->Parent()) {
            if (p->IsCallExpression()) {
                isInsideCallArg = true;
                break;
            }
        }
        if (IsTypeNotPreservedByErasure(elementType) && !IsInitForSyntheticVariable(expr) && !isInsideCallArg) {
            return checker->TypeError(expr, diagnostic::TYPE_NOT_PRESERVED_BY_ERASURE_FOR_FIXED_ARRAY,
                                      {elementType->ToString()}, expr->Start());
        }
    }

    return preferredType;
}

checker::Type *ETSAnalyzer::Check(ir::ArrayExpression *expr) const
{
    ETSChecker *checker = GetETSChecker();
    if (expr->TsType() != nullptr) {
        return expr->TsType();
    }

    Type *preferredType = ResolvePreferredTypeForArrayLiteral(checker, expr);
    if (preferredType->IsTypeError()) {
        return preferredType;
    }

    if (!ValidArrayExprSizeForTupleSize(checker, preferredType, expr) ||
        (!expr->Elements().empty() && !CheckArrayExpressionElements(checker, expr))) {
        return checker->InvalidateType(expr);
    }

    expr->SetTsType(preferredType);
    if (!preferredType->IsETSResizableArrayType() && !preferredType->IsETSTupleType()) {
        ES2PANDA_ASSERT(preferredType->IsETSArrayType());
        const auto *const arrayType = preferredType->AsETSArrayType();
        checker->CreateBuiltinArraySignature(arrayType, arrayType->Rank());
    }
    return expr->TsType();
}

static bool IsReturnTypeBoxedPrimitive(checker::ETSChecker *checker, Type *tp, bool isAsyncFunc)
{
    if (isAsyncFunc && tp->IsETSObjectType()) {
        ES2PANDA_ASSERT(checker->IsPromiseType(tp));
        // Unwrap Promise for the async functions
        tp = checker->PromiseTypeArg(tp->AsETSObjectType());
    }
    return tp->IsETSObjectType() && tp->AsETSObjectType()->IsBoxedPrimitive();
}

static Type *DoReturnTypeInference(checker::ETSChecker *checker, ir::ScriptFunction *func)
{
    ES2PANDA_ASSERT(func->ReturnTypeAnnotation() == nullptr && func->HasBody());

    const auto isAsyncFunc = func->IsAsyncFunc();

    /*
     * 15.7.2: If a lambda body contains no return statement but all execution paths leads to a throw statement
     * execution, then the lambda return type is never;
     * 15.7.2: If a function, a method, or a lambda is async, a return type is inferred by applying the above rules,
     * and the return type T is not Promise, then the return type is assumed to be Promise<T>;
     */
    if (!func->HasReturnStatement() && func->HasThrowStatement() && checker->HasStatus(CheckerStatus::MEET_THROW)) {
        return isAsyncFunc ? checker->CreatePromiseOf(checker->GlobalETSNeverType()) : checker->GlobalETSNeverType();
    }

    std::vector<Type *> returnTypes {};
    std::function<void(ir::AstNode *)> retCheck = [&](ir::AstNode *ast) {
        if (ast->IsScriptFunction()) {
            return;
        }
        ast->Iterate(retCheck);

        if (!ast->IsReturnStatement()) {
            return;
        }
        auto *ret = ast->AsReturnStatement();

        /*
         * 15.7.2: If at least one of return statements has no expression, then type undefined is added to the return
         * type union;
         */
        if (ret->Argument() == nullptr) {
            returnTypes.push_back(checker->GlobalETSUndefinedType());
            return;
        }

        /*
         * Account for possible implicit conversions;
         * For async functions, unwrap the promise type to try and match numerics, since:
         * 16.3.1: An asynchronous function with the return type Promise<T> can explicitly return a Promise<T> instance
         * (in this case, the returned value is returned “as is”) or a value of type T;
         */
        auto *expectedPrimitive = GetAppropriatePreferredType(
            ret->ReturnType(), [&](Type *tp) { return IsReturnTypeBoxedPrimitive(checker, tp, isAsyncFunc); });

        // NOTE(gogabr): `IsLegalBoxedPrimitiveConversion` does not work when these flags are not set;
        // Subject to further refactoring;
        SavedTypeRelationFlagsContext trCtx(checker->Relation(), TypeRelationFlag::IN_ASSIGNMENT_CONTEXT);
        checker->Relation()->SetNode(ret->Argument());

        const auto argType = ret->Argument()->Check(checker);
        ES2PANDA_ASSERT(argType);

        if (expectedPrimitive != nullptr &&
            checker->Relation()->IsLegalBoxedPrimitiveConversion(expectedPrimitive, argType)) {
            returnTypes.push_back(expectedPrimitive);
        } else {
            const auto argTypeNonConst = checker->GetNonConstantType(argType);
            ES2PANDA_ASSERT(argTypeNonConst);
            returnTypes.push_back(argTypeNonConst);
        }
    };
    func->Iterate(retCheck);

    Type *inferredReturnType = nullptr;

    if (returnTypes.empty()) {
        /*
         * 15.7.2: If there is no return statement, or if all return statements have no expressions, then the return
         * type is void;
         */
        ES2PANDA_ASSERT(!func->HasThrowStatement() || !checker->HasStatus(CheckerStatus::MEET_THROW));
        inferredReturnType = checker->GlobalETSUndefinedType();
    } else {
        /*
         * 15.7.2: If there are k return statements (where k is 1 or more) with the same type expression R, then R is
         * the return type;
         * 15.7.2: If there are k return statements (where k is 2 or more) with expressions of types T1, ..., Tk, then R
         * is the union type of these types (T1 | ... | Tk), and its normalized version is the return type;
         */
        inferredReturnType = checker->CreateETSUnionType(std::move(returnTypes));
    }

    /*
     * 15.7.2: If a function, a method, or a lambda is async, a return type is inferred by applying the above rules, and
     * the return type T is not Promise, then the return type is assumed to be Promise<T>;
     */
    // NOTE(knazarov): Already implemented as `... and the return type is assumed to be Promise<Awaited<T>>` here, need
    // to consult with the spec before merge;
    inferredReturnType =
        isAsyncFunc ? checker->CreatePromiseOf(checker->UnwrapPromiseType(inferredReturnType)) : inferredReturnType;

    ES2PANDA_ASSERT(!isAsyncFunc ||
                    (inferredReturnType->IsETSObjectType() && checker->IsPromiseType(inferredReturnType)));

    return inferredReturnType;
}

static Type *GetPreferredTypeFromPromiseSupertypes(ETSChecker *checker, Type *originalType)
{
    /*
     * NOTE(knazarov): Emulate first step of intersection - try to match the type by constructing the
     * Promise<T> with the same type argument as originalType and check their compatibility;
     */
    const auto promiseTypeArgCount = checker->GlobalBuiltinPromiseType()->TypeArguments().size();
    if (originalType == nullptr || !originalType->IsETSObjectType() ||
        originalType->AsETSObjectType()->TypeArguments().size() != promiseTypeArgCount) {
        return nullptr;
    }
    const auto promiseType = checker->CreatePromiseOf(originalType->AsETSObjectType()->TypeArguments().front());
    if (!checker->Relation()->IsSupertypeOf(originalType->AsETSObjectType(), promiseType)) {
        return nullptr;
    }
    return promiseType;
}

static Type *ExtractAsyncFunctionPreferredReturnType(ETSChecker *checker, Type *origType)
{
    ES2PANDA_ASSERT(checker);
    ES2PANDA_ASSERT(origType);

    while (origType->IsETSTypeAliasType()) {
        origType = origType->AsETSTypeAliasType()->GetTargetType();
    }

    const auto preferredPromiseType = GetPreferredTypeFromPromiseSupertypes(checker, origType);
    if (preferredPromiseType != nullptr) {
        return preferredPromiseType;
    }

    return GetAppropriatePreferredType(origType, [&](Type *t) { return checker->IsPromiseType(t); });
}

Type *ETSChecker::ResolvePreferredReturnTypeForAsyncFunction(ir::ScriptFunction *expr)
{
    ES2PANDA_ASSERT(expr);
    ES2PANDA_ASSERT(expr->IsAsyncFunc());
    ES2PANDA_ASSERT(expr->GetPreferredReturnType());

    auto preferredType = expr->GetPreferredReturnType();
    if (preferredType->IsTypeError()) {
        return preferredType;
    }

    // if preferredType <: Promise<Any> -- return immediately;
    if (IsPromiseType(preferredType)) {
        return preferredType;
    }

    // Try to first match the whole preferredType to a Promise type;
    Type *preferredReturnType = ExtractAsyncFunctionPreferredReturnType(this, preferredType);
    if (preferredReturnType != nullptr) {
        return preferredReturnType;
    }
    // GetAppropriatePreferredType will fail in case of ambiguity, so we collect all candidates separately here to later
    // join them in a single union;
    if (preferredType->IsETSUnionType()) {
        std::vector<Type *> possibleReturnTypes = {};
        for (const auto &ct : preferredType->AsETSUnionType()->ConstituentTypes()) {
            auto candidate = ExtractAsyncFunctionPreferredReturnType(this, ct);
            if (candidate) {
                possibleReturnTypes.push_back(candidate);
            }
        }
        if (!possibleReturnTypes.empty()) {
            return CreateETSUnionType(std::move(possibleReturnTypes));
        }
    }

    // If no candidates found -- return original preferred type and fail down the line;
    return preferredType;
}

static void CheckArrowFunctionAfterSignatureBuild(checker::ETSChecker *checker, ir::ArrowFunctionExpression *expr)
{
    if (expr->Function()->HasReceiver()) {
        checker->AddStatus(checker::CheckerStatus::IN_EXTENSION_METHOD);
        CheckExtensionMethod(checker, expr->Function(), expr);
    }

    checker->Context().SetContainingSignature(expr->Function()->Signature());

    if (!expr->Function()->HasBody()) {
        return;
    }
    expr->Function()->Body()->Check(checker);

    if (expr->Function()->ReturnTypeAnnotation() != nullptr || expr->Function()->Signature() == nullptr) {
        return;
    }
    ES2PANDA_ASSERT(expr->Function()->ReturnTypeAnnotation() == nullptr && expr->Function()->Signature() != nullptr);

    const auto sig = expr->Function()->Signature();

    // The return type has to be refined in three cases:
    // - if it serves to determine the value of a type parameter under inference.
    // - if a `void` return should be replaced by `never` because all control paths end in `throw`.
    // - if signature for async function does not have Promise as return type.
    const auto isUnderInference = util::Helpers::TypeContainsParameterUnderInference(sig->ReturnType());
    const auto isReturnUndefined = sig->ReturnType()->IsETSUndefinedType();
    const auto isAsyncReturnNonPromise =
        expr->Function()->IsAsyncFunc() &&
        !(sig->ReturnType()->IsETSObjectType() && checker->IsPromiseType(sig->ReturnType()));
    if (isUnderInference || isReturnUndefined || isAsyncReturnNonPromise) {
        Type *computedRetType = DoReturnTypeInference(checker, expr->Function());
        sig->SetReturnType(computedRetType);
        for (auto &returnStatement : expr->Function()->ReturnStatements()) {
            returnStatement->SetReturnType(checker, computedRetType);
        }
    }
}

static void TryInferPreferredType(ir::ArrowFunctionExpression *expr, checker::Type *preferredType, ETSChecker *checker)
{
    ES2PANDA_ASSERT(preferredType->IsETSUnionType() || preferredType->IsETSArrowType());

    if (preferredType->IsETSArrowType()) {
        ES2PANDA_ASSERT(preferredType->IsETSFunctionType());
        if (!preferredType->AsETSFunctionType()->CallSignaturesOfMethodOrArrow().empty()) {
            checker->TryInferTypeForLambdaTypeAlias(expr, preferredType->AsETSFunctionType());
            checker->BuildFunctionSignature(expr->Function(), false);
            CheckArrowFunctionAfterSignatureBuild(checker, expr);
        }
        return;
    }

    ES2PANDA_ASSERT(preferredType->IsETSUnionType());
    for (auto &ct : preferredType->AsETSUnionType()->ConstituentTypes()) {
        if (!ct->IsETSArrowType() || ct->AsETSFunctionType()->CallSignaturesOfMethodOrArrow().empty()) {
            continue;
        }
        InferMatchContext specificTypeMatchCtx(checker, util::DiagnosticType::SEMANTIC, expr->Range(), false);
        checker->TryInferTypeForLambdaTypeAlias(expr, ct->AsETSFunctionType());
        checker->BuildFunctionSignature(expr->Function(), false);
        CheckArrowFunctionAfterSignatureBuild(checker, expr);
        if (specificTypeMatchCtx.ValidMatchStatus()) {
            return;
        }
        expr->CleanCheckInformation();
    }

    // Note: no matching preferred type, but the signature still need to be created.
    checker->BuildFunctionSignature(expr->Function(), false);
}

checker::Type *ETSAnalyzer::Check(ir::ArrowFunctionExpression *expr) const
{
    ETSChecker *checker = GetETSChecker();
    checker->CheckAnnotations(expr);
    if (expr->TsType() != nullptr) {
        return expr->TsType();
    }
    checker::ScopeContext scopeCtx(checker, expr->Function()->Scope());

    if (checker->HasStatus(checker::CheckerStatus::IN_EXTENSION_METHOD) && !expr->Function()->HasReceiver()) {
        /*
        example code:
        ```
            class A {
                prop:number
            }
            function method(this: A) {
                let a = () => {
                    console.log(this.prop)
                }
            }
        ```
        here the enclosing class of arrow function should be Class A
        */
        checker->Context().SetContainingClass(
            checker->Scope()->Find(varbinder::VarBinder::MANDATORY_PARAM_THIS).variable->TsType()->AsETSObjectType());
    }

    auto lambdaSavedSmartCasts = checker->Context().CloneSmartCasts();
    checker::SavedCheckerContext savedContext(checker, checker->Context().Status(),
                                              checker->Context().ContainingClass());

    if (expr->Parent()->IsCallExpression() && !expr->Function()->IsAsyncFunc()) {
        checker->Context().RestoreSmartCasts(lambdaSavedSmartCasts);
    }

    checker->AddStatus(checker::CheckerStatus::IN_LAMBDA);
    checker->Context().SetContainingLambda(expr);

    if (expr->PreferredType() != nullptr &&
        (expr->PreferredType()->IsETSArrowType() || expr->PreferredType()->IsETSUnionType())) {
        TryInferPreferredType(expr, expr->PreferredType(), checker);
    } else {
        checker->BuildFunctionSignature(expr->Function(), false);
    }

    if (expr->Function()->Signature() == nullptr || util::Helpers::IsErrorPlaceHolder(expr->Function()->Id())) {
        return checker->InvalidateType(expr);
    }

    CheckArrowFunctionAfterSignatureBuild(checker, expr);
    CheckAsyncFunctionReturnType(checker, expr->Function());
    auto *funcType = checker->CreateETSArrowType(expr->Function()->Signature());
    checker->Context().SetContainingSignature(nullptr);
    return expr->SetTsType(funcType);
}

static bool IsInvalidArrayMemberAssignment(const ir::AssignmentExpression *const expr, ETSChecker *checker)
{
    if (!expr->Left()->IsMemberExpression()) {
        return false;
    }

    const auto *const leftExpr = expr->Left()->AsMemberExpression();
    if (leftExpr->Object()->TsType()->IsETSArrayType() || leftExpr->Object()->TsType()->IsETSTupleType() ||
        leftExpr->Object()->TsType()->IsETSResizableArrayType()) {
        if (leftExpr->Object()->TsType()->IsETSArrayType() && leftExpr->Property()->IsIdentifier() &&
            leftExpr->Property()->AsIdentifier()->Name().Is("length")) {
            checker->LogError(diagnostic::ARRAY_LENGTH_MODIFICATION, {}, expr->Left()->Start());
            return true;
        }

        if (leftExpr->Object()->TsType()->HasTypeFlag(TypeFlag::READONLY)) {
            checker->LogError(diagnostic::READONLY_ARRAYLIKE_MODIFICATION, {}, expr->Left()->Start());
            return true;
        }
    }

    return false;
}

static checker::Type *GetSmartType(ETSChecker *checker, ir::Identifier *ident, varbinder::Variable const *variable,
                                   checker::Type *sourceType, checker::Type *targetType, std::optional<double> value)
{
    checker::Type *smartType = targetType;

    //  Now try to define the actual type of Identifier so that smart cast can be used in further checker processing
    smartType = checker->ResolveSmartType(sourceType, targetType, value);

    //  Add/Remove/Modify smart cast for identifier
    //  (excluding the variables defined at top-level scope or captured in lambda-functions!)
    auto const *const variableScope = variable->GetScope();
    auto const topLevelVariable =
        variableScope != nullptr && (variableScope->IsGlobalScope() ||
                                     (variableScope->Parent() != nullptr && variableScope->Parent()->IsGlobalScope()));
    if (!topLevelVariable) {
        if (checker->Relation()->IsIdenticalTo(targetType, smartType)) {
            checker->Context().RemoveSmartCast(variable);
        } else {
            ident->SetTsType(smartType);
            checker->Context().SetSmartCast(variable, smartType);
        }
    }

    return smartType;
}

checker::Type *ETSAnalyzer::GetSmartTypeForAssignment(ir::AssignmentExpression *const expr,
                                                      checker::Type *const leftType, checker::Type *const rightType,
                                                      ir::Expression *const relationNode) const
{
    auto isLazyImportObject =
        leftType->IsETSObjectType() && leftType->AsETSObjectType()->HasObjectFlag(ETSObjectFlags::LAZY_IMPORT_OBJECT);
    if (leftType->IsTypeError() || isLazyImportObject) {
        return rightType;
    }

    ETSChecker *checker = GetETSChecker();
    if (const auto ctx =
            checker::AssignmentContext(checker->Relation(), relationNode, rightType, leftType, expr->Right()->Start(),
                                       {{diagnostic::INVALID_ASSIGNMNENT, {rightType, leftType}}});
        !ctx.IsAssignable()) {
        return rightType;
    }

    auto *smartType = rightType;
    if (expr->Left()->IsIdentifier() && expr->Target() != nullptr) {
        auto const value = expr->Right()->IsNumberLiteral()
                               ? std::make_optional(expr->Right()->AsNumberLiteral()->Number().GetDouble())
                               : std::nullopt;
        smartType = GetSmartType(checker, expr->Left()->AsIdentifier(), expr->Target(), rightType, leftType, value);
    }
    return smartType;
}

static checker::Type const *ResolveMethodDefinition(const ir::Expression *const expression, ETSChecker *checker)
{
    if (!expression->IsMemberExpression()) {
        return nullptr;
    }

    auto const *memberExpression = expression->AsMemberExpression();
    if (memberExpression->Kind() != ir::MemberExpressionKind::PROPERTY_ACCESS ||
        memberExpression->Property() == nullptr || !memberExpression->Property()->IsIdentifier()) {
        return nullptr;
    }

    auto const *variable = memberExpression->Property()->Variable();
    if (variable == nullptr) {
        if (auto *objectType = const_cast<checker::Type *>(memberExpression->Object()->TsType());
            objectType != nullptr && objectType->IsETSObjectType()) {
            // Process possible case of the same name method with receiver defined
            auto resolved = checker->ResolveMemberReference(memberExpression, objectType->AsETSObjectType());
            if (resolved.size() == 2U && resolved[1]->Kind() == checker::ResolvedKind::PROPERTY) {
                variable = resolved[1U]->Variable()->AsLocalVariable();
            }
        }
    }

    if (variable != nullptr) {
        return variable->TsType();
    }

    return nullptr;
}

static bool IsInvalidMethodAssignment(const ir::AssignmentExpression *const expr, ETSChecker *checker)
{
    auto left = expr->Left();
    if (auto const *methodType = ResolveMethodDefinition(left, checker);
        methodType != nullptr && methodType->IsETSMethodType()) {
        auto const callSigs = methodType->AsETSFunctionType()->CallSignatures();
        for (auto callSig : callSigs) {
            if (callSig->HasSignatureFlag(SignatureFlags::SETTER)) {
                return false;
            }
        }
        checker->LogError(diagnostic::METHOD_ASSIGNMENT, left->Start());
        return true;
    }
    return false;
}

static bool TryExtractNumberFromIdentifier(ir::Expression *expr, lexer::Number &outNum, std::string &outStr)
{
    auto *id = expr->AsIdentifier();
    auto *var = id->Variable();
    if (var == nullptr || var->Declaration() == nullptr || var->Declaration()->Node() == nullptr) {
        return false;
    }

    auto *declNode = var->Declaration()->Node();
    auto *parent = declNode->Parent();
    if (parent->IsClassProperty() || parent->IsVariableDeclarator()) {
        declNode = parent;
    }

    auto getNumberFromLiteral = [&](ir::Expression *literal) -> bool {
        if (literal == nullptr || !literal->IsNumberLiteral()) {
            return false;
        }
        outNum = literal->AsNumberLiteral()->Number();
        outStr = literal->DumpEtsSrc();
        return true;
    };

    if (declNode->IsClassProperty()) {
        auto *prop = declNode->AsClassProperty();
        return prop->IsConst() && getNumberFromLiteral(prop->Value());
    }

    if (declNode->IsVariableDeclarator()) {
        auto *decl = declNode->AsVariableDeclarator();
        bool isConst = decl->Flag() == ir::VariableDeclaratorFlag::CONST;
        return isConst && getNumberFromLiteral(decl->Init());
    }

    return false;
}

static bool TryExtractNumber(ir::Expression *expr, lexer::Number &outNum, std::string &outStr)
{
    if (expr->IsNumberLiteral()) {
        outNum = expr->AsNumberLiteral()->Number();
        outStr = expr->DumpEtsSrc();
        return true;
    }
    if (expr->IsIdentifier()) {
        return TryExtractNumberFromIdentifier(expr, outNum, outStr);
    }
    return false;
}

static bool FitsNumericType(Type *ctype, const lexer::Number &number)
{
    if (!ctype->IsETSObjectType()) {
        return false;
    }

    auto *obj = ctype->AsETSObjectType();

    struct FitCase {
        ETSObjectFlags flag;
        bool isInteger;
        std::function<bool()> canFit;
    };

    const std::vector<FitCase> fitCases = {
        {ETSObjectFlags::BUILTIN_BYTE, true, [&] { return number.CanGetValue<int8_t>(); }},
        {ETSObjectFlags::BUILTIN_SHORT, true, [&] { return number.CanGetValue<int16_t>(); }},
        {ETSObjectFlags::BUILTIN_INT, true, [&] { return number.CanGetValue<int32_t>(); }},
        {ETSObjectFlags::BUILTIN_LONG, true, [&] { return number.CanGetValue<int64_t>(); }},
        {ETSObjectFlags::BUILTIN_FLOAT, false, [&] { return number.CanGetValue<float>(); }},
        {ETSObjectFlags::BUILTIN_DOUBLE, false, [&] { return number.CanGetValue<double>(); }},
    };

    for (const auto &f : fitCases) {
        if (obj->HasObjectFlag(f.flag) && f.canFit() &&
            ((f.isInteger && number.IsInteger()) || (!f.isInteger && number.IsReal()))) {
            return true;
        }
    }

    return false;
}

// In assignment expression or object literal, we need the type of the setter instead of the type of the getter
static checker::Type *GetSetterType(varbinder::Variable *const var, ETSChecker *checker)
{
    if (var == nullptr || !checker->IsVariableGetterSetter(var)) {
        return nullptr;
    }

    if (var->TsType()->IsETSFunctionType()) {
        auto *funcType = var->TsType()->AsETSFunctionType();
        if (funcType->HasTypeFlag(checker::TypeFlag::SETTER)) {
            auto *setter = funcType->FindSetter();
            ES2PANDA_ASSERT(setter != nullptr && setter->Params().size() == 1);
            return setter->Params()[0]->TsType();
        }
    }

    return nullptr;
}

// Helper to set the target of assignment expression
bool ETSAnalyzer::SetAssignmentExpressionTarget(ir::AssignmentExpression *const expr, ETSChecker *checker) const
{
    if (expr->Left()->IsIdentifier()) {
        expr->target_ = expr->Left()->AsIdentifier()->Variable();
    } else if (expr->Left()->IsMemberExpression()) {
        if (!expr->IsIgnoreConstAssign() &&
            expr->Left()->AsMemberExpression()->Object()->TsType()->HasTypeFlag(TypeFlag::READONLY)) {
            checker->LogError(diagnostic::READONLY_PROPERTY_REASSIGN, {}, expr->Left()->Start());
        }
        expr->target_ = expr->Left()->AsMemberExpression()->PropVar();
    } else {
        return false;
    }
    return true;
}

static void IsAmbiguousUnionInit(checker::ETSUnionType *unionType, ir::Expression *initExpr, ETSChecker *checker)
{
    // A folded `as` expression already carries the explicit target type; the
    // ambiguity rule applies only to untyped integer literals.
    if (initExpr->IsNumberLiteral() && initExpr->AsNumberLiteral()->IsFolded() && initExpr->TsType() != nullptr) {
        return;
    }

    lexer::Number initNumber;
    std::string str;
    // CC-OFFNXT(G.FMT.17-CPP) false positive
    if (!TryExtractNumber(initExpr, initNumber, str)) {
        return;
    }
    // Per spec examples, real literals without explicit narrowing are inferred as 'double'
    // in union contexts (e.g. float|double <- 3.14), so ambiguity check applies to integer literals only.
    if (initNumber.IsReal()) {
        return;
    }
    lexer::Number number {initNumber.GetValueAndCastTo<int64_t>()};
    int fits = std::count_if(unionType->ConstituentTypes().begin(), unionType->ConstituentTypes().end(),
                             [&](Type *t) { return FitsNumericType(t, number); });
    if (fits > 1) {
        checker->LogError(diagnostic::AMBIGUOUS_UNION_VALUE, {str}, initExpr->Start());
    }
}

checker::Type *ETSAnalyzer::Check(ir::ETSDestructuring *const expr) const
{
    ETSChecker *checker = GetETSChecker();

    std::vector<checker::Type *> tupleTypeList;

    for (auto *elem : expr->Elements()) {
        if (elem->IsOmittedExpression()) {
            continue;
        }
        if (elem->IsRestElement()) {
            checker->LogError(diagnostic::REST_UNSUPPORTED_IN_DESTRUCTURING, {}, elem->Start());
            continue;
        }
        if (elem->IsAssignmentPattern()) {
            checker->LogError(diagnostic::DEFAULT_UNSUPPORTED_IN_DESTRUCTURING, {}, elem->Start());
            continue;
        }
        elem->Check(checker);
        tupleTypeList.emplace_back(elem->TsType());
    }

    return expr->SetTsType(checker->CreateETSTupleType(std::move(tupleTypeList), false));
}

static checker::Type *CheckETSArrayOrTupleTypeInDestructuring(ETSChecker *checker, ir::ETSDestructuring *dstrNode,
                                                              ir::Expression *initializer)
{
    bool isVarDecl = dstrNode->Parent()->IsVariableDeclarator();
    auto initType = initializer->Check(checker);
    if (initType->IsAnyETSArrayOrTupleType()) {
        bool isTuple = initType->IsETSTupleType();
        auto *initArrayElementType = !isTuple ? GetArrayElementType(checker, initType) : nullptr;
        if (isTuple && initType->AsETSTupleType()->GetTupleSize() < dstrNode->Size()) {
            checker->LogError(diagnostic::INVALID_DESTRUCTURING_INIT_SIZE,
                              {initType->AsETSTupleType()->GetTupleSize(), dstrNode->Size()}, initializer->Start());
            return initType;
        }

        for (uint32_t idx = 0; idx < dstrNode->Size(); idx++) {
            auto *initElementType = isTuple ? initType->AsETSTupleType()->GetTypeAtIndex(idx) : initArrayElementType;
            auto *dstrElement = dstrNode->GetExpressionAtIndex(idx);
            auto *targetType = dstrElement->TsType();
            ES2PANDA_ASSERT(initElementType != nullptr);

            if (dstrElement->IsOmittedExpression() || dstrElement->IsRestElement() ||
                dstrElement->IsAssignmentPattern()) {
                continue;
            }

            if (isVarDecl) {
                dstrElement->SetTsType(initElementType);
                continue;
            }

            if (!checker->Relation()->IsAssignableTo(initElementType, targetType)) {
                checker->LogError(diagnostic::INVALID_ASSIGNMNENT, {initElementType, targetType}, dstrElement->Start());
            } else if (dstrElement->IsIdentifier() && dstrElement->Variable() != nullptr) {
                //  Now try to define the actual type of target so that smart cast can be used in further processing
                GetSmartType(checker, dstrElement->AsIdentifier(), dstrElement->Variable(), initElementType, targetType,
                             std::nullopt);
            }
        }
    } else {
        checker->LogError(diagnostic::INVALID_DESTRUCTURING_TARGET, {}, initializer->Start());
    }

    return initType;
}

checker::Type *CheckDestructuringExpression(ETSChecker *checker, ir::ETSDestructuring *dstrNode,
                                            ir::Expression *initializer)
{
    if (initializer == nullptr) {
        return checker->GlobalTypeError();
    }

    bool isVarDecl = dstrNode->Parent()->IsVariableDeclarator();
    if (initializer->IsArrayExpression()) {
        std::vector<Type *> tupleTypeList;
        auto arrayElements = initializer->AsArrayExpression()->Elements();
        for (uint32_t idx = 0; idx < dstrNode->Size() && idx < arrayElements.size(); idx++) {
            auto *arrayElement = arrayElements.at(idx);
            auto *dstrElement = dstrNode->GetExpressionAtIndex(idx);

            // NOTE(mozgovoykirill): not supported nested destructuring #275
            if (dstrElement->IsArrayPattern()) {
                checker->LogError(diagnostic::NOT_IMPLEMENTED, {}, dstrElement->Start());
            }

            if (dstrElement->IsOmittedExpression() || dstrElement->IsRestElement() ||
                dstrElement->IsAssignmentPattern()) {
                tupleTypeList.emplace_back(arrayElements.at(idx)->Check(checker));
                continue;
            }

            if (isVarDecl) {
                auto initElementType = arrayElement->Check(checker);
                dstrElement->SetTsType(initElementType);
                tupleTypeList.emplace_back(initElementType);
                continue;
            }

            auto dstrType = dstrElement->TsType();
            checker->SetPreferredTypeForExpression(arrayElement, nullptr, arrayElement, dstrType);
            auto initElementType = arrayElement->Check(checker);
            if (checker->Relation()->IsAssignableTo(initElementType, dstrType)) {
                tupleTypeList.emplace_back(initElementType);
            } else {
                checker->LogError(diagnostic::INVALID_ASSIGNMNENT, {initElementType, dstrType}, arrayElement->Start());
                tupleTypeList.emplace_back(checker->GlobalTypeError());
            }
        }

        if (arrayElements.size() < dstrNode->Size()) {
            checker->LogError(diagnostic::INVALID_DESTRUCTURING_INIT_SIZE, {arrayElements.size(), dstrNode->Size()},
                              initializer->Start());
        } else {
            for (uint32_t idx = dstrNode->Size(); idx < arrayElements.size(); idx++) {
                tupleTypeList.emplace_back(arrayElements.at(idx)->Check(checker));
            }
        }

        return initializer->SetTsType(checker->CreateETSTupleType(std::move(tupleTypeList), false));
    }

    return CheckETSArrayOrTupleTypeInDestructuring(checker, dstrNode, initializer);
}

checker::Type *ETSAnalyzer::Check(ir::AssignmentExpression *const expr) const
{
    if (expr->TsType() != nullptr) {
        return expr->TsType();
    }

    ETSChecker *checker = GetETSChecker();

    if (checker->HasStatus(CheckerStatus::IN_SETTER) && expr->Left()->IsMemberExpression()) {
        checker->WarnForEndlessLoopInGetterSetter(expr->Left()->AsMemberExpression());
    }

    checker::Type *leftType = expr->Left()->Check(checker);

    if (expr->Left()->IsETSDestructuring()) {
        return expr->SetTsType(
            CheckDestructuringExpression(checker, expr->Left()->AsETSDestructuring(), expr->Right()));
    }

    if (IsInvalidArrayMemberAssignment(expr, checker) || IsInvalidMethodAssignment(expr, checker)) {
        expr->SetTsType(checker->GlobalTypeError());
        return expr->TsType();
    }

    if (!SetAssignmentExpressionTarget(expr, checker)) {
        checker->LogError(diagnostic::ASSIGNMENT_INVALID_LHS, {}, expr->Left()->Start());
        expr->SetTsType(checker->GlobalTypeError());
        return expr->TsType();
    }

    if (expr->target_ != nullptr && !expr->IsIgnoreConstAssign()) {
        checker->ValidateUnaryOperatorOperand(expr->target_, expr);
    }

    checker->InferLambdaInAssignmentExpression(expr);

    if (auto setterType = GetSetterType(expr->target_, checker); setterType != nullptr) {
        leftType = setterType;
        expr->Left()->SetTsType(leftType);
    }

    auto [rightType, relationNode] = CheckAssignmentExprOperatorType(expr, leftType);
    if (rightType->IsTypeError()) {
        return expr->SetTsType(leftType);
    }

    if (leftType->IsETSUnionType()) {
        IsAmbiguousUnionInit(leftType->AsETSUnionType(), expr->Right(), checker);
    }

    return expr->SetTsType(GetSmartTypeForAssignment(expr, leftType, rightType, relationNode));
}

static bool IsLoweredGlobalImmediateInitAssignment(ir::AssignmentExpression const *expr)
{
    if (expr->Parent() == nullptr || !expr->Parent()->IsExpressionStatement() || !expr->Left()->IsIdentifier()) {
        return false;
    }

    auto *lhsIdent = expr->Left()->AsIdentifier();
    if (lhsIdent->Variable() == nullptr || lhsIdent->Variable()->Declaration() == nullptr ||
        lhsIdent->Variable()->Declaration()->Node() == nullptr) {
        return false;
    }

    auto *declNode = lhsIdent->Variable()->Declaration()->Node();
    if (!declNode->IsClassProperty()) {
        return false;
    }

    auto *classProp = declNode->AsClassProperty();
    if (!classProp->IsImmediateInit() || classProp->TypeAnnotation() != nullptr || classProp->Value() == nullptr) {
        return false;
    }

    auto const *ownerClass = util::Helpers::GetContainingClassDefinition(classProp);
    auto const *exprClass = util::Helpers::GetContainingClassDefinition(expr);
    if (ownerClass == nullptr || exprClass == nullptr || ownerClass != exprClass ||
        !util::Helpers::IsGlobalClass(ownerClass)) {
        return false;
    }

    // Lowering contract:
    // GlobalDeclTransformer::CreateAssignmentStatement() sets generated statement range to classProperty range.
    // For unannotated immediate init, RHS is a clone of classProperty value with the same source range.
    bool sameStmtRange = !(expr->Parent()->Range() != classProp->Range());
    bool sameRhsRange = !(expr->Right()->Range() != classProp->Value()->Range());
    return sameStmtRange && sameRhsRange;
}

static bool ShouldSkipContextualTypingForArrayLiteral(ETSChecker *checker, Type *leftType)
{
    if (!util::Helpers::IsArrayType(leftType)) {
        return false;
    }

    auto *elementType = checker->GetElementTypeOfArray(leftType);
    if (elementType == nullptr || !elementType->IsETSUnionType()) {
        return false;
    }

    size_t arrayLikeCandidates = 0;
    for (auto *candidate : elementType->AsETSUnionType()->ConstituentTypes()) {
        if (candidate->IsAnyETSArrayOrTupleType()) {
            if (++arrayLikeCandidates > 1) {
                return true;
            }
        }
    }

    return false;
}

static checker::Type *HandleSubstitution(ETSChecker *checker, ir::AssignmentExpression *expr, Type *const leftType)
{
    bool possibleInferredTypeOfArray =
        util::Helpers::IsArrayType(leftType) || leftType->IsETSTupleType() || leftType->IsETSUnionType();
    if (expr->Right()->IsArrayExpression() && possibleInferredTypeOfArray) {
        auto *arrayExpr = expr->Right()->AsArrayExpression();
        // NOTE(likaizheng): Temporary narrow workaround for SPEC issue 838.
        // Top-level unannotated immediate-init declarations are lowered into ETSGLOBAL::cctor assignments.
        // Reapplying contextual typing on the lowered assignment may trigger ambiguity that does not appear for
        // function-local unannotated declarations. Keep this guard narrow and remove it after top-level and
        // function-body declaration checking paths are aligned.
        bool const skipContextualTyping = IsLoweredGlobalImmediateInitAssignment(expr) &&
                                          ShouldSkipContextualTypingForArrayLiteral(checker, leftType);
        checker->ModifyPreferredType(arrayExpr, skipContextualTyping ? nullptr : leftType);
    } else if (expr->Right()->IsArrowFunctionExpression() &&
               (leftType->IsETSArrowType() || leftType->IsETSUnionType())) {
        if (auto *preferredType = GetAppropriatePreferredType(leftType, [](Type *tp) { return tp->IsETSArrowType(); });
            preferredType != nullptr) {
            checker->TryInferTypeForLambdaTypeAlias(expr->Right()->AsArrowFunctionExpression(),
                                                    preferredType->AsETSFunctionType());
        } else {
            // Try infer the type from the UnionType.
            expr->Right()->SetPreferredType(leftType);
        }
    } else if (expr->Right()->IsObjectExpression()) {
        expr->Right()->AsObjectExpression()->SetPreferredType(leftType);
    } else {
        checker->SetPreferredTypeForExpression(expr->Left(), nullptr, expr->Right(), leftType);
    }

    return expr->Right()->Check(checker);
}

std::tuple<Type *, ir::Expression *> ETSAnalyzer::CheckAssignmentExprOperatorType(ir::AssignmentExpression *expr,
                                                                                  Type *const leftType) const
{
    ETSChecker *checker = GetETSChecker();
    checker::Type *sourceType {};
    ir::Expression *relationNode = expr->Right();
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
        case lexer::TokenType::PUNCTUATOR_BITWISE_OR_EQUAL:
        case lexer::TokenType::PUNCTUATOR_PLUS_EQUAL: {
            std::tie(std::ignore, expr->operationType_) = checker->CheckBinaryOperator(
                expr->Left(), expr->Right(), expr, expr->OperatorType(), expr->Start(), true);
            sourceType = leftType;
            relationNode = expr;
            break;
        }
        case lexer::TokenType::PUNCTUATOR_SUBSTITUTION: {
            sourceType = HandleSubstitution(checker, expr, leftType);
            break;
        }
        default: {
            ES2PANDA_UNREACHABLE();
            break;
        }
    }

    return {sourceType, relationNode};
}

checker::Type *ETSAnalyzer::Check(ir::AwaitExpression *expr) const
{
    ETSChecker *checker = GetETSChecker();
    if (expr->TsType() != nullptr) {
        return expr->TsType();
    }

    /*
     * NOTE(knazarov): Spec 16.3.4.
     * A compile-time error occurs if await is used outside of an asynchronous function, method or lambda body.
     * Check only the nearest ScriptFunction.
     */
    const auto ancestor = util::Helpers::FindAncestorGivenByType(expr, ir::AstNodeType::SCRIPT_FUNCTION);
    const auto isAncestorAsync = (ancestor != nullptr) && (ancestor->AsScriptFunction()->IsDeclaredAsync());
    if (!isAncestorAsync) {
        checker->LogError(diagnostic::AWAIT_IN_NON_ASYNC_DEPRECATED, {}, expr->Argument()->Start());
    }

    expr->SetTsType(checker->HandleAwaitedUtilityType(expr->argument_->Check(checker)));
    return expr->TsType();
}

checker::Type *ETSAnalyzer::Check(ir::BinaryExpression *expr) const
{
    if (expr->TsType() != nullptr) {
        return expr->TsType();
    }

    ETSChecker *checker = GetETSChecker();

    bool inSmartExpr = false;
    if (!checker->Context().IsInTestExpression()) {
        switch (expr->OperatorType()) {
            case lexer::TokenType::KEYW_INSTANCEOF:
            case lexer::TokenType::PUNCTUATOR_EQUAL:
            case lexer::TokenType::PUNCTUATOR_NOT_EQUAL:
            case lexer::TokenType::PUNCTUATOR_STRICT_EQUAL:
            case lexer::TokenType::PUNCTUATOR_NOT_STRICT_EQUAL:
            case lexer::TokenType::PUNCTUATOR_LOGICAL_AND:
            case lexer::TokenType::PUNCTUATOR_LOGICAL_OR: {
                inSmartExpr = true;
                SmartCastArray smartCasts = checker->Context().EnterTestExpression();
                break;
            }
            default:
                break;
        }
    }

    auto [newTsType, operationType] =
        checker->CheckBinaryOperator(expr->Left(), expr->Right(), expr, expr->OperatorType(), expr->Start());
    expr->SetTsType(newTsType);
    expr->SetOperationType(operationType);

    checker->Context().CheckBinarySmartCastCondition(expr);

    if (inSmartExpr) {
        checker->Context().ExitTestExpression();
    }

    return expr->TsType();
}

checker::Type *ETSAnalyzer::Check(ir::BlockExpression *st) const
{
    if (st->TsType() != nullptr) {
        return st->TsType();
    }

    ETSChecker *checker = GetETSChecker();
    checker::ScopeContext scopeCtx(checker, st->Scope());

    // NOLINTNEXTLINE(modernize-loop-convert)
    for (std::size_t idx = 0; idx < st->Statements().size(); idx++) {
        st->Statements()[idx]->Check(checker);
    }

    auto lastStmt = st->Statements().back();
    ES2PANDA_ASSERT(lastStmt->IsExpressionStatement());
    st->SetTsType(lastStmt->AsExpressionStatement()->GetExpression()->TsType());
    return st->TsType();
}

static bool LambdaIsField(ir::CallExpression *expr)
{
    if (!expr->Callee()->IsMemberExpression()) {
        return false;
    }
    auto *me = expr->Callee()->AsMemberExpression();
    return me->PropVar() != nullptr;
}

static Signature *CreateRelaxedAnySyntheticCallSignature(ETSChecker *checker)
{
    auto *info = checker->CreateSignatureInfo();
    info->minArgCount = 0;

    auto *paramVar =
        varbinder::Scope::CreateVar(checker->ProgramAllocator(), "args", varbinder::VariableFlags::NONE, nullptr);
    paramVar->SetTsType(checker->CreateETSArrayType(checker->GlobalETSRelaxedAnyType(), false));
    info->restVar = paramVar;
    // owner is not set

    return checker->CreateSignature(info, checker->GlobalETSRelaxedAnyType(), ir::ScriptFunctionFlags::NONE, false);
}

static checker::Signature *ResolveSignature(ETSChecker *checker, ir::CallExpression *expr, checker::Type *calleeType)
{
    if (calleeType->IsETSExtensionFuncHelperType()) {
        auto *signature =
            ResolveCallForETSExtensionFuncHelperType(calleeType->AsETSExtensionFuncHelperType(), checker, expr);
        return signature;
    }

    // when a lambda with receiver is a class field or interface property,
    // then it can only be called like a lambda without receiver.
    if (checker->IsExtensionETSFunctionType(calleeType) && !LambdaIsField(expr)) {
        auto *signature = ResolveCallExtensionFunction(calleeType, checker, expr);
        if (signature != nullptr && signature->IsExtensionAccessor() &&
            !checker->HasStatus(CheckerStatus::IN_EXTENSION_ACCESSOR_CHECK)) {
            checker->LogError(diagnostic::EXTENSION_ACCESSOR_INVALID_CALL, {}, expr->Start());
            return nullptr;
        }
        return signature;
    }

    auto noSignatures = ArenaVector<checker::Signature *> {checker->Allocator()->Adapter()};
    if (calleeType->IsETSRelaxedAnyType()) {
        noSignatures.push_back(CreateRelaxedAnySyntheticCallSignature(checker));
    }

    auto &signatures = expr->IsETSConstructorCall() ? calleeType->AsETSObjectType()->ConstructSignatures()
                       : calleeType->IsETSRelaxedAnyType()
                           ? noSignatures
                           : calleeType->AsETSFunctionType()->CallSignaturesOfMethodOrArrow();

    return checker->FirstMatchSignatures(signatures, expr);
}

static ETSObjectType *GetCallExpressionCalleeObject(ETSChecker *checker, ir::CallExpression *expr, Type *calleeType)
{
    if (expr->IsETSConstructorCall()) {
        return calleeType->AsETSObjectType();
    }
    auto callee = expr->Callee();
    if (callee->IsMemberExpression()) {
        return callee->AsMemberExpression()->ObjType();
    }
    ES2PANDA_ASSERT(callee->IsIdentifier());
    return checker->Context().ContainingClass();
}

static Type *ResolveThisTypeForThisReturnCall(ETSChecker *checker, ir::CallExpression *expr, Type *calleeType)
{
    auto *callee = expr->Callee();
    if (callee->IsMemberExpression() && callee->AsMemberExpression()->Object()->IsSuperExpression()) {
        return checker->Context().ContainingClass();
    }
    return GetCallExpressionCalleeObject(checker, expr, calleeType);
}

static bool CalleeIsBuiltinArrayStaticCreate(ir::CallExpression *expr)
{
    auto *callee = expr->Callee();
    if (!callee->IsMemberExpression()) {
        return false;
    }
    auto *mem = callee->AsMemberExpression();
    auto *prop = mem->Property();
    if (!prop->IsIdentifier()) {
        return false;
    }
    static constexpr std::string_view ARRAY_CREATE = "create";
    if (prop->AsIdentifier()->Name() != ARRAY_CREATE) {
        return false;
    }
    auto *obj = mem->Object();
    if (obj->TsType() == nullptr || !obj->TsType()->IsETSObjectType()) {
        return false;
    }
    return obj->TsType()->AsETSObjectType()->HasObjectFlag(ETSObjectFlags::BUILTIN_ARRAY);
}

static void EmitBuiltinArrayCreateMismatch(ETSChecker *checker, ir::CallExpression *expr, Type *calleeType)
{
    static constexpr std::string_view SIG_KIND_CALL = "call";
    if (!calleeType->IsETSFunctionType()) {
        return;
    }
    checker->LogSignatureMismatch(calleeType->AsETSFunctionType()->CallSignaturesOfMethodOrArrow(), expr->Arguments(),
                                  expr->Start(), SIG_KIND_CALL);
}

static Type *GetReturnType(ETSChecker *checker, ir::CallExpression *expr, Type *calleeType)
{
    if (calleeType->IsTypeError()) {
        return checker->GlobalTypeError();
    }

    if (!calleeType->IsETSFunctionType() && !expr->IsETSConstructorCall() &&
        !calleeType->IsETSExtensionFuncHelperType() && !calleeType->IsETSRelaxedAnyType()) {
        checker->LogError(diagnostic::NO_CALL_SIGNATURE, {calleeType}, expr->Start());
        return checker->GlobalTypeError();
    }

    if (CalleeIsBuiltinArrayStaticCreate(expr) && expr->Arguments().size() == 2U) {
        if (!checker->ValidateResizableArrayDimension(expr->Arguments()[0])) {
            EmitBuiltinArrayCreateMismatch(checker, expr, calleeType);
            return checker->GlobalTypeError();
        }
    }

    Signature *const signature = ResolveSignature(checker, expr, calleeType);

    if (signature == nullptr) {
        if (CalleeIsBuiltinArrayStaticCreate(expr)) {
            EmitBuiltinArrayCreateMismatch(checker, expr, calleeType);
        }
        return checker->GlobalTypeError();
    }

    CheckObjectLiteralArguments(checker, signature, expr->Arguments());

    if (calleeType->IsETSMethodType()) {
        ETSObjectType *calleeObj = GetCallExpressionCalleeObject(checker, expr, calleeType);
        checker->ValidateSignatureAccessibility(calleeObj, signature, expr->Start());
    }

    expr->SetSignature(signature);

    // #22951: this type should not be encoded as a signature flag
    if (signature->HasSignatureFlag(SignatureFlags::THIS_RETURN_TYPE)) {
        if (signature->HasSignatureFlag(SignatureFlags::EXTENSION_FUNCTION)) {
            return expr->Arguments()[0]->TsType();
        }
        return ResolveThisTypeForThisReturnCall(checker, expr, calleeType);
    }
    return signature->ReturnType();
}

static void CheckAbstractCall(ETSChecker *checker, ir::CallExpression *expr)
{
    if (expr->Callee()->IsMemberExpression()) {
        auto obj = expr->Callee()->AsMemberExpression()->Object();
        if (obj != nullptr && obj->IsSuperExpression()) {
            if ((expr->Signature() != nullptr) && (expr->Signature()->HasSignatureFlag(SignatureFlags::ABSTRACT))) {
                checker->LogError(diagnostic::ABSTRACT_CALL, {}, expr->Start());
                expr->SetTsType(checker->GlobalTypeError());
            }
        }
    }
}

static bool IsDirectlyInStaticInit(const ir::AstNode *node)
{
    for (auto *iter = node->Parent(); iter != nullptr; iter = iter->Parent()) {
        if (iter->IsClassProperty()) {
            return iter->IsStatic();
        }
        if (iter->IsScriptFunction()) {
            auto *func = iter->AsScriptFunction();
            if (func->IsStaticBlock()) {
                return true;
            }
            if (func->Id() != nullptr && IsInitializerBlockTransfer(func->Id()->Name().Utf8())) {
                return true;
            }
            return false;
        }
    }
    return false;
}

static void CheckAsyncCallInStaticInit(ETSChecker *checker, ir::CallExpression *expr)
{
    auto *sig = expr->Signature();
    if (sig == nullptr || !sig->HasFunction() || !sig->Function()->IsAsyncFunc()) {
        return;
    }
    if (IsDirectlyInStaticInit(expr)) {
        checker->LogError(diagnostic::ASYNC_CALL_IN_STATIC_INIT, {}, expr->Start());
    }
}

static void CheckCallee(ETSChecker *checker, ir::CallExpression *expr)
{
    checker->CheckNonNullish(expr->Callee());
    if (!expr->Callee()->IsMemberExpression()) {
        return;
    }
    auto memberExpr = expr->Callee()->AsMemberExpression();
    if (memberExpr->Object() == nullptr) {
        return;
    }
    auto baseType = memberExpr->Object()->TsType();
    auto *baseTypeObj = baseType->IsETSObjectType() ? baseType->AsETSObjectType() : nullptr;
    if (baseTypeObj != nullptr &&
        (baseTypeObj->HasObjectFlag(ETSObjectFlags::READONLY) ||
         (baseTypeObj->HasTypeFlag(TypeFlag::READONLY) && !baseType->IsAnyETSArrayOrTupleType()))) {
        checker->LogError(diagnostic::READONLY_CALL, {}, expr->Start());
        expr->SetTsType(checker->GlobalTypeError());
    }
    // NOTE(fantianqi): #33001 Need remove method (even any getter or setter) of Required<T>
    if (baseTypeObj != nullptr && (baseTypeObj->HasObjectFlag(ETSObjectFlags::REQUIRED))) {
        checker->LogError(diagnostic::REQUIRED_CALL, {}, expr->Start());
        expr->SetTsType(checker->GlobalTypeError());
    }
}

// Restore CheckerContext of the owner class if we want to perform checking
static checker::SavedCheckerContext ReconstructOwnerClassContext(ETSChecker *checker, ETSObjectType *owner)
{
    if (owner == nullptr) {
        return SavedCheckerContext(checker, CheckerStatus::NO_OPTS, nullptr);
    }
    ES2PANDA_ASSERT(!owner->HasObjectFlag(ETSObjectFlags::ENUM));
    CheckerStatus const status =
        (owner->HasObjectFlag(ETSObjectFlags::CLASS) ? CheckerStatus::IN_CLASS : CheckerStatus::IN_INTERFACE) |
        (owner->HasObjectFlag(ETSObjectFlags::ABSTRACT) ? CheckerStatus::IN_ABSTRACT : CheckerStatus::NO_OPTS) |
        (owner->HasObjectFlag(ETSObjectFlags::INNER) ? CheckerStatus::INNER_CLASS : CheckerStatus::NO_OPTS) |
        (owner->GetDeclNode()->IsClassDefinition() && owner->GetDeclNode()->AsClassDefinition()->IsLocal()
             ? CheckerStatus::IN_LOCAL_CLASS
             : CheckerStatus::NO_OPTS);

    return SavedCheckerContext(checker, status, owner);
}

static checker::Type *GetCallExpressionReturnType(ETSChecker *checker, ir::CallExpression *expr,
                                                  checker::Type *calleeType)
{
    checker::Type *returnType = GetReturnType(checker, expr, calleeType);

    if (returnType->IsTypeError()) {
        return checker->GlobalTypeError();
    }

    auto *const signature = expr->Signature();
    if (signature->RestVar() != nullptr && signature->RestVar()->TsType()->IsETSArrayType()) {
        auto originalArray =
            signature->RestVar()->TsType()->AsETSArrayType();  // NOTE(vpukhov): no idea why it is copied currently
        auto *const arrayType =
            checker->CreateETSArrayType(originalArray->ElementType(), originalArray->IsValueArray())->AsETSArrayType();
        checker->CreateBuiltinArraySignature(arrayType, arrayType->Rank());
    }

    if (!signature->HasSignatureFlag(checker::SignatureFlags::NEED_RETURN_TYPE) ||
        (signature->HasSignatureFlag(checker::SignatureFlags::CONSTRUCTOR))) {
        return returnType;
    }

    if (!signature->HasFunction()) {
        return checker->GlobalTypeError();
    }

    auto owner = const_cast<ETSObjectType *>(util::Helpers::GetContainingObjectType(signature->Function()));
    SavedCheckerContext savedCtx(ReconstructOwnerClassContext(checker, owner));

    ir::AstNode *methodDef = signature->Function();
    while (!methodDef->IsMethodDefinition()) {
        methodDef = methodDef->Parent();
        ES2PANDA_ASSERT(methodDef != nullptr);
    }
    ES2PANDA_ASSERT(methodDef->IsMethodDefinition());
    methodDef->Check(checker);

    if (!signature->Function()->HasBody()) {
        return signature->ReturnType();
    }

    if (signature->Function()->IsExternal()) {
        checker->VarBinder()->AsETSBinder()->ResolveReferencesForScopeWithContext(signature->Function()->Body(),
                                                                                  signature->Function()->Scope());
    }

    checker::ScopeContext scopeCtx(checker, signature->Function()->Body()->Scope());
    checker->CollectReturnStatements(signature->Function());

    auto *const baseSig = signature->Function() != nullptr ? signature->Function()->Signature() : nullptr;
    if (baseSig == nullptr || signature == baseSig) {
        return signature->ReturnType();
    }

    // Generic calls may hold a substituted signature copy. Re-resolve after lazy
    // return type inference so the call sees the updated instantiated return type.
    auto *updatedSignature = ResolveSignature(checker, expr, calleeType);
    if (updatedSignature == nullptr) {
        return checker->GlobalTypeError();
    }
    expr->SetSignature(updatedSignature);
    return updatedSignature->ReturnType();
}

static void CheckOverloadCall(ETSChecker *checker, ir::CallExpression *expr)
{
    if (!expr->Callee()->IsMemberExpression() || !checker->IsOverloadDeclaration(expr->Callee())) {
        return;
    }

    auto *sig = expr->Signature();
    auto *functionNode = sig->OwnerVar()->Declaration()->Node();
    ir::AstNode *parent = functionNode->Parent();

    bool isExported = util::Helpers::IsExported(functionNode);
    if (parent != nullptr && parent->IsClassDefinition() && parent->AsClassDefinition()->IsNamespaceTransformed() &&
        !parent->AsClassDefinition()->IsDeclare() && !isExported) {
        checker->LogError(diagnostic::NOT_EXPORTED,
                          {sig->OwnerVar()->Declaration()->Name(), parent->AsClassDefinition()->Ident()->Name()},
                          expr->Start());
    }
}

checker::Type *ETSAnalyzer::Check(ir::CallExpression *expr) const
{
    ETSChecker *checker = GetETSChecker();
    if (expr->TsType() != nullptr) {
        return expr->TsType();
    }
    ES2PANDA_ASSERT(!expr->IsOptional());

    auto *oldCallee = expr->Callee();
    checker::Type *calleeType = checker->GetApparentType(expr->Callee()->Check(checker));
    if (calleeType->IsTypeError()) {
        return checker->InvalidateType(expr);
    }

    if (expr->Callee() != oldCallee) {
        // If it is a static invoke, the callee will be transformed from an identifier to a member expression
        // Type check the callee again for member expression
        calleeType = checker->GetApparentType(expr->Callee()->Check(checker));
    }

    CheckCallee(checker, expr);

    checker::TypeStackElement tse(checker, expr, {{diagnostic::CYCLIC_CALLEE, {}}}, expr->Start());
    ERROR_SANITY_CHECK(checker, !tse.HasTypeError(), return expr->SetTsType(checker->GlobalTypeError()));

    checker::Type *const returnType = GetCallExpressionReturnType(checker, expr, calleeType);
    expr->SetTsType(returnType);
    if (returnType->IsTypeError()) {
        return returnType;
    }
    if (calleeType->IsETSArrowType() || calleeType->IsETSRelaxedAnyType()) {
        expr->SetUncheckedType(
            checker->GuaranteedTypeForUncheckedCast(checker->GlobalETSAnyType(), expr->Signature()->ReturnType()));
    } else {
        expr->SetUncheckedType(checker->GuaranteedTypeForUncheckedCallReturn(expr->Signature()));
    }

    if (expr->UncheckedType() != nullptr) {
        ES2PANDA_ASSERT(expr->UncheckedType()->IsETSReferenceType());
        checker->ComputeApparentType(returnType);
    }

    CheckOverloadCall(checker, expr);
    CheckAbstractCall(checker, expr);
    CheckAsyncCallInStaticInit(checker, expr);
    return expr->TsType();
}

static bool IsNumericType(ETSChecker *checker, Type *type)
{
    ES2PANDA_ASSERT(!type->IsETSNeverType());
    return !type->IsETSTypeParameter() &&
           checker->Relation()->IsSupertypeOf(checker->GetGlobalTypesHolder()->GlobalNumericBuiltinType(), type);
}

static Type *BiggerNumericType(ETSChecker *checker, Type *t1, Type *t2)
{
    ES2PANDA_ASSERT(IsNumericType(checker, t1));
    ES2PANDA_ASSERT(IsNumericType(checker, t2));

    auto *rel = checker->Relation();

    if (rel->IsSupertypeOf(checker->GlobalDoubleBuiltinType(), t1) ||
        rel->IsSupertypeOf(checker->GlobalDoubleBuiltinType(), t2)) {
        return checker->GlobalDoubleBuiltinType();
    }
    if (rel->IsSupertypeOf(checker->GlobalFloatBuiltinType(), t1) ||
        rel->IsSupertypeOf(checker->GlobalFloatBuiltinType(), t2)) {
        return checker->GlobalFloatBuiltinType();
    }
    if (rel->IsSupertypeOf(checker->GlobalLongBuiltinType(), t1) ||
        rel->IsSupertypeOf(checker->GlobalLongBuiltinType(), t2)) {
        return checker->GlobalLongBuiltinType();
    }
    if (rel->IsSupertypeOf(checker->GlobalIntBuiltinType(), t1) ||
        rel->IsSupertypeOf(checker->GlobalIntBuiltinType(), t2)) {
        return checker->GlobalIntBuiltinType();
    }
    if (rel->IsSupertypeOf(checker->GlobalShortBuiltinType(), t1) ||
        rel->IsSupertypeOf(checker->GlobalShortBuiltinType(), t2)) {
        return checker->GlobalShortBuiltinType();
    }
    if (rel->IsSupertypeOf(checker->GlobalByteBuiltinType(), t1) ||
        rel->IsSupertypeOf(checker->GlobalByteBuiltinType(), t2)) {
        return checker->GlobalByteBuiltinType();
    }
    ES2PANDA_UNREACHABLE();
}

static void ApplyTestedSmartCasts(ETSChecker *const checker, const SmartCastTypes &testedTypes,
                                  bool const consequentBranch)
{
    if (!testedTypes.has_value()) {
        return;
    }

    for (auto [variable, consequentType, alternateType] : *testedTypes) {
        checker->ApplySmartCast(variable, consequentBranch ? consequentType : alternateType);
    }
}

checker::Type *ETSAnalyzer::Check(ir::ConditionalExpression *expr) const
{
    if (expr->TsType() != nullptr) {
        return expr->TsType();
    }

    ETSChecker *const checker = GetETSChecker();

    SmartCastArray smartCasts = checker->Context().EnterTestExpression();
    checker->CheckTruthinessOfType(expr->Test());
    SmartCastTypes testedTypes = checker->Context().ExitTestExpression();
    auto const testValue = TryResolveConditionalTestValue(expr->Test());
    if (testValue.has_value()) {
        auto const checkBranch = [checker, &testedTypes](ir::Expression *branch, bool takeConsequentSmartCast,
                                                         bool suppressDiagnostics) {
            ApplyTestedSmartCasts(checker, testedTypes, takeConsequentSmartCast);
            if (suppressDiagnostics) {
                checker::InferMatchContext silentBranchCheckCtx(checker, util::DiagnosticType::SEMANTIC,
                                                                branch->Range(), false);
                Type *const branchType = branch->Check(checker);
                return branchType;
            }
            Type *const branchType = branch->Check(checker);
            return branchType;
        };

        auto *const takenBranch = testValue.value() ? expr->Consequent() : expr->Alternate();
        auto *const deadBranch = testValue.value() ? expr->Alternate() : expr->Consequent();
        auto *const takenType = checkBranch(takenBranch, testValue.value(), false);

        checker->Context().RestoreSmartCasts(smartCasts);
        (void)checkBranch(deadBranch, !testValue.value(), true);

        expr->SetTsType(takenType);

        // Restore smart casts to initial state.
        checker->Context().RestoreSmartCasts(smartCasts);
        return expr->TsType();
    }

    ApplyTestedSmartCasts(checker, testedTypes, true);

    auto *consequent = expr->Consequent();
    Type *consequentType = consequent->Check(checker);

    SmartCastArray consequentSmartCasts = checker->Context().CloneSmartCasts();
    checker->Context().RestoreSmartCasts(smartCasts);

    ApplyTestedSmartCasts(checker, testedTypes, false);

    auto *alternate = expr->Alternate();
    Type *alternateType = alternate->Check(checker);

    // Here we need to combine types from consequent and alternate if blocks.
    checker->Context().CombineSmartCasts(consequentSmartCasts);

    if (consequentType->IsETSNeverType()) {
        expr->SetTsType(alternateType);
    } else if (alternateType->IsNeverType()) {
        expr->SetTsType(consequentType);
    } else if (checker->IsTypeIdenticalTo(consequentType, alternateType)) {
        expr->SetTsType(consequentType);
    } else {
        Type *consequentTypeUnderly =
            consequentType->IsETSNumericEnumType() ? consequentType->AsETSEnumType()->Underlying() : consequentType;
        Type *alternateTypeUnderly =
            alternateType->IsETSNumericEnumType() ? alternateType->AsETSEnumType()->Underlying() : alternateType;
        if (IsNumericType(GetETSChecker(), consequentTypeUnderly) &&
            IsNumericType(GetETSChecker(), alternateTypeUnderly)) {
            if (consequentType->IsETSNumericEnumType()) {
                expr->Consequent()->AddAstNodeFlags(ir::AstNodeFlags::GENERATE_VALUE_OF);
            }
            if (alternateType->IsETSNumericEnumType()) {
                expr->Alternate()->AddAstNodeFlags(ir::AstNodeFlags::GENERATE_VALUE_OF);
            }
            expr->SetTsType(BiggerNumericType(GetETSChecker(), consequentTypeUnderly, alternateTypeUnderly));
        } else {
            expr->SetTsType(checker->CreateETSUnionType({consequentType, alternateType}));
        }
    }

    // Restore smart casts to initial state.
    checker->Context().RestoreSmartCasts(smartCasts);

    return expr->TsType();
}

static bool HasGenericTypeParams(const checker::Type *type)
{
    if (type == nullptr || type->IsTypeError() || !type->IsETSFunctionType()) {
        return false;
    }
    auto *functionType = type->AsETSFunctionType();
    auto &sigs = functionType->CallSignaturesOfMethodOrArrow();
    for (auto *sig : sigs) {
        if (!sig->TypeParams().empty()) {
            return true;
        }
    }
    return false;
}

static bool IsUsedAsCall(const ir::Expression *expr)
{
    auto *p = expr->Parent();
    return p != nullptr && p->IsCallExpression() && expr == p->AsCallExpression()->Callee();
}

// Generic function/method references used as values require explicit type arguments.
static Type *CheckExplicitTypeArgumentsRequired(ETSChecker *checker, ir::Expression *const use, Type *type)
{
    ir::Expression *top = use;
    while (top->Parent()->IsMemberExpression() && !top->Parent()->AsMemberExpression()->IsComputed() &&
           top->Parent()->AsMemberExpression()->Property() == top) {
        top = top->Parent()->AsMemberExpression();
    }
    const bool usedAsCall = IsUsedAsCall(top);
    const bool usedAsExplicitInstantiation = top->Parent()->IsETSGenericInstantiatedNode();
    const bool hasGenericTypeParams = HasGenericTypeParams(type);
    if (hasGenericTypeParams && !usedAsCall && !usedAsExplicitInstantiation) {
        util::StringView name;
        util::StringView func = "function";
        if (use->IsIdentifier()) {
            name = use->AsIdentifier()->Name();
        } else {
            auto *prop = use->AsMemberExpression()->Property();
            name = prop->IsIdentifier() ? prop->AsIdentifier()->Name() : func;
        }
        checker->LogError(diagnostic::EXPLICIT_TYPE_ARGUMENTS_REQUIRED, {name}, use->Start());
        return checker->GlobalTypeError();
    }
    return nullptr;
}

// Handles non-method types and cases where the method reference is actually a call or overload declaration.
static Type *HandleNonMethodAndCallSite(ETSChecker *checker, ir::Expression *use, Type *type)
{
    auto *parent = use->Parent();

    if (!type->IsETSMethodType()) {
        if (parent != nullptr && parent->IsCallExpression() && type->IsETSObjectType() && use->IsMemberExpression()) {
            checker->ValidateCallExpressionIdentifier(use->AsMemberExpression()->Property()->AsIdentifier(), type);
        }
        return type;
    }

    ir::Expression *expr = use;
    while (expr->Parent()->IsMemberExpression() && !expr->Parent()->AsMemberExpression()->IsComputed() &&
           expr->Parent()->AsMemberExpression()->Property() == expr) {
        expr = expr->Parent()->AsMemberExpression();
    }

    parent = expr->Parent();
    if (parent != nullptr) {
        if (parent->IsCallExpression() && parent->AsCallExpression()->Callee() == expr) {
            return type;
        }
        if (parent->IsOverloadDeclaration()) {
            return type;
        }
        if (parent->IsAssignmentExpression() && parent->AsAssignmentExpression()->Left() == expr) {
            return type;
        }
    }

    ES2PANDA_ASSERT(type->IsETSMethodType());
    const auto *funcType = type->AsETSFunctionType();
    for (auto *sig : funcType->CallSignatures()) {
        if (sig->Function() != nullptr && sig->Function()->IsGetter()) {
            return sig->ReturnType();
        }
    }

    return nullptr;
}

// Validates method reference usage and converts method types to arrow function types when valid.
static Type *TransformMethodTypeToArrow(ETSChecker *checker, ir::Expression *use, Type *methodType)
{
    auto *functionType = methodType->AsETSFunctionType();
    auto &signatures = functionType->CallSignatures();

    auto getUseSite = [use]() {
        return use->IsIdentifier() ? use->Start() : use->AsMemberExpression()->Property()->Start();
    };

    if (!signatures.empty()) {
        auto *first = signatures.front();
        if (first->HasSignatureFlag(SignatureFlags::PRIVATE)) {
            checker->LogError(diagnostic::PRIVATE_OR_PROTECTED_METHOD_AS_VALUE, {"Private"}, getUseSite());
            return checker->GlobalTypeError();
        }
        if (first->HasSignatureFlag(SignatureFlags::PROTECTED)) {
            checker->LogError(diagnostic::PRIVATE_OR_PROTECTED_METHOD_AS_VALUE, {"Protected"}, getUseSite());
            return checker->GlobalTypeError();
        }
    }

    auto it = signatures.begin();
    while (it != signatures.end()) {
        if ((*it)->HasSignatureFlag(SignatureFlags::ABSTRACT) &&
            !(*it)->Owner()->GetDeclNode()->IsTSInterfaceDeclaration()) {
            it = signatures.erase(it);
        } else {
            ++it;
        }
    }

    if (signatures.size() > 1U) {
        checker->LogError(diagnostic::OVERLOADED_METHOD_AS_VALUE, getUseSite());
        return checker->GlobalTypeError();
    }

    auto *otherFuncType = functionType->MethodToArrow(checker);
    return otherFuncType == nullptr ? checker->GlobalTypeError() : otherFuncType;
}

// Method-reference handling: generic rules, call-site behavior, and performs arrow conversion.
static Type *TransformTypeForMethodReference(ETSChecker *checker, ir::Expression *const use, Type *type)
{
    ES2PANDA_ASSERT(use->IsIdentifier() || use->IsMemberExpression());

    if (auto *errType = CheckExplicitTypeArgumentsRequired(checker, use, type); errType != nullptr) {
        return errType;
    }

    if (auto *early = HandleNonMethodAndCallSite(checker, use, type); early != nullptr) {
        return early;
    }

    return TransformMethodTypeToArrow(checker, use, type);
}

checker::Type *ETSAnalyzer::Check(ir::Identifier *expr) const
{
    if (expr->TsType() != nullptr) {
        return expr->TsType();
    }

    ETSChecker *checker = GetETSChecker();

    auto *type = checker->ResolveIdentifier(expr);
    auto *identType = TransformTypeForMethodReference(checker, expr, type);

    if (expr->TsType() != nullptr && expr->TsType()->IsTypeError()) {
        return expr->TsType();
    }
    ES2PANDA_ASSERT(expr->Variable() != nullptr);
    if (expr->Parent() != nullptr &&
        !(expr->Parent()->IsAssignmentExpression() && expr == expr->Parent()->AsAssignmentExpression()->Left()) &&
        !expr->Parent()->IsETSDestructuring()) {
        auto *const smartType = checker->Context().GetSmartCast(expr->Variable());
        if (smartType != nullptr) {
            identType = smartType;
        }
    }

    ES2PANDA_ASSERT(identType != nullptr);
    expr->SetTsType(identType);
    ES2PANDA_ASSERT(identType != nullptr);
    // NOLINTNEXTLINE(clang-analyzer-core.CallAndMessage)
    if (!identType->IsTypeError()) {
        checker->Context().CheckIdentifierSmartCastCondition(expr);
    }
    return expr->TsType();
}

std::pair<checker::Type *, util::StringView> SearchReExportsType(ETSObjectType *baseType, ir::MemberExpression *expr,
                                                                 util::StringView const &aliasName, ETSChecker *checker)
{
    std::pair<ETSObjectType *, util::StringView> ret {};

    for (auto *const item : baseType->ReExports()) {
        auto name = item->GetReExportAliasValue(aliasName);
        if (name == aliasName && item->IsReExportHaveAliasValue(name)) {
            continue;
        }

        if (item->GetProperty(name, PropertySearchFlags::SEARCH_ALL) != nullptr) {
            if (ret.first != nullptr) {
                checker->LogError(diagnostic::AMBIGUOUS_REFERENCE, {aliasName}, expr->Start());
                expr->SetTsType(checker->GlobalTypeError());
                return ret;
            }
            ret = {item, name};
        }

        if (auto reExportType = SearchReExportsType(item, expr, name, checker); reExportType.first != nullptr) {
            return reExportType;
        }
    }

    return ret;
}

static void TypeErrorOnMissingProperty(ir::MemberExpression *expr, checker::Type *baseType,
                                       checker::ETSChecker *checker)
{
    std::ignore = checker->TypeError(expr, diagnostic::PROPERTY_NONEXISTENT,
                                     {expr->Property()->AsIdentifier()->Name(), baseType}, expr->Object()->Start());
}

checker::Type *ETSAnalyzer::ResolveMemberExpressionByBaseType(ETSChecker *checker, checker::Type *baseType,
                                                              ir::MemberExpression *expr) const
{
    if (baseType->IsTypeError()) {
        return checker->InvalidateType(expr);
    }

    if (baseType->IsETSRelaxedAnyType()) {
        return expr->AdjustType(checker, checker->GlobalETSRelaxedAnyType());
    }

    if (baseType->IsETSArrayType()) {
        if (expr->Property()->AsIdentifier()->Name().Is("length")) {
            return expr->AdjustType(checker, checker->GlobalIntBuiltinType());
        }

        return expr->SetAndAdjustType(checker, checker->GlobalETSObjectType());
    }

    if (baseType->IsETSTupleType()) {
        return expr->SetAndAdjustType(checker, checker->GlobalETSObjectType());
    }

    if (baseType->IsETSFunctionType()) {
        return expr->SetAndAdjustType(checker, checker->GlobalBuiltinFunctionType());
    }

    if (baseType->IsETSObjectType()) {
        checker->ETSObjectTypeDeclNode(checker, baseType->AsETSObjectType());
        auto *memberType = expr->SetAndAdjustType(checker, baseType->AsETSObjectType());
        if (!checker->CheckSuperMemberBeforeCtorCall(expr)) {
            return checker->InvalidateType(expr);
        }

        return expr->SetTsType(TransformTypeForMethodReference(checker, expr, memberType));
    }

    if (baseType->IsETSUnionType()) {
        return expr->AdjustType(checker, expr->CheckUnionMember(checker, baseType));
    }

    TypeErrorOnMissingProperty(expr, baseType, checker);
    return expr->TsType();
}

checker::Type *ETSAnalyzer::Check(ir::MemberExpression *expr) const
{
    if (expr->TsType() != nullptr) {
        return expr->TsType();
    }
    ES2PANDA_ASSERT(!expr->IsOptional());
    ETSChecker *checker = GetETSChecker();
    if (expr->Object()->IsSuperExpression() && IsExpressionInClassPropertyInitializer(expr)) {
        checker->LogDiagnostic(diagnostic::THIS_OR_SUPER_IN_FIELD_INITIALIZER, {"super"}, expr->Object()->Start());
    }
    auto *baseType = checker->GetNonConstantType(checker->GetApparentType(expr->Object()->Check(checker)));
    //  Note: don't use possible smart cast to null-like types.
    //        Such situation should be correctly resolved in the subsequent lowering.
    ES2PANDA_ASSERT(baseType != nullptr);
    if (baseType->DefinitelyETSNullish() && expr->Object()->IsIdentifier()) {
        baseType = expr->Object()->AsIdentifier()->Variable()->TsType();
    }

    if (baseType->IsETSObjectType() && !baseType->AsETSObjectType()->ReExports().empty() &&
        baseType->AsETSObjectType()->GetProperty(expr->Property()->AsIdentifier()->Name(),
                                                 PropertySearchFlags::SEARCH_ALL) == nullptr) {
        if (auto reExportType = SearchReExportsType(baseType->AsETSObjectType(), expr,
                                                    expr->Property()->AsIdentifier()->Name(), checker);
            reExportType.first != nullptr) {
            baseType = reExportType.first;
            expr->object_->SetTsType(baseType);
            expr->property_->AsIdentifier()->SetName(reExportType.second);
        }
    }

    if (!checker->CheckNonNullish(expr->Object())) {
        auto *invalidType = checker->HasStatus(checker::CheckerStatus::IN_EXTENSION_ACCESSOR_CHECK)
                                ? checker->GlobalETSUnionUndefinedNull()
                                : checker->InvalidateType(expr);
        return invalidType;
    }

    if (expr->IsComputed()) {
        return expr->AdjustType(checker, expr->CheckComputed(checker, baseType));
    }

    if (baseType->IsETSTupleType()) {
        baseType = baseType->AsETSTupleType()->GetWrapperType();
    }

    if (checker->IsPromiseType(baseType) &&
        expr->Property()->AsIdentifier()->Name().Is(compiler::Signatures::AWAIT_SYNC)) {
        checker->LogDiagnostic(diagnostic::AWAIT_ASYNC_USAGE, {}, expr->Start());
    }

    return ResolveMemberExpressionByBaseType(checker, baseType, expr);
}

checker::Type *ETSAnalyzer::CheckDynamic(ir::ObjectExpression *expr) const
{
    ETSChecker *checker = GetETSChecker();
    for (ir::Expression *propExpr : expr->Properties()) {
        ES2PANDA_ASSERT(propExpr->IsProperty());
        ir::Property *prop = propExpr->AsProperty();
        ir::Expression *value = prop->Value();
        value->Check(checker);
        ES2PANDA_ASSERT(value->TsType());
    }

    expr->SetTsType(expr->PreferredType());
    return expr->PreferredType();
}

static bool ValidatePreferredType(ETSChecker *checker, ir::ObjectExpression *expr)
{
    auto preferredType = expr->PreferredType();
    if (preferredType == nullptr) {
        checker->LogError(diagnostic::CLASS_COMPOSITE_UNKNOWN_TYPE, {}, expr->Start());
        return false;
    }

    if (preferredType->IsTypeError()) {
        //  Don't need to duplicate error message for a single error.
        return false;
    }

    if (!preferredType->IsETSObjectType()) {
        checker->LogError(diagnostic::CLASS_COMPOSITE_INVALID_TARGET, {preferredType}, expr->Start());
        return false;
    }

    return true;
}

static void SetTypeforRecordProperties(const ir::ObjectExpression *expr, checker::ETSObjectType *objType,
                                       ETSChecker *checker)
{
    const auto &recordProperties = expr->Properties();
    auto typeArguments = objType->TypeArguments();
    auto *const valueType = typeArguments[1];  //  Record<K, V>  type arguments

    for (auto *const recordProperty : recordProperties) {
        ir::Expression *recordPropertyExpr = nullptr;
        if (recordProperty->IsProperty()) {
            recordProperty->AsProperty()->Key()->SetPreferredType(typeArguments[0]);
            recordProperty->AsProperty()->Key()->Check(checker);
            recordPropertyExpr = recordProperty->AsProperty()->Value();
        } else if (recordProperty->IsSpreadElement()) {
            recordPropertyExpr = recordProperty->AsSpreadElement()->Argument();
        } else if (recordProperty->IsIdentifier() && recordProperty->AsIdentifier()->IsErrorPlaceHolder()) {
            ES2PANDA_ASSERT(checker->IsAnyError());
            continue;
        } else {
            ES2PANDA_UNREACHABLE();
        }

        recordPropertyExpr->SetPreferredType(valueType);
        recordPropertyExpr->Check(checker);
    }
}

// Helper to check for parameterless constructor
static bool HasParameterlessConstructor(checker::ETSObjectType *objType, ETSChecker *checker,
                                        const lexer::SourcePosition &pos)
{
    for (checker::Signature *sig : objType->ConstructSignatures()) {
        if (sig->MinArgCount() == 0) {
            checker->ValidateSignatureAccessibility(objType, sig, pos);
            return true;
        }
    }
    return false;
}

// Helper to resolve property name from key expression
static std::optional<util::StringView> GetPropertyNameFromKey(ir::Expression *key)
{
    if (key->IsStringLiteral()) {
        return key->AsStringLiteral()->Str();
    }
    if (key->IsIdentifier()) {
        return key->AsIdentifier()->Name();
    }
    return std::nullopt;  // Indicates invalid key type
}

// Helper to determine property search flags based on object type
static checker::PropertySearchFlags DetermineSearchFlagsForLiteral(checker::ETSObjectType *potentialObjType)
{
    if (potentialObjType->HasObjectFlag(checker::ETSObjectFlags::INTERFACE)) {
        return checker::PropertySearchFlags::SEARCH_INSTANCE_FIELD |
               checker::PropertySearchFlags::SEARCH_INSTANCE_METHOD |
               checker::PropertySearchFlags::SEARCH_INSTANCE_DECL | checker::PropertySearchFlags::SEARCH_IN_INTERFACES;
    }
    return checker::PropertySearchFlags::SEARCH_INSTANCE_FIELD | checker::PropertySearchFlags::SEARCH_IN_BASE |
           checker::PropertySearchFlags::SEARCH_INSTANCE_METHOD;
}

static bool CheckSinglePropertyCompatibility(ir::Expression *propExpr, checker::ETSObjectType *potentialObjType)
{
    if (!propExpr->IsProperty()) {
        return false;  // Not a key-value property
    }
    ir::Expression *key = propExpr->AsProperty()->Key();

    std::optional<util::StringView> optPname = GetPropertyNameFromKey(key);
    if (!optPname.has_value()) {
        return false;  // Invalid key type in literal
    }
    util::StringView pname = optPname.value();

    checker::PropertySearchFlags searchFlags = DetermineSearchFlagsForLiteral(potentialObjType);
    return potentialObjType->GetProperty(pname, searchFlags) != nullptr;
}

static bool CheckObjectLiteralCompatibility(ir::ObjectExpression *expr, checker::ETSObjectType *potentialObjType)
{
    for (ir::Expression *propExpr : expr->Properties()) {
        if (!CheckSinglePropertyCompatibility(propExpr, potentialObjType)) {
            return false;
        }
    }
    return true;  // All properties found
}

// Helper to check if a property type indicates optionality (union with undefined)
static bool IsPropertyTypeOptional(checker::Type *propertyType)
{
    if (!propertyType->IsETSUnionType()) {
        return false;
    }

    auto *unionType = propertyType->AsETSUnionType();
    for (auto *constituentType : unionType->ConstituentTypes()) {
        if (constituentType->IsETSUndefinedType()) {
            return true;
        }
    }
    return false;
}

// Helper to check if a property has a default value in its declaration
static bool HasPropertyDefaultValue(varbinder::LocalVariable *property)
{
    auto *decl = property->Declaration();
    if (decl == nullptr || decl->Node() == nullptr || !decl->Node()->IsClassProperty()) {
        return false;
    }

    auto *classProp = decl->Node()->AsClassProperty();
    return classProp->Value() != nullptr;
}

// Helper to check if a property is optional (flag or declaration)
static bool IsPropertyOptional(varbinder::LocalVariable *property, checker::Type *propertyType)
{
    // Check if property is marked as optional
    if (property->HasFlag(varbinder::VariableFlags::OPTIONAL)) {
        return true;
    }

    // Check if property type includes undefined (indicating optional)
    if (IsPropertyTypeOptional(propertyType)) {
        return true;
    }

    // Check if declaration has optional modifier
    auto *decl = property->Declaration();
    if (decl != nullptr && decl->Node() != nullptr && decl->Node()->IsClassProperty()) {
        auto *classProp = decl->Node()->AsClassProperty();
        if (classProp->IsOptionalDeclaration()) {
            return true;
        }
    }

    return false;
}

// Helper to check if a method property is only getters/setters
static bool IsMethodOnlyAccessors(checker::Type *propertyType)
{
    if (!propertyType->IsETSMethodType()) {
        return false;
    }

    auto methodType = propertyType->AsETSFunctionType();
    for (auto *sig : methodType->CallSignatures()) {
        if (!sig->HasSignatureFlag(checker::SignatureFlags::GETTER) &&
            !sig->HasSignatureFlag(checker::SignatureFlags::SETTER)) {
            // Regular method found
            return false;
        }
    }
    return true;
}

// Helper to check if an interface property is compatible with object literal property
static bool IsInterfacePropertyCompatible(ir::Expression *propExpr, checker::ETSObjectType *interfaceType,
                                          ETSChecker *checker)
{
    if (!propExpr->IsProperty()) {
        return false;
    }

    ir::Expression *key = propExpr->AsProperty()->Key();
    std::optional<util::StringView> optPname = GetPropertyNameFromKey(key);
    if (!optPname.has_value()) {
        return false;
    }
    util::StringView pname = optPname.value();

    // Check if property exists in interface
    varbinder::LocalVariable *property =
        interfaceType->GetProperty(pname, checker::PropertySearchFlags::SEARCH_INSTANCE_FIELD |
                                              checker::PropertySearchFlags::SEARCH_INSTANCE_METHOD |
                                              checker::PropertySearchFlags::SEARCH_INSTANCE_DECL |
                                              checker::PropertySearchFlags::SEARCH_IN_INTERFACES);

    if (property == nullptr) {
        return false;
    }

    auto *propertyType = checker->GetTypeOfVariable(property);

    // If it's a method type, it should only be getter/setter, not regular methods
    if (propertyType->IsETSMethodType()) {
        return IsMethodOnlyAccessors(propertyType);
    }

    return true;
}

// Helper to check if all required interface properties are satisfied
static bool AreAllRequiredInterfacePropertiesSatisfied(ir::ObjectExpression *expr,
                                                       checker::ETSObjectType *interfaceType, ETSChecker *checker)
{
    // Get all properties of the interface using GetAllProperties
    auto allProperties = interfaceType->GetAllProperties();

    // Create a set of property names provided in the object literal
    std::unordered_set<std::string_view> literalProperties;
    for (ir::Expression *propExpr : expr->Properties()) {
        if (propExpr->IsProperty()) {
            ir::Expression *key = propExpr->AsProperty()->Key();
            if (auto optPname = GetPropertyNameFromKey(key); optPname.has_value()) {
                literalProperties.insert(optPname.value().Utf8());
            }
        }
    }

    // Check if all literal properties exist in this interface
    for (const auto &litPropName : literalProperties) {
        bool found = false;
        for (auto *property : allProperties) {
            if (property->Name().Utf8() == litPropName) {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }

    // Check that all required interface properties are satisfied
    for (auto *property : allProperties) {
        auto *propertyType = checker->GetTypeOfVariable(property);

        // Skip method types that aren't getters/setters (they make interface incompatible anyway)
        if (propertyType->IsETSMethodType()) {
            if (!IsMethodOnlyAccessors(propertyType)) {
                // Regular methods not allowed
                return false;
            }
        }
        // Check if this property is provided in the literal
        bool isInLiteral = literalProperties.find(property->Name().Utf8()) != literalProperties.end();
        if (!isInLiteral) {
            // Property not in literal - check if it's optional or has default value
            bool isOptional = IsPropertyOptional(property, propertyType);
            bool hasDefaultValue = HasPropertyDefaultValue(property);
            if (!isOptional && !hasDefaultValue) {
                return false;
            }
        }
    }

    return true;  // All required properties are satisfied
}

static bool IsObjectTypeCompatibleWithLiteral(ETSChecker *checker, ir::ObjectExpression *expr,
                                              checker::ETSObjectType *potentialObjType)
{
    // Split record/map types, class types and interfaces as requested by reviewer

    checker::ETSObjectType *originalBaseType = potentialObjType->GetOriginalBaseType();
    checker::GlobalTypesHolder *globalTypes = checker->GetGlobalTypesHolder();

    // Handle Record/Map types
    if (checker->IsTypeIdenticalTo(originalBaseType, globalTypes->GlobalMapBuiltinType()) ||
        checker->IsTypeIdenticalTo(originalBaseType, globalTypes->GlobalRecordBuiltinType())) {
        // Maps and Records are always compatible with object literals
        return true;
    }

    // Handle interface types
    if (potentialObjType->HasObjectFlag(checker::ETSObjectFlags::INTERFACE)) {
        // For interface types, check that all literal properties exist in the interface
        // and that interface has no regular methods (only getters/setters allowed)

        // For non-empty literals, check that all literal properties exist in interface
        // and all required interface properties are satisfied
        for (ir::Expression *propExpr : expr->Properties()) {
            if (!IsInterfacePropertyCompatible(propExpr, potentialObjType, checker)) {
                return false;
            }
        }

        // Check all required interface properties are satisfied
        return AreAllRequiredInterfacePropertiesSatisfied(expr, potentialObjType, checker);
    }

    // Handle class types
    // For class types, you need to check:
    // - that there is a parameterless constructor, and
    // - that all fields/properties set in the object literal are present in the class

    if (!HasParameterlessConstructor(potentialObjType, checker, expr->Start())) {
        return false;
    }

    // Check that all properties in literal exist in class
    return CheckObjectLiteralCompatibility(expr, potentialObjType);
}

static checker::ETSObjectType *ResolveObjectTypeFromPreferredType(ir::ObjectExpression *expr)
{
    // Assume not null, checked by caller in Check()
    checker::Type *preferredType = expr->PreferredType();

    if (preferredType->IsETSObjectType()) {
        return preferredType->AsETSObjectType();
    }

    return nullptr;
}

// Helper to handle interface type objects
static checker::Type *HandleInterfaceType(ETSChecker *checker, ir::ObjectExpression *expr,
                                          checker::ETSObjectType *objType)
{
    auto *analyzer = static_cast<checker::ETSAnalyzer *>(checker->GetAnalyzer());
    analyzer->CheckObjectExprProps(
        expr, objType,
        checker::PropertySearchFlags::SEARCH_INSTANCE_FIELD | checker::PropertySearchFlags::SEARCH_INSTANCE_METHOD |
            checker::PropertySearchFlags::SEARCH_INSTANCE_DECL | checker::PropertySearchFlags::SEARCH_IN_INTERFACES);
    expr->SetTsType(objType);
    return objType;
}

// Helper to handle Record/Map types
static checker::Type *HandleRecordOrMapType(ETSChecker *checker, ir::ObjectExpression *expr,
                                            checker::ETSObjectType *objType)
{
    expr->SetTsType(objType);
    SetTypeforRecordProperties(expr, objType, checker);
    return objType;
}

static bool IsMapOrRecordBuiltinType(checker::ETSChecker *checker, checker::ETSObjectType *objType)
{
    checker::ETSObjectType *originalBaseObjType = objType->GetOriginalBaseType();
    checker::GlobalTypesHolder *globalTypes = checker->GetGlobalTypesHolder();
    return checker->IsTypeIdenticalTo(originalBaseObjType, globalTypes->GlobalMapBuiltinType()) ||
           checker->IsTypeIdenticalTo(originalBaseObjType, globalTypes->GlobalRecordBuiltinType());
}

checker::Type *ETSAnalyzer::CheckObjectExprBaseOnObjectType(ir::ObjectExpression *expr,
                                                            checker::ETSObjectType *objType) const
{
    ETSChecker *checker = GetETSChecker();
    if (objType == nullptr) {
        if (!expr->PreferredType()->IsETSUnionType()) {
            checker->LogError(diagnostic::CLASS_COMPOSITE_INVALID_TARGET, {expr->PreferredType()}, expr->Start());
        }
        return checker->GlobalTypeError();
    }

    if (objType->HasObjectFlag(checker::ETSObjectFlags::INTERFACE)) {
        return HandleInterfaceType(checker, expr, objType);
    }

    if (IsMapOrRecordBuiltinType(checker, objType)) {
        return HandleRecordOrMapType(checker, expr, objType);
    }

    // If we reach here, objType is a class. It must have a parameterless constructor
    if (!HasParameterlessConstructor(objType, checker, expr->Start())) {
        expr->SetTsType(checker->TypeError(expr, diagnostic::NO_PARAMLESS_CTOR, {objType->Name()}, expr->Start()));
        return expr->TsType();
    }

    CheckObjectExprProps(expr, objType,
                         checker::PropertySearchFlags::SEARCH_INSTANCE_FIELD |
                             checker::PropertySearchFlags::SEARCH_IN_BASE |
                             checker::PropertySearchFlags::SEARCH_INSTANCE_METHOD);
    return objType;
}

static checker::Type *TryFindPublicBaseType(checker::ETSChecker *checker,
                                            const std::vector<checker::ETSObjectType *> &matchingObjectTypes)
{
    // Note: after we remove the type normalization between Base type and Derived type at the Union creating side, we
    // need to do some modification at type using side. if we got `Father | ... | Son_x` from the context, then just
    // return the base type as the specific type.
    ES2PANDA_ASSERT(matchingObjectTypes.size() > 1);
    Type *candidateBaseType = matchingObjectTypes.front();
    for (auto *tp : matchingObjectTypes) {
        if (checker->Relation()->IsSupertypeOf(tp, candidateBaseType)) {
            candidateBaseType = tp;
        }
    }

    for (auto *tp : matchingObjectTypes) {
        if (!checker->Relation()->IsSupertypeOf(candidateBaseType, tp)) {
            return nullptr;
        }
    }

    return candidateBaseType;
}

checker::Type *ETSAnalyzer::CheckObjectExprBaseOnUnionType(ir::ObjectExpression *expr,
                                                           checker::ETSUnionType *preferredType) const
{
    ETSChecker *checker = GetETSChecker();
    std::vector<checker::ETSObjectType *> candidateObjectTypes;
    // Phase 1: Gather all ETSObjectTypes from the union
    for (auto *constituentType : preferredType->ConstituentTypes()) {
        if (constituentType->IsETSObjectType()) {
            candidateObjectTypes.push_back(constituentType->AsETSObjectType());
        }
    }

    std::vector<checker::ETSObjectType *> matchingObjectTypes;
    // Phase 2: Filter candidates using the helper function
    for (auto *potentialObjType : candidateObjectTypes) {
        if (!IsObjectTypeCompatibleWithLiteral(checker, expr, potentialObjType)) {
            continue;
        }

        expr->SetPreferredType(potentialObjType);
        InferMatchContext specificTypeMatchCtx(checker, util::DiagnosticType::SEMANTIC, expr->Range(), false);
        CheckObjectExprBaseOnObjectType(expr, potentialObjType);
        if (IsMapOrRecordBuiltinType(checker, potentialObjType)) {
            checker->CheckRecordType(expr, potentialObjType);
        }

        if (specificTypeMatchCtx.ValidMatchStatus()) {
            matchingObjectTypes.emplace_back(potentialObjType);
        }
        expr->CleanCheckInformation();
        expr->SetPreferredType(preferredType);
    }

    // Phase 3: Decide based on number of matches
    if (matchingObjectTypes.empty()) {
        // No candidate ETSObjectType from the union matched all properties
        checker->LogError(diagnostic::CLASS_COMPOSITE_INVALID_TARGET, {expr->PreferredType()}, expr->Start());
        return checker->GlobalTypeError();
    }

    if (matchingObjectTypes.size() > 1) {
        if (auto *specificType = TryFindPublicBaseType(checker, matchingObjectTypes); specificType != nullptr) {
            expr->SetPreferredType(specificType);
            CheckObjectExprBaseOnObjectType(expr, specificType->AsETSObjectType());
            return specificType;
        }
        // Ambiguous
        checker->LogError(diagnostic::AMBIGUOUS_REFERENCE, {expr->PreferredType()->ToString()}, expr->Start());
        return checker->GlobalTypeError();
    }
    expr->SetPreferredType(matchingObjectTypes.front());
    CheckObjectExprBaseOnObjectType(expr, matchingObjectTypes.front()->AsETSObjectType());
    return matchingObjectTypes.front();
}

checker::Type *ETSAnalyzer::Check(ir::ObjectExpression *expr) const
{
    ETSChecker *checker = GetETSChecker();
    if (expr->TsType() != nullptr) {
        return expr->TsType();
    }

    if (expr->PreferredType() == nullptr) {
        checker->LogError(diagnostic::CLASS_COMPOSITE_UNKNOWN_TYPE, {}, expr->Start());
        expr->SetTsType(checker->GlobalTypeError());
        return expr->TsType();
    }
    auto actualPreferredType = expr->PreferredType();
    if (actualPreferredType->IsETSTypeAliasType()) {
        actualPreferredType = actualPreferredType->AsETSTypeAliasType()->GetTargetType();
        expr->SetPreferredType(actualPreferredType);
    }

    if (!expr->PreferredType()->IsETSUnionType() && !ValidatePreferredType(checker, expr)) {
        expr->SetTsType(checker->GlobalTypeError());
        return expr->TsType();
    }

    checker::Type *tsType = nullptr;
    if (actualPreferredType->IsETSUnionType()) {
        tsType = CheckObjectExprBaseOnUnionType(expr, actualPreferredType->AsETSUnionType());
    } else {
        tsType = CheckObjectExprBaseOnObjectType(expr, ResolveObjectTypeFromPreferredType(expr));
    }

    expr->SetTsType(tsType);
    return tsType;
}

static void CollectNonOptionalPropertyInterface(checker::ETSChecker *checker, const ETSObjectType *objType,
                                                std::unordered_map<util::StringView, ETSObjectType *> &props)
{
    // Note: all the properties of an interface will be lowered as accessor before checker.
    auto const &methodMap = objType->InstanceMethods();
    for (const auto &[propName, var] : methodMap) {
        if (!checker->IsVariableGetterSetter(var)) {
            continue;
        }

        auto propertyType = checker->GetTypeOfVariable(var);
        if (propertyType->IsTypeError()) {
            // Note: error handle later.
            continue;
        }

        if (var->Declaration()->Node()->IsOptionalDeclaration()) {
            // optional properties
            continue;
        }
        props.insert({propName, const_cast<ETSObjectType *>(objType)});
    }
}

static void CollectLateInitPropertyClass(const ETSObjectType *objType,
                                         std::unordered_map<util::StringView, ETSObjectType *> &props)
{
    auto const &fields = objType->InstanceFields();
    for (const auto &[propName, var] : fields) {
        if (!var->Declaration()->Node()->IsDefinite()) {
            continue;
        }
        props.insert({propName, const_cast<ETSObjectType *>(objType)});
    }
}

void ETSAnalyzer::CollectNonOptionalProperty(const ETSObjectType *objType,
                                             std::unordered_map<util::StringView, ETSObjectType *> &props) const
{
    ETSChecker *checker = GetETSChecker();
    if (objType->HasObjectFlag(ETSObjectFlags::INTERFACE)) {
        CollectNonOptionalPropertyInterface(checker, objType, props);
        for (auto const *superInterface : objType->Interfaces()) {
            CollectNonOptionalProperty(superInterface, props);
        }
    }

    if (objType->HasObjectFlag(ETSObjectFlags::CLASS)) {
        CollectLateInitPropertyClass(objType, props);
        if (objType->SuperType() != nullptr) {
            CollectNonOptionalProperty(objType->SuperType(), props);
        }
    }
}

static std::optional<util::StringView> GetNameForProperty(ETSChecker *checker, ir::Expression *const propExpr) noexcept
{
    ir::Expression const *const key = propExpr->AsProperty()->Key();
    if (key->IsStringLiteral()) {
        checker->LogDiagnostic(diagnostic::CLASS_COMPOSITE_KEY_USE_STRING, {}, propExpr->Start());
        return std::make_optional(key->AsStringLiteral()->Str());
    }

    if (key->IsIdentifier()) {
        return std::make_optional(key->AsIdentifier()->Name());
    }

    checker->LogError(diagnostic::CLASS_COMPOSITE_INVALID_KEY, {}, propExpr->Start());
    propExpr->SetTsType(checker->GlobalTypeError());

    return std::nullopt;
}

//  Helper function extracted from 'ETSAnalyzer::IsPropertyAssignable(...)' to reduce its size
static bool IsMethodPropertyAssignable(ETSChecker *const checker, std::string_view const propertyName,
                                       Type const *const propertyType, ir::Expression *const value)
{
    ES2PANDA_ASSERT(propertyType->IsETSMethodType() && !propertyType->AsETSFunctionType()->CallSignatures().empty());

    Type *const valueType = value->TsType();
    if (!valueType->IsETSArrowType()) {
        checker->LogError(diagnostic::PROP_INCOMPAT, {valueType, propertyType, propertyName}, value->Start());
        return false;
    }

    auto *const relation = checker->Relation();
    auto *const sourceSignature = valueType->AsETSFunctionType()->CallSignaturesOfMethodOrArrow()[0U];
    std::string methodType {};

    for (auto *const targetSignature : propertyType->AsETSFunctionType()->CallSignatures()) {
        if (propertyName != targetSignature->Function()->Id()->Name().Utf8()) {
            continue;
        }

        if (relation->CheckTypeParameterConstraints(sourceSignature->TypeParams(), targetSignature->TypeParams())) {
            auto *const substSignature = checker->AdjustForTypeParameters(sourceSignature, targetSignature);

            SavedTypeRelationFlagsContext savedFlagsCtx(relation, TypeRelationFlag::OVERRIDING_CONTEXT);
            if (relation->SignatureIsSupertypeOf(substSignature, sourceSignature)) {
                return true;
            }
        }
        methodType += targetSignature->ToString() + " | ";
    }

    methodType.resize(methodType.size() - 3U);
    checker->LogError(diagnostic::PROP_INCOMPAT, {sourceSignature->ToString(), methodType, propertyName},
                      value->Start());

    return false;
}

static bool ReportInterfaceAccessorMismatchIfNeeded(ETSChecker *const checker, ir::Expression *const propExpr,
                                                    varbinder::LocalVariable *const lv,
                                                    const util::StringView &propertyName,
                                                    ETSObjectType const *const objectType)
{
    if (!objectType->HasObjectFlag(ETSObjectFlags::INTERFACE)) {
        return false;
    }

    if (lv->TsType() == nullptr) {
        checker->GetTypeOfVariable(lv);
    }

    auto *const lvType = lv->TsType();
    if (lvType != nullptr && lvType->IsETSFunctionType() &&
        checker->ReportInterfaceAccessorTypeMismatchIfNeeded(lvType->AsETSFunctionType(), propertyName,
                                                             propExpr->Start())) {
        propExpr->SetTsType(checker->GlobalTypeError());
        return true;
    }

    return false;
}

static bool ReportReadonlyClassPropertyAssignmentIfNeeded(ETSChecker *const checker, ir::Expression *const propExpr,
                                                          varbinder::LocalVariable *const lv,
                                                          ETSObjectType const *const objectType)
{
    if (objectType->HasObjectFlag(ETSObjectFlags::INTERFACE)) {
        return false;
    }

    auto *const decl = lv->Declaration();
    if (decl == nullptr || decl->Node() == nullptr || !decl->Node()->IsClassProperty()) {
        return false;
    }

    auto *const classProp = decl->Node()->AsClassProperty();
    if (!classProp->IsReadonly()) {
        return false;
    }

    checker->LogError(diagnostic::FIELD_ASSIGN_TO_READONLY, {lv->Name()}, propExpr->Start());
    propExpr->SetTsType(checker->GlobalTypeError());
    return true;
}

static bool ReportObjectLiteralMethodRedefinitionIfNeeded(ETSChecker *const checker, ir::Expression *const propExpr,
                                                          Type *const propType, ETSObjectType const *const objectType)
{
    if (!propType->IsETSMethodType() ||
        objectType->HasObjectFlag(ETSObjectFlags::INTERFACE | ETSObjectFlags::ABSTRACT)) {
        return false;
    }

    checker->LogError(diagnostic::OBJECT_LITERAL_METHOD_KEY, {}, propExpr->Start());
    propExpr->SetTsType(checker->GlobalTypeError());
    return true;
}

static bool IsPropertyAssignable(ETSChecker *const checker, ir::Expression *const propExpr,
                                 varbinder::LocalVariable *const lv, const util::StringView &pname,
                                 ETSObjectType const *const objectType)
{
    if (ReportInterfaceAccessorMismatchIfNeeded(checker, propExpr, lv, pname, objectType)) {
        return false;
    }

    if (ReportReadonlyClassPropertyAssignmentIfNeeded(checker, propExpr, lv, objectType)) {
        return false;
    }

    auto *propType = checker->GetTypeOfVariable(lv);

    if (auto *setterType = GetSetterType(lv, checker); setterType != nullptr) {
        propType = setterType;
    }

    propExpr->SetTsType(propType);

    ir::Expression *key = propExpr->AsProperty()->Key();
    key->SetTsType(propType);

    ir::Expression *value = propExpr->AsProperty()->Value();
    value->SetPreferredType(propType);

    if (ReportObjectLiteralMethodRedefinitionIfNeeded(checker, propExpr, propType, objectType)) {
        return false;
    }

    Type *const valueType = value->Check(checker);
    bool assignable;
    if (!propType->IsETSMethodType()) {
        assignable = checker::AssignmentContext(checker->Relation(), value, valueType, propType, value->Start(),
                                                {{diagnostic::PROP_INCOMPAT, {valueType, propType, pname}}})
                         // CC-OFFNXT(G.FMT.06-CPP) project code style
                         .IsAssignable();
    } else {
        assignable = IsMethodPropertyAssignable(checker, pname.Utf8(), propType, value);
    }

    if (!assignable) {
        propExpr->SetTsType(checker->GlobalTypeError());
        return false;
    }

    return true;
}

static void CheckObjectExprPropsHelper(ETSChecker *const checker, const ir::ObjectExpression *expr,
                                       checker::ETSObjectType *objType, checker::PropertySearchFlags const searchFlags,
                                       std::unordered_map<util::StringView, ETSObjectType *> &properties)
{
    for (ir::Expression *propExpr : expr->Properties()) {
        if (!propExpr->IsProperty()) {
            checker->LogError(diagnostic::OBJECT_LITERAL_NOT_KV, {}, expr->Start());
            propExpr->SetTsType(checker->GlobalTypeError());
            continue;
        }

        std::optional<util::StringView> propertyName = GetNameForProperty(checker, propExpr);
        if (!propertyName.has_value()) {
            continue;
        }

        varbinder::LocalVariable *lv = objType->GetProperty(*propertyName, searchFlags);
        if (lv == nullptr) {
            checker->LogError(diagnostic::UNDEFINED_PROPERTY, {objType->Name(), *propertyName}, propExpr->Start());
            propExpr->SetTsType(checker->GlobalTypeError());
            continue;
        }
        // The property is present in the literal, so it must not be reported as "missing required" later.
        properties.erase(*propertyName);

        if (ir::Expression *key = propExpr->AsProperty()->Key(); key->IsIdentifier()) {
            key->AsIdentifier()->SetVariable(lv);
        }

        checker->ValidatePropertyAccess(lv, objType, propExpr);
        if (IsTypeError(lv->TsType())) {
            propExpr->SetTsType(checker->GlobalTypeError());
            continue;
        }

        if (!IsPropertyAssignable(checker, propExpr, lv, *propertyName, objType)) {
            continue;
        }
    }
}

void ETSAnalyzer::CheckObjectExprProps(const ir::ObjectExpression *expr,
                                       checker::ETSObjectType *objectTypeForProperties,
                                       checker::PropertySearchFlags searchFlags) const
{
    ETSChecker *checker = GetETSChecker();
    checker::ETSObjectType *objType = objectTypeForProperties;
    if (objType->IsGlobalETSObjectType() && !expr->Properties().empty()) {
        checker->LogError(diagnostic::ERROR_ARKTS_NO_UNTYPED_OBJ_LITERALS, expr->Start());
    }

    std::unordered_map<util::StringView, ETSObjectType *> propertyWithNonOptionalType;

    CollectNonOptionalProperty(objType, propertyWithNonOptionalType);

    CheckObjectExprPropsHelper(checker, expr, objType, searchFlags, propertyWithNonOptionalType);

    for (const auto &[propName, ownerType] : propertyWithNonOptionalType) {
        if (objType == ownerType) {
            checker->LogError(diagnostic::OBJECT_LITERAL_NON_OPTIONAL_PROP_LOST, {propName, objType}, expr->Start());
        } else {
            checker->LogError(diagnostic::OBJECT_LITERAL_NON_OPTIONAL_PROP_OF_SUPER_LOST,
                              {propName, ownerType, objType}, expr->Start());
        }
    }

    if (objType->HasObjectFlag(ETSObjectFlags::REQUIRED)) {
        checker->ValidateObjectLiteralForRequiredType(objType, expr);
    }
}

checker::Type *ETSAnalyzer::Check(ir::OpaqueTypeNode *expr) const
{
    return expr->TsType();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::BrokenTypeNode *expr) const
{
    return GetETSChecker()->GlobalTypeError();
}

checker::Type *ETSAnalyzer::Check(ir::SequenceExpression *expr) const
{
    ETSChecker *checker = GetETSChecker();
    if (expr->TsType() != nullptr) {
        return expr->TsType();
    }

    for (auto *it : expr->Sequence()) {
        it->Check(checker);
    }
    ES2PANDA_ASSERT(!expr->Sequence().empty());
    return expr->SetTsType(expr->Sequence().back()->TsType());
}

checker::Type *ETSAnalyzer::Check(ir::SuperExpression *expr) const
{
    ETSChecker *checker = GetETSChecker();
    if (expr->TsType() != nullptr) {
        return expr->TsType();
    }

    return expr->SetTsType(
        checker->CheckThisOrSuperAccess(expr, checker->Context().ContainingClass()->SuperType(), "super"));
}

checker::Type *ETSAnalyzer::Check(ir::TemplateLiteral *expr) const
{
    ETSChecker *checker = GetETSChecker();

    for (auto *it : expr->Expressions()) {
        it->Check(checker);
    }

    if (expr->TsType() != nullptr) {
        return expr->TsType();
    }

    if (expr->Quasis().size() != expr->Expressions().size() + 1U) {
        checker->LogError(diagnostic::TEMPLATE_COUNT_MISMATCH, {}, expr->Start());
        return expr->SetTsType(checker->GlobalTypeError());
    }

    for (auto *it : expr->Quasis()) {
        it->Check(checker);
    }

    return expr->SetTsType(checker->CreateETSStringLiteralType(expr->GetMultilineString()));
}

checker::Type *ETSAnalyzer::Check(ir::ThisExpression *expr) const
{
    ETSChecker *checker = GetETSChecker();
    if (expr->TsType() != nullptr) {
        return expr->TsType();
    }

    /*
    example code:
    ```
        class A {
            prop
        }
        function A.method() {
            let a = () => {
                console.println(this.prop)
            }
        }
        is identical to
        function method(this: A) {
            let a = () => {
                console.println(this.prop)
            }
        }
    ```
    here when "this" is used inside an extension function, we need to bind "this" to the first
    parameter(MANDATORY_PARAM_THIS), and capture the parameter's variable other than containing class's variable
    */
    auto *variable = checker->AsETSChecker()->Scope()->Find(varbinder::VarBinder::MANDATORY_PARAM_THIS).variable;
    if (checker->HasStatus(checker::CheckerStatus::IN_EXTENSION_METHOD)) {
        ES2PANDA_ASSERT(variable != nullptr);
        expr->SetTsType(variable->TsType());
    } else {
        auto *thisContextType = checker->Context().ContainingClass();
        if (auto *objectLiteralThisType = GetObjectLiteralMethodThisType(expr); objectLiteralThisType != nullptr) {
            // In object literal methods, ArkTS spec requires `this` to be the object literal type.
            thisContextType = objectLiteralThisType;
        }
        expr->SetTsType(checker->CheckThisOrSuperAccess(expr, thisContextType, "this"));
    }

    return expr->TsType();
}

// NOLINTNEXTLINE(readability-identifier-naming)
static checker::Type *checkUnboxedTypeKind(TypeFlag unboxedFlag, ETSChecker *checker)
{
    switch (unboxedFlag) {
        case TypeFlag::ETS_BOOLEAN:
            return checker->CreateETSStringLiteralType("boolean");
        case TypeFlag::BYTE:
            return checker->CreateETSStringLiteralType("byte");
        case TypeFlag::CHAR:
            return checker->CreateETSStringLiteralType("char");
        case TypeFlag::SHORT:
            return checker->CreateETSStringLiteralType("short");
        case TypeFlag::INT:
            return checker->CreateETSStringLiteralType("int");
        case TypeFlag::LONG:
            return checker->CreateETSStringLiteralType("long");
        case TypeFlag::FLOAT:
            return checker->CreateETSStringLiteralType("float");
        case TypeFlag::DOUBLE:
            return checker->CreateETSStringLiteralType("number");
        case TypeFlag::ETS_VOID:
            return checker->CreateETSStringLiteralType("undefined");
        default:
            ES2PANDA_UNREACHABLE();
    }
}

// Get string literal type as potential typeof result type with respect to spec p.7.17
static checker::Type *GetTypeOfStringType(checker::Type *argType, ETSChecker *checker)
{
    if (auto unboxed = checker->MaybeUnboxType(argType); unboxed->IsETSPrimitiveType()) {
        return checkUnboxedTypeKind(checker->TypeKind(unboxed), checker);
    }
    if (argType->IsETSUndefinedType()) {
        return checker->CreateETSStringLiteralType("undefined");
    }
    if (argType->IsETSArrayType() || argType->IsETSNullType() || argType->IsETSResizableArrayType()) {
        return checker->CreateETSStringLiteralType("object");
    }
    if (argType->IsETSStringType()) {
        return checker->CreateETSStringLiteralType("string");
    }
    if (argType->IsETSBigIntType()) {
        return checker->CreateETSStringLiteralType("bigint");
    }
    if (argType->IsETSFunctionType()) {
        return checker->CreateETSStringLiteralType("function");
    }
    if (argType->IsETSNumericEnumType()) {
        auto unboxedType = checker->MaybeUnboxType(argType->AsETSEnumType()->Underlying());
        return checkUnboxedTypeKind(checker->TypeKind(unboxedType), checker);
    }
    if (argType->IsETSStringEnumType()) {
        return checker->CreateETSStringLiteralType("string");
    }
    return checker->GlobalBuiltinETSStringType();
}

static checker::Type *ComputeTypeOfType(ETSChecker *checker, checker::Type *argType)
{
    checker::Type *ret = nullptr;
    std::vector<checker::Type *> types;
    ES2PANDA_ASSERT(argType != nullptr);
    if (argType->IsETSUnionType()) {
        for (auto *it : argType->AsETSUnionType()->ConstituentTypes()) {
            checker::Type *elType = ComputeTypeOfType(checker, it);
            types.push_back(elType);
        }
        ret = checker->CreateETSUnionType(std::move(types));
    } else {
        ret = GetTypeOfStringType(argType, checker);
    }
    return ret;
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::TypeofExpression *expr) const
{
    ETSChecker *checker = GetETSChecker();
    if (expr->TsType() != nullptr) {
        return expr->TsType();
    }

    auto argType = expr->Argument()->Check(checker);
    if (argType->IsTypeError()) {
        return expr->SetTsType(checker->GlobalTypeError());
    }

    return expr->SetTsType(ComputeTypeOfType(checker, argType));
}

checker::Type *ETSAnalyzer::Check(ir::UnaryExpression *expr) const
{
    ETSChecker *checker = GetETSChecker();

    if (expr->TsType() != nullptr) {
        return expr->TsType();
    }

    auto argType = expr->argument_->Check(checker);
    const auto isCondExpr = expr->OperatorType() == lexer::TokenType::PUNCTUATOR_EXCLAMATION_MARK;
    checker::Type *operandType = checker->ApplyUnaryOperatorPromotion(expr->argument_, argType, isCondExpr);
    if (argType != nullptr && argType->IsETSBigIntType() && argType->HasTypeFlag(checker::TypeFlag::BIGINT_LITERAL)) {
        switch (expr->OperatorType()) {
            case lexer::TokenType::PUNCTUATOR_MINUS: {
                auto *type = checker->GlobalETSBigIntType();
                ES2PANDA_ASSERT(type != nullptr);
                // Unary minus is evaluated at runtime, so the result is the regular BigInt builtin type.
                expr->argument_->SetTsType(type);
                expr->SetTsType(type);
                return expr->TsType();
            }
            default:
                // Handled below
                // NOTE(kkonsw): handle other unary operators for bigint literals
                break;
        }
    }

    if (argType != nullptr && argType->IsETSBigIntType()) {
        switch (expr->OperatorType()) {
            case lexer::TokenType::PUNCTUATOR_MINUS:
            case lexer::TokenType::PUNCTUATOR_PLUS:
            case lexer::TokenType::PUNCTUATOR_TILDE: {
                expr->SetTsType(argType);
                return expr->TsType();
            }
            default:
                break;
        }
    }

    SetTsTypeForUnaryExpression(checker, expr, operandType);

    checker->Context().CheckUnarySmartCastCondition(expr);

    return expr->TsType();
}

checker::Type *ETSAnalyzer::Check(ir::UpdateExpression *expr) const
{
    ETSChecker *checker = GetETSChecker();
    if (expr->TsType() != nullptr) {
        return expr->TsType();
    }

    checker::Type *operandType = expr->argument_->Check(checker);
    FORWARD_TYPE_ERROR(checker, operandType, expr);

    if (expr->Argument()->IsIdentifier()) {
        checker->ValidateUnaryOperatorOperand(expr->Argument()->AsIdentifier()->Variable(), expr);
    } else if (expr->Argument()->IsTSAsExpression()) {
        if (auto *const asExprVar = expr->Argument()->AsTSAsExpression()->Variable(); asExprVar != nullptr) {
            checker->ValidateUnaryOperatorOperand(asExprVar, expr);
        }
    } else if (expr->Argument()->IsTSNonNullExpression()) {
        if (auto *const nonNullExprVar = expr->Argument()->AsTSNonNullExpression()->Variable();
            nonNullExprVar != nullptr) {
            checker->ValidateUnaryOperatorOperand(nonNullExprVar, expr);
        }
    } else if (expr->Argument()->IsMemberExpression()) {
        varbinder::LocalVariable *propVar = expr->argument_->AsMemberExpression()->PropVar();
        if (propVar != nullptr) {
            checker->ValidateUnaryOperatorOperand(propVar, expr);
        }
    } else {
        ES2PANDA_ASSERT(checker->IsAnyError());
        expr->Argument()->SetTsType(checker->GlobalTypeError());
        return expr->SetTsType(checker->GlobalTypeError());
    }

    if (operandType->IsETSBigIntType()) {
        return expr->SetTsType(operandType);
    }

    auto unboxedType = checker->MaybeUnboxInRelation(operandType);
    if (unboxedType == nullptr || !unboxedType->HasTypeFlag(checker::TypeFlag::ETS_CONVERTIBLE_TO_NUMERIC)) {
        checker->LogError(diagnostic::OPERAND_NOT_NUMERIC, {}, expr->Argument()->Start());
        return expr->SetTsType(checker->GlobalTypeError());
    }

    return expr->SetTsType(operandType);
}

// compile methods for LITERAL EXPRESSIONS in alphabetical order
checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::BigIntLiteral *expr) const
{
    ETSChecker *checker = GetETSChecker();
    expr->SetTsType(checker->CreateETSBigIntLiteralType(expr->Str()));
    return expr->TsType();
}

checker::Type *ETSAnalyzer::Check(ir::BooleanLiteral *expr) const
{
    ETSChecker *checker = GetETSChecker();
    if (expr->TsType() == nullptr) {
        expr->SetTsType(checker->GetConstantBuiltinType(checker->GlobalETSBooleanBuiltinType()));
    }
    return expr->TsType();
}

checker::Type *ETSAnalyzer::Check(ir::CharLiteral *expr) const
{
    ETSChecker *checker = GetETSChecker();
    if (expr->TsType() == nullptr) {
        expr->SetTsType(checker->GetConstantBuiltinType(checker->GlobalCharBuiltinType()));
    }
    return expr->TsType();
}

checker::Type *ETSAnalyzer::Check(ir::NullLiteral *expr) const
{
    ETSChecker *checker = GetETSChecker();
    if (expr->TsType() == nullptr) {
        expr->SetTsType(checker->GlobalETSNullType());
    }
    return expr->TsType();
}

static bool CheckIfLiteralValueIsAppropriate(ETSChecker *checker, Type *type, ir::NumberLiteral *expr)
{
    auto number = expr->Number();
    auto relation = checker->Relation();
    if (relation->IsSupertypeOf(checker->GetGlobalTypesHolder()->GlobalIntegralBuiltinType(), type)) {
        if (number.IsReal()) {
            return false;
        }
        auto val = number.GetValueAndCastTo<int64_t>();
        if (relation->IsIdenticalTo(type, checker->GlobalByteBuiltinType())) {
            return val >= std::numeric_limits<int8_t>::min() && val <= std::numeric_limits<int8_t>::max();
        }
        if (relation->IsIdenticalTo(type, checker->GlobalShortBuiltinType())) {
            return val >= std::numeric_limits<int16_t>::min() && val <= std::numeric_limits<int16_t>::max();
        }
        if (relation->IsIdenticalTo(type, checker->GlobalIntBuiltinType())) {
            return val >= std::numeric_limits<int32_t>::min() && val <= std::numeric_limits<int32_t>::max();
        }
    } else if (relation->IsIdenticalTo(type, checker->GlobalCharBuiltinType())) {
        auto val = number.GetValueAndCastTo<int64_t>();
        return !number.IsReal() && val >= std::numeric_limits<uint16_t>::min() &&
               val <= std::numeric_limits<uint16_t>::max();
    } else if (number.IsDouble()) {
        if (relation->IsIdenticalTo(type, checker->GlobalFloatBuiltinType())) {
            auto doubleVal = number.GetDouble();
            if (doubleVal < std::numeric_limits<float>::min() || doubleVal > std::numeric_limits<float>::max()) {
                return false;
            }
            auto floatVal = static_cast<float>(doubleVal);
            return static_cast<double>(floatVal) == doubleVal;
        }
        return relation->IsIdenticalTo(checker->GlobalDoubleBuiltinType(), type);
    }
    return true;
}

checker::Type *ETSAnalyzer::Check(ir::NumberLiteral *expr) const
{
    if (expr->TsType() != nullptr) {
        return expr->TsType();
    }

    ETSChecker *checker = GetETSChecker();
    Type *type;

    if (auto *preferredType =
            GetAppropriatePreferredType(expr->PreferredType(), [&](Type *tp) { return checker->CheckIfNumeric(tp); });
        preferredType != nullptr && CheckIfLiteralValueIsAppropriate(checker, preferredType, expr)) {
        type = preferredType;
    } else if (expr->Number().IsDouble()) {
        type = checker->GlobalDoubleBuiltinType();
    } else if (expr->Number().IsFloat()) {
        type = checker->GlobalFloatBuiltinType();
    } else if (expr->Number().IsLong()) {
        type = checker->GlobalLongBuiltinType();
    } else if (expr->Number().IsInt()) {
        type = checker->GlobalIntBuiltinType();
    } else if (expr->Number().IsShort()) {
        type = checker->GlobalShortBuiltinType();
    } else if (expr->Number().IsByte()) {
        type = checker->GlobalByteBuiltinType();
    } else {
        return checker->GlobalTypeError();
    }

    return expr->SetTsType(checker->GetConstantBuiltinType(type));
}

checker::Type *ETSAnalyzer::Check(ir::StringLiteral *expr) const
{
    ETSChecker *checker = GetETSChecker();
    if (expr->TsType() == nullptr) {
        expr->SetTsType(checker->CreateETSStringLiteralType(expr->Str()));
    }
    return expr->TsType();
}

static bool HasRealSourceLocation(const lexer::SourceRange &range)
{
    return range.start.line != 0 || range.start.index != 0 || range.end.line != 0 || range.end.index != 0;
}

static void ValidateImportTypeUsage(ETSChecker *checker, ir::ImportDeclaration *st, ir::AstNode *spec)
{
    // to prevent auto-generated codes which has invalid sourcePosition (0:0)
    if (!HasRealSourceLocation(spec->Range())) {
        return;
    }

    if (st->IsTypeKind() && spec->IsImportSpecifier()) {
        auto importSpec = spec->AsImportSpecifier();
        if (importSpec->Local()->IsIdentifier() && importSpec->Local()->AsIdentifier()->Variable() != nullptr) {
            auto var = importSpec->Local()->AsIdentifier()->Variable();
            if (var->Declaration() != nullptr && var->Declaration()->Node()->IsAnnotationDeclaration()) {
                checker->LogError(diagnostic::IMPORT_TYPE_NOT_ALLOWED, {}, spec->Start());
            }
        }
    }
}

checker::Type *ETSAnalyzer::Check(ir::ImportDeclaration *st) const
{
    ETSChecker *checker = GetETSChecker();
    checker::Type *type = nullptr;
    for (auto *spec : st->Specifiers()) {
        ValidateImportTypeUsage(checker, st, spec);
        if (spec->IsImportNamespaceSpecifier()) {
            type = spec->AsImportNamespaceSpecifier()->Check(checker);
        }
    }

    return type;
}

checker::Type *ETSAnalyzer::Check(ir::ImportNamespaceSpecifier *st) const
{
    ETSChecker *checker = GetETSChecker();
    if (st->Local()->Name().Empty()) {
        return ReturnTypeForStatement(st);
    }

    if (st->Local()->AsIdentifier()->TsType() != nullptr) {
        return st->Local()->TsType();
    }

    ir::ETSImportDeclaration *importDecl = nullptr;
    if (st->Parent()->IsETSImportDeclaration()) {
        importDecl = st->Parent()->AsETSImportDeclaration();
    } else if (st->Parent()->IsETSReExportDeclaration()) {
        importDecl = st->Parent()->AsETSReExportDeclaration()->GetETSImportDeclarations();
    } else {
        ES2PANDA_UNREACHABLE();
    }

    auto topScopeCtx = varbinder::TopScopeContext(checker->VarBinder(),
                                                  importDecl->Parent() != nullptr
                                                      ? importDecl->Parent()->AsETSModule()->Scope()->AsGlobalScope()
                                                      : checker->VarBinder()->GetScope()->AsGlobalScope());

    if (importDecl->IsPureDynamic()) {
        auto *type = checker->GetImportSpecifierObjectType(importDecl, st->Local()->AsIdentifier())->AsETSObjectType();
        checker->SetrModuleObjectTsType(st->Local(), type);
        return type;
    }

    return checker->GetImportSpecifierObjectType(importDecl, st->Local()->AsIdentifier());
}

// compile methods for STATEMENTS in alphabetical order
checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::AssertStatement *st) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check(ir::BlockStatement *st) const
{
    ETSChecker *checker = GetETSChecker();
    checker::ScopeContext scopeCtx(checker, st->Scope());

    // Iterator type checking of statements is modified to index type, to allow modifying the statement list during
    // checking without invalidating the iterator
    //---- Don't modify this to iterator, as it may break things during checking
    for (std::size_t idx = 0; idx < st->Statements().size(); ++idx) {
        auto *stmt = st->Statements()[idx];

        stmt->Check(checker);

        //  NOTE! Processing of trailing blocks was moved here so that smart casts could be applied correctly
        if (auto *const trailingBlock = st->SearchStatementInTrailingBlock(stmt); trailingBlock != nullptr) {
            // For now, an additional check is needed here to see whether there is a return stmt at top-level.
            // If there is, it is required to throw a CTE. However, it would be better to move the entire trailing
            // lambda handling into a lowering step, so this check could run as part of the regular return statement
            // validations.
            bool isReturnAllowed = !checker->HasStatus(CheckerStatus::RESTRICTED_RETURN_IN_BLOCK);
            if (checker->Context().ContainingSignature() == nullptr && isReturnAllowed) {
                checker->AddStatus(CheckerStatus::RESTRICTED_RETURN_IN_BLOCK);
            }

            trailingBlock->Check(checker);
            if (isReturnAllowed) {
                checker->RemoveStatus(CheckerStatus::RESTRICTED_RETURN_IN_BLOCK);
            }

            st->AddStatement(idx, trailingBlock);
            ++idx;
        }
    }
    if (UNLIKELY(checker->GetDebugInfoPlugin() != nullptr)) {
        // Compilation in eval-mode might require to create additional statements.
        // In this case, they must be created after iteration through statements ends.
        checker->GetDebugInfoPlugin()->AddPrologueEpilogue(st);
    }

    auto const *const scope = st->Scope();
    if (scope == nullptr) {
        return ReturnTypeForStatement(st);
    }

    //  Remove possible smart casts for variables declared in inner scope:
    if (scope->IsFunctionScope() && st->Parent()->Parent()->Parent()->IsMethodDefinition()) {
        // When exiting method definition, just clear all smart casts
        checker->Context().ClearSmartCasts();
    } else if (!scope->IsGlobalScope()) {
        // otherwise only check inner declarations
        for (auto const *const decl : scope->Decls()) {
            if (decl->IsLetOrConstDecl() && decl->Node() != nullptr && decl->Node()->IsIdentifier()) {
                checker->Context().RemoveSmartCast(decl->Node()->AsIdentifier()->Variable());
            }
        }
    }

    // Note: Guarantee all the const property need to be initialized in initializer block is initialized.
    if (st->IsETSModule() && st->AsETSModule()->Program()->Is<util::ModuleKind::PACKAGE>() &&
        (checker->Context().Status() & checker::CheckerStatus::IN_EXTERNAL) == 0) {
        CheckAllConstPropertyInitialized(checker, st->AsETSModule());
    }
    return ReturnTypeForStatement(st);
}

static void CheckJumpStatement(ir::AstNode *st, ETSChecker *checker)
{
    const ir::AstNode *target = nullptr;
    ir::Identifier *ident = nullptr;
    if (st->IsContinueStatement()) {
        target = st->AsContinueStatement()->Target();
        ident = st->AsContinueStatement()->Ident();
    } else {
        target = st->AsBreakStatement()->Target();
        ident = st->AsBreakStatement()->Ident();
    }

    // CTE if target is outside the function
    auto getEnclosingMethod = [](const ir::AstNode *node) {
        const ir::AstNode *enclosingMethod = node->Parent();
        while (enclosingMethod != nullptr && !enclosingMethod->IsMethodDefinition() &&
               !enclosingMethod->IsArrowFunctionExpression()) {
            enclosingMethod = enclosingMethod->Parent();
        }
        return enclosingMethod;
    };
    if (ident != nullptr && getEnclosingMethod(st) != getEnclosingMethod(target)) {
        checker->LogError(diagnostic::CONTINUE_OR_BREAK_TARGET_OUTSIDE_FUNCTION, {}, st->Start());
    }
}

checker::Type *ETSAnalyzer::Check(ir::BreakStatement *st) const
{
    ETSChecker *checker = GetETSChecker();

    if (!st->HasTarget()) {
        compiler::SetJumpTargetPhase setJumpTarget;
        setJumpTarget.FindJumpTarget(st);
    }

    if (st->Target() == nullptr) {
        return checker->GlobalTypeError();
    }

    CheckJumpStatement(st, checker);

    checker->Context().OnBreakStatement(st);
    return ReturnTypeForStatement(st);
}

checker::Type *ETSAnalyzer::Check(ir::ClassDeclaration *st) const
{
    ETSChecker *checker = GetETSChecker();
    st->Definition()->Check(checker);
    return ReturnTypeForStatement(st);
}

checker::Type *ETSAnalyzer::Check(ir::AnnotationDeclaration *st) const
{
    if (st->Expr()->TsType() != nullptr) {
        return ReturnTypeForStatement(st);
    }
    ETSChecker *checker = GetETSChecker();
    st->Expr()->Check(checker);

    if (st->HasAnnotations()) {
        for (auto *anno : st->Annotations()) {
            checker->CheckStandardAnnotation(anno);
            anno->Check(checker);
        }
    }

    ScopeContext scopeCtx(checker, st->Scope());
    for (auto *it : st->Properties()) {
        auto *property = it->AsClassProperty();
        if (checker::Type *propertyType = property->Check(checker); !propertyType->IsTypeError()) {
            checker->CheckAnnotationPropertyType(property);
        }
    }

    // Check if types used in properties are exported (for exported annotation declarations)
    if (st->IsExported() || st->IsDefaultExported()) {
        const auto checkTypeNode = [checker](const ir::AstNode *node) { CheckExportForTypeAlias(checker, node); };
        for (auto *it : st->Properties()) {
            auto *property = it->AsClassProperty();
            if (property->TypeAnnotation() == nullptr) {
                continue;
            }
            property->TypeAnnotation()->IterateRecursively(checkTypeNode);
        }
    }

    auto baseName = st->GetBaseName();
    if (!baseName->IsErrorPlaceHolder() && baseName->Variable()->Declaration()->Node()->IsAnnotationDeclaration()) {
        auto *annoDecl = baseName->Variable()->Declaration()->Node()->AsAnnotationDeclaration();
        if (annoDecl != st && annoDecl->IsDeclare()) {
            checker->CheckAmbientAnnotation(st, annoDecl);
        }
    }

    return ReturnTypeForStatement(st);
}

static void ProcessRequiredFields(ArenaUnorderedMap<util::StringView, ir::ClassProperty *> &fieldMap,
                                  ir::AnnotationUsage *st, ETSChecker *checker)
{
    for (const auto &entry : fieldMap) {
        if (entry.second->Value() == nullptr) {
            checker->LogError(diagnostic::ANNOT_FIELD_NO_VAL, {entry.first}, st->Start());
            continue;
        }
    }
}

checker::Type *ETSAnalyzer::Check(ir::AnnotationUsage *st) const
{
    if (st->Expr()->TsType() != nullptr) {
        return ReturnTypeForStatement(st);
    }
    ETSChecker *checker = GetETSChecker();
    st->Expr()->Check(checker);

    auto *baseName = st->GetBaseName();
    if (baseName->Variable() == nullptr || !baseName->Variable()->Declaration()->Node()->IsAnnotationDeclaration()) {
        if (!baseName->IsErrorPlaceHolder()) {
            checker->LogError(diagnostic::NOT_AN_ANNOTATION, {baseName->Name()}, baseName->Start());
        }

        ES2PANDA_ASSERT(checker->IsAnyError());
        return ReturnTypeForStatement(st);
    }

    if (baseName->Name().Is(compiler::Signatures::ANNO_UNSAFE_VARIANCE) && !util::Helpers::IsStdLib(st->Program())) {
        checker->LogError(diagnostic::UNSAFE_VARIANCE_ONLY_IN_STDLIB, {}, st->Start());
    }

    auto *annoDecl = baseName->Variable()->Declaration()->Node()->AsAnnotationDeclaration();
    annoDecl->Check(checker);

    auto *parentNode = st->Parent();
    if (parentNode != nullptr && (parentNode->IsExported() || parentNode->IsDefaultExported())) {
        if (!util::Helpers::IsExported(annoDecl)) {
            checker->LogError(diagnostic::USED_TYPE_IS_NOT_EXPORTED, {baseName->Name()}, st->Start());
        }
    }

    ArenaUnorderedMap<util::StringView, ir::ClassProperty *> fieldMap {checker->ProgramAllocator()->Adapter()};
    for (auto *it : annoDecl->Properties()) {
        auto *field = it->AsClassProperty();
        ES2PANDA_ASSERT(field->Id() != nullptr);
        fieldMap.insert(std::make_pair(field->Id()->Name(), field));
    }

    if (annoDecl->Properties().size() < st->Properties().size()) {
        checker->LogError(diagnostic::ANNOTATION_ARG_COUNT_MISMATCH, {}, st->Start());
        return ReturnTypeForStatement(st);
    }

    if (st->Properties().size() == 1 && st->Properties().at(0)->AsClassProperty()->Id() != nullptr &&
        st->Properties().at(0)->AsClassProperty()->Id()->Name() == compiler::Signatures::ANNOTATION_KEY_VALUE) {
        checker->CheckSinglePropertyAnnotation(st, annoDecl);
        fieldMap.clear();
    } else {
        checker->CheckMultiplePropertiesAnnotation(st, baseName->Name(), fieldMap);
    }

    ProcessRequiredFields(fieldMap, st, checker);
    return ReturnTypeForStatement(st);
}

checker::Type *ETSAnalyzer::Check(ir::ContinueStatement *st) const
{
    ETSChecker *checker = GetETSChecker();

    if (!st->HasTarget()) {
        compiler::SetJumpTargetPhase setJumpTarget;
        setJumpTarget.FindJumpTarget(st);
    }

    if (st->Target() == nullptr) {
        return checker->GlobalTypeError();
    }

    CheckJumpStatement(st, checker);

    checker->AddStatus(CheckerStatus::MEET_CONTINUE);
    return ReturnTypeForStatement(st);
}

checker::Type *ETSAnalyzer::Check(ir::DoWhileStatement *st) const
{
    ETSChecker *checker = GetETSChecker();
    checker::ScopeContext scopeCtx(checker, st->Scope());

    //  NOTE: Smart casts are not processed correctly within the loops now, thus clear them at this point.
    auto [smartCasts, clearFlag] = checker->Context().EnterLoop(*st, std::nullopt);

    checker->CheckTruthinessOfType(st->Test());
    st->Body()->Check(checker);

    checker->Context().ExitLoop(smartCasts, clearFlag, st);
    return ReturnTypeForStatement(st);
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::EmptyStatement *st) const
{
    return ReturnTypeForStatement(st);
}

checker::Type *ETSAnalyzer::Check(ir::ExpressionStatement *st) const
{
    ETSChecker *checker = GetETSChecker();
    return st->GetExpression()->Check(checker);
}

static bool IsArrayOrStringIterableType(Type *type)
{
    return type != nullptr && type->IsETSArrayType();
}

static bool IsNeutralIterableType(Type *type)
{
    return type != nullptr && (type->IsETSResizableArrayType() || type->IsETSStringType());
}

static bool IsUnsupportedMixedIterableUnion(Type *type)
{
    if (type == nullptr || !type->IsETSUnionType()) {
        return false;
    }

    bool hasArrayOrString = false;
    bool hasOtherIterable = false;
    for (auto *const ct : type->AsETSUnionType()->ConstituentTypes()) {
        if (IsNeutralIterableType(ct)) {
            continue;
        }
        if (IsArrayOrStringIterableType(ct)) {
            hasArrayOrString = true;
        } else {
            hasOtherIterable = true;
        }
    }

    return hasArrayOrString && hasOtherIterable;
}

static bool ValidateAndProcessIteratorType(ETSChecker *checker, Type *elemType, ir::ForOfStatement *const st)
{
    checker::Type *iterType = GetIteratorType(checker, elemType, st->Left());
    if (iterType->IsTypeError()) {
        return false;
    }

    const auto ident = st->Left()->IsVariableDeclaration()
                           ? st->Left()->AsVariableDeclaration()->Declarators().front()->Id()->AsIdentifier()
                           : st->Left()->AsIdentifier();
    auto *const relation = checker->Relation();
    relation->SetFlags(checker::TypeRelationFlag::ASSIGNMENT_CONTEXT);
    relation->SetNode(ident);
    if (auto ctx = checker::AssignmentContext(checker->Relation(), ident, elemType, iterType, ident->Start(),
                                              {{diagnostic::ITERATOR_ELEMENT_TYPE_MISMATCH, {elemType, iterType}}});
        !ctx.IsAssignable()) {
        return false;
    }

    relation->SetNode(nullptr);
    relation->SetFlags(checker::TypeRelationFlag::NONE);

    const auto variable = ident->Variable();
    if (variable != nullptr) {
        // Set smart type for variable of 'for-of' statement
        const auto smartType = checker->ResolveSmartType(elemType, variable->TsType());
        checker->Context().SetSmartCast(variable, smartType);
    }

    return true;
}

checker::Type *ETSAnalyzer::Check(ir::ForOfStatement *const st) const
{
    ETSChecker *checker = GetETSChecker();
    checker::ScopeContext scopeCtx(checker, st->Scope());

    //  NOTE: Smart casts are not processed correctly within the loops now, thus clear them at this point.
    auto [smartCasts, clearFlag] = checker->Context().EnterLoop(*st, std::nullopt);

    checker::Type *const exprType = st->Right()->Check(checker);
    if (IsUnsupportedMixedIterableUnion(exprType)) {
        checker->LogError(diagnostic::FOROF_UNSUPPORTED_MIXED_ITERABLE_UNION, {exprType}, st->Right()->Start());
        return checker->GlobalTypeError();
    }

    checker::Type *elemType = checker->GlobalTypeError();

    if (exprType->IsETSStringType()) {
        elemType = checker->GlobalBuiltinETSStringType();
    } else if (exprType->IsETSArrayType() || exprType->IsETSResizableArrayType()) {
        elemType = checker->GetElementTypeOfArray(exprType);
    } else if (exprType->IsETSObjectType() || exprType->IsETSUnionType() || exprType->IsETSTypeParameter()) {
        elemType = st->CheckIteratorMethod(checker);
    } else if (exprType->IsETSTupleType()) {
        elemType = checker->GlobalETSAnyType();
        st->Right()->SetTsType(exprType->AsETSTupleType()->GetWrapperType());
    }

    if (elemType == checker->GlobalTypeError()) {
        checker->LogError(diagnostic::FOROF_SOURCE_NONITERABLE, {}, st->Right()->Start());
        return checker->GlobalTypeError();
    }

    st->Left()->Check(checker);

    if (!ValidateAndProcessIteratorType(checker, elemType, st)) {
        return checker->GlobalTypeError();
    };

    st->Body()->Check(checker);

    checker->Context().ExitLoop(smartCasts, clearFlag, st);
    return ReturnTypeForStatement(st);
}

static bool HasMissingInitOrType(ir::VariableDeclaration *varDecl, ETSChecker *checker)
{
    for (auto *decl : varDecl->Declarators()) {
        if (decl->Id()->IsIdentifier() && (decl->Id()->AsIdentifier()->TypeAnnotation() == nullptr) &&
            (decl->Init() == nullptr)) {
            auto *ident = decl->Id()->AsIdentifier();
            checker->LogError(diagnostic::MISSING_INIT_OR_TYPE, {}, ident->Start());
            return true;
        }
    }
    return false;
}

checker::Type *ETSAnalyzer::Check(ir::ForUpdateStatement *st) const
{
    ETSChecker *checker = GetETSChecker();
    checker::ScopeContext scopeCtx(checker, st->Scope());

    //  NOTE: Smart casts are not processed correctly within the loops now, thus clear them at this point.
    auto [smartCasts, clearFlag] = checker->Context().EnterLoop(*st, std::nullopt);

    if (st->Init() != nullptr) {
        st->Init()->Check(checker);
        if (st->Init()->IsVariableDeclaration()) {
            auto *varDecl = st->Init()->AsVariableDeclaration();
            if (HasMissingInitOrType(varDecl, checker)) {
                return checker->GlobalTypeError();
            }
        }
    }

    // Invalidate smart casts once more after init: for-init can introduce fresh smart casts that should not be kept
    // in the loop header if they are invalidated by loop reassignment rules.
    checker->Context().InvalidateSmartCastsForLoopHeader(*st, std::nullopt);

    if (st->Test() != nullptr) {
        checker->CheckTruthinessOfType(st->Test());
    }

    if (st->Update() != nullptr) {
        st->Update()->Check(checker);
    }

    st->Body()->Check(checker);

    checker->Context().ExitLoop(smartCasts, clearFlag, st);
    return ReturnTypeForStatement(st);
}

checker::Type *ETSAnalyzer::Check(ir::IfStatement *st) const
{
    ETSChecker *const checker = GetETSChecker();

    SmartCastArray smartCasts = checker->Context().EnterTestExpression();
    checker->CheckTruthinessOfType(st->Test());
    SmartCastTypes testedTypes = checker->Context().ExitTestExpression();
    ApplyTestedSmartCasts(checker, testedTypes, true);

    checker->Context().EnterPath();
    st->Consequent()->Check(checker);
    bool const consequentTerminated = checker->Context().ExitPath();
    SmartCastArray consequentSmartCasts = checker->Context().CloneSmartCasts();

    // Restore smart casts to initial state.
    checker->Context().RestoreSmartCasts(smartCasts);
    //  Apply the alternate smart casts
    ApplyTestedSmartCasts(checker, testedTypes, false);

    if (st->Alternate() != nullptr) {
        checker->Context().EnterPath();
        st->Alternate()->Check(checker);
        bool const alternateTerminated = checker->Context().ExitPath();
        if (alternateTerminated) {
            if (!consequentTerminated) {
                // Here we need to restore types from consequent if block.
                checker->Context().RestoreSmartCasts(consequentSmartCasts);
            } else {
                // Here we need to restore initial smart types.
                checker->Context().RestoreSmartCasts(smartCasts);
            }
        } else if (!consequentTerminated) {
            // Here we need to combine types from consequent and alternate if blocks.
            checker->Context().CombineSmartCasts(consequentSmartCasts);
        }
    } else {
        if (!consequentTerminated) {
            // Here we need to combine types from consequent if block and initial.
            checker->Context().CombineSmartCasts(consequentSmartCasts);
        }
    }

    return ReturnTypeForStatement(st);
}

checker::Type *ETSAnalyzer::Check(ir::LabelledStatement *st) const
{
    ETSChecker *checker = GetETSChecker();
    st->body_->Check(checker);
    return ReturnTypeForStatement(st);
}

static bool ValidateThisReturnOperand(ir::Expression *arg, ir::ReturnStatement *st, ETSChecker *checker)
{
    if (arg->IsThisExpression()) {
        return true;
    }
    if (arg->IsCallExpression()) {
        auto *sig = arg->AsCallExpression()->Signature();
        if (sig != nullptr && sig->HasSignatureFlag(SignatureFlags::THIS_RETURN_TYPE)) {
            return true;
        }
    }
    if (arg->IsConditionalExpression()) {
        auto *br = arg->AsConditionalExpression();
        return ValidateThisReturnOperand(br->Consequent(), st, checker) &&
               ValidateThisReturnOperand(br->Alternate(), st, checker);
    }
    checker->LogError(diagnostic::RETURN_THIS_OUTSIDE_METHOD, {}, st->Start());
    return false;
}

static bool CheckIsValidReturnTypeAnnotation(ir::ReturnStatement *st, ir::ScriptFunction *containingFunc,
                                             ir::TypeNode *returnTypeAnnotation, ETSChecker *checker)
{
    // check valid `this` type as return type
    if (containingFunc->GetPreferredReturnType() != nullptr ||
        (returnTypeAnnotation != nullptr && !returnTypeAnnotation->IsTSThisType())) {
        return true;
    }
    if (containingFunc->HasReceiver() && containingFunc->ReturnTypeAnnotation()->IsTSThisType()) {
        checker->LogError(diagnostic::THIS_INCORRECTLY_USED_AS_TYPE_ANNOTAITON, {},
                          containingFunc->ReturnTypeAnnotation()->Start());
        return false;
    }

    if (!containingFunc->HasReceiver()) {
        return true;
    }

    // only extension function could return `this` (instance methods validated after argument Check);
    bool inValidNormalFuncReturnThisType = st->Argument() == nullptr || !st->Argument()->IsThisExpression();
    bool inValidExtensionFuncReturnThisType =
        !containingFunc->HasReceiver() ||
        (containingFunc->HasReceiver() && (st->Argument() == nullptr || !st->Argument()->IsIdentifier() ||
                                           !st->Argument()->AsIdentifier()->IsReceiver()));
    if (inValidNormalFuncReturnThisType && inValidExtensionFuncReturnThisType) {
        checker->LogError(diagnostic::RETURN_THIS_OUTSIDE_METHOD, {}, st->Start());
        return false;
    }

    return true;
}

bool ETSAnalyzer::CheckInferredFunctionReturnType(ir::ReturnStatement *st, ir::ScriptFunction *containingFunc,
                                                  checker::Type *&funcReturnType, ir::TypeNode *returnTypeAnnotation,
                                                  ETSChecker *checker) const
{
    if (!CheckIsValidReturnTypeAnnotation(st, containingFunc, returnTypeAnnotation, checker)) {
        return false;
    }

    if (containingFunc->ReturnTypeAnnotation() != nullptr) {
        /*
         * NOTE(knazarov): To not break compatibility with existing behaviour,
         * we keep return type of the AsyncImpl methods as Object, so here we only check
         * AsyncFunc itself.
         */
        const auto retType = containingFunc->ReturnTypeAnnotation()->GetType(checker);
        if (containingFunc->IsAsyncFunc()) {
            if (!retType->IsETSObjectType() || !checker->IsPromiseType(retType->AsETSObjectType())) {
                checker->LogError(diagnostic::ASYNC_FUNCTION_RETURN_TYPE, {},
                                  containingFunc->ReturnTypeAnnotation()->Start());
                return false;
            }
        }
        funcReturnType = retType;
    } else {
        funcReturnType = containingFunc->GetPreferredReturnType();
    }

    // Case when function's return type is defined explicitly:
    if (st->Argument() == nullptr) {
        ES2PANDA_ASSERT(funcReturnType != nullptr);
        const auto undef = containingFunc->IsDeclaredAsync()
                               ? checker->CreatePromiseOf(checker->GlobalETSUndefinedType())
                               : checker->GlobalETSUndefinedType();
        const auto relation = checker->Relation();
        const auto isReturnUndef = relation->IsSupertypeOf(funcReturnType, undef);
        if (!isReturnUndef) {
            checker->LogError(diagnostic::RETURN_WITHOUT_VALUE, {}, st->Start());
            return false;
        }
    } else {
        return CheckReturnStatementArgumentType(st, containingFunc, funcReturnType, checker);
    }
    return true;
}

bool ETSAnalyzer::CheckReturnStatementArgumentType(ir::ReturnStatement *st, ir::ScriptFunction *containingFunc,
                                                   checker::Type *funcReturnType, ETSChecker *checker) const
{
    CheckMainFunctionReturnType(funcReturnType, checker, containingFunc, st);

    checker::Type *preferredType = funcReturnType;
    // NOTE(knazarov): if false, funcReturnType is either invalid or under inference;
    // In first case, CTE will be thrown down the line;
    // In second case, no action is needed;
    if (containingFunc->IsAsyncFunc() && checker->IsPromiseType(funcReturnType)) {
        ES2PANDA_ASSERT(funcReturnType->IsETSObjectType() || funcReturnType->IsETSUnionType());
        /**
         * Set PREFERRED type for the return statement to Promise<T> | T, as stated in spec;
         *
         * 16.3.1: An asynchronous function with the return type Promise<T> can explicitly return a Promise<T> instance
         * (in this case, the returned value is returned "as is") or a value of type T, which is then automatically
         * boxed in an instance of Promise<T>. Both options are allowed to be the expression of the return statement
         * inside the async function body. T here is a subtype of Type Any.
         */
        if (funcReturnType->IsETSObjectType()) {
            preferredType = checker->CreateETSUnionType(
                {funcReturnType, checker->PromiseTypeArg(funcReturnType->AsETSObjectType())});
        } else {
            /**
             * NOTE(knazarov): Since PREFERRED return type can be union (Promise<T1> | Promise<T2> <: Promise<Any>), we
             * should unwrap it to the union of possible arguments (here, and ONLY here!); DO NOT implement such
             * unwrapping anywhere else;
             */
            // Check that we are indeed checking against PREFERRED return type; Otherwise, should not be a union;
            ES2PANDA_ASSERT(containingFunc->ReturnTypeAnnotation() == nullptr);
            ES2PANDA_ASSERT(funcReturnType == containingFunc->GetPreferredReturnType());
            std::vector<Type *> constituentTypes = {};
            for (const auto &ct : funcReturnType->AsETSUnionType()->ConstituentTypes()) {
                ES2PANDA_ASSERT(ct->IsETSObjectType() && checker->IsPromiseType(ct));
                constituentTypes.push_back(ct);
                constituentTypes.push_back(checker->PromiseTypeArg(ct->AsETSObjectType()));
            }
            preferredType = checker->CreateETSUnionType(std::move(constituentTypes));
        }
    }

    if (st->Argument()->IsMemberExpression()) {
        checker->SetArrayPreferredTypeForNestedMemberExpressions(st->Argument()->AsMemberExpression(), preferredType);
    } else {
        st->Argument()->SetPreferredType(preferredType);
    }

    checker::Type *argumentType = st->Argument()->Check(checker);
    auto *retAnn = containingFunc->ReturnTypeAnnotation();
    if (retAnn != nullptr && retAnn->IsTSThisType() && !containingFunc->HasReceiver()) {
        if (!ValidateThisReturnOperand(st->Argument(), st, checker)) {
            return false;
        }
    }
    if (funcReturnType->IsETSUnionType()) {
        IsAmbiguousUnionInit(funcReturnType->AsETSUnionType(), st->Argument(), checker);
    }
    return CheckReturnType(checker, funcReturnType, argumentType, st->Argument(), containingFunc);
}

checker::Type *ETSAnalyzer::GetFunctionReturnType(ir::ReturnStatement *st, ir::ScriptFunction *containingFunc) const
{
    ES2PANDA_ASSERT(containingFunc->ReturnTypeAnnotation() != nullptr ||
                    containingFunc->Signature()->ReturnType() != nullptr ||
                    containingFunc->GetPreferredReturnType() != nullptr);

    ETSChecker *checker = GetETSChecker();
    checker::Type *funcReturnType = nullptr;

    if (auto *const returnTypeAnnotation = containingFunc->ReturnTypeAnnotation();
        returnTypeAnnotation != nullptr || containingFunc->GetPreferredReturnType() != nullptr) {
        if (!CheckInferredFunctionReturnType(st, containingFunc, funcReturnType, returnTypeAnnotation, checker)) {
            return checker->GlobalTypeError();
        }
    } else {
        //  Case when function's return type should be inferred from return statement(s):
        if (containingFunc->Signature()->HasSignatureFlag(checker::SignatureFlags::NEED_RETURN_TYPE)) {
            funcReturnType = InferReturnType(checker, containingFunc,
                                             st->Argument());  // This removes the NEED_RETURN_TYPE flag, so only the
                                                               // first return statement going to land here...
        } else {
            //  All subsequent return statements:
            funcReturnType = ProcessReturnStatements(checker, containingFunc, st, st->Argument());
        }
    }

    if ((st->Argument() != nullptr) && st->Argument()->IsArrayExpression() && funcReturnType->IsArrayType()) {
        checker->ModifyPreferredType(st->Argument()->AsArrayExpression(), funcReturnType);
        st->Argument()->Check(checker);
    }

    return funcReturnType;
}

checker::Type *ETSAnalyzer::Check(ir::ReturnStatement *st) const
{
    ETSChecker *checker = GetETSChecker();

    if (checker->HasStatus(checker::CheckerStatus::RESTRICTED_RETURN_IN_BLOCK) &&
        !checker->HasStatus(ark::es2panda::checker::CheckerStatus::IN_LAMBDA)) {
        checker->LogError(diagnostic::RETURN_IN_FUN_BODY, {}, st->Start());
    }

    ir::AstNode *ancestor = util::Helpers::FindAncestorGivenByType(st, ir::AstNodeType::SCRIPT_FUNCTION);
    ES2PANDA_ASSERT(ancestor != nullptr);
    ES2PANDA_ASSERT(ancestor->IsScriptFunction());

    auto *containingFunc = ancestor->AsScriptFunction();
    containingFunc->AddFlag(ir::ScriptFunctionFlags::HAS_RETURN);

    if (containingFunc->Signature() == nullptr) {
        ES2PANDA_ASSERT(checker->IsAnyError());
        return ReturnTypeForStatement(st);
    }

    checker->AddStatus(CheckerStatus::MEET_RETURN);

    if (containingFunc->IsConstructor()) {
        if (st->Argument() != nullptr) {
            checker->LogError(diagnostic::NON_VOID_RETURN_IN_CONSTRUCTOR, {}, st->Start());
            return checker->GlobalTypeError();
        }
        return ReturnTypeForStatement(st);
    }

    const auto functionRetType = GetFunctionReturnType(st, containingFunc);
    if (!st->ReturnType() || !checker->Relation()->IsSupertypeOf(functionRetType, st->ReturnType())) {
        st->SetReturnType(checker, functionRetType);
    }

    if (containingFunc->ReturnTypeAnnotation() == nullptr) {
        containingFunc->AddReturnStatement(st);
    }

    return ReturnTypeForStatement(st);
}

checker::Type *ETSAnalyzer::Check(ir::SwitchStatement *st) const
{
    ETSChecker *checker = GetETSChecker();
    checker::ScopeContext scopeCtx(checker, st->Scope());

    auto *comparedExprType = checker->CheckSwitchDiscriminant(st->Discriminant());

    SmartCastArray smartCasts = checker->Context().CloneSmartCasts();
    bool hasDefaultCase = false;

    for (auto &it : st->Cases()) {
        checker->Context().EnterPath();
        it->CheckAndTestCase(checker, comparedExprType, st->Discriminant(), hasDefaultCase);
        bool const caseTerminated = checker->Context().ExitPath();

        if (it != st->Cases().back()) {
            if (!caseTerminated) {
                checker->Context().CombineSmartCasts(smartCasts);
            } else {
                checker->Context().RestoreSmartCasts(smartCasts);
            }
        } else {
            if (!caseTerminated) {
                //  if the recent switch case isn't terminated in any way, copy actual smart casts to the array of
                //  smart casts for the other case blocks so that it can be processed in unified way
                checker->Context().AddBreakSmartCasts(st, checker->Context().CloneSmartCasts());
            }
            checker->Context().ClearSmartCasts();
        }
    }

    // If default case is absent initial smart casts should be also applied here
    if (!hasDefaultCase) {
        checker->Context().AddBreakSmartCasts(st, std::move(smartCasts));
    }

    // Combine smart casts from all [non-terminated] case blocks with 'break'
    checker->Context().CombineBreakSmartCasts(st);

    checker->CheckForSameSwitchCases(st->Cases());
    return ReturnTypeForStatement(st);
}

checker::Type *ETSAnalyzer::Check(ir::ThrowStatement *st) const
{
    ETSChecker *checker = GetETSChecker();
    const auto *arg = st->argument_;
    checker::Type *argType = st->argument_->Check(checker);

    bool isRethrow = false;
    if (arg->IsIdentifier() && !catchParamStack_.empty()) {
        const varbinder::Variable *sym = arg->AsIdentifier()->Variable();
        ES2PANDA_ASSERT(sym != nullptr);
        if (!catchParamStack_.empty() && sym == catchParamStack_.back()) {
            isRethrow = true;
        }
    }
    if (!isRethrow && !argType->IsTypeError()) {
        checker->CheckExceptionOrErrorType(argType, st->Start());
    }

    checker->AddStatus(CheckerStatus::MEET_THROW);
    return ReturnTypeForStatement(st);
}

checker::Type *ETSAnalyzer::Check(ir::TryStatement *st) const
{
    ETSChecker *checker = GetETSChecker();
    std::vector<checker::ETSObjectType *> exceptions {};

    std::vector<SmartCastArray> casts {};
    auto smartCasts = checker->Context().CheckTryBlock(*st->Block());
    checker->Context().EnterPath();
    st->Block()->Check(checker);

    bool const tryWillThrow = checker->HasStatus(CheckerStatus::MEET_THROW);
    [[maybe_unused]] bool const tryTerminated = checker->Context().ExitPath();

    bool defaultCatchFound = false;
    // Default to true: if no catch clauses, all catches "throw" (nothing handles the exception)
    bool allCatchClausesThrow = true;

    // Note(daizihan): #33030 Refactor me after multiple catch clauses is changed to CTE.
    for (auto *catchClause : st->CatchClauses()) {
        if (defaultCatchFound) {
            checker->LogError(diagnostic::CATCH_DEFAULT_NOT_LAST, {}, catchClause->Start());
            return checker->GlobalTypeError();
        }

        checker->Context().RestoreSmartCasts(smartCasts);

        checker->Context().EnterPath();
        if (auto const exceptionType = catchClause->Check(checker); !exceptionType->IsTypeError()) {
            auto *clauseType = exceptionType->AsETSObjectType();
            checker->CheckExceptionClauseType(exceptions, catchClause, clauseType);
            exceptions.emplace_back(clauseType);
        }

        bool const catchMeetThrow = checker->HasStatus(CheckerStatus::MEET_THROW);
        [[maybe_unused]] bool const catchTerminated = checker->Context().ExitPath();

        allCatchClausesThrow = allCatchClausesThrow && catchMeetThrow;

        defaultCatchFound = catchClause->IsDefaultCatchClause();

        casts.emplace_back(checker->Context().CloneSmartCasts());
    }

    checker->Context().RestoreSmartCasts(smartCasts);
    if (!casts.empty()) {
        for (auto const &cast : casts) {
            checker->Context().CombineSmartCasts(cast);
        }
    }

    if (tryWillThrow && allCatchClausesThrow) {
        checker->AddStatus(CheckerStatus::MEET_THROW);
    }

    if (st->HasFinalizer()) {
        st->FinallyBlock()->Check(checker);
    }

    return ReturnTypeForStatement(st);
}

// Helper function extracted from 'ETSAnalyzer::Check(ir::VariableDeclarator *st)' to reduce its size.
checker::Type *InferSmartType(ETSChecker *checker, Type *variableType, ir::Identifier *ident, ir::Expression *init)
{
    TypeRelation *relation = checker->Relation();

    Type *smartType = variableType;
    Type *initType = nullptr;
    Type *undefinedType = checker->GlobalETSUndefinedType();

    std::optional<double> value = {};

    if (init != nullptr) {
        initType = init->TsType();
        if (init->IsNumberLiteral()) {
            value = std::make_optional(init->AsNumberLiteral()->Number().GetDouble());
        }
    } else if (variableType != nullptr && relation->IsSupertypeOf(variableType, undefinedType)) {
        // For type 'undefined' and all its supertypes the default value is 'undefined'
        initType = undefinedType;
    }

    if (initType != nullptr) {
        smartType = checker->ResolveSmartType(initType, variableType, value);
        //  Set smart type for identifier if it differs from annotated type
        //  Top-level and captured variables are not processed here!
        if (!relation->IsIdenticalTo(variableType, smartType)) {
            ident->SetTsType(smartType);
            checker->Context().SetSmartCast(ident->Variable(), smartType);
        }
    }

    return smartType;
}

checker::Type *ETSAnalyzer::Check(ir::VariableDeclarator *st) const
{
    bool initChecked = st->Init() != nullptr ? st->Init()->TsType() != nullptr : true;
    if (st->TsType() != nullptr && initChecked) {
        return st->TsType();
    }

    ETSChecker *checker = GetETSChecker();

    if (st->Id()->IsETSDestructuring()) {
        return st->SetTsType(CheckDestructuringExpression(checker, st->Id()->AsETSDestructuring(), st->Init()));
    }

    ES2PANDA_ASSERT(st->Id()->IsIdentifier());
    auto *const ident = st->Id()->AsIdentifier();
    ir::ModifierFlags flags = ir::ModifierFlags::NONE;

    if (ident->Parent()->Parent()->AsVariableDeclaration()->Kind() ==
        ir::VariableDeclaration::VariableDeclarationKind::CONST) {
        flags |= ir::ModifierFlags::CONST;
    }

    if (ident->IsOptionalDeclaration()) {
        flags |= ir::ModifierFlags::OPTIONAL;
    }

    // Processing possible parser errors
    if (ident->Variable() == nullptr) {
        ident->Check(checker);
    }

    auto *init = st->Init();
    auto *variableType = checker->CheckVariableDeclaration(ident, ident->TypeAnnotation(), init, flags);
    if (variableType != nullptr) {
        if (variableType->IsTypeError()) {
            return st->SetTsType(variableType);
        }
        if (variableType->IsETSWildcardType()) {
            variableType = variableType->AsETSWildcardType()->GetUnderlying()->GetConstraintType();
            ident->Variable()->SetTsType(variableType);
        }
    }

    //  Now try to define the actual type of Identifier so that smart cast can be used in further checker processing
    //  NOTE: T_S and K_o_t_l_i_n don't act in such way, but we can try - why not? :)
    auto *smartType = InferSmartType(checker, variableType, ident, init);

    if (variableType != nullptr && variableType->IsETSUnionType() && init != nullptr) {
        IsAmbiguousUnionInit(variableType->AsETSUnionType(), init, checker);
    }

    return st->SetTsType(smartType);
}

checker::Type *ETSAnalyzer::Check(ir::VariableDeclaration *st) const
{
    ETSChecker *checker = GetETSChecker();

    checker->CheckAnnotations(st);

    for (auto *it : st->Declarators()) {
        it->Check(checker);
    }

    return ReturnTypeForStatement(st);
}

checker::Type *ETSAnalyzer::Check(ir::WhileStatement *st) const
{
    ETSChecker *checker = GetETSChecker();
    checker::ScopeContext scopeCtx(checker, st->Scope());

    // Invalidate smart cast for variables in the test condition, that will be reassigned in the loop body
    const auto reassignedVars = checker->Context().GetReassignedVariablesInNode(st->Body());
    for (const auto &[var, _] : reassignedVars) {
        checker->Context().RemoveSmartCast(var);
    }

    SmartCastArray savedSmartCasts = checker->Context().EnterTestExpression();
    checker->CheckTruthinessOfType(st->Test());
    SmartCastTypes testedTypes = checker->Context().ExitTestExpression();
    ApplyTestedSmartCasts(checker, testedTypes, true);

    auto [smartCasts, clearFlag] = checker->Context().EnterLoop(*st, testedTypes);
    st->Body()->Check(checker);
    checker->Context().ExitLoop(savedSmartCasts, clearFlag, st);
    return ReturnTypeForStatement(st);
}

checker::Type *ETSAnalyzer::Check(ir::TSArrayType *node) const
{
    ETSChecker *checker = GetETSChecker();
    checker->CheckAnnotations(node);
    node->elementType_->Check(checker);
    node->SetTsType(node->GetType(checker));

    const auto *arrayType = node->TsType()->AsETSArrayType();
    checker->CreateBuiltinArraySignature(arrayType, arrayType->Rank());
    return node->TsType();
}

static bool ValueFitsTargetType(ir::TSAsExpression *expr)
{
    if (!expr->Expr()->IsNumberLiteral() || !expr->TypeAnnotation()->IsETSPrimitiveType()) {
        return true;
    }

    auto primitiveType = expr->TypeAnnotation()->AsETSPrimitiveType()->GetPrimitiveType();
    lexer::Number number = expr->Expr()->AsNumberLiteral()->Number();
    if (!std::isfinite(number.GetValue<double>())) {
        return true;
    }

    if (number.IsReal() && primitiveType != ir::PrimitiveType::FLOAT && primitiveType != ir::PrimitiveType::DOUBLE) {
        auto val = number.GetDouble();
        val = val < 0 ? std::floor(val) : std::ceil(val);
        number.SetValue(int64_t(val));
    }

    switch (primitiveType) {
        case ir::PrimitiveType::BYTE:
            return number.CanGetValue<int8_t>();
            break;
        case ir::PrimitiveType::SHORT:
            return number.CanGetValue<int16_t>();
            break;
        case ir::PrimitiveType::INT:
            return number.CanGetValue<int32_t>();
            break;
        case ir::PrimitiveType::LONG:
            return number.CanGetValue<int64_t>();
            break;
        case ir::PrimitiveType::FLOAT:
            return number.CanGetValue<float>();
            break;
        case ir::PrimitiveType::DOUBLE:
            return number.CanGetValue<double>();
            break;
        default:
            break;
    }

    return true;
}

//  Extracted from 'ETSAnalyzer::Check(ir::TSAsExpression *expr)' function to reduce its size
static bool CheckTSAsExpressionInvalidCast(ir::TSAsExpression *expr, checker::Type *sourceType,
                                           checker::Type *targetType, ETSChecker *checker)
{
    if (sourceType->DefinitelyETSNullish() && !targetType->PossiblyETSNullish()) {
        checker->LogError(diagnostic::NULLISH_CAST_TO_NONNULLISH, expr->Expr()->Start());
        expr->SetTsType(targetType);
        return false;
    }

    if (expr->Expr()->IsLiteral() && sourceType->IsBuiltinNumeric()) {
        if (targetType->IsETSTypeParameter()) {
            checker->LogError(diagnostic::INVALID_CAST, {sourceType, targetType}, expr->Expr()->Start());
            expr->SetTsType(targetType);
            return false;
        } else if (targetType->IsETSUnionType() && targetType->AsETSUnionType()->AllOfConstituentTypes(
                                                       // CC-OFFNXT(G.FMT.06-CPP) project code style
                                                       [](Type *type) { return type->IsETSTypeParameter(); })) {
            checker->LogError(diagnostic::INVALID_CAST, {sourceType, targetType}, expr->Expr()->Start());
            expr->SetTsType(targetType);
            return false;
        }
    }

    if (!ValueFitsTargetType(expr)) {
        checker->LogError(diagnostic::TOO_LARGE_TO_CAST,
                          {expr->Expr()->AsNumberLiteral()->ToString(), expr->TypeAnnotation()->DumpEtsSrc()},
                          expr->Expr()->Start());
        expr->SetTsType(targetType);
        return false;
    }

    return true;
}

//  Extracted from 'ETSAnalyzer::Check(ir::TSAsExpression *expr)' function to reduce its size
static checker::CastingContext const CheckTSAsExpressionCastable(ir::Expression *castExpr, checker::Type *sourceType,
                                                                 checker::Type *targetType, ETSChecker *checker)
{
    diagnostic::DiagnosticKind const *message = &diagnostic::INVALID_CAST;
    util::DiagnosticMessageParams parameters = {sourceType, targetType};
    if (sourceType->IsBuiltinNumeric() && targetType->IsBuiltinNumeric()) {
        message = &diagnostic::IMPROPER_NUMERIC_CAST;
    } else if (castExpr->IsArrayExpression() || castExpr->IsObjectExpression() || castExpr->IsNumberLiteral() ||
               castExpr->IsStringLiteral()) {
        message = &diagnostic::INVALID_LITERAL_CAST;
        char const *literalType = "array";
        if (castExpr->IsObjectExpression()) {
            literalType = "object";
        } else if (castExpr->IsNumberLiteral()) {
            literalType = "number";
        } else if (castExpr->IsStringLiteral()) {
            literalType = "string";
        }
        parameters = util::DiagnosticMessageParams {literalType, targetType};
    }

    const checker::CastingContext ctx(
        checker->Relation(), *message, parameters,
        checker::CastingContext::ConstructorData {castExpr, sourceType, targetType, castExpr->Start()});

    if (checker->Relation()->IsTrue() && !ctx.TrivialCast() && targetType->IsETSObjectType() &&
        targetType->AsETSObjectType()->IsGeneric() && !castExpr->IsArrayExpression() &&
        !castExpr->IsObjectExpression() && !compiler::IsSyntheticIdentifier(castExpr) &&
        compiler::GetPhaseManager()->CurrentPhase()->Name() == compiler::CheckerPhase::NAME &&
        std::any_of(targetType->AsETSObjectType()->TypeArguments().begin(),
                    targetType->AsETSObjectType()->TypeArguments().end(),
                    [](Type *item) { return !item->IsETSTypeParameter() && !item->IsETSWildcardType(); })) {
        checker->LogDiagnostic(diagnostic::GENERIC_TYPE_CAST, {targetType->ToString()}, castExpr->Start());
    }

    return ctx;
}

checker::Type *ETSAnalyzer::Check(ir::TSAsExpression *expr) const
{
    ETSChecker *checker = GetETSChecker();

    if (expr->TsType() != nullptr) {
        return expr->TsType();
    }

    checker->CheckAnnotations(expr->TypeAnnotation());
    auto *const targetType = expr->TypeAnnotation()->AsTypeNode()->GetType(checker);
    FORWARD_TYPE_ERROR(checker, targetType, expr);

    auto *castExpr = expr->Expr();
    castExpr->SetPreferredType(targetType);

    auto const sourceType = castExpr->Check(checker);
    if (sourceType->IsTypeError() && checker->HasStatus(checker::CheckerStatus::IN_TYPE_INFER)) {
        return expr->SetTsType(checker->GlobalTypeError());
    }
    FORWARD_TYPE_ERROR(checker, sourceType, expr);

    if (!CheckTSAsExpressionInvalidCast(expr, sourceType, targetType, checker)) {
        return expr->TsType();
    }

    if (castExpr->IsArrayExpression() && targetType->IsETSObjectType() &&
        targetType->AsETSObjectType()->IsInterface() &&
        GetPreferredTypeFromArraySupertypes(checker, targetType) == nullptr) {
        checker->LogError(diagnostic::NOT_A_SUPERINTERFACE_OF_ARRAY, {castExpr->DumpEtsSrc(), targetType, targetType},
                          expr->Start());
        return expr->SetTsType(targetType);
    }

    const checker::CastingContext ctx = CheckTSAsExpressionCastable(castExpr, sourceType, targetType, checker);
    expr->isUncheckedCast_ = ctx.UncheckedCast();

    // Make sure the array type symbol gets created for the assembler to be able to emit checkcast.
    // Because it might not exist, if this particular array type was never created explicitly.
    if (!expr->isUncheckedCast_ && targetType->IsETSArrayType()) {
        const auto *const targetArrayType = targetType->AsETSArrayType();
        checker->CreateBuiltinArraySignature(targetArrayType, targetArrayType->Rank());
    }

    return expr->SetTsType(targetType);
}

checker::Type *ETSAnalyzer::Check(ir::TSEnumDeclaration *st) const
{
    // Some invalid TSEnumDeclaration will not be transformed to class.
    return ReturnTypeForStatement(st);
}

checker::Type *ETSAnalyzer::Check(ir::TSInterfaceDeclaration *st) const
{
    if (st->TsType() != nullptr) {
        return st->TsType();
    }

    ETSChecker *checker = GetETSChecker();
    auto *stmtType = checker->BuildBasicInterfaceProperties(st);
    ES2PANDA_ASSERT(stmtType != nullptr);

    FORWARD_TYPE_ERROR(checker, stmtType, st);

    auto *interfaceType = stmtType->AsETSObjectType();
    checker->CheckInterfaceAnnotations(st);

    if (!interfaceType->IsGradual()) {
        interfaceType->SetSuperType(checker->GlobalETSObjectType());
    }
    checker->CheckInvokeMethodsLegitimacy(interfaceType);

    st->SetTsType(stmtType);  // NOTE(vpukhov): #31391
    checker->CheckDynamicInheritanceAndImplement(interfaceType->AsETSObjectType());
    checker::ScopeContext scopeCtx(checker, st->Scope());
    auto savedContext = checker::SavedCheckerContext(checker, checker::CheckerStatus::IN_INTERFACE, interfaceType);

    for (auto *it : st->Body()->Body()) {
        it->Check(checker);
    }

    checker->CheckTypeParameterVariance(st);

    return st->TsType();
}

checker::Type *ETSAnalyzer::Check(ir::TSNonNullExpression *expr) const
{
    if (expr->TsType() != nullptr) {
        return expr->TsType();
    }

    ETSChecker *checker = GetETSChecker();
    auto exprType = expr->expr_->Check(checker);

    checker::Type *currentType = exprType;
    while (currentType->IsETSTypeParameter()) {
        currentType = currentType->AsETSTypeParameter()->GetConstraintType();
    }

    //  If the actual [smart] type is definitely 'null' or 'undefined' then probably CTE should be thrown.
    //  Anyway we'll definitely obtain NullPointerError at runtime.
    if (currentType->DefinitelyETSNullish()) {
        checker->LogDiagnostic(diagnostic::NULLISH_OPERAND, {"ensure-not-nullish"}, expr->Expr()->Start());

        if (expr->expr_->IsIdentifier()) {
            ES2PANDA_ASSERT(expr->expr_->AsIdentifier()->Variable() != nullptr);
            auto originalType = expr->expr_->AsIdentifier()->Variable()->TsType();
            if (originalType != nullptr) {
                expr->SetTsType(checker->GetNonNullishType(originalType));
            }
        }
    } else if (currentType->DefinitelyNotETSNullish()) {
        checker->LogDiagnostic(diagnostic::NON_NULLISH_OPERAND, {"ensure-not-nullish"}, expr->Expr()->Start());
    }

    if (expr->TsType() == nullptr) {
        expr->SetTsType(checker->GetNonNullishType(exprType));
    }

    expr->SetOriginalType(expr->TsType());
    return expr->TsType();
}

checker::Type *ETSAnalyzer::Check(ir::TSThisType *node) const
{
    ETSChecker *checker = GetETSChecker();
    return node->GetType(checker);
}

static varbinder::Variable *FindInReExports(ETSObjectType *baseType, util::StringView &searchName)
{
    for (auto *reExport : baseType->ReExports()) {
        PropertySearchFlags flags = PropertySearchFlags::SEARCH_STATIC_FIELD | PropertySearchFlags::SEARCH_STATIC_DECL;
        if (auto *var = reExport->GetProperty(searchName, flags); var != nullptr) {
            return var;
        }
        auto *result = FindInReExports(reExport, searchName);
        if (result != nullptr) {
            return result;
        }
    }
    return nullptr;
}

static varbinder::Variable *FindNameForImportNamespace(ETSChecker *checker, util::StringView &searchName,
                                                       ETSObjectType *baseType)
{
    /* This function try to find name1.name2, name1.A in file file1.ets,
     * ./file1.ets:
     * import * as name1 from "./file2"
     *
     * ./file2.ets:
     * import * as name2 from "./file3"
     * import {A} from "./file3"
     * export {name2}
     * export {A}
     *
     * ./file3.ets
     * export class A{}
     *
     * 1. Find in file2->program->ast->scope first
     * 2. Find in varbinder->selectiveExportAliasMultimap second
     * if both found, return variable
     */
    auto declNode = baseType->GetDeclNode();
    if (!declNode->IsIdentifier()) {
        return nullptr;
    }
    if (declNode->Parent() == nullptr || declNode->Parent()->Parent() == nullptr) {
        return nullptr;
    }
    auto importDeclNode = declNode->Parent()->Parent();
    if (!importDeclNode->IsETSImportDeclaration()) {
        return nullptr;
    }

    auto importDecl = importDeclNode->AsETSImportDeclaration();

    parser::Program *program = checker->VarBinder()->AsETSBinder()->GetExternalProgram(importDecl);
    auto &bindings = program->Ast()->Scope()->Bindings();

    if (auto result = bindings.find(searchName); result != bindings.end()) {
        auto &sMap = checker->VarBinder()
                         ->AsETSBinder()
                         ->GetSelectiveExportAliasMultimap()
                         .find(importDecl->ImportInfo().ResolvedSource())
                         ->second;
        if (auto it = sMap.find(searchName); it != sMap.end()) {
            return result->second;
        }
    }
    return FindInReExports(baseType, searchName);
}

checker::Type *ETSAnalyzer::Check(ir::TSQualifiedName *expr) const
{
    if (expr->TsType() != nullptr) {
        return expr->TsType();
    }

    ETSChecker *checker = GetETSChecker();
    checker::Type *baseType = expr->Left()->Check(checker);
    if (baseType->IsETSObjectType()) {
        auto searchName = expr->Right()->Name();
        // NOTE (oeotvos) This should be done differently in the follow-up patch.
        if (searchName.Empty()) {
            searchName = expr->Right()->Name();
        }
        varbinder::Variable *prop =
            baseType->AsETSObjectType()->GetProperty(searchName, PropertySearchFlags::SEARCH_DECL);

        if (prop == nullptr) {
            prop = FindNameForImportNamespace(GetETSChecker(), searchName, baseType->AsETSObjectType());
        }
        // NOTE(dslynko): in debugger evaluation mode must lazily generate module's properties here.
        if (prop == nullptr) {
            checker->LogError(diagnostic::NONEXISTENT_TYPE, {expr->Right()->Name()}, expr->Right()->Start());
            return checker->GlobalTypeError();
        }

        checker->ValidateNamespaceProperty(prop, baseType->AsETSObjectType(), expr->Right());
        expr->Right()->SetVariable(prop);
        return checker->GetTypeOfVariable(prop);
    }

    checker->LogError(diagnostic::NONEXISTENT_TYPE, {expr->Right()->Name()}, expr->Right()->Start());
    return checker->GlobalTypeError();
}

checker::Type *ETSAnalyzer::Check(ir::TSTypeAliasDeclaration *st) const
{
    ETSChecker *checker = GetETSChecker();
    auto checkerContext = SavedCheckerContext(checker, CheckerStatus::NO_OPTS, checker->Context().ContainingClass());

    checker->CheckAnnotations(st);

    if (st->TypeParams() == nullptr) {
        if (st->TypeAnnotation()->TsType() == nullptr) {
            st->TypeAnnotation()->Check(checker);
        }
        if ((st->IsExported() || st->IsDefaultExported()) && st->TypeAnnotation()->TsType() != nullptr) {
            CheckExport(checker, st->TypeAnnotation()->TsType());
        }
        return ReturnTypeForStatement(st);
    }

    if (st->TypeParameterTypes().empty()) {
        auto [typeParamTypes, ok] = checker->CreateUnconstrainedTypeParameters(st->TypeParams());
        for (auto *tpt : typeParamTypes) {
            tpt->AsETSTypeParameter()->SetUnderInference();
        }
        st->SetTypeParameterTypes(std::move(typeParamTypes));
        if (ok) {
            ok = checker->ValidateTypeParameterConstraints(st->TypeParams());
        }
        if (ok) {
            checker->AssignTypeParameterConstraints(st->TypeParams());
        }
    }

    if (st->TypeAnnotation()->TsType() == nullptr) {
        st->TypeAnnotation()->Check(checker);
    }
    if ((st->IsExported() || st->IsDefaultExported()) && st->TypeAnnotation()->TsType() != nullptr) {
        CheckExport(checker, st->TypeAnnotation()->TsType());
    }

    return ReturnTypeForStatement(st);
}

checker::Type *ETSAnalyzer::Check(ir::ETSGenericInstantiatedNode *expr) const
{
    ES2PANDA_ASSERT(expr->GetExpression()->IsIdentifier() || expr->GetExpression()->IsMemberExpression());

    ETSChecker *checker = GetETSChecker();

    auto exprType = expr->GetExpression()->Check(checker);
    if (exprType->IsTypeError()) {
        expr->SetTsType(exprType);
    }
    if (!exprType->IsETSFunctionType()) {
        return exprType;
    }

    // NOTE (smartin): If there more than 1 call signature exist for a function type, then the reference is ambiguous,
    // as inference from the context of a function reference is not implemented yet. This will need to be changed when
    // the selection of the overloaded target signature is implemented based on the type inference from the context.

    auto *funcType = exprType->AsETSFunctionType();
    if (funcType->CallSignaturesOfMethodOrArrow().size() != 1) {
        checker->LogError(diagnostic::OVERLOADED_METHOD_AS_VALUE, expr->Start());
        return checker->GlobalBuiltinErrorType();
    }

    auto *callSig = funcType->CallSignaturesOfMethodOrArrow().front();

    if (funcType->CallSignaturesOfMethodOrArrow().front()->TypeParams().empty()) {
        checker->LogError(diagnostic::TYPE_ARGS_FOR_NON_GENERIC_SIGNATURE, {callSig, expr->TypeParams()->DumpEtsSrc()},
                          expr->Start());
        return checker->GlobalBuiltinErrorType();
    }

    const auto newSub = checker->CheckTypeParamsAndBuildSubstitutionIfValid(callSig, expr->TypeParams()->Params(),
                                                                            expr->TypeParams()->Start());
    if (!newSub.has_value()) {
        return checker->GlobalBuiltinErrorType();
    }

    auto *const substitutedType = exprType->Substitute(checker->Relation(), &newSub.value());
    auto *const substitutedSig = substitutedType->AsETSFunctionType()->CallSignaturesOfMethodOrArrow().front();

    // After substituted the actual type arguments into the signatures, they are not generic anymore, remove the
    // type params
    substitutedSig->TypeParams().clear();

    return expr->SetTsType(substitutedType);
}

checker::Type *ETSAnalyzer::ReturnTypeForStatement([[maybe_unused]] const ir::Statement *const st) const
{
    ES2PANDA_ASSERT(st->IsStatement());
    return nullptr;
}

}  // namespace ark::es2panda::checker

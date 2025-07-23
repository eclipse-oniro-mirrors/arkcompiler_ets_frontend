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

#include "declgenEts2Ts.h"

#include "isolatedDeclgenChecker.h"
#include "checker/types/ets/etsTupleType.h"
#include "compiler/lowering/phase.h"
#include "generated/diagnostic.h"
#include "ir/base/classProperty.h"
#include "ir/base/methodDefinition.h"
#include "ir/base/scriptFunction.h"
#include "ir/ets/etsImportDeclaration.h"
#include "ir/ets/etsReExportDeclaration.h"
#include "ir/ets/etsTuple.h"
#include "ir/ets/etsUnionType.h"
#include "ir/expressions/identifier.h"
#include "ir/expressions/literals/numberLiteral.h"
#include "ir/statements/classDeclaration.h"
#include "ir/ts/tsInterfaceBody.h"
#include "ir/ts/tsTypeAliasDeclaration.h"
#include "ir/ts/tsTypeParameter.h"
#include "compiler/lowering/util.h"

#define DEBUG_PRINT 0

namespace ark::es2panda::declgen_ets2ts {

static void DebugPrint([[maybe_unused]] const std::string &msg)
{
#if DEBUG_PRINT
    std::cerr << msg << std::endl;
#endif
}

bool TSDeclGen::Generate()
{
    if (!GenGlobalDescriptor()) {
        return false;
    }
    CollectIndirectExportDependencies();
    CollectGlueCodeImportSet();
    GenDeclarations();
    return true;
}

bool TSDeclGen::GenGlobalDescriptor()
{
    if (program_->GlobalClass() == nullptr) {
        const auto loc = lexer::SourcePosition();
        LogError(diagnostic::UNSUPPORTED_ENCODING_SPECIFICATIONS, {}, loc);
        return false;
    }
    globalDesc_ =
        checker::ETSObjectType::NameToDescriptor(program_->GlobalClass()->TsType()->AsETSObjectType()->AssemblerName());
    OutTs("let ETSGLOBAL = (globalThis as any).Panda.getClass('", globalDesc_, "');");
    OutEndlTs();
    OutTs("ETSGLOBAL.", compiler::Signatures::INIT_METHOD, "();");
    OutEndlTs();
    OutTs("export {};");
    OutEndlTs();
    return true;
}

void TSDeclGen::CollectGlueCodeImportSet()
{
    for (auto *globalStatement : program_->Ast()->Statements()) {
        if (globalStatement->IsETSImportDeclaration()) {
            auto importDeclaration = globalStatement->AsETSImportDeclaration();
            if (importDeclaration->IsPureDynamic()) {
                return;
            }
            const auto &specifiers = importDeclaration->Specifiers();
            if (specifiers.empty()) {
                return;
            }
            const auto specifierFirst = specifiers[0];
            if (importDeclaration->IsTypeKind()) {
                continue;
            }
            if (specifierFirst->IsImportDefaultSpecifier()) {
                CollectDefaultImport(specifierFirst);
            } else if (specifierFirst->IsImportSpecifier()) {
                CollectNamedImports(specifiers);
            }
        }
    }
}

void TSDeclGen::CollectDefaultImport(const ir::AstNode *specifier)
{
    auto importDefaultSpecifier = specifier->AsImportDefaultSpecifier();
    auto variable = importDefaultSpecifier->Local()->Variable();
    const auto local = importDefaultSpecifier->Local()->Name().Mutf8();
    bool isTypeDeclaration = false;
    if (variable != nullptr && variable->Declaration() != nullptr && variable->Declaration()->Node() != nullptr) {
        auto *node = variable->Declaration()->Node();
        isTypeDeclaration = node->IsTSTypeAliasDeclaration() || node->IsTSInterfaceDeclaration();
    }
    if (!isTypeDeclaration) {
        glueCodeImportSet_.insert(local);
    }
}

void TSDeclGen::CollectNamedImports(const ArenaVector<ir::AstNode *> &specifiers)
{
    if (specifiers.empty()) {
        return;
    }
    for (auto *specifier : specifiers) {
        if (!specifier->IsImportSpecifier()) {
            continue;
        }
        auto importSpecifier = specifier->AsImportSpecifier();
        auto variable = importSpecifier->Imported()->Variable();
        bool isTypeDeclaration = false;
        if (variable != nullptr && variable->Declaration() != nullptr && variable->Declaration()->Node() != nullptr) {
            auto *node = variable->Declaration()->Node();
            isTypeDeclaration = node->IsTSTypeAliasDeclaration() || node->IsTSInterfaceDeclaration();
        }
        if (!isTypeDeclaration) {
            glueCodeImportSet_.insert(importSpecifier->Local()->Name().Mutf8());
        }
    }
}

void TSDeclGen::CollectIndirectExportDependencies()
{
    for (auto *stmt : program_->Ast()->Statements()) {
        if (stmt->IsTSTypeAliasDeclaration()) {
            ProcessTypeAliasDependencies(stmt->AsTSTypeAliasDeclaration());
        } else if (stmt->IsClassDeclaration()) {
            ProcessClassDependencies(stmt->AsClassDeclaration());
        } else if (stmt->IsTSInterfaceDeclaration()) {
            ProcessInterfaceDependencies(stmt->AsTSInterfaceDeclaration());
        }
    }
}

void TSDeclGen::ProcessTypeAliasDependencies(const ir::TSTypeAliasDeclaration *typeAliasDecl)
{
    const auto name = typeAliasDecl->Id()->Name().Mutf8();
    const auto *aliasedType = typeAliasDecl->TypeAnnotation()->GetType(checker_);
    const auto typeFlag = checker::ETSChecker::ETSType(aliasedType);
    const auto *parent = typeAliasDecl->Id()->Parent();
    if (!parent->IsExported() && !parent->IsDefaultExported()) {
        return;
    }
    if (typeFlag == checker::TypeFlag::ETS_OBJECT) {
        auto objectType = aliasedType->AsETSObjectType();
        if (objectType->IsETSStringType() || objectType->IsETSBigIntType() || objectType->IsETSUnboxableObject() ||
            objectType->HasObjectFlag(checker::ETSObjectFlags::FUNCTIONAL)) {
            return;
        }
        auto typeName = objectType->Name();
        AddObjectDependencies(typeName, name);
    }
    ProcessTypeAnnotationDependencies(typeAliasDecl->TypeAnnotation());
}

void TSDeclGen::ProcessTypeAnnotationDependencies(const ir::TypeNode *typeAnnotation)
{
    if (typeAnnotation->IsETSTypeReference()) {
        ProcessETSTypeReferenceDependencies(typeAnnotation->AsETSTypeReference());
    } else if (typeAnnotation->IsETSUnionType()) {
        GenSeparated(
            typeAnnotation->AsETSUnionType()->Types(),
            [this](ir::TypeNode *arg) { ProcessTypeAnnotationDependencies(arg); }, "");
    }
}

void TSDeclGen::ProcessETSTypeReferenceDependencies(const ir::ETSTypeReference *typeReference)
{
    const auto part = typeReference->Part();
    auto partName = part->GetIdent()->Name().Mutf8();
    if (part->TypeParams() != nullptr && part->TypeParams()->IsTSTypeParameterInstantiation()) {
        indirectDependencyObjects_.insert(partName);
        GenSeparated(
            part->TypeParams()->Params(), [this](ir::TypeNode *param) { ProcessTypeAnnotationDependencies(param); },
            "");
    } else if (part->Name()->IsTSQualifiedName() && part->Name()->AsTSQualifiedName()->Name() != nullptr) {
        const auto qualifiedName = part->Name()->AsTSQualifiedName()->Name().Mutf8();
        std::istringstream stream(qualifiedName);
        std::string firstSegment;
        if (std::getline(stream, firstSegment, '.')) {
            importSet_.insert(firstSegment);
            indirectDependencyObjects_.insert(firstSegment);
        }
    } else {
        indirectDependencyObjects_.insert(partName);
    }
}

void TSDeclGen::ProcessClassDependencies(const ir::ClassDeclaration *classDecl)
{
    auto *classDef = classDecl->Definition();
    if (classDef->Ident()->Name().Mutf8().find('#') != std::string::npos) {
        return;
    }

    if (!classDef->IsExported() && !classDef->IsDefaultExported()) {
        return;
    }

    state_.super = classDef->Super();
    if (state_.super != nullptr) {
        AddSuperType(state_.super);
    }

    if (classDef->TsType() != nullptr && classDef->TsType()->IsETSObjectType()) {
        ProcessInterfacesDependencies(classDef->TsType()->AsETSObjectType()->Interfaces());
    }

    if (classDef->TypeParams() != nullptr) {
        GenSeparated(
            classDef->TypeParams()->Params(),
            [this](ir::TSTypeParameter *param) {
                if (param->Constraint() == nullptr) {
                    return;
                }
                AddSuperType(param->Constraint());
            },
            "");
    }

    ProcessClassPropDependencies(classDef);
}

void TSDeclGen::ProcessClassPropDependencies(const ir::ClassDefinition *classDef)
{
    for (const auto *prop : classDef->Body()) {
        if (prop->IsClassProperty()) {
            auto value = prop->AsClassProperty()->Value();
            if (value != nullptr && value->IsETSNewClassInstanceExpression() &&
                value->AsETSNewClassInstanceExpression()->GetTypeRef() != nullptr &&
                value->AsETSNewClassInstanceExpression()->GetTypeRef()->IsETSTypeReference()) {
                auto typeReference = value->AsETSNewClassInstanceExpression()->GetTypeRef()->AsETSTypeReference();
                ProcessETSTypeReferenceDependencies(typeReference);
                continue;
            }
            if (prop->AsClassProperty()->TypeAnnotation() != nullptr) {
                ProcessTypeAnnotationDependencies(prop->AsClassProperty()->TypeAnnotation());
                continue;
            }
        } else if (prop->IsMethodDefinition()) {
            ProcessClassMethodDependencies(prop->AsMethodDefinition());
        } else if (prop->IsClassDeclaration() && classDef->IsNamespaceTransformed()) {
            ProcessClassDependencies(prop->AsClassDeclaration());
        }
    }
}

void TSDeclGen::ProcessClassMethodDependencies(const ir::MethodDefinition *methodDef)
{
    if (!methodDef->IsExported() && !methodDef->IsExportedType() && !methodDef->IsDefaultExported()) {
        return;
    }
    auto methDefFunc = methodDef->Function();
    if (methDefFunc == nullptr) {
        return;
    }
    auto sig = methDefFunc->Signature();
    GenSeparated(
        sig->Params(), [this](varbinder::LocalVariable *param) { AddSuperType(param->TsType()); }, "");

    AddSuperType(sig->ReturnType());
}

void TSDeclGen::ProcessInterfaceDependencies(const ir::TSInterfaceDeclaration *interfaceDecl)
{
    if (interfaceDecl->Id()->Name().Mutf8().find('#') != std::string::npos) {
        return;
    }

    if (!interfaceDecl->IsExported() && !interfaceDecl->IsExportedType()) {
        return;
    }

    if (interfaceDecl->TsType() != nullptr && interfaceDecl->TsType()->IsETSObjectType()) {
        ProcessInterfacesDependencies(interfaceDecl->TsType()->AsETSObjectType()->Interfaces());
    }

    if (interfaceDecl->TypeParams() != nullptr) {
        GenSeparated(
            interfaceDecl->TypeParams()->Params(),
            [this](ir::TSTypeParameter *param) {
                if (param->Constraint() == nullptr) {
                    return;
                }
                AddSuperType(param->Constraint());
            },
            "");
    }

    ProcessInterfacePropDependencies(interfaceDecl);
}

void TSDeclGen::ProcessInterfacePropDependencies(const ir::TSInterfaceDeclaration *interfaceDecl)
{
    for (const auto *prop : interfaceDecl->Body()->Body()) {
        if (prop->IsMethodDefinition()) {
            ProcessInterfaceMethodDependencies(prop->AsMethodDefinition());
        }
    }
}

void TSDeclGen::ProcessInterfaceMethodDependencies(const ir::MethodDefinition *methodDef)
{
    auto methDefFunc = methodDef->Function();
    if (methDefFunc == nullptr) {
        return;
    }
    auto sig = methDefFunc->Signature();
    GenSeparated(
        sig->Params(), [this](varbinder::LocalVariable *param) { AddSuperType(param->TsType()); }, "");

    AddSuperType(sig->ReturnType());
}

void TSDeclGen::AddSuperType(const ir::Expression *super)
{
    if (super->TsType() == nullptr) {
        return;
    }
    AddSuperType(super->TsType());
}

void TSDeclGen::AddSuperType(const checker::Type *tsType)
{
    const auto superType = checker::ETSChecker::ETSType(tsType);
    if (superType == checker::TypeFlag::ETS_OBJECT) {
        auto objectType = tsType->AsETSObjectType();
        AddObjectDependencies(objectType->Name());
    } else if (superType == checker::TypeFlag::ETS_UNION) {
        auto unionType = tsType->AsETSUnionType();
        std::vector<checker::Type *> filteredTypes = FilterUnionTypes(unionType->ConstituentTypes());
        GenSeparated(
            filteredTypes, [this](checker::Type *type) { AddSuperType(type); }, "");
    }
}

void TSDeclGen::ProcessInterfacesDependencies(const ArenaVector<checker::ETSObjectType *> &interfaces)
{
    GenSeparated(
        interfaces,
        [this](checker::ETSObjectType *interface) {
            if (checker::ETSChecker::ETSType(interface) == checker::TypeFlag::ETS_OBJECT) {
                AddObjectDependencies(interface->Name());
            }
        },
        "");
}

void TSDeclGen::AddObjectDependencies(const util::StringView &typeName, const std::string &alias)
{
    if (typeName.Empty()) {
        return;
    }
    importSet_.insert(typeName.Mutf8());
    indirectDependencyObjects_.insert(typeName.Mutf8());
    if (!alias.empty()) {
        typeAliasMap_[alias] = typeName.Mutf8();
    }
}

void TSDeclGen::GenDeclarations()
{
    for (auto *globalStatement : program_->Ast()->Statements()) {
        ResetState();
        ResetClassNode();
        const auto jsdoc = compiler::JsdocStringFromDeclaration(globalStatement);
        if (jsdoc.Utf8().find(NON_INTEROP_FLAG) != std::string_view::npos) {
            continue;
        } else if (globalStatement->IsClassDeclaration()) {
            GenClassDeclaration(globalStatement->AsClassDeclaration());
        } else if (globalStatement->IsTSInterfaceDeclaration()) {
            GenInterfaceDeclaration(globalStatement->AsTSInterfaceDeclaration());
        } else if (globalStatement->IsTSTypeAliasDeclaration()) {
            GenTypeAliasDeclaration(globalStatement->AsTSTypeAliasDeclaration());
        } else if (globalStatement->IsETSReExportDeclaration()) {
            GenReExportDeclaration(globalStatement->AsETSReExportDeclaration());
        }
    }
}

void TSDeclGen::GenExportNamedDeclarations()
{
    for (auto *globalStatement : program_->Ast()->Statements()) {
        if (globalStatement->IsExportNamedDeclaration()) {
            GenExportNamedDeclaration(globalStatement->AsExportNamedDeclaration());
        }
    }
}

void TSDeclGen::GenImportDeclarations()
{
    for (auto *globalStatement : program_->Ast()->Statements()) {
        if (globalStatement->IsETSImportDeclaration()) {
            GenImportDeclaration(globalStatement->AsETSImportDeclaration());
        }
    }
}

void TSDeclGen::GenImportRecordDeclarations(const std::string &source)
{
    const std::string recordKey = "Record";
    if (indirectDependencyObjects_.find(recordKey) != indirectDependencyObjects_.end()) {
        OutDts("import type { Record } from \"", source, "\";\n");
    }
}

template <class T, class CB>
void TSDeclGen::GenSeparated(const T &container, const CB &cb, const char *separator, bool isReExport, bool isDtsExport)
{
    if (container.empty()) {
        return;
    }

    cb(container[0]);
    for (std::size_t i = 1; i < container.size(); ++i) {
        if (isReExport) {
            OutTs(separator);
        }
        if (isDtsExport) {
            OutDts(separator);
        }
        cb(container[i]);
    }
}

void TSDeclGen::LogError(const diagnostic::DiagnosticKind &kind, const util::DiagnosticMessageParams &params = {},
                         const lexer::SourcePosition &pos = lexer::SourcePosition())
{
    diagnosticEngine_.LogDiagnostic(kind, params, pos);
}

void TSDeclGen::LogWarning(const diagnostic::DiagnosticKind &kind, const util::DiagnosticMessageParams &params = {},
                           const lexer::SourcePosition &pos = lexer::SourcePosition())
{
    ES2PANDA_ASSERT(kind.Type() == util::DiagnosticType::DECLGEN_ETS2TS_WARNING);
    LogError(kind, params, pos);
}

const ir::Identifier *TSDeclGen::GetKeyIdent(const ir::Expression *key)
{
    if (!key->IsIdentifier()) {
        LogError(diagnostic::IDENT_KEY_SUPPORT, {}, key->Start());
    }

    return key->AsIdentifier();
}

static char const *GetDebugTypeName(const checker::Type *checkerType)
{
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define TYPE_CHECKS(type_flag, typeName)                                                    \
    if (checkerType->Is##typeName()) {                                                      \
        /* CC-OFFNXT(G.PRE.05) The macro is used to generate a function. Return is needed*/ \
        return #typeName;                                                                   \
    }
    TYPE_MAPPING(TYPE_CHECKS)
#undef TYPE_CHECKS
    return "unknown type";
}

void TSDeclGen::GenType(const checker::Type *checkerType)
{
    DebugPrint("  GenType: ");
#if DEBUG_PRINT
    const auto var_name = checkerType->Variable() == nullptr ? "" : checkerType->Variable()->Name().Mutf8();
    DebugPrint(std::string("  Converting type: ") + GetDebugTypeName(checkerType) + " (" + var_name + ")");
#endif

    importSet_.insert(checkerType->ToString());

    if (HandleBasicTypes(checkerType)) {
        return;
    }

    if (checkerType->IsETSFunctionType()) {
        HandleFunctionType(checkerType);
        return;
    }

    if (HandleETSSpecificTypes(checkerType)) {
        return;
    }

    LogError(diagnostic::UNSUPPORTED_TYPE, {GetDebugTypeName(checkerType)});
}

bool TSDeclGen::HandleBasicTypes(const checker::Type *checkerType)
{
    if (checkerType->IsETSEnumType()) {
        OutDts(checkerType->ToString());
        return true;
    }
    if (checkerType->HasTypeFlag(checker::TypeFlag::CHAR)) {
        OutDts("string");
        return true;
    }
    if (checkerType->HasTypeFlag(checker::TypeFlag::ETS_CONVERTIBLE_TO_NUMERIC)) {
        OutDts("number");
        return true;
    }
    return false;
}

void TSDeclGen::HandleFunctionType(const checker::Type *checkerType)
{
    if (!state_.inUnionBodyStack.empty() && state_.inUnionBodyStack.top()) {
        OutDts("(");
        GenFunctionType(checkerType->AsETSFunctionType());
        OutDts(")");
    } else {
        GenFunctionType(checkerType->AsETSFunctionType());
    }
}

bool TSDeclGen::HandleETSSpecificTypes(const checker::Type *checkerType)
{
    switch (checker::ETSChecker::ETSType(checkerType)) {
        case checker::TypeFlag::ETS_VOID:
        case checker::TypeFlag::ETS_NULL:
        case checker::TypeFlag::ETS_UNDEFINED:
        case checker::TypeFlag::ETS_BOOLEAN:
        case checker::TypeFlag::ETS_TYPE_PARAMETER:
        case checker::TypeFlag::ETS_NONNULLISH:
        case checker::TypeFlag::ETS_PARTIAL_TYPE_PARAMETER:
        case checker::TypeFlag::ETS_NEVER:
        case checker::TypeFlag::ETS_READONLY:
            OutDts(checkerType->ToString());
            return true;

        case checker::TypeFlag::ETS_OBJECT:
            return HandleObjectType(checkerType);

        case checker::TypeFlag::ETS_ARRAY:
            HandleArrayType(checkerType);
            return true;

        case checker::TypeFlag::ETS_UNION:
            GenUnionType(checkerType->AsETSUnionType());
            return true;
        case checker::TypeFlag::ETS_TUPLE:
            GenTupleType(checkerType->AsETSTupleType());
            return true;
        case checker::TypeFlag::ETS_ANY:
            OutDts("ESObject");
            return true;
        default:
            LogError(diagnostic::UNSUPPORTED_TYPE, {GetDebugTypeName(checkerType)});
    }
    return false;
}

bool TSDeclGen::HandleObjectType(const checker::Type *checkerType)
{
    std::string typeStr = checkerType->ToString();
    if (typeStr == "Boolean") {
        OutDts("boolean");
    } else if (stringTypes_.count(typeStr) != 0U) {
        OutDts("string");
    } else if (numberTypes_.count(typeStr) != 0U) {
        OutDts("number");
    } else if (typeStr == "BigInt") {
        OutDts("bigint");
    } else if (typeStr == "ESValue") {
        OutDts("ESObject");
    } else {
        GenObjectType(checkerType->AsETSObjectType());
    }
    return true;
}

void TSDeclGen::HandleArrayType(const checker::Type *checkerType)
{
    const auto *elementType = checkerType->AsETSArrayType()->ElementType();
    bool needParentheses = elementType->IsETSUnionType() || elementType->IsETSFunctionType();
    if (needParentheses) {
        OutDts("(");
        GenType(elementType);
        OutDts(")");
    } else {
        GenType(elementType);
    }
    OutDts("[]");
}

void TSDeclGen::GenLiteral(const ir::Literal *literal)
{
    if (literal->IsNumberLiteral()) {
        const auto number = literal->AsNumberLiteral()->Number();
        if (number.IsInt()) {
            OutDts(std::to_string(number.GetInt()));
            return;
        }
        if (number.IsLong()) {
            OutDts(std::to_string(number.GetLong()));
            return;
        }
        if (number.IsFloat()) {
            OutDts(std::to_string(number.GetFloat()));
            return;
        }
        if (number.IsDouble()) {
            OutDts(std::to_string(number.GetDouble()));
            return;
        }
        LogError(diagnostic::UNEXPECTED_NUMBER_LITERAL_TYPE, {}, literal->Start());
    } else if (literal->IsStringLiteral()) {
        const auto string = literal->AsStringLiteral()->ToString();
        importSet_.insert(string);
        OutDts("\"" + string + "\"");
    } else if (literal->IsBooleanLiteral()) {
        OutDts(literal->AsBooleanLiteral()->ToString());
    } else {
        LogError(diagnostic::UNSUPPORTED_LITERAL_TYPE, {}, literal->Start());
    }
}

void TSDeclGen::ProcessParamDefaultToMap(const ir::Statement *stmt)
{
    if (!stmt->IsVariableDeclaration()) {
        return;
    }
    GenSeparated(
        stmt->AsVariableDeclaration()->Declarators(),
        [this](ir::VariableDeclarator *declarator) {
            const auto *init = declarator->Init();
            if (init != nullptr && init->IsConditionalExpression() &&
                init->AsConditionalExpression()->Test()->IsBinaryExpression()) {
                const auto *left = init->AsConditionalExpression()->Test()->AsBinaryExpression()->Left();
                if (left->IsIdentifier()) {
                    const auto varName = GetKeyIdent(declarator->Id())->Name();
                    paramDefaultMap_.insert({left->AsIdentifier()->Name(), varName});
                }
            }
        },
        "");
}

const checker::Signature *TSDeclGen::GetFuncSignature(const checker::ETSFunctionType *etsFunctionType,
                                                      const ir::MethodDefinition *methodDef)
{
    if (etsFunctionType->IsETSArrowType()) {
        return etsFunctionType->ArrowSignature();
    }
    if (methodDef != nullptr) {
        auto methDefFunc = methodDef->Function();
        return methDefFunc != nullptr ? methDefFunc->Signature() : nullptr;
    }
    if (etsFunctionType->CallSignatures().size() != 1) {
        const auto loc = methodDef != nullptr ? methodDef->Start() : lexer::SourcePosition();
        LogError(diagnostic::NOT_OVERLOAD_SUPPORT, {}, loc);
    }
    return etsFunctionType->CallSignatures()[0];
}

void TSDeclGen::ProcessParameterName(varbinder::LocalVariable *param)
{
    const auto *paramDeclNode = param->Declaration()->Node();
    const std::string prefix = "gensym%%_";

    if (!paramDefaultMap_.empty() && paramDefaultMap_.find(param->Name()) != paramDefaultMap_.end()) {
        OutDts(paramDefaultMap_[param->Name()]);
        paramDefaultMap_.erase(param->Name());
    } else if (param->Name().Is("=t")) {
        OutDts("this");
    } else if (paramDeclNode->IsETSParameterExpression() && paramDeclNode->AsETSParameterExpression()->IsOptional() &&
               paramDeclNode->AsETSParameterExpression()->Name().StartsWith(prefix)) {
        OutDts("arg", param->Name().Mutf8().substr(prefix.size()));
    } else {
        OutDts(param->Name());
    }
}

void TSDeclGen::ProcessFuncParameter(varbinder::LocalVariable *param)
{
    if (std::string(param->Name()).find("<property>") != std::string::npos) {
        return;
    }

    ProcessParameterName(param);

    const auto *paramType = param->TsType();
    const auto *paramDeclNode = param->Declaration()->Node();

    if (!paramDeclNode->IsETSParameterExpression()) {
        if (param->HasFlag(varbinder::VariableFlags::OPTIONAL)) {
            OutDts("?");
        }
        OutDts(": ");
        GenType(paramType);
        return;
    }

    const auto *expr = paramDeclNode->AsETSParameterExpression();
    OutDts(expr->IsOptional() ? "?" : "");
    OutDts(": ");

    const auto *typeAnnotation = expr->TypeAnnotation();
    if (typeAnnotation != nullptr) {
        if (expr->IsOptional()) {
            ProcessTypeAnnotationType(typeAnnotation);
            return;
        }
        ProcessTypeAnnotationType(typeAnnotation, paramType);
        return;
    }
    OutDts("ESObject");
}

void TSDeclGen::ProcessFuncParameters(const checker::Signature *sig)
{
    GenSeparated(sig->Params(), [this](varbinder::LocalVariable *param) { ProcessFuncParameter(param); });
}

void TSDeclGen::GenFunctionType(const checker::ETSFunctionType *etsFunctionType, const ir::MethodDefinition *methodDef)
{
    const bool isConstructor = methodDef != nullptr ? methodDef->IsConstructor() : false;
    const bool isSetter = methodDef != nullptr ? methodDef->Kind() == ir::MethodDefinitionKind::SET : false;
    const bool isStatic = methodDef != nullptr ? methodDef->IsStatic() : false;
    // CC-OFFNXT(G.FMT.14-CPP) project code style
    const auto *sig = GetFuncSignature(etsFunctionType, methodDef);
    ES2PANDA_ASSERT(sig != nullptr);
    if (sig->HasFunction()) {
        GenTypeParameters(sig->Function()->TypeParams(), isStatic, sig->Owner()->AsETSObjectType());
        const auto *funcBody = sig->Function()->Body();
        if (funcBody != nullptr && funcBody->IsBlockStatement() &&
            !funcBody->AsBlockStatement()->Statements().empty()) {
            for (const auto *statement : funcBody->AsBlockStatement()->Statements()) {
                ProcessParamDefaultToMap(statement);
            }
        }
    }
    OutDts("(");

    ProcessFuncParameters(sig);

    const auto *sigInfo = sig->GetSignatureInfo();
    if (sigInfo->restVar != nullptr) {
        if (!sig->Params().empty()) {
            OutDts(", ");
        }
        OutDts("...", sigInfo->restVar->Name().Mutf8(), ": ");
        GenType(sigInfo->restVar->TsType());
    }
    OutDts(")");
    if (!isSetter && !isConstructor) {
        OutDts(methodDef != nullptr ? ": " : " => ");
        if (!sig->HasFunction()) {
            GenType(sig->ReturnType());
            return;
        }
        ProcessFunctionReturnType(sig);
    }
}

void TSDeclGen::SplitUnionTypes(std::string &unionTypeString)
{
    std::vector<std::string> result;
    std::string currentType;

    for (char c : unionTypeString) {
        if (std::isspace(c) != 0) {
            continue;
        }
        if (c == '|') {
            if (!currentType.empty()) {
                importSet_.insert(currentType);
                currentType.clear();
            }
        } else {
            currentType += c;
        }
    }
    if (!currentType.empty()) {
        importSet_.insert(currentType);
    }
}

void TSDeclGen::ProcessFunctionReturnType(const checker::Signature *sig)
{
    const auto returnStatements = sig->Function()->ReturnStatements();
    if (!returnStatements.empty() && returnStatements.size() == 1 && returnStatements.at(0)->Argument() != nullptr &&
        returnStatements.at(0)->Argument()->IsETSNewClassInstanceExpression()) {
        auto newExpr = returnStatements.at(0)->Argument()->AsETSNewClassInstanceExpression();
        if (newExpr->GetTypeRef() != nullptr && newExpr->GetTypeRef()->IsETSTypeReference()) {
            ProcessETSTypeReferenceType(newExpr->GetTypeRef()->AsETSTypeReference(), sig->ReturnType());
            return;
        }
    }

    const auto returnTypeAnnotation = sig->Function()->ReturnTypeAnnotation();
    if (returnTypeAnnotation != nullptr) {
        ProcessTypeAnnotationType(returnTypeAnnotation, sig->ReturnType());
        return;
    }

    if (sig->HasSignatureFlag(checker::SignatureFlags::SETTER)) {
        const auto param = sig->Function()->Params();
        if (!param.empty() && param.size() == 1 && param.at(0)->IsETSParameterExpression() &&
            param.at(0)->AsETSParameterExpression()->Ident()->TypeAnnotation() != nullptr) {
            ProcessTypeAnnotationType(param.at(0)->AsETSParameterExpression()->Ident()->TypeAnnotation(),
                                      sig->Params()[0]->TsType());
            return;
        }
        if (!sig->Params().empty() && sig->Params().size() == 1) {
            GenType(sig->Params()[0]->TsType());
            return;
        }
    }

    std::string typeStr = sig->ReturnType()->ToString();
    if (declgenOptions_.isolated && typeStr.find(ERROR_TYPE) != std::string::npos) {
        typeStr = isolatedDeclgenChecker_->Check(const_cast<ir::ScriptFunction *>(sig->Function()));
        OutDts(typeStr);
        SplitUnionTypes(typeStr);
        return;
    }
    GenType(sig->ReturnType());
}

void TSDeclGen::GenUnionType(const checker::ETSUnionType *unionType)
{
    state_.inUnionBodyStack.push(true);
    std::vector<checker::Type *> filteredTypes = FilterUnionTypes(unionType->ConstituentTypes());
    GenSeparated(
        filteredTypes, [this](checker::Type *arg) { GenType(arg); }, " | ");
    state_.inUnionBodyStack.pop();
}

template <class UnionType>
std::vector<UnionType *> TSDeclGen::FilterUnionTypes(const ArenaVector<UnionType *> &originTypes)
{
    if (originTypes.empty()) {
        return {};
    }
    bool hasNumber = false;
    bool hasString = false;
    std::vector<UnionType *> filteredTypes;
    for (auto originType : originTypes) {
        std::string typeStr = originType->ToString();
        if constexpr (std::is_same_v<UnionType, ir::TypeNode>) {
            if (originType->IsTSThisType()) {
                filteredTypes.push_back(originType);
                continue;
            }
            auto type = originType->GetType(checker_);
            if (type == nullptr) {
                continue;
            }
            typeStr = type->ToString();
            typeStr[0] = std::toupper(typeStr[0]);
        }
        if (stringTypes_.count(typeStr) != 0U) {
            if (hasString) {
                continue;
            }
            hasString = true;
        } else if (numberTypes_.count(typeStr) != 0U) {
            if (hasNumber) {
                continue;
            }
            hasNumber = true;
        }
        filteredTypes.push_back(originType);
    }
    return filteredTypes;
}

void TSDeclGen::GenTupleType(const checker::ETSTupleType *tupleType)
{
    OutDts("[");
    GenSeparated(
        tupleType->GetTupleTypesList(), [this](checker::Type *arg) { GenType(arg); }, " , ");
    OutDts("]");
}

bool TSDeclGen::HandleSpecificObjectTypes(const checker::ETSObjectType *objectType)
{
    if (objectType->IsETSStringType()) {
        OutDts("string");
        return true;
    }
    if (objectType->IsETSBigIntType()) {
        OutDts("bigint");
        return true;
    }
    if (objectType->IsETSUnboxableObject()) {
        OutDts("number");  // NOTE(ivagin): create precise builtin type
        return true;
    }
    if (objectType->HasObjectFlag(checker::ETSObjectFlags::FUNCTIONAL)) {
        const auto *invoke = objectType->GetFunctionalInterfaceInvokeType();
        ES2PANDA_ASSERT(invoke && invoke->IsETSFunctionType());
        GenType(invoke);
        return true;
    }
    return false;
}

void TSDeclGen::HandleTypeArgument(checker::Type *arg, const std::string &typeStr)
{
    if (typeStr == "Promise" && arg != nullptr && arg->HasTypeFlag(checker::TypeFlag::ETS_UNDEFINED)) {
        OutDts("void");
    } else if (arg != nullptr) {
        if (!state_.currentTypeAliasName.empty() && !arg->HasTypeFlag(checker::TypeFlag::ETS_TYPE_PARAMETER) &&
            (arg->IsETSObjectType() && !arg->AsETSObjectType()->HasObjectFlag(checker::ETSObjectFlags::BUILTIN_TYPE))) {
            OutDts(state_.currentTypeAliasName);
            if (state_.currentTypeParams != nullptr) {
                importSet_.insert(state_.currentTypeParams->Params()[0]->Name()->Name().Mutf8());
                OutDts("<");
                OutDts(state_.currentTypeParams->Params()[0]->Name()->Name());
                OutDts(">");
            }
        } else {
            GenType(arg);
        }
    }
}

void TSDeclGen::GenObjectType(const checker::ETSObjectType *objectType)
{
    if (HandleSpecificObjectTypes(objectType)) {
        return;
    }
    std::string typeStr = objectType->Name().Mutf8();
    if (objectType->Name().Empty()) {
        LogWarning(diagnostic::EMPTY_TYPE_NAME);
        OutDts("ESObject");
    } else {
        if (typeStr == "Exception" || typeStr == "NullPointerError") {
            OutDts("Error");
        } else if (size_t partialPos = typeStr.find("$partial"); partialPos != std::string::npos) {
            OutDts("Partial<", typeStr.substr(0, partialPos), ">");
        } else {
            OutDts(typeStr);
        }
        importSet_.insert(typeStr);
        indirectDependencyObjects_.insert(typeStr);
    }

    const auto &typeArgs = objectType->TypeArguments();
    if (typeArgs.empty()) {
        return;
    }

    OutDts("<");
    GenSeparated(typeArgs, [this, typeStr](checker::Type *arg) { HandleTypeArgument(arg, typeStr); });
    OutDts(">");
}

void TSDeclGen::GenTypeParameters(const ir::TSTypeParameterDeclaration *typeParams, bool isStatic,
                                  const checker::ETSObjectType *ownerObj)
{
    if (typeParams != nullptr) {
        OutDts("<");
        GenSeparated(typeParams->Params(), [this](ir::TSTypeParameter *param) {
            OutDts(param->Name()->Name());
            auto *constraint = param->Constraint();
            if (constraint != nullptr) {
                OutDts(" extends ");
                GenType(constraint->GetType(checker_));
            }
            auto *defaultType = param->DefaultType();
            if (defaultType != nullptr) {
                OutDts(" = ");
                defaultType->IsETSTypeReference() ? ProcessETSTypeReferenceType(defaultType->AsETSTypeReference())
                                                  : GenType(defaultType->TsType());
            }
        });
        OutDts(">");
        return;
    }
    if (ownerObj == nullptr || !isStatic) {
        return;
    }
    auto typeArguments = ownerObj->TypeArguments();
    if (!typeArguments.empty()) {
        OutDts("<");
        for (auto arg : typeArguments) {
            OutDts(arg->ToString());
            if (arg != typeArguments.back()) {
                OutDts(",");
            }
        }
        OutDts(">");
    }
}

void TSDeclGen::GenExport(const ir::Identifier *symbol)
{
    const auto symbolName = symbol->Name().Mutf8();
    OutDts("export {", symbolName, "};");
    OutEndlDts();
    if (!symbol->Parent()->IsTSTypeAliasDeclaration() && !symbol->Parent()->IsTSInterfaceDeclaration()) {
        OutDts("exports.", symbolName, " = ", symbolName, ";");
    }
    OutEndlDts();
}

void TSDeclGen::GenExport(const ir::Identifier *symbol, const std::string &alias)
{
    const auto symbolName = symbol->Name().Mutf8();
    OutDts("export {", symbolName, " as ", alias, "};");
    OutEndlDts();
    if (!symbol->Parent()->IsTSTypeAliasDeclaration() && !symbol->Parent()->IsTSInterfaceDeclaration()) {
        OutDts("exports.", alias, " = ", symbolName, ";");
    }
    OutEndlDts();
}

void TSDeclGen::GenDefaultExport(const ir::Identifier *symbol)
{
    const auto symbolName = symbol->Name().Mutf8();
    OutDts("export default ", symbolName, ";");
    OutEndlDts();
    if (!symbol->Parent()->IsTSTypeAliasDeclaration() && !symbol->Parent()->IsTSInterfaceDeclaration()) {
        OutDts("exports.default = ", symbolName, ";");
    }
    OutEndlDts();
}

bool TSDeclGen::ShouldEmitDeclarationSymbol(const ir::Identifier *symbol)
{
    if (declgenOptions_.exportAll) {
        return true;
    }
    if (symbol->Parent()->IsExported() || symbol->Parent()->IsDefaultExported()) {
        return true;
    }
    if (state_.isDeclareNamespace) {
        return true;
    }
    if (indirectDependencyObjects_.find(symbol->Name().Mutf8()) != indirectDependencyObjects_.end()) {
        classNode_.isIndirect = true;
        return true;
    }

    return false;
}

template <class T>
void TSDeclGen::GenModifier(const T *node, bool isProp)
{
    if (state_.inInterface) {
        return;
    }

    if (state_.inNamespace && isProp && !state_.isClassInNamespace) {
        OutDts(node->IsConst() ? "const " : "let ");
        return;
    }
    if (state_.inNamespace && !isProp && !state_.isClassInNamespace) {
        OutDts("function ");
        return;
    }
    if (node->IsPublic()) {
        OutDts("public ");
    } else if (node->IsPrivate()) {
        OutDts("private ");
    } else if (node->IsProtected()) {
        OutDts("protected ");
    }
    if (node->IsStatic()) {
        OutDts("static ");
    }
    if (node->IsReadonly() && isProp) {
        OutDts("readonly ");
    }
}

std::string TSDeclGen::RemoveModuleExtensionName(const std::string &filepath)
{
    for (const auto &extension : extensions_) {
        auto pos = filepath.rfind(extension);
        if (pos != std::string::npos && pos == filepath.length() - extension.length()) {
            return filepath.substr(0, pos);
        }
    }
    return filepath;
}

template <class T>
void TSDeclGen::GenAnnotations(const ir::AnnotationAllowed<T> *node)
{
    if (node == nullptr || !node->HasAnnotations()) {
        return;
    }
    GenSeparated(
        node->Annotations(),
        [this](ir::AnnotationUsage *anno) {
            if (annotationList_.count(anno->GetBaseName()->Name().Mutf8()) == 0U) {
                return;
            }
            if (!state_.inGlobalClass && (state_.inClass || state_.inInterface)) {
                ProcessIndent();
            }
            importSet_.insert(anno->GetBaseName()->Name().Mutf8());
            OutDts("@", anno->GetBaseName()->Name());
            GenAnnotationProperties(anno);
            OutEndlDts();
        },
        "");
}

void TSDeclGen::GenAnnotationProperties(const ir::AnnotationUsage *anno)
{
    if (anno->Properties().empty()) {
        return;
    }

    const auto &properties = anno->Properties();
    if (properties.size() == 1 && properties.at(0)->IsClassProperty() &&
        properties.at(0)->AsClassProperty()->Id() != nullptr &&
        properties.at(0)->AsClassProperty()->Id()->Name() == compiler::Signatures::ANNOTATION_KEY_VALUE) {
        OutDts("(");
        if (properties.at(0)->AsClassProperty()->Value() != nullptr) {
            GenAnnotationPropertyValue(properties.at(0)->AsClassProperty()->Value());
        }
        OutDts(")");
        return;
    }

    OutDts("({");
    OutEndlDts();
    for (auto *prop : properties) {
        ProcessIndent();
        ES2PANDA_ASSERT(prop->AsClassProperty()->Id() != nullptr);
        OutDts(prop->AsClassProperty()->Id()->Name());
        OutDts(": ");
        if (prop->AsClassProperty()->Value() != nullptr) {
            GenAnnotationPropertyValue(prop->AsClassProperty()->Value());
        }
        if (prop != properties.back()) {
            OutDts(",");
        }
        OutEndlDts();
    }
    OutDts("})");
}

void TSDeclGen::GenAnnotationPropertyValue(ir::Expression *propValue)
{
    if (propValue->IsLiteral()) {
        GenLiteral(propValue->AsLiteral());
    } else if (propValue->IsArrayExpression()) {
        OutDts("[");
        GenSeparated(propValue->AsArrayExpression()->Elements(),
                     [this](ir::Expression *element) { GenAnnotationPropertyValue(element); });
        OutDts("]");
    } else {
        GenType(propValue->Check(checker_));
    }
}

void TSDeclGen::GenExportNamedDeclaration(const ir::ExportNamedDeclaration *exportDeclaration)
{
    DebugPrint("GenExportNamedDeclaration");
    const auto &specifiers = exportDeclaration->Specifiers();
    if (specifiers.empty()) {
        return;
    }
    GenNamedExports(exportDeclaration, specifiers);
}

void TSDeclGen::GenImportDeclaration(const ir::ETSImportDeclaration *importDeclaration)
{
    DebugPrint("GenImportDeclaration");
    if (importDeclaration->IsPureDynamic()) {
        return;
    }
    const auto &specifiers = importDeclaration->Specifiers();
    if (specifiers.empty()) {
        return;
    }
    auto source = importDeclaration->Source()->Str().Mutf8();
    source = RemoveModuleExtensionName(source);
    const auto specifierFirst = specifiers[0];
    bool isTypeKind = importDeclaration->IsTypeKind();
    if (specifierFirst->IsImportNamespaceSpecifier()) {
        GenNamespaceImport(specifierFirst, source);
    } else if (specifierFirst->IsImportDefaultSpecifier()) {
        GenDefaultImport(specifierFirst, source, isTypeKind);
    } else if (specifierFirst->IsImportSpecifier()) {
        GenNamedImports(importDeclaration, specifiers, isTypeKind);
    }
}

void TSDeclGen::GenNamespaceImport(const ir::AstNode *specifier, const std::string &source)
{
    const auto local = specifier->AsImportNamespaceSpecifier()->Local()->Name();
    OutTs("import * as ", local, " from \"", source, "\";");
    OutEndlTs();
    if (importSet_.find(local.Mutf8()) == importSet_.end()) {
        return;
    }
    OutDts("import * as ", local, " from \"", source, "\";");
    OutEndlDts();
}

void TSDeclGen::GenDefaultImport(const ir::AstNode *specifier, const std::string &source, bool isTypeKind)
{
    auto importDefaultSpecifier = specifier->AsImportDefaultSpecifier();
    auto variable = importDefaultSpecifier->Local()->Variable();
    const auto local = importDefaultSpecifier->Local()->Name().Mutf8();
    bool isTypeDeclaration = false;
    if (variable != nullptr && variable->Declaration() != nullptr && variable->Declaration()->Node() != nullptr) {
        auto *node = variable->Declaration()->Node();
        isTypeDeclaration = node->IsTSTypeAliasDeclaration() || node->IsTSInterfaceDeclaration();
    }
    if (!isTypeKind && !isTypeDeclaration) {
        OutTs("import ", local, " from \"", source, "\";");
        OutEndlTs();
    }

    if (importSet_.find(local) == importSet_.end()) {
        return;
    }
    OutDts(isTypeKind ? "import type " : "import ", local, " from \"", source, "\";");
    OutEndlDts();
}

void TSDeclGen::GenNamedImports(const ir::ETSImportDeclaration *importDeclaration,
                                const ArenaVector<ir::AstNode *> &specifiers, bool isTypeKind)
{
    if (specifiers.empty()) {
        return;
    }
    std::vector<ir::AstNode *> interfaceSpecifiers;
    std::vector<ir::AstNode *> normalSpecifiers;
    SeparateInterfaceSpecifiers(specifiers, interfaceSpecifiers, normalSpecifiers);

    if (!isTypeKind) {
        GenTsImportStatement(normalSpecifiers, importDeclaration);
    }

    auto importSpecifiers = FilterValidImportSpecifiers(specifiers);
    GenDtsImportStatement(importSpecifiers, importDeclaration, isTypeKind);
}

void TSDeclGen::GenTsImportStatement(std::vector<ir::AstNode *> &specifiers,
                                     const ir::ETSImportDeclaration *importDeclaration, bool isInterface)
{
    if (specifiers.empty()) {
        return;
    }

    auto source = importDeclaration->Source()->Str().Mutf8();
    source = RemoveModuleExtensionName(source);
    OutTs(isInterface ? "import type" : "import", " { ");

    GenSeparated(
        specifiers,
        [this, importDeclaration](ir::AstNode *specifier) { GenSingleNamedImport(specifier, importDeclaration, true); },
        ", ", true, false);

    OutTs(" } from \"", source, "\";\n");
}

void TSDeclGen::GenDtsImportStatement(std::vector<ir::AstNode *> &specifiers,
                                      const ir::ETSImportDeclaration *importDeclaration, bool isTypeKind)
{
    if (specifiers.empty()) {
        return;
    }

    auto source = importDeclaration->Source()->Str().Mutf8();
    source = RemoveModuleExtensionName(source);
    OutDts(isTypeKind ? "import type" : "import", " { ");

    GenSeparated(
        specifiers,
        [this, importDeclaration](ir::AstNode *specifier) { GenSingleNamedImport(specifier, importDeclaration); },
        ", ");

    OutDts(" } from \"", source, "\";\n");
}

void TSDeclGen::GenNamedExports(const ir::ExportNamedDeclaration *exportDeclaration,
                                const ArenaVector<ir::ExportSpecifier *> &specifiers)
{
    auto exportSpecifiers = FilterValidExportSpecifiers(specifiers);
    if (exportSpecifiers.empty()) {
        return;
    }
    if ((exportDeclaration->Modifiers() & (ir::ModifierFlags::DEFAULT_EXPORT)) != 0U) {
        const auto local = specifiers[0]->Local()->Name().Mutf8();
        importSet_.insert(local);
        OutDts("export default ", local, ";\n");
        if (glueCodeImportSet_.find(local) != glueCodeImportSet_.end()) {
            OutTs("export default ", local, ";\n");
        }
        return;
    }

    if ((exportDeclaration->Modifiers() & (ir::ModifierFlags::EXPORT_TYPE)) != 0U) {
        OutDts("export type { ");
    } else {
        OutDts("export { ");
    }

    GenSeparated(
        exportSpecifiers, [this](ir::AstNode *specifier) { GenSingleNamedExport(specifier); }, ", ");
    OutDts(" };\n");

    if ((exportDeclaration->Modifiers() & (ir::ModifierFlags::EXPORT_TYPE)) != 0U) {
        return;
    }
    auto guleCodeExportSpecifiers = FilterGlueCodeExportSpecifiers(exportSpecifiers);
    if (guleCodeExportSpecifiers.empty()) {
        return;
    }
    OutTs("export { ");
    GenSeparated(
        guleCodeExportSpecifiers, [this](ir::AstNode *specifier) { GenSingleNamedExport(specifier, true); }, ", ", true,
        false);
    OutTs(" };\n");
}

void TSDeclGen::GenSingleNamedImport(ir::AstNode *specifier, const ir::ETSImportDeclaration *importDeclaration,
                                     bool isGlueCode)
{
    if (!specifier->IsImportSpecifier()) {
        LogError(diagnostic::IMPORT_SPECIFIERS_SUPPORT, {}, importDeclaration->Start());
    }
    const auto local = specifier->AsImportSpecifier()->Local()->Name().Mutf8();
    const auto imported = specifier->AsImportSpecifier()->Imported()->Name().Mutf8();
    if (local != imported) {
        isGlueCode ? OutTs(imported, " as ", local) : OutDts(imported, " as ", local);
    } else {
        isGlueCode ? OutTs(local) : OutDts(local);
    }
}

void TSDeclGen::GenSingleNamedExport(ir::AstNode *specifier, bool isGlueCode)
{
    const auto local = specifier->AsExportSpecifier()->Local()->Name().Mutf8();
    const auto imported = specifier->AsExportSpecifier()->Exported()->Name().Mutf8();
    if (local != imported) {
        importSet_.insert(imported);
        isGlueCode ? OutTs(imported, " as ", local) : OutDts(imported, " as ", local);
    } else {
        importSet_.insert(imported);
        isGlueCode ? OutTs(local) : OutDts(local);
    }
}

std::vector<ir::AstNode *> TSDeclGen::FilterValidExportSpecifiers(const ArenaVector<ir::ExportSpecifier *> &specifiers)
{
    std::vector<ir::AstNode *> exportSpecifiers;
    for (auto specifier : specifiers) {
        const auto local = specifier->AsExportSpecifier()->Local()->Name().Mutf8();
        if (exportSet_.find(local) == exportSet_.end()) {
            exportSpecifiers.push_back(specifier);
        }
    }
    return exportSpecifiers;
}

std::vector<ir::AstNode *> TSDeclGen::FilterGlueCodeExportSpecifiers(const std::vector<ir::AstNode *> &specifiers)
{
    std::vector<ir::AstNode *> glueCodeExportSpecifiers;
    for (auto specifier : specifiers) {
        const auto local = specifier->AsExportSpecifier()->Local()->Name().Mutf8();
        if (glueCodeImportSet_.find(local) != glueCodeImportSet_.end()) {
            glueCodeExportSpecifiers.push_back(specifier);
        }
    }
    return glueCodeExportSpecifiers;
}

std::vector<ir::AstNode *> TSDeclGen::FilterValidImportSpecifiers(const ArenaVector<ir::AstNode *> &specifiers)
{
    std::vector<ir::AstNode *> importSpecifiers;
    for (auto specifier : specifiers) {
        if (specifier->AsImportSpecifier()->IsRemovable()) {
            continue;
        }
        const auto local = specifier->AsImportSpecifier()->Local()->Name().Mutf8();
        if (importSet_.find(local) != importSet_.end()) {
            importSpecifiers.push_back(specifier);
        }
    }
    return importSpecifiers;
}

void TSDeclGen::GenReExportDeclaration(const ir::ETSReExportDeclaration *reExportDeclaration)
{
    DebugPrint("GenReExportDeclaration");
    auto importDeclaration = reExportDeclaration->GetETSImportDeclarations();
    if (importDeclaration->IsPureDynamic()) {
        return;
    }
    const auto &specifiers = importDeclaration->Specifiers();

    if (specifiers.size() == 1 && GenNamespaceReExportDeclaration(specifiers[0], importDeclaration)) {
        return;
    }

    bool isTypeKind = reExportDeclaration->IsExportedType();
    std::vector<ir::AstNode *> interfaceSpecifiers;
    std::vector<ir::AstNode *> normalSpecifiers;
    SeparateInterfaceSpecifiers(specifiers, interfaceSpecifiers, normalSpecifiers);

    GenDtsReExportStatement(specifiers, importDeclaration, isTypeKind);

    if (!isTypeKind) {
        GenTsReExportStatement(normalSpecifiers, importDeclaration);
    }
}

bool TSDeclGen::GenNamespaceReExportDeclaration(const ir::AstNode *specifier,
                                                const ir::ETSImportDeclaration *importDeclaration)
{
    if (specifier->IsImportNamespaceSpecifier()) {
        const auto local = specifier->AsImportNamespaceSpecifier()->Local()->Name();
        if (local.Empty()) {
            auto source = importDeclaration->Source()->Str().Mutf8();
            source = RemoveModuleExtensionName(source);
            OutDts("export * from \"", source, "\";\n");
            OutTs("export * from \"", source, "\";\n");
            return true;
        }
    }
    return false;
}

void TSDeclGen::SeparateInterfaceSpecifiers(const ArenaVector<ir::AstNode *> &specifiers,
                                            std::vector<ir::AstNode *> &interfaceSpecifiers,
                                            std::vector<ir::AstNode *> &normalSpecifiers)
{
    for (auto *specifier : specifiers) {
        if (!specifier->IsImportSpecifier()) {
            continue;
        }
        auto importSpecifier = specifier->AsImportSpecifier();
        auto variable = importSpecifier->Imported()->Variable();
        bool isTypeDeclaration = false;
        if (variable != nullptr && variable->Declaration() != nullptr && variable->Declaration()->Node() != nullptr) {
            auto *node = variable->Declaration()->Node();
            isTypeDeclaration = node->IsTSTypeAliasDeclaration() || node->IsTSInterfaceDeclaration();
        }
        if (isTypeDeclaration) {
            interfaceSpecifiers.push_back(specifier);
        } else {
            normalSpecifiers.push_back(specifier);
        }
    }
}

void TSDeclGen::GenSingleNamedReExport(ir::AstNode *specifier, const ir::ETSImportDeclaration *importDeclaration,
                                       bool isGlueCode)
{
    if (specifier->IsImportSpecifier()) {
        const auto local = specifier->AsImportSpecifier()->Local()->Name().Mutf8();
        const auto imported = specifier->AsImportSpecifier()->Imported()->Name().Mutf8();
        importSet_.insert(local);
        if (local != imported) {
            isGlueCode ? OutTs(imported, " as ", local) : OutDts(imported, " as ", local);
        } else {
            isGlueCode ? OutTs(local) : OutDts(local);
        }
    } else if (specifier->IsImportNamespaceSpecifier()) {
        const auto local = specifier->AsImportNamespaceSpecifier()->Local()->Name().Mutf8();
        importSet_.insert(local);
        isGlueCode ? OutTs(local) : OutDts(local);
    } else {
        LogError(diagnostic::IMPORT_SPECIFIERS_SUPPORT, {}, importDeclaration->Start());
    }
}

void TSDeclGen::GenDtsReExportStatement(const ArenaVector<ir::AstNode *> &specifiers,
                                        const ir::ETSImportDeclaration *importDeclaration, bool isTypeKind)
{
    if (specifiers.empty()) {
        return;
    }

    auto source = importDeclaration->Source()->Str().Mutf8();
    source = RemoveModuleExtensionName(source);
    OutDts(isTypeKind ? "export type" : "export", " { ");

    GenSeparated(
        specifiers,
        [this, importDeclaration](ir::AstNode *specifier) { GenSingleNamedReExport(specifier, importDeclaration); },
        ", ");

    OutDts(" } from \"", source, "\";\n");
}

void TSDeclGen::GenTsReExportStatement(const std::vector<ir::AstNode *> &specifiers,
                                       const ir::ETSImportDeclaration *importDeclaration, bool isInterface)
{
    if (specifiers.empty()) {
        return;
    }
    auto source = importDeclaration->Source()->Str().Mutf8();
    source = RemoveModuleExtensionName(source);
    OutTs(isInterface ? "export type" : "export", " { ");

    GenSeparated(
        specifiers,
        [this, importDeclaration](ir::AstNode *specifier) {
            GenSingleNamedReExport(specifier, importDeclaration, true);
        },
        ", ", true, false);

    OutTs(" } from \"", source, "\";\n");
}

std::string TSDeclGen::ReplaceETSGLOBAL(const std::string &typeName)
{
    if (typeName.empty()) {
        return globalDesc_;
    }
    const std::string target = "ETSGLOBAL";
    std::size_t pos = globalDesc_.find(target);
    if (pos != std::string::npos) {
        return globalDesc_.substr(0, pos) + typeName + globalDesc_.substr(pos + target.length());
    }
    return globalDesc_;
}

bool TSDeclGen::ProcessTSQualifiedName(const ir::ETSTypeReference *typeReference)
{
    if (typeReference->Part()->Name()->IsTSQualifiedName() &&
        typeReference->Part()->Name()->AsTSQualifiedName()->Name() != nullptr) {
        const auto qualifiedName = typeReference->Part()->Name()->AsTSQualifiedName()->Name().Mutf8();
        std::istringstream stream(qualifiedName);
        std::string firstSegment;
        if (std::getline(stream, firstSegment, '.') && stdlibNamespaceList_.count(firstSegment) != 0U) {
            OutDts("ESObject");
            return true;
        }
        importSet_.insert(firstSegment);
        indirectDependencyObjects_.insert(firstSegment);
        std::string segment;
        while (std::getline(stream, segment, '.')) {
            importSet_.insert(segment);
            indirectDependencyObjects_.insert(segment);
        }
        OutDts(qualifiedName);
        return true;
    }
    return false;
}

void TSDeclGen::ProcessETSTypeReferenceType(const ir::ETSTypeReference *typeReference, const checker::Type *checkerType)
{
    auto typePart = typeReference->Part();
    auto partName = typePart->GetIdent()->Name().Mutf8();
    if (partName == "Type" || partName == "Function0") {
        OutDts("ESObject");
        return;
    }
    importSet_.insert(partName);
    if (typePart->TypeParams() != nullptr && typePart->TypeParams()->IsTSTypeParameterInstantiation()) {
        indirectDependencyObjects_.insert(partName);
        if (partName == "FixedArray") {
            GenSeparated(typePart->TypeParams()->Params(),
                         [this](ir::TypeNode *param) { ProcessTypeAnnotationType(param, param->GetType(checker_)); });
            OutDts("[]");
            return;
        }
        OutDts(partName);
        OutDts("<");
        GenSeparated(typePart->TypeParams()->Params(),
                     [this](ir::TypeNode *param) { ProcessTypeAnnotationType(param, param->GetType(checker_)); });
        OutDts(">");
    } else if (ProcessTSQualifiedName(typeReference)) {
        return;
    } else if (checkerType != nullptr && checkerType->IsETSFunctionType()) {
        indirectDependencyObjects_.insert(partName);
        OutDts(partName);
    } else {
        GenPartName(partName);
        indirectDependencyObjects_.insert(partName);
        OutDts(partName);
    }
}

bool TSDeclGen::ProcessTypeAnnotationSpecificTypes(const checker::Type *checkerType)
{
    if (checkerType == nullptr) {
        return false;
    }

    importSet_.insert(checkerType->ToString());
    indirectDependencyObjects_.insert(checkerType->ToString());
    if (HandleBasicTypes(checkerType)) {
        return true;
    }
    switch (checker::ETSChecker::ETSType(checkerType)) {
        case checker::TypeFlag::ETS_VOID:
        case checker::TypeFlag::ETS_NULL:
        case checker::TypeFlag::ETS_UNDEFINED:
        case checker::TypeFlag::ETS_BOOLEAN:
        case checker::TypeFlag::ETS_TYPE_PARAMETER:
        case checker::TypeFlag::ETS_NONNULLISH:
        case checker::TypeFlag::ETS_PARTIAL_TYPE_PARAMETER:
        case checker::TypeFlag::ETS_NEVER:
        case checker::TypeFlag::ETS_READONLY:
            OutDts(checkerType->ToString());
            return true;
        case checker::TypeFlag::ETS_ANY:
            OutDts("ESObject");
            return true;
        default:
            return false;
    }
    return false;
}

void TSDeclGen::ProcessTypeAnnotationType(const ir::TypeNode *typeAnnotation, const checker::Type *checkerType)
{
    auto *aliasedType = const_cast<ir::TypeNode *>(typeAnnotation)->GetType(checker_);

    if (typeAnnotation->IsTSThisType()) {
        OutDts("this");
        return;
    }
    if (typeAnnotation->IsETSPrimitiveType() &&
        typeAnnotation->AsETSPrimitiveType()->GetPrimitiveType() == ir::PrimitiveType::VOID) {
        OutDts("void");
        return;
    }
    if (typeAnnotation->IsETSStringLiteralType() && aliasedType != nullptr) {
        importSet_.insert(aliasedType->ToString());
        OutDts(aliasedType->ToString());
        return;
    }
    if (typeAnnotation->IsETSTypeReference()) {
        ProcessETSTypeReference(typeAnnotation, checkerType);
        return;
    }
    if (typeAnnotation->IsETSTuple()) {
        ProcessETSTuple(typeAnnotation->AsETSTuple());
        return;
    }
    if (typeAnnotation->IsETSUnionType()) {
        ProcessETSUnionType(typeAnnotation->AsETSUnionType());
        return;
    }
    if (typeAnnotation->IsTSArrayType() && typeAnnotation->AsTSArrayType()->ElementType() != nullptr) {
        ProcessTSArrayType(typeAnnotation->AsTSArrayType());
        return;
    }
    if (typeAnnotation->IsETSFunctionType()) {
        ProcessETSFunctionType(typeAnnotation->AsETSFunctionType());
        return;
    }
    checkerType != nullptr ? GenType(checkerType) : GenType(aliasedType);
}

void TSDeclGen::ProcessETSTypeReference(const ir::TypeNode *typeAnnotation, const checker::Type *checkerType)
{
    if (ProcessTSQualifiedName(typeAnnotation->AsETSTypeReference())) {
        return;
    }
    if (ProcessTypeAnnotationSpecificTypes(checkerType)) {
        return;
    }
    if (checkerType != nullptr && typeAnnotation->AsETSTypeReference()->Part()->GetIdent()->Name().Is("NullishType")) {
        OutDts(typeAnnotation->Parent()->IsTSArrayType() ? "(" : "");
        GenType(checkerType);
        OutDts(typeAnnotation->Parent()->IsTSArrayType() ? ")" : "");
        return;
    }
    ProcessETSTypeReferenceType(typeAnnotation->AsETSTypeReference(), checkerType);
}

void TSDeclGen::ProcessETSTuple(const ir::ETSTuple *etsTuple)
{
    OutDts("[");
    GenSeparated(
        etsTuple->GetTupleTypeAnnotationsList(),
        [this](ir::TypeNode *arg) { ProcessTypeAnnotationType(arg, arg->GetType(checker_)); }, " , ");
    OutDts("]");
}

void TSDeclGen::ProcessETSUnionType(const ir::ETSUnionType *etsUnionType)
{
    state_.inUnionBodyStack.push(true);
    std::vector<ir::TypeNode *> filteredTypes = FilterUnionTypes(etsUnionType->Types());
    GenSeparated(
        filteredTypes, [this](ir::TypeNode *arg) { ProcessTypeAnnotationType(arg, arg->GetType(checker_)); }, " | ");
    state_.inUnionBodyStack.pop();
}

void TSDeclGen::ProcessTSArrayType(const ir::TSArrayType *tsArrayType)
{
    auto *elementType = tsArrayType->ElementType();
    auto *elementCheckerType = const_cast<ir::TypeNode *>(elementType)->GetType(checker_);
    if (elementCheckerType == nullptr) {
        return;
    }
    bool needParentheses = !elementType->IsETSTypeReference() && elementCheckerType->IsETSUnionType();
    OutDts(needParentheses ? "(" : "");
    ProcessTypeAnnotationType(elementType, elementCheckerType);
    OutDts(needParentheses ? ")" : "");
    OutDts("[]");
}

void TSDeclGen::ProcessETSFunctionType(const ir::ETSFunctionType *etsFunction)
{
    if (etsFunction->TypeParams() != nullptr) {
        GenTypeParameters(etsFunction->TypeParams());
    }
    bool inUnionBody = !state_.inUnionBodyStack.empty() && state_.inUnionBodyStack.top();
    OutDts(inUnionBody ? "((" : "(");
    GenSeparated(etsFunction->Params(), [this](ir::Expression *param) {
        const auto paramExpr = param->AsETSParameterExpression();
        const auto paramName = paramExpr->Name();
        const bool isRestParam = paramExpr->IsRestParameter();
        const bool isOptional = paramExpr->IsOptional();
        OutDts(isRestParam ? "..." : "", paramName.Is("=t") ? "this" : paramName, isOptional ? "?: " : ": ");
        ProcessTypeAnnotationType(paramExpr->TypeAnnotation(), paramExpr->TypeAnnotation()->TsType());
    });
    OutDts(") => ");
    ProcessTypeAnnotationType(etsFunction->ReturnType(), etsFunction->ReturnType()->TsType());
    OutDts(inUnionBody ? ")" : "");
}

void TSDeclGen::GenTypeAliasDeclaration(const ir::TSTypeAliasDeclaration *typeAlias)
{
    const auto name = typeAlias->Id()->Name().Mutf8();
    state_.currentTypeAliasName = name;
    state_.currentTypeParams = typeAlias->TypeParams();
    DebugPrint("GenTypeAliasDeclaration: " + name);
    if (!ShouldEmitDeclarationSymbol(typeAlias->Id())) {
        return;
    }
    if (state_.inClass) {
        auto indent = GetIndent();
        OutDts(indent);
        OutTs(indent);
    }
    GenAnnotations(typeAlias);
    if (classNode_.isIndirect || state_.inNamespace || typeAlias->IsDefaultExported()) {
        OutDts("type ", name);
    } else {
        exportSet_.insert(name);
        OutDts("export type ", name);
    }
    GenTypeParameters(typeAlias->TypeParams());
    OutDts(" = ");
    ProcessTypeAnnotationType(typeAlias->TypeAnnotation(), typeAlias->TypeAnnotation()->GetType(checker_));
    OutDts(";");
    OutEndlDts();
    if (typeAlias->IsDefaultExported()) {
        exportSet_.insert(name);
        OutDts("export default ", name, ";");
        OutEndlDts();
    }
}

void TSDeclGen::GenEnumDeclaration(const ir::ClassProperty *enumMember)
{
    const auto *originEnumMember = enumMember->OriginEnumMember();
    if (originEnumMember == nullptr) {
        return;
    }

    ProcessIndent();

    OutDts(GetKeyIdent(enumMember->Key())->Name());

    const auto *init = originEnumMember->Init();
    if (init != nullptr) {
        OutDts(" = ");
        if (!init->IsLiteral()) {
            LogError(diagnostic::NOT_LITERAL_ENUM_INITIALIZER, {}, init->Start());
        }

        GenLiteral(init->AsLiteral());
    }

    OutDts(",");
    OutEndlDts();
}

void TSDeclGen::GenInterfaceDeclaration(const ir::TSInterfaceDeclaration *interfaceDecl)
{
    const auto interfaceName = interfaceDecl->Id()->Name().Mutf8();
    DebugPrint("GenInterfaceDeclaration: " + interfaceName);
    if (interfaceName.find("$partial") != std::string::npos) {
        return;
    }
    if (!ShouldEmitDeclarationSymbol(interfaceDecl->Id())) {
        return;
    }
    GenAnnotations(interfaceDecl);
    state_.inInterface = true;
    if (classNode_.isIndirect) {
        OutDts(state_.isInterfaceInNamespace ? "interface " : "declare interface ", interfaceName);
    } else if (!interfaceDecl->IsDefaultExported()) {
        if (state_.isInterfaceInNamespace) {
            OutDts("interface ", interfaceName);
        } else {
            exportSet_.insert(interfaceName);
            OutDts("export declare interface ", interfaceName);
        }
    } else {
        if (state_.isInterfaceInNamespace) {
            OutDts("interface ", interfaceName);
        } else {
            exportSet_.insert(interfaceName);
            OutDts("export default interface ", interfaceName);
        }
    }

    GenTypeParameters(interfaceDecl->TypeParams());
    if (!interfaceDecl->Extends().empty()) {
        OutDts(" extends ");
        GenSeparated(interfaceDecl->Extends(), [this](ir::TSInterfaceHeritage *param) {
            if (param->Expr()->IsETSTypeReference()) {
                ProcessETSTypeReferenceType(param->Expr()->AsETSTypeReference());
            }
        });
    }

    OutDts(" {");
    OutEndlDts();
    ProcessInterfaceBody(interfaceDecl->Body());
    if (state_.isInterfaceInNamespace) {
        classNode_.indentLevel--;
        OutDts(GetIndent());
    }
    OutDts("}");
    OutEndlDts();
}

void TSDeclGen::ProcessInterfaceBody(const ir::TSInterfaceBody *body)
{
    for (auto *prop : body->Body()) {
        if (prop->IsMethodDefinition()) {
            ProcessInterfaceMethodDefinition(prop->AsMethodDefinition());
        }
    }
}

void TSDeclGen::ProcessInterfaceMethodDefinition(const ir::MethodDefinition *methodDef)
{
    if (GenInterfaceProp(methodDef)) {
        return;
    }

    if (methodDef->IsGetter() || methodDef->IsSetter()) {
        GenMethodDeclaration(methodDef);
    }
    if (!methodDef->Overloads().empty()) {
        for (const auto *overloadMethd : methodDef->Overloads()) {
            if (overloadMethd->IsGetter() || overloadMethd->IsSetter()) {
                GenMethodDeclaration(overloadMethd);
            }
        }
        return;
    }
    if (!methodDef->IsGetter() && !methodDef->IsSetter()) {
        GenMethodDeclaration(methodDef);
    }
}

bool TSDeclGen::GenInterfaceProp(const ir::MethodDefinition *methodDef)
{
    if (!methodDef->IsGetter()) {
        return false;
    }
    if (methodDef->OriginalNode() == nullptr) {
        return false;
    }
    if (!methodDef->OriginalNode()->IsClassProperty()) {
        return false;
    }

    const auto methodName = GetKeyIdent(methodDef->Key())->Name().Mutf8();
    const auto classProp = methodDef->OriginalNode()->AsClassProperty();
    bool isReadOnly = classProp->IsReadonly();
    bool isOptional = classProp->IsOptionalDeclaration();
    ProcessIndent();
    if (isReadOnly) {
        OutDts("readonly ");
    }
    OutDts(methodName);
    if (isOptional) {
        OutDts("?");
    }
    OutDts(": ");
    if (methodDef->TsType()->IsETSFunctionType()) {
        const auto *sig = GetFuncSignature(methodDef->TsType()->AsETSFunctionType(), methodDef);
        ProcessFunctionReturnType(sig);
        OutDts(";");
        OutEndlDts();
        return true;
    }
    ES2PANDA_ASSERT(methodDef->Function() != nullptr);
    GenType(methodDef->Function()->Signature()->ReturnType());
    OutDts(";");
    OutEndlDts();
    return true;
}

void TSDeclGen::ProcessMethodDefinition(const ir::MethodDefinition *methodDef,
                                        std::unordered_set<std::string> &processedMethods)
{
    const auto methodName = GetKeyIdent(methodDef->Key())->Name().Mutf8();
    if (processedMethods.find(methodName) != processedMethods.end()) {
        return;
    }
    if (methodDef->IsGetter() || methodDef->IsSetter()) {
        GenMethodDeclaration(methodDef);
        processedMethods.insert(methodName);
    }
    if (!methodDef->Overloads().empty()) {
        for (const auto *overloadMethd : methodDef->Overloads()) {
            if (overloadMethd->IsGetter() || overloadMethd->IsSetter()) {
                GenMethodDeclaration(overloadMethd);
            }
        }
        return;
    }
    if (!methodDef->IsGetter() && !methodDef->IsSetter()) {
        GenMethodDeclaration(methodDef);
        processedMethods.insert(methodName);
    }
}

void TSDeclGen::PrepareClassDeclaration(const ir::ClassDefinition *classDef)
{
    std::string classDescriptor = "L" + classDef->InternalName().Mutf8() + ";";
    std::replace(classDescriptor.begin(), classDescriptor.end(), '.', '/');
    state_.currentClassDescriptor = classDescriptor;
    state_.inGlobalClass = classDef->IsGlobal();
    if (classDef->IsNamespaceTransformed()) {
        state_.inNamespace = true;
        state_.isClassInNamespace = false;
        state_.isDeclareNamespace = classDef->IsDeclare();
    } else {
        state_.isClassInNamespace = true;
    }
    classNode_.isStruct = classDef->IsFromStruct();
    classNode_.isIndirect = false;
}

bool TSDeclGen::ShouldSkipClassDeclaration(const std::string_view &className) const
{
    return className == compiler::Signatures::DYNAMIC_MODULE_CLASS || className == compiler::Signatures::JSNEW_CLASS ||
           className == compiler::Signatures::JSCALL_CLASS || (className.find("$partial") != std::string::npos);
}

void TSDeclGen::EmitDeclarationPrefix(const ir::ClassDefinition *classDef, const std::string &typeName,
                                      const std::string_view &className)
{
    if (classDef->IsDefaultExported()) {
        OutDts(classNode_.indentLevel > 1 ? typeName : "declare " + typeName, className);
    } else if (classDef->IsExported() || declgenOptions_.exportAll) {
        if (classNode_.indentLevel > 1) {
            OutDts(typeName, className);
        } else {
            exportSet_.insert(std::string(className));
            OutDts("export declare " + typeName, className);
        }
    } else {
        OutDts(classNode_.indentLevel > 1 ? typeName : "declare " + typeName, className);
    }
}

void TSDeclGen::EmitClassDeclaration(const ir::ClassDefinition *classDef, const std::string_view &className)
{
    if (classDef->IsNamespaceTransformed()) {
        EmitDeclarationPrefix(classDef, "namespace ", className);
        classDef->IsDefaultExported() ? OutTs("namespace ", className, " {")
                                      : OutTs("export namespace ", className, " {");
        OutEndlTs();
    } else if (classDef->IsEnumTransformed()) {
        EmitDeclarationPrefix(classDef, "enum ", className);
    } else if (classDef->IsFromStruct()) {
        EmitDeclarationPrefix(classDef, "struct ", className);
    } else if (classDef->IsAbstract()) {
        EmitDeclarationPrefix(classDef, "abstract class ", className);
    } else {
        EmitDeclarationPrefix(classDef, "class ", className);
    }
}

std::string TSDeclGen::GetIndent() const
{
    return std::string(classNode_.indentLevel * INDENT.size(), ' ');
}

void TSDeclGen::GenPartName(std::string &partName)
{
    if (partName == "Boolean") {
        partName = "boolean";
    } else if (stringTypes_.count(partName) != 0U) {
        partName = "string";
    } else if (numberTypes_.count(partName) != 0U) {
        partName = "number";
    } else if (partName == "ESValue") {
        partName = "ESObject";
    } else if (partName == "BigInt") {
        partName = "bigint";
    } else if (partName == "Exception" || partName == "NullPointerError") {
        partName = "Error";
    } else if (partName == "Any") {
        partName = "ESObject";
    } else if (partName == "Floating" || partName == "Integral") {
        partName = "number";
    }
}

void TSDeclGen::ProcessIndent()
{
    if (state_.isInterfaceInNamespace || state_.inEnum) {
        OutDts(GetIndent());
    } else if (classNode_.hasNestedClass || state_.inNamespace) {
        auto indent = GetIndent();
        OutDts(indent);
        OutTs(indent);
    } else {
        OutDts(INDENT);
    }
}

void TSDeclGen::HandleClassDeclarationTypeInfo(const ir::ClassDefinition *classDef, const std::string_view &className)
{
    if (classNode_.hasNestedClass) {
        classNode_.indentLevel--;
        ES2PANDA_ASSERT(classNode_.indentLevel != static_cast<decltype(classNode_.indentLevel)>(-1));
    }
    GenAnnotations(classDef);
    if (classNode_.hasNestedClass) {
        OutDts(GetIndent());
        classNode_.indentLevel++;
    }
    EmitClassDeclaration(classDef, className);
    GenTypeParameters(classDef->TypeParams());

    const auto *super = classDef->Super();
    state_.super = super;
    if (super != nullptr && !classDef->IsEnumTransformed()) {
        OutDts(" extends ");
        HandleClassInherit(super);
    }

    if (!classDef->Implements().empty()) {
        OutDts(" implements ");
        GenSeparated(classDef->Implements(), [this](ir::TSClassImplements *impl) { HandleClassInherit(impl->Expr()); });
    } else if (classDef->TsType() != nullptr && classDef->TsType()->IsETSObjectType() &&
               !classDef->TsType()->AsETSObjectType()->Interfaces().empty()) {
        OutDts(" implements ");
        const auto &interfaces = classDef->TsType()->AsETSObjectType()->Interfaces();
        GenSeparated(interfaces, [this](checker::ETSObjectType *interface) { GenType(interface); });
    }

    OutDts(" {");
    OutEndlDts();
}

void TSDeclGen::HandleClassInherit(const ir::Expression *expr)
{
    if (expr->IsETSTypeReference()) {
        ProcessETSTypeReferenceType(expr->AsETSTypeReference());
    } else if (!expr->TsType()->IsTypeError()) {
        GenType(expr->TsType());
    }
}

void TSDeclGen::EmitClassGlueCode(const ir::ClassDefinition *classDef, const std::string &className)
{
    if (classNode_.isIndirect || classDef->IsExportedType()) {
        return;
    }
    const std::string exportPrefix = classDef->Parent()->IsDefaultExported() ? "const " : "export const ";
    OutTs(exportPrefix, className, " = (globalThis as any).Panda.getClass('", state_.currentClassDescriptor, "');");
    OutEndlTs();

    if (classDef->Parent()->IsDefaultExported()) {
        OutTs("export default ", className, ";");
        OutEndlTs();
    }
}

void TSDeclGen::ProcessMethodsFromInterfaces(std::unordered_set<std::string> &processedMethods,
                                             const ArenaVector<checker::ETSObjectType *> &interfaces)
{
    if (interfaces.empty()) {
        return;
    }
    for (const auto &interface : interfaces) {
        auto methods = interface->Methods();
        std::unordered_set<std::string> processedInterfaceMethods;
        for (const auto &method : methods) {
            if ((method->Flags() & (varbinder::VariableFlags::PUBLIC)) != 0U &&
                (method->Flags() & (varbinder::VariableFlags::STATIC)) == 0U &&
                processedMethods.find(method->Name().Mutf8()) == processedMethods.end()) {
                ProcessMethodDefinition(
                    method->AsLocalVariable()->Declaration()->AsFunctionDecl()->Node()->AsMethodDefinition(),
                    processedInterfaceMethods);
            }
        }
        ProcessMethodsFromInterfaces(processedMethods, interface->Interfaces());
    }
}

void TSDeclGen::ProcessClassBody(const ir::ClassDefinition *classDef)
{
    state_.inClass = true;
    std::unordered_set<std::string> processedMethods;
    for (const auto *prop : classDef->Body()) {
        const auto jsdoc = compiler::JsdocStringFromDeclaration(prop);
        if (jsdoc.Utf8().find(NON_INTEROP_FLAG) != std::string_view::npos) {
            continue;
        } else if (classDef->IsEnumTransformed()) {
            if (prop->IsClassProperty()) {
                state_.inEnum = true;
                GenPropDeclaration(prop->AsClassProperty());
            }
        } else if (prop->IsTSInterfaceDeclaration()) {
            state_.isInterfaceInNamespace = true;
            OutDts(GetIndent());
            classNode_.indentLevel++;
            GenInterfaceDeclaration(prop->AsTSInterfaceDeclaration());
            state_.inInterface = false;
            state_.isInterfaceInNamespace = false;
        } else if (prop->IsTSTypeAliasDeclaration()) {
            GenTypeAliasDeclaration(prop->AsTSTypeAliasDeclaration());
        } else if (prop->IsMethodDefinition()) {
            ProcessMethodDefinition(prop->AsMethodDefinition(), processedMethods);
        } else if (prop->IsClassProperty()) {
            const auto classProp = prop->AsClassProperty();
            const auto propName = GetKeyIdent(classProp->Key())->Name().Mutf8();
            if (propName.find("<property>") != std::string::npos) {
                continue;
            }
            GenPropDeclaration(classProp);
        } else if (prop->IsClassDeclaration() && classDef->IsNamespaceTransformed()) {
            classNode_.hasNestedClass = true;
            OutTs(GetIndent());
            classNode_.indentLevel++;
            GenClassDeclaration(prop->AsClassDeclaration());
            state_.isClassInNamespace = false;
        } else if (prop->IsClassDeclaration() && classDef->IsFromStruct()) {
            GenClassDeclaration(prop->AsClassDeclaration());
        }
    }
    if (classDef->TsType() != nullptr && classDef->TsType()->IsETSObjectType()) {
        ProcessMethodsFromInterfaces(processedMethods, classDef->TsType()->AsETSObjectType()->Interfaces());
    }
}

void TSDeclGen::CloseClassBlock(const bool isDts)
{
    auto indent = GetIndent();
    if (isDts) {
        OutDts(indent, "}");
        OutEndlDts();
    } else {
        OutTs(indent, "}");
        OutEndlTs();
    }
}

void TSDeclGen::GenClassDeclaration(const ir::ClassDeclaration *classDecl)
{
    const auto *classDef = classDecl->Definition();
    PrepareClassDeclaration(classDef);
    const auto className = classDef->Ident()->Name().Mutf8();
    DebugPrint("GenClassDeclaration: " + className);
    if (ShouldSkipClassDeclaration(className)) {
        return;
    }
    if (state_.inGlobalClass) {
        classNode_.indentLevel = 1;
        ProcessClassBody(classDef);
    }
    if (!state_.inGlobalClass && ShouldEmitDeclarationSymbol(classDef->Ident())) {
        HandleClassDeclarationTypeInfo(classDef, className);
        if (!classDef->IsNamespaceTransformed()) {
            EmitClassGlueCode(classDef, className);
        }
        ProcessClassBody(classDef);
        std::size_t originalIndentLevel = classNode_.indentLevel;
        classNode_.indentLevel > 0 ? classNode_.indentLevel-- : classNode_.indentLevel = 0;
        CloseClassBlock(true);
        classNode_.indentLevel = originalIndentLevel;
    }
    if (classNode_.hasNestedClass || state_.inNamespace || state_.inEnum) {
        classNode_.indentLevel > 1 ? classNode_.indentLevel-- : classNode_.indentLevel = 1;
        if (!ShouldEmitDeclarationSymbol(classDef->Ident())) {
            return;
        }
        ES2PANDA_ASSERT(classNode_.indentLevel != static_cast<decltype(classNode_.indentLevel)>(-1));
        if (!state_.isClassInNamespace) {
            CloseClassBlock(false);
        }
        if (state_.inEnum) {
            state_.inEnum = false;
        }
    }
    if (classDef->IsDefaultExported()) {
        exportSet_.insert(className);
        OutDts("export default ", className, ";");
        OutEndlDts();
        if (classDef->IsNamespaceTransformed()) {
            OutTs("export default ", className, ";");
            OutEndlTs();
        }
    }
}

bool TSDeclGen::ShouldSkipMethodDeclaration(const ir::MethodDefinition *methodDef)
{
    const auto methodIdent = GetKeyIdent(methodDef->Key());
    const auto methodName = methodIdent->Name().Mutf8();
    if (methodName.find('#') != std::string::npos || methodName.find("%%async-") != std::string::npos ||
        (!state_.inGlobalClass && (methodName == compiler::Signatures::INIT_METHOD ||
                                   methodName == compiler::Signatures::INITIALIZER_BLOCK_INIT))) {
        return true;
    }
    if (methodDef->IsPrivate() && (methodDef->IsConstructor() || state_.inInterface)) {
        return true;
    }
    if (methodName == compiler::Signatures::INIT_METHOD) {
        return true;
    }
    if (classNode_.isStruct && methodDef->IsConstructor()) {
        return true;
    }
    return false;
}

void TSDeclGen::EmitMethodGlueCode(const std::string &methodName, const ir::Identifier *methodIdentifier)
{
    if (!state_.inGlobalClass && (!state_.inNamespace || state_.isClassInNamespace || state_.isInterfaceInNamespace)) {
        return;
    }
    if (!ShouldEmitDeclarationSymbol(methodIdentifier)) {
        return;
    }
    if (state_.inNamespace) {
        OutTs("export const ", methodName,
              " = (globalThis as any).Panda.getClass('" + state_.currentClassDescriptor + "')." + methodName + ";");
        OutEndlTs();
        return;
    }
    if (methodIdentifier->Parent()->IsDefaultExported()) {
        OutTs("const ", methodName, " = (globalThis as any).Panda.getFunction('", state_.currentClassDescriptor, "', '",
              methodName, "');");
        OutEndlTs();
        OutTs("export default ", methodName, ";");
        OutEndlTs();
    } else {
        OutTs("export const ", methodName, " = (globalThis as any).Panda.getFunction('", state_.currentClassDescriptor,
              "', '", methodName, "');");
        OutEndlTs();
    }
}

void TSDeclGen::GenMethodDeclaration(const ir::MethodDefinition *methodDef)
{
    if (ShouldSkipMethodDeclaration(methodDef)) {
        return;
    }
    const auto methodIdent = GetKeyIdent(methodDef->Key());
    auto methodName = methodIdent->Name().Mutf8();
    if (methodName.compare("$_iterator") == 0) {
        methodName = "[Symbol.iterator]";
    }
    if (GenMethodDeclarationPrefix(methodDef, methodIdent, methodName)) {
        return;
    }
    GenMethodSignature(methodDef, methodIdent, methodName);

    OutDts(";");
    OutEndlDts();

    if (methodDef->IsDefaultExported()) {
        exportSet_.insert(methodName);
        OutDts("export default ", methodName, ";");
        OutEndlDts();
    }
}

bool TSDeclGen::GenMethodDeclarationPrefix(const ir::MethodDefinition *methodDef, const ir::Identifier *methodIdent,
                                           const std::string &methodName)
{
    if (state_.inGlobalClass) {
        if (!ShouldEmitDeclarationSymbol(methodIdent)) {
            return true;
        }
        if (methodDef->IsDefaultExported()) {
            OutDts("declare function ");
        } else {
            exportSet_.insert(methodName);
            OutDts("export declare function ");
        }
    } else {
        if (state_.inNamespace && !state_.isClassInNamespace && !state_.isInterfaceInNamespace &&
            !ShouldEmitDeclarationSymbol(methodIdent) && !methodDef->IsConstructor()) {
            return true;
        }
        auto methDefFunc = methodDef->Function();
        GenAnnotations(methDefFunc);
        ProcessIndent();
        GenModifier(methodDef);
    }
    EmitMethodGlueCode(methodName, methodIdent);

    ES2PANDA_ASSERT(methodDef->Function() != nullptr);
    if (methodDef->Function()->IsAbstract() && !state_.inInterface &&
        !(methodDef->Parent()->IsTSInterfaceBody() ||
          (methodDef->BaseOverloadMethod() != nullptr &&
           methodDef->BaseOverloadMethod()->Parent()->IsTSInterfaceBody()))) {
        OutDts("abstract ");
    }
    if (methodDef->IsGetter()) {
        OutDts("get ");
    }
    if (methodDef->IsSetter()) {
        OutDts("set ");
    }
    return false;
}

void TSDeclGen::GenMethodSignature(const ir::MethodDefinition *methodDef, const ir::Identifier *methodIdent,
                                   const std::string &methodName)
{
    if (methodDef->IsSetter()) {
        OutDts(methodName, "(value: ");
        const auto *sig = methodDef->TsType()->AsETSFunctionType()->CallSignatures()[0];
        if (sig->HasFunction()) {
            ProcessFunctionReturnType(sig);
        } else {
            GenType(sig->Params()[0]->TsType());
        }
        OutDts(")");
    } else {
        DebugPrint("  GenMethodDeclaration: " + methodName);
        OutDts(methodName);

        if (methodDef->TsType() == nullptr) {
            LogWarning(diagnostic::UNTYPED_METHOD, {methodName}, methodIdent->Start());
            OutDts(": ESObject");
            return;
        }
        if (methodDef->TsType()->IsETSFunctionType()) {
            GenFunctionType(methodDef->TsType()->AsETSFunctionType(), methodDef);
        }
    }
}

void TSDeclGen::EmitPropGlueCode(const ir::ClassProperty *classProp, const std::string &propName)
{
    std::string propAccess;
    if (state_.inGlobalClass) {
        propAccess = " = (globalThis as any).Panda.getClass('" + globalDesc_ + "')." + propName + ";";
    } else {
        propAccess = " = (globalThis as any).Panda.getClass('" + state_.currentClassDescriptor + "')." + propName + ";";
    }
    const bool isConst = classProp->IsConst();
    const bool isDefaultExported = classProp->IsDefaultExported();
    if (isDefaultExported) {
        OutTs(isConst ? "const " : "let ", propName, propAccess);
        OutEndlTs();
        OutTs("export default ", propName, ";");
    } else {
        OutTs(isConst ? "export const " : "export let ", propName, propAccess);
    }
    OutEndlTs();
}

void TSDeclGen::ProcessClassPropertyType(const ir::ClassProperty *classProp)
{
    auto value = classProp->AsClassProperty()->Value();
    if (value != nullptr && value->IsETSNewClassInstanceExpression() &&
        value->AsETSNewClassInstanceExpression()->GetTypeRef() != nullptr &&
        value->AsETSNewClassInstanceExpression()->GetTypeRef()->IsETSTypeReference()) {
        auto typeReference = classProp->Value()->AsETSNewClassInstanceExpression()->GetTypeRef()->AsETSTypeReference();
        ProcessETSTypeReferenceType(typeReference);
        return;
    }
    if (classProp->TypeAnnotation() != nullptr) {
        ProcessTypeAnnotationType(classProp->TypeAnnotation(), classProp->TsType());
        return;
    }
    if (value != nullptr && value->IsArrowFunctionExpression() &&
        value->AsArrowFunctionExpression()->Function() != nullptr &&
        value->AsArrowFunctionExpression()->Function()->TypeParams() != nullptr) {
        GenTypeParameters(value->AsArrowFunctionExpression()->Function()->TypeParams());
    }
    GenType(classProp->TsType());
}

void TSDeclGen::GenPropDeclaration(const ir::ClassProperty *classProp)
{
    if (state_.inGlobalClass) {
        GenGlobalVarDeclaration(classProp);
        return;
    }
    if (classProp->IsPrivate()) {
        return;
    }

    const auto propName = GetKeyIdent(classProp->Key())->Name().Mutf8();
    // The class property generated for enums starts with "#" are invalid properties, and should not be generated.
    if (propName.find('#') != std::string::npos) {
        DebugPrint("  Skip Generate enum PropDeclaration: " + propName);
        return;
    }

    if (state_.inEnum) {
        GenEnumDeclaration(classProp);
        return;
    }

    DebugPrint("  GenPropDeclaration: " + propName);

    ProcessClassPropDeclaration(classProp);

    if (classNode_.hasNestedClass || state_.inNamespace) {
        EmitPropGlueCode(classProp, propName);
    }
}

void TSDeclGen::ProcessClassPropDeclaration(const ir::ClassProperty *classProp)
{
    if (!state_.inInterface && (!state_.inNamespace || state_.isClassInNamespace) && !classNode_.isStruct) {
        GenPropAccessor(classProp, "get ");
        if (!classProp->IsReadonly()) {
            GenPropAccessor(classProp, "set ");
        }
    } else {
        GenAnnotations(classProp);
        ProcessIndent();
        if (!classNode_.isStruct) {
            GenModifier(classProp, true);
        }
        const auto propName = GetKeyIdent(classProp->Key())->Name().Mutf8();
        OutDts(propName);
        OutDts(": ");
        if (!state_.inNamespace) {
            classProp->IsStatic() ? OutDts("ESObject") : GenType(classProp->TsType());
        } else {
            ProcessClassPropertyType(classProp);
        }
        OutDts(";");
        OutEndlDts();
    }
}

void TSDeclGen::GenPropAccessor(const ir::ClassProperty *classProp, const std::string &accessorKind)
{
    if (accessorKind != "set " && accessorKind != "get ") {
        return;
    }
    GenAnnotations(classProp);
    ProcessIndent();
    GenModifier(classProp);

    const auto propName = GetKeyIdent(classProp->Key())->Name().Mutf8();
    OutDts(accessorKind, propName, accessorKind == "set " ? "(value: " : "(): ");
    auto typeAnnotation = classProp->TypeAnnotation();
    auto tsType = classProp->TsType();
    if (typeAnnotation != nullptr) {
        ProcessTypeAnnotationType(typeAnnotation, tsType);
    } else {
        GenType(tsType);
    }
    OutDts(accessorKind == "set " ? ");" : ";");
    OutEndlDts();
}

void TSDeclGen::GenGlobalVarDeclaration(const ir::ClassProperty *globalVar)
{
    if (!globalVar->IsExported() && !globalVar->IsDefaultExported() && !declgenOptions_.exportAll) {
        return;
    }

    const auto symbol = GetKeyIdent(globalVar->Key());
    auto varName = symbol->Name().Mutf8();
    const std::string prefix = "gensym%%_";
    if (varName.rfind(prefix, 0) == 0) {
        varName = varName.substr(prefix.size());
    }
    const bool isConst = globalVar->IsConst();
    const bool isDefaultExported = globalVar->IsDefaultExported();
    DebugPrint("GenGlobalVarDeclaration: " + varName);

    GenAnnotations(globalVar);

    exportSet_.insert(varName);
    if (isDefaultExported) {
        OutDts(isConst ? "declare const " : "declare let ", varName, ": ");
    } else {
        OutDts(isConst ? "export declare const " : "export declare let ", varName, ": ");
    }
    ProcessClassPropertyType(globalVar);
    OutDts(";");
    OutEndlDts();
    if (isDefaultExported) {
        OutDts("export default ", varName, ";");
        OutEndlDts();
    }

    EmitPropGlueCode(globalVar, varName);
}

bool WriteToFile(const std::string &path, const std::string &content, checker::ETSChecker *checker)
{
    std::ofstream outStream(path);  // ark::os::GetAbsolutePath(*pathValue)
    if (outStream.fail()) {
        checker->DiagnosticEngine().LogDiagnostic(diagnostic::OPEN_FAILED, util::DiagnosticMessageParams {path});
        return false;
    }
    outStream << content;
    outStream.close();
    return true;
}

bool GenerateTsDeclarations(checker::ETSChecker *checker, const ark::es2panda::parser::Program *program,
                            const DeclgenOptions &declgenOptions)
{
    declgen::IsolatedDeclgenChecker isolatedDeclgenChecker(checker->DiagnosticEngine(), *program);
    if (declgenOptions.isolated) {
        isolatedDeclgenChecker.Check();
    }

    TSDeclGen declBuilder(checker, &isolatedDeclgenChecker, program);
    declBuilder.SetDeclgenOptions(declgenOptions);

    if (!ValidateDeclgenOptions(declBuilder.GetDeclgenOptions(), checker)) {
        return false;
    }

    if (!declBuilder.Generate()) {
        return false;
    }

    std::string outputEts = declBuilder.GetTsOutput();
    std::string outputDEts = declBuilder.GetDtsOutput();

    declBuilder.ResetTsOutput();
    declBuilder.ResetDtsOutput();

    auto afterCheckerId = compiler::GetPhaseManager()->CurrentPhaseId().minor;
    int32_t firstPhaseId = -1;
    compiler::GetPhaseManager()->SetCurrentPhaseIdWithoutReCheck(firstPhaseId);
    declBuilder.GenExportNamedDeclarations();

    std::string exportOutputEts = declBuilder.GetTsOutput();
    std::string exportOutputDEts = declBuilder.GetDtsOutput();

    declBuilder.ResetTsOutput();
    declBuilder.ResetDtsOutput();

    compiler::GetPhaseManager()->SetCurrentPhaseIdWithoutReCheck(afterCheckerId);
    declBuilder.GenImportDeclarations();

    std::string importOutputEts = declBuilder.GetTsOutput();
    std::string importOutputDEts = declBuilder.GetDtsOutput();

    std::string combineEts = importOutputEts + outputEts + exportOutputEts;
    std::string combinedDEts = importOutputDEts + outputDEts + exportOutputDEts;

    if (!declBuilder.GetDeclgenOptions().recordFile.empty()) {
        declBuilder.ResetDtsOutput();
        declBuilder.GenImportRecordDeclarations(declBuilder.GetDeclgenOptions().recordFile);
        std::string recordImportOutputDEts = declBuilder.GetDtsOutput();
        combinedDEts = recordImportOutputDEts + combinedDEts;
    }

    return WriteOutputFiles(declBuilder.GetDeclgenOptions(), combineEts, combinedDEts, checker);
}

bool ValidateDeclgenOptions(const DeclgenOptions &options, checker::ETSChecker *checker)
{
    if ((options.outputDeclEts.empty() && !options.outputEts.empty()) ||
        (!options.outputDeclEts.empty() && options.outputEts.empty())) {
        checker->DiagnosticEngine().LogDiagnostic(diagnostic::GENERATE_DYNAMIC_DECLARATIONS,
                                                  util::DiagnosticMessageParams {});
        return false;
    }
    if (options.outputDeclEts.empty() && options.outputEts.empty()) {
        checker->DiagnosticEngine().LogDiagnostic(diagnostic::MISSING_OUTPUT_FILE, util::DiagnosticMessageParams {""});
        return false;
    }
    return true;
}

bool WriteOutputFiles(const DeclgenOptions &options, const std::string &combinedEts, const std::string &combinedDEts,
                      checker::ETSChecker *checker)
{
    if (!options.outputDeclEts.empty()) {
        if (!WriteToFile(options.outputDeclEts, combinedDEts, checker)) {
            return false;
        }
    }

    if (!options.outputEts.empty()) {
        if (!WriteToFile(options.outputEts, combinedEts, checker)) {
            return false;
        }
    }
    return true;
}
}  // namespace ark::es2panda::declgen_ets2ts

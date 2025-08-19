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

#include "ETSemitter.h"

#include "annotation.h"
#include "compiler/core/ETSGen.h"
#include "varbinder/varbinder.h"
#include "varbinder/ETSBinder.h"
#include "ir/astNode.h"
#include "ir/expressions/identifier.h"
#include "ir/base/decorator.h"
#include "ir/base/methodDefinition.h"
#include "ir/base/classDefinition.h"
#include "ir/base/scriptFunction.h"
#include "ir/base/classProperty.h"
#include "ir/statements/annotationDeclaration.h"
#include "ir/ts/tsInterfaceDeclaration.h"
#include "ir/ts/tsInterfaceBody.h"
#include "ir/ts/tsTypeParameterDeclaration.h"
#include "ir/ts/tsTypeParameter.h"
#include "ir/typeNode.h"
#include "parser/program/program.h"
#include "checker/checker.h"
#include "checker/types/signature.h"
#include "checker/ETSchecker.h"
#include "checker/types/type.h"
#include "checker/types/gradualType.h"
#include "checker/types/ets/types.h"
#include "checker/types/ets/etsPartialTypeParameter.h"
#include "public/public.h"
#include "util/nameMangler.h"

#include "assembly-program.h"

namespace {
uint32_t g_litArrayValueCount = 0;
}  // namespace

namespace ark::es2panda::compiler {

#ifdef PANDA_WITH_ETS
static constexpr auto EXTENSION = panda_file::SourceLang::ETS;
#else
// NOTE: temporary dummy gn buildfix until ETS plugin has gn build support
static constexpr auto EXTENSION = panda_file::SourceLang::PANDA_ASSEMBLY;
#endif

static uint32_t TranslateModifierFlags(ir::ModifierFlags modifierFlags)
{
    uint32_t accessFlags = 0;

    if ((modifierFlags & ir::ModifierFlags::PRIVATE) != 0) {
        accessFlags = ACC_PRIVATE;
    } else if ((modifierFlags & ir::ModifierFlags::INTERNAL) != 0) {
        if ((modifierFlags & ir::ModifierFlags::PROTECTED) != 0) {
            accessFlags = ACC_PROTECTED;
        }
        // NOTE: torokg. Add ACC_INTERNAL access flag to libpandabase
    } else if ((modifierFlags & ir::ModifierFlags::PROTECTED) != 0) {
        accessFlags = ACC_PROTECTED;
    } else {
        accessFlags = ACC_PUBLIC;
    }

    if ((modifierFlags & ir::ModifierFlags::STATIC) != 0) {
        accessFlags |= ACC_STATIC;
    }
    if ((modifierFlags & ir::ModifierFlags::FINAL) != 0) {
        accessFlags |= ACC_FINAL;
    }
    // NOTE: should be ModifierFlags::READONLY
    if ((modifierFlags & ir::ModifierFlags::READONLY) != 0) {
        accessFlags |= ACC_READONLY;
    }
    if ((modifierFlags & ir::ModifierFlags::ABSTRACT) != 0) {
        accessFlags |= ACC_ABSTRACT;
    }
    if ((modifierFlags & ir::ModifierFlags::NATIVE) != 0) {
        accessFlags |= ACC_NATIVE;
    }

    return accessFlags;
}

static pandasm::Type PandasmTypeWithRank(checker::Type const *type, uint32_t rank = 0)
{
    if (type->IsGradualType()) {
        return PandasmTypeWithRank(type->AsGradualType()->GetBaseType());
    }
    if (type->IsETSTypeParameter()) {
        return PandasmTypeWithRank(type->AsETSTypeParameter()->GetConstraintType());
    }
    if (type->IsETSNonNullishType()) {
        return PandasmTypeWithRank(type->AsETSNonNullishType()->GetUnderlying());
    }
    if (type->IsETSPartialTypeParameter()) {
        return PandasmTypeWithRank(type->AsETSPartialTypeParameter()->GetUnderlying());
    }

    std::stringstream ss;
    type->ToAssemblerType(ss);
    return pandasm::Type(ss.str(), rank == 0 ? type->Rank() : rank);
}

static pandasm::Function GenScriptFunction(const ir::ScriptFunction *scriptFunc, ETSEmitter *emitter)
{
    auto *funcScope = scriptFunc->Scope();
    auto *paramScope = funcScope->ParamScope();

    auto func = pandasm::Function(funcScope->InternalName().Mutf8(), EXTENSION);
    func.params.reserve(paramScope->Params().size());

    for (const auto *var : paramScope->Params()) {
        func.params.emplace_back(PandasmTypeWithRank(var->TsType()), EXTENSION);
        if (scriptFunc->IsExternal() || var->Declaration()->Node() == nullptr ||
            !var->Declaration()->Node()->IsETSParameterExpression()) {
            continue;
        }
        if (var->Declaration()->Node()->AsETSParameterExpression()->HasAnnotations()) {
            func.params.back().GetOrCreateMetadata()->SetAnnotations(emitter->GenCustomAnnotations(
                var->Declaration()->Node()->AsETSParameterExpression()->Annotations(), var->Name().Mutf8()));
        }
    }

    if (scriptFunc->IsConstructor() || scriptFunc->IsStaticBlock()) {
        func.returnType = pandasm::Type(Signatures::PRIMITIVE_VOID, 0);
    } else {
        func.returnType = PandasmTypeWithRank(scriptFunc->Signature()->ReturnType());
    }

    uint32_t accessFlags = 0;
    if (!scriptFunc->IsStaticBlock()) {
        const auto *methodDef = util::Helpers::GetContainingClassMethodDefinition(scriptFunc);
        ES2PANDA_ASSERT(methodDef != nullptr);
        accessFlags |= TranslateModifierFlags(methodDef->Modifiers());
    }
    if (scriptFunc->HasRestParameter()) {
        accessFlags |= ACC_VARARGS;
    }
    func.metadata->SetAccessFlags(accessFlags);
    if (!scriptFunc->IsExternal() && scriptFunc->HasAnnotations()) {
        func.metadata->SetAnnotations(emitter->GenCustomAnnotations(scriptFunc->Annotations(), func.name));
    }

    if (scriptFunc->IsConstructor()) {
        func.metadata->SetAttribute(Signatures::CONSTRUCTOR);
    }

    return func;
}

pandasm::Function *ETSFunctionEmitter::GenFunctionSignature()
{
    auto *scriptFunc = Cg()->RootNode()->AsScriptFunction();
    auto *emitter = static_cast<ETSEmitter *>(Cg()->Context()->emitter);
    auto func = GenScriptFunction(scriptFunc, emitter);

    if (Cg()->RootNode()->AsScriptFunction()->IsExternal()) {
        func.metadata->SetAttribute(Signatures::EXTERNAL);
    }

    auto *funcElement = new pandasm::Function(func.name, func.language);
    *funcElement = std::move(func);
    GetProgramElement()->SetFunction(funcElement);
    funcElement->regsNum = VReg::REG_START - Cg()->TotalRegsNum();

    return funcElement;
}

void ETSFunctionEmitter::GenVariableSignature(pandasm::debuginfo::LocalVariable &variableDebug,
                                              [[maybe_unused]] varbinder::LocalVariable *variable) const
{
    variableDebug.signature = Signatures::ANY;
    variableDebug.signatureType = Signatures::ANY;
}

void ETSFunctionEmitter::GenSourceFileDebugInfo(pandasm::Function *func)
{
    func->sourceFile = std::string {Cg()->VarBinder()->Program()->RelativeFilePath()};

    if (!Cg()->IsDebug()) {
        return;
    }

    ES2PANDA_ASSERT(Cg()->RootNode()->IsScriptFunction());
    auto *fn = Cg()->RootNode()->AsScriptFunction();
    bool isInitMethod = fn->Id()->Name().Is(compiler::Signatures::INIT_METHOD);
    // Write source code of whole file into debug-info of init method
    if (isInitMethod) {
        func->sourceCode = SourceCode().Utf8();
    }
}

void ETSFunctionEmitter::GenFunctionAnnotations([[maybe_unused]] pandasm::Function *func) {}

static pandasm::Function GenExternalFunction(checker::Signature *signature, bool isCtor)
{
    auto func = pandasm::Function(signature->InternalName().Mutf8(), EXTENSION);

    for (auto param : signature->Params()) {
        func.params.emplace_back(PandasmTypeWithRank(param->TsType()), EXTENSION);
    }
    func.returnType = PandasmTypeWithRank(signature->ReturnType());

    if (isCtor) {
        func.metadata->SetAttribute(Signatures::CONSTRUCTOR);
    }
    func.metadata->SetAttribute(Signatures::EXTERNAL);

    return func;
}

void FilterForSimultaneous(varbinder::ETSBinder *varbinder)
{
    ArenaSet<ir::ClassDefinition *> &classDefinitions = varbinder->GetGlobalRecordTable()->ClassDefinitions();
    for (auto it = classDefinitions.begin(); it != classDefinitions.end(); ++it) {
        if ((*it)->InternalName().Is(Signatures::ETS_GLOBAL)) {
            classDefinitions.erase(it);
            break;
        }
    }
    std::vector<std::string_view> filterFunctions = {
        Signatures::UNUSED_ETSGLOBAL_CTOR, Signatures::UNUSED_ETSGLOBAL_INIT, Signatures::UNUSED_ETSGLOBAL_MAIN};
    auto &functions = varbinder->Functions();
    functions.erase(std::remove_if(functions.begin(), functions.end(),
                                   [&filterFunctions](varbinder::FunctionScope *scope) -> bool {
                                       return std::any_of(
                                           filterFunctions.begin(), filterFunctions.end(),
                                           [&scope](std::string_view &s) { return scope->InternalName().Is(s); });
                                   }),  // CC-OFF(G.FMT.02)
                    functions.end());
}

void ETSEmitter::GenAnnotation()
{
    Program()->lang = EXTENSION;
    auto *varbinder = static_cast<varbinder::ETSBinder *>(Context()->parserProgram->VarBinder());

    if (Context()->config->options->GetCompilationMode() == CompilationMode::GEN_ABC_FOR_EXTERNAL_SOURCE) {
        FilterForSimultaneous(varbinder);
    }

    auto *globalRecordTable = varbinder->GetGlobalRecordTable();
    auto baseName = varbinder->GetRecordTable()->RecordName().Mutf8();
    for (auto *annoDecl : globalRecordTable->AnnotationDeclarations()) {
        std::string newBaseName = util::NameMangler::GetInstance()->CreateMangledNameForAnnotation(
            baseName, annoDecl->GetBaseName()->Name().Mutf8());
        GenCustomAnnotationRecord(annoDecl, newBaseName, annoDecl->IsDeclare());
    }

    for (auto *classDecl : globalRecordTable->ClassDefinitions()) {
        GenClassRecord(classDecl, classDecl->IsDeclare());
    }

    for (auto *interfaceDecl : globalRecordTable->InterfaceDeclarations()) {
        GenInterfaceRecord(interfaceDecl, interfaceDecl->IsDeclare());
    }

    for (auto *signature : globalRecordTable->Signatures()) {
        auto *scriptFunc = signature->Node()->AsScriptFunction();
        auto func = GenScriptFunction(scriptFunc, this);
        if (scriptFunc->IsDeclare()) {
            func.metadata->SetAttribute(Signatures::EXTERNAL);
        }
        if (scriptFunc->IsAsyncFunc()) {
            std::vector<pandasm::AnnotationData> annotations;
            annotations.push_back(GenAnnotationAsync(scriptFunc));
            func.metadata->AddAnnotations(annotations);
        }
        Program()->AddToFunctionTable(std::move(func));
    }

    for (auto [extProg, recordTable] : varbinder->GetExternalRecordTable()) {
        if (recordTable == varbinder->GetRecordTable()) {
            continue;
        }
        GenExternalRecord(recordTable, extProg);
    }

    const auto *checker = static_cast<checker::ETSChecker *>(Context()->GetChecker());

    for (auto [arrType, signature] : checker->GlobalArrayTypes()) {
        GenGlobalArrayRecord(arrType, signature);
    }
    for (auto unionType : checker->UnionAssemblerTypes()) {
        auto unionRecord = pandasm::Record(unionType.Mutf8(), Program()->lang);
        unionRecord.metadata->SetAttribute(Signatures::EXTERNAL);
        Program()->recordTable.emplace(unionRecord.name, std::move(unionRecord));
    }
}

static bool IsFromSelfHeadFile(const std::string &name, const parser::Program *curProg, const parser::Program *extProg)
{
    const auto *curBinder = static_cast<const varbinder::ETSBinder *>(curProg->VarBinder());
    return extProg->FileName() == curProg->FileName() &&
           std::any_of(curBinder->Functions().begin(), curBinder->Functions().end(),
                       [&](auto function) { return function->InternalName().Is(name); });
}

void ETSEmitter::GenExternalRecord(varbinder::RecordTable *recordTable, const parser::Program *extProg)
{
    bool isExternalFromCompile =
        !recordTable->Program()->VarBinder()->IsGenStdLib() && !recordTable->Program()->IsGenAbcForExternal();
    const auto *varbinder = static_cast<const varbinder::ETSBinder *>(Context()->parserProgram->VarBinder());
    auto baseName = varbinder->GetRecordTable()->RecordName().Mutf8();
    for (auto *annoDecl : recordTable->AnnotationDeclarations()) {
        std::string newBaseName = util::NameMangler::GetInstance()->CreateMangledNameForAnnotation(
            baseName, annoDecl->GetBaseName()->Name().Mutf8());
        GenCustomAnnotationRecord(annoDecl, newBaseName, isExternalFromCompile || annoDecl->IsDeclare());
    }

    for (auto *classDecl : recordTable->ClassDefinitions()) {
        GenClassRecord(classDecl, isExternalFromCompile || classDecl->IsDeclare());
    }

    for (auto *interfaceDecl : recordTable->InterfaceDeclarations()) {
        GenInterfaceRecord(interfaceDecl, isExternalFromCompile || interfaceDecl->IsDeclare());
    }

    for (auto *signature : recordTable->Signatures()) {
        auto scriptFunc = signature->Node()->AsScriptFunction();
        auto func = GenScriptFunction(scriptFunc, this);

        if (isExternalFromCompile || scriptFunc->IsDeclare()) {
            func.metadata->SetAttribute(Signatures::EXTERNAL);
        }

        if (extProg->IsGenAbcForExternal() && scriptFunc->IsAsyncFunc()) {
            std::vector<pandasm::AnnotationData> annotations;
            annotations.push_back(GenAnnotationAsync(scriptFunc));
            func.metadata->AddAnnotations(annotations);
        }

        if (func.metadata->IsForeign() && IsFromSelfHeadFile(func.name, Context()->parserProgram, extProg)) {
            continue;
        }

        if (Program()->functionStaticTable.find(func.name) == Program()->functionStaticTable.cend()) {
            Program()->AddToFunctionTable(std::move(func));
        }
    }
}

// Helper function to reduce EmitDefaultFieldValue size and pass code check
// We assume that all the checks have been passes successfully and the value in number literal is valid.
// CC-OFFNXT(huge_method[C++], G.FUN.01-CPP, G.FUD.05) solid logic, big switch case
static pandasm::ScalarValue CreateScalarValue(ir::Literal const *literal, checker::TypeFlag typeKind)
{
    switch (typeKind) {
        case checker::TypeFlag::ETS_BOOLEAN: {
            ES2PANDA_ASSERT(literal->IsBooleanLiteral());
            return pandasm::ScalarValue::Create<pandasm::Value::Type::U1>(
                static_cast<uint8_t>(literal->AsBooleanLiteral()->Value()));
        }
        case checker::TypeFlag::BYTE: {
            ES2PANDA_ASSERT(literal->IsNumberLiteral());
            return pandasm::ScalarValue::Create<pandasm::Value::Type::I8>(
                literal->AsNumberLiteral()
                    ->Number()
                    .GetValueAndCastTo<pandasm::ValueTypeHelperT<pandasm::Value::Type::I8>>());
        }
        case checker::TypeFlag::SHORT: {
            ES2PANDA_ASSERT(literal->IsNumberLiteral());
            return pandasm::ScalarValue::Create<pandasm::Value::Type::I16>(
                literal->AsNumberLiteral()
                    ->Number()
                    .GetValueAndCastTo<pandasm::ValueTypeHelperT<pandasm::Value::Type::I16>>());
        }
        case checker::TypeFlag::INT: {
            ES2PANDA_ASSERT(literal->IsNumberLiteral());
            return pandasm::ScalarValue::Create<pandasm::Value::Type::I32>(
                literal->AsNumberLiteral()
                    ->Number()
                    .GetValueAndCastTo<pandasm::ValueTypeHelperT<pandasm::Value::Type::I32>>());
        }
        case checker::TypeFlag::LONG: {
            ES2PANDA_ASSERT(literal->IsNumberLiteral());
            return pandasm::ScalarValue::Create<pandasm::Value::Type::I64>(
                literal->AsNumberLiteral()
                    ->Number()
                    .GetValueAndCastTo<pandasm::ValueTypeHelperT<pandasm::Value::Type::I64>>());
        }
        case checker::TypeFlag::FLOAT: {
            ES2PANDA_ASSERT(literal->IsNumberLiteral());
            return pandasm::ScalarValue::Create<pandasm::Value::Type::F32>(
                literal->AsNumberLiteral()
                    ->Number()
                    .GetValueAndCastTo<pandasm::ValueTypeHelperT<pandasm::Value::Type::F32>>());
        }
        case checker::TypeFlag::DOUBLE: {
            ES2PANDA_ASSERT(literal->IsNumberLiteral());
            return pandasm::ScalarValue::Create<pandasm::Value::Type::F64>(
                literal->AsNumberLiteral()->Number().GetDouble());
        }
        case checker::TypeFlag::CHAR: {
            ES2PANDA_ASSERT(literal->IsCharLiteral());
            return pandasm::ScalarValue::Create<pandasm::Value::Type::U16>(literal->AsCharLiteral()->Char());
        }
        case checker::TypeFlag::ETS_OBJECT: {
            ES2PANDA_ASSERT(literal->IsStringLiteral());
            return pandasm::ScalarValue::Create<pandasm::Value::Type::STRING>(
                literal->AsStringLiteral()->Str().Mutf8());
        }
        default: {
            ES2PANDA_UNREACHABLE();
        }
    }
}

void ETSEmitter::EmitDefaultFieldValue(pandasm::Field &classField, const ir::Expression *init)
{
    if (init == nullptr || !init->IsLiteral()) {
        return;
    }

    const auto *type = init->TsType();
    auto typeKind = checker::ETSChecker::TypeKind(type);
    classField.metadata->SetFieldType(classField.type);
    classField.metadata->SetValue(CreateScalarValue(init->AsLiteral(), typeKind));
}

void ETSEmitter::GenInterfaceMethodDefinition(const ir::MethodDefinition *methodDef, bool external)
{
    auto *scriptFunc = methodDef->Function();
    auto func = GenScriptFunction(scriptFunc, this);

    if (external) {
        func.metadata->SetAttribute(Signatures::EXTERNAL);
    }

    if (scriptFunc->Body() != nullptr) {
        return;
    }

    func.metadata->SetAccessFlags(func.metadata->GetAccessFlags() | ACC_ABSTRACT);
    Program()->AddToFunctionTable(std::move(func));
}

void ETSEmitter::GenClassField(const ir::ClassProperty *prop, pandasm::Record &classRecord, bool external)
{
    auto field = pandasm::Field(Program()->lang);
    ES2PANDA_ASSERT(prop->Id() != nullptr);
    field.name = prop->Id()->Name().Mutf8();
    field.type = PandasmTypeWithRank(prop->TsType());
    field.metadata->SetAccessFlags(TranslateModifierFlags(prop->Modifiers()));

    if (!external && prop->HasAnnotations()) {
        field.metadata->SetAnnotations(GenCustomAnnotations(prop->Annotations(), field.name));
    }

    if (external || prop->IsDeclare()) {
        field.metadata->SetAttribute(Signatures::EXTERNAL);
    } else if (prop->TsType()->IsETSPrimitiveType() || prop->TsType()->IsETSStringType()) {
        EmitDefaultFieldValue(field, prop->Value());
    }

    classRecord.fieldList.emplace_back(std::move(field));
}

void ETSEmitter::GenClassInheritedFields(const checker::ETSObjectType *baseType, pandasm::Record &classRecord)
{
    std::vector<const varbinder::LocalVariable *> foreignProps = baseType->ForeignProperties();

    for (const auto *foreignProp : foreignProps) {
        auto *declNode = foreignProp->Declaration()->Node();
        if (!declNode->IsClassProperty()) {
            continue;
        }

        GenClassField(declNode->AsClassProperty(), classRecord, true);
    }
}

void ETSEmitter::GenGlobalArrayRecord(const checker::ETSArrayType *arrayType, checker::Signature *signature)
{
    std::stringstream ss;
    arrayType->ToAssemblerTypeWithRank(ss);

    auto arrayRecord = pandasm::Record(ss.str(), Program()->lang);

    auto func = GenExternalFunction(signature, true);
    func.params.emplace(func.params.begin(), pandasm::Type(ss.str(), 0), EXTENSION);

    Program()->AddToFunctionTable(std::move(func));

    arrayRecord.metadata->SetAttribute(Signatures::EXTERNAL);
    Program()->recordTable.emplace(arrayRecord.name, std::move(arrayRecord));
    Program()->arrayTypes.emplace(PandasmTypeWithRank(arrayType));
}

void ETSEmitter::GenInterfaceRecord(const ir::TSInterfaceDeclaration *interfaceDecl, bool external)
{
    auto *baseType = interfaceDecl->TsType()->IsGradualType()
                         ? interfaceDecl->TsType()->AsGradualType()->GetBaseType()->AsETSObjectType()
                         : interfaceDecl->TsType()->AsETSObjectType();
    auto interfaceRecord = pandasm::Record(interfaceDecl->InternalName().Mutf8(), Program()->lang);

    uint32_t accessFlags = ACC_PUBLIC | ACC_ABSTRACT | ACC_INTERFACE;
    if (interfaceDecl->IsStatic()) {
        accessFlags |= ACC_STATIC;
    }

    if (interfaceDecl->HasAnnotations()) {
        interfaceRecord.metadata->SetAnnotations(
            GenCustomAnnotations(interfaceDecl->Annotations(), interfaceRecord.name));
    }
    interfaceRecord.metadata->SetAccessFlags(accessFlags);
    interfaceRecord.sourceFile = std::string {Context()->parserProgram->VarBinder()->Program()->RelativeFilePath()};
    interfaceRecord.metadata->SetAttributeValue(Signatures::EXTENDS_ATTRIBUTE, Signatures::BUILTIN_OBJECT);

    GenClassInheritedFields(baseType, interfaceRecord);

    for (const auto *prop : interfaceDecl->Body()->Body()) {
        if (prop->IsClassProperty()) {
            GenClassField(prop->AsClassProperty(), interfaceRecord, external);
        } else if (prop->IsMethodDefinition()) {
            GenInterfaceMethodDefinition(prop->AsMethodDefinition(), external);
            for (auto *overload : prop->AsMethodDefinition()->Overloads()) {
                GenInterfaceMethodDefinition(overload, external);
            }
        }
    }

    if (std::any_of(interfaceDecl->Body()->Body().begin(), interfaceDecl->Body()->Body().end(),
                    [](const ir::AstNode *node) { return node->IsOverloadDeclaration(); })) {
        std::vector<pandasm::AnnotationData> annotations {};
        annotations.emplace_back(GenAnnotationFunctionOverload(interfaceDecl->Body()->Body()));
        interfaceRecord.metadata->AddAnnotations(annotations);
    }

    if (external) {
        interfaceRecord.metadata->SetAttribute(Signatures::EXTERNAL);
        Program()->recordTable.emplace(interfaceRecord.name, std::move(interfaceRecord));
        return;
    }

    for (auto *it : baseType->Interfaces()) {
        auto *declNode = it->GetDeclNode();
        ES2PANDA_ASSERT(declNode->IsTSInterfaceDeclaration());
        std::string name = declNode->AsTSInterfaceDeclaration()->InternalName().Mutf8();
        interfaceRecord.metadata->SetAttributeValue(Signatures::IMPLEMENTS_ATTRIBUTE, name);
    }

    Program()->recordTable.emplace(interfaceRecord.name, std::move(interfaceRecord));
}

std::vector<pandasm::AnnotationData> ETSEmitter::GenAnnotations(const ir::ClassDefinition *classDef)
{
    std::vector<pandasm::AnnotationData> annotations;
    const ir::AstNode *parent = classDef->Parent();
    while (parent != nullptr) {
        if ((classDef->Modifiers() & ir::ClassDefinitionModifiers::FUNCTIONAL_REFERENCE) != 0U) {
            annotations.emplace_back(GenAnnotationFunctionalReference(classDef));
            break;
        }
        if (parent->IsMethodDefinition()) {
            annotations.emplace_back(GenAnnotationEnclosingMethod(parent->AsMethodDefinition()));
            annotations.emplace_back(GenAnnotationInnerClass(classDef, parent));
            break;
        }
        if (parent->IsClassDefinition()) {
            annotations.emplace_back(GenAnnotationEnclosingClass(
                parent->AsClassDefinition()->TsType()->AsETSObjectType()->AssemblerName().Utf8()));
            annotations.emplace_back(GenAnnotationInnerClass(classDef, parent));
            break;
        }
        parent = parent->Parent();
    }

    auto classIdent = classDef->Ident()->Name().Mutf8();
    bool isConstruct = classIdent == Signatures::JSNEW_CLASS;
    if (isConstruct || classIdent == Signatures::JSCALL_CLASS) {
        auto *callNames = Context()->GetChecker()->AsETSChecker()->DynamicCallNames(isConstruct);
        annotations.push_back(GenAnnotationDynamicCall(*callNames));
    }

    if (std::any_of(classDef->Body().begin(), classDef->Body().end(),
                    [](const ir::AstNode *node) { return node->IsOverloadDeclaration(); })) {
        annotations.push_back(GenAnnotationFunctionOverload(classDef->Body()));
    }

    return annotations;
}

static uint32_t GetAccessFlags(const ir::ClassDefinition *classDef)
{
    uint32_t accessFlags = ACC_PUBLIC;
    if (classDef->IsAbstract()) {
        accessFlags |= ACC_ABSTRACT;
    } else if (classDef->IsFinal()) {
        accessFlags |= ACC_FINAL;
    }

    if (classDef->IsStatic()) {
        accessFlags |= ACC_STATIC;
    }

    return accessFlags;
}

void ETSEmitter::GenClassRecord(const ir::ClassDefinition *classDef, bool external)
{
    auto classRecord = pandasm::Record(classDef->InternalName().Mutf8(), Program()->lang);
    uint32_t accessFlags = GetAccessFlags(classDef);
    classRecord.metadata->SetAccessFlags(accessFlags);
    classRecord.sourceFile = std::string {Context()->parserProgram->VarBinder()->Program()->RelativeFilePath()};
    auto *baseType = classDef->TsType()->IsGradualType()
                         ? classDef->TsType()->AsGradualType()->GetBaseType()->AsETSObjectType()
                         : classDef->TsType()->AsETSObjectType();
    GenClassInheritedFields(baseType, classRecord);
    for (const auto *prop : classDef->Body()) {
        if (!prop->IsClassProperty()) {
            continue;
        }

        GenClassField(prop->AsClassProperty(), classRecord, external);
    }

    if (external) {
        classRecord.metadata->SetAttribute(Signatures::EXTERNAL);
        Program()->recordTable.emplace(classRecord.name, std::move(classRecord));
        return;
    }

    if (baseType->SuperType() != nullptr) {
        classRecord.metadata->SetAttributeValue(Signatures::EXTENDS_ATTRIBUTE,
                                                baseType->SuperType()->AssemblerName().Mutf8());
    } else {
        // NOTE: rtakacs. Replace the whole if block (below) with assert when lambda objects have super class.
        if (baseType->AssemblerName().Mutf8() != Signatures::BUILTIN_OBJECT) {
            classRecord.metadata->SetAttributeValue(Signatures::EXTENDS_ATTRIBUTE, Signatures::BUILTIN_OBJECT);
        }
    }

    for (auto *it : baseType->Interfaces()) {
        auto *declNode = it->GetDeclNode();
        // NOTE: itrubachev. replace it with ES2PANDA_ASSERT(decl_node->IsTSInterfaceDeclaration())
        // after adding proper creation of lambda object in ETSFunctionType::AssignmentSource
        if (!declNode->IsTSInterfaceDeclaration()) {
            continue;
        }
        std::string name = declNode->AsTSInterfaceDeclaration()->InternalName().Mutf8();
        classRecord.metadata->SetAttributeValue(Signatures::IMPLEMENTS_ATTRIBUTE, name);
    }

    if (classDef->HasAnnotations()) {
        classRecord.metadata->SetAnnotations(GenCustomAnnotations(classDef->Annotations(), classRecord.name));
    }

    std::vector<pandasm::AnnotationData> annotations = GenAnnotations(classDef);
    if (classDef->IsNamespaceTransformed() || classDef->IsGlobalInitialized()) {
        annotations.push_back(GenAnnotationModule(classDef));
    }

    if (!annotations.empty() && !classDef->IsLazyImportObjectClass()) {
        classRecord.metadata->AddAnnotations(annotations);
    }

    Program()->recordTable.emplace(classRecord.name, std::move(classRecord));
}

void ETSEmitter::ProcessArrayExpression(
    std::string &baseName, std::vector<std::pair<std::string, std::vector<pandasm::LiteralArray::Literal>>> &result,
    std::vector<pandasm::LiteralArray::Literal> &literals, const ir::Expression *elem)
{
    auto litArrays = CreateLiteralArray(baseName, elem);
    auto emplaceLiteral = [&literals](panda_file::LiteralTag tag, const auto &value) {
        literals.emplace_back(pandasm::LiteralArray::Literal {tag, value});
    };

    emplaceLiteral(panda_file::LiteralTag::TAGVALUE, static_cast<uint8_t>(panda_file::LiteralTag::LITERALARRAY));
    emplaceLiteral(panda_file::LiteralTag::LITERALARRAY, litArrays.back().first);
    for (const auto &item : litArrays) {
        result.push_back(item);
    }
}

static void CreateEnumProp(const ir::ClassProperty *prop, pandasm::Field &field)
{
    if (prop->Value() == nullptr) {
        return;
    }
    field.metadata->SetFieldType(field.type);
    ES2PANDA_ASSERT(prop->Value()->AsMemberExpression()->PropVar() != nullptr);
    auto declNode = prop->Value()->AsMemberExpression()->PropVar()->Declaration()->Node();
    auto *init = declNode->AsClassProperty()->OriginEnumMember()->Init();
    if (init->IsNumberLiteral()) {
        auto value = init->AsNumberLiteral()->Number().GetInt();
        field.metadata->SetValue(pandasm::ScalarValue::Create<pandasm::Value::Type::I32>(value));
    } else if (init->IsStringLiteral()) {
        auto value = init->AsStringLiteral()->Str().Mutf8();
        field.metadata->SetValue(pandasm::ScalarValue::Create<pandasm::Value::Type::STRING>(value));
    } else {
        ES2PANDA_UNREACHABLE();
    }
}

static void ProcessEnumExpression(std::vector<pandasm::LiteralArray::Literal> &literals, const ir::Expression *elem)
{
    auto *memberExpr = elem->IsCallExpression() ? elem->AsCallExpression()->Arguments()[0]->AsMemberExpression()
                                                : elem->AsMemberExpression();
    ES2PANDA_ASSERT(memberExpr->PropVar() != nullptr);
    auto *init = memberExpr->PropVar()->Declaration()->Node()->AsClassProperty()->OriginEnumMember()->Init();
    if (init->IsNumberLiteral()) {
        auto enumValue = static_cast<uint32_t>(init->AsNumberLiteral()->Number().GetInt());
        literals.emplace_back(pandasm::LiteralArray::Literal {panda_file::LiteralTag::TAGVALUE,
                                                              static_cast<uint8_t>(panda_file::LiteralTag::INTEGER)});
        literals.emplace_back(pandasm::LiteralArray::Literal {panda_file::LiteralTag::INTEGER, enumValue});
    } else {
        auto enumValue = init->AsStringLiteral()->Str().Mutf8();
        literals.emplace_back(pandasm::LiteralArray::Literal {panda_file::LiteralTag::TAGVALUE,
                                                              static_cast<uint8_t>(panda_file::LiteralTag::STRING)});
        literals.emplace_back(pandasm::LiteralArray::Literal {panda_file::LiteralTag::STRING, enumValue});
    }
}

void ETSEmitter::ProcessArrayElement(const ir::Expression *elem, std::vector<pandasm::LiteralArray::Literal> &literals,
                                     std::string &baseName, LiteralArrayVector &result)
{
    ES2PANDA_ASSERT(elem->IsLiteral() || elem->IsArrayExpression() || elem->IsMemberExpression());
    if (elem->IsMemberExpression()) {
        ProcessEnumExpression(literals, elem);
        return;
    }
    auto emplaceLiteral = [&literals](panda_file::LiteralTag tag, auto value) {
        literals.emplace_back(
            pandasm::LiteralArray::Literal {panda_file::LiteralTag::TAGVALUE, static_cast<uint8_t>(tag)});
        literals.emplace_back(pandasm::LiteralArray::Literal {tag, value});
    };
    // NOTE(dkofanov): Why 'LiteralTag::ARRAY_*'-types isn't used?
    switch (checker::ETSChecker::TypeKind(elem->TsType())) {
        case checker::TypeFlag::ETS_BOOLEAN: {
            emplaceLiteral(panda_file::LiteralTag::BOOL, elem->AsBooleanLiteral()->Value());
            break;
        }
        case checker::TypeFlag::CHAR:
        case checker::TypeFlag::BYTE:
        case checker::TypeFlag::SHORT:
        case checker::TypeFlag::INT: {
            emplaceLiteral(panda_file::LiteralTag::INTEGER,
                           static_cast<uint32_t>(elem->AsNumberLiteral()->Number().GetInt()));
            break;
        }
        case checker::TypeFlag::LONG: {
            emplaceLiteral(panda_file::LiteralTag::BIGINT,
                           static_cast<uint64_t>(elem->AsNumberLiteral()->Number().GetInt()));
            break;
        }
        case checker::TypeFlag::FLOAT: {
            emplaceLiteral(panda_file::LiteralTag::FLOAT, elem->AsNumberLiteral()->Number().GetFloat());
            break;
        }
        case checker::TypeFlag::DOUBLE: {
            emplaceLiteral(panda_file::LiteralTag::DOUBLE, elem->AsNumberLiteral()->Number().GetDouble());
            break;
        }
        case checker::TypeFlag::ETS_OBJECT: {
            emplaceLiteral(panda_file::LiteralTag::STRING, elem->AsStringLiteral()->ToString());
            break;
        }
        case checker::TypeFlag::ETS_ARRAY: {
            ProcessArrayExpression(baseName, result, literals, elem);
            break;
        }
        default: {
            ES2PANDA_UNREACHABLE();
        }
    }
}

LiteralArrayVector ETSEmitter::CreateLiteralArray(std::string &baseName, const ir::Expression *array)
{
    LiteralArrayVector result;
    std::vector<pandasm::LiteralArray::Literal> literals;
    ArenaVector<ir::Expression *> elements {array->AsArrayExpression()->Elements()};

    for (const auto *elem : elements) {
        ProcessArrayElement(elem, literals, baseName, result);
    }

    std::string litArrayName =
        util::NameMangler::GetInstance()->AppendToAnnotationName(baseName, std::to_string(g_litArrayValueCount++));
    result.emplace_back(litArrayName, literals);
    return result;
}

void ETSEmitter::CreateLiteralArrayProp(const ir::ClassProperty *prop, std::string &baseName, pandasm::Field &field)
{
    auto *checker = Context()->GetChecker()->AsETSChecker();
    uint8_t rank = 1;
    auto *elemType = checker->GetElementTypeOfArray(prop->TsType());
    while (elemType->IsETSArrayType() || elemType->IsETSResizableArrayType()) {
        ++rank;
        elemType = checker->GetElementTypeOfArray(elemType);
    }
    std::stringstream ss;
    elemType->ToAssemblerType(ss);
    field.type = pandasm::Type(ss.str(), rank);

    auto value = prop->Value();
    if (value != nullptr) {
        std::string newBaseName = util::NameMangler::GetInstance()->AppendToAnnotationName(baseName, field.name);
        auto litArray = CreateLiteralArray(newBaseName, value);
        for (const auto &item : litArray) {
            Program()->literalarrayTable.emplace(item.first, item.second);
        }
        field.metadata->SetValue(
            pandasm::ScalarValue::Create<pandasm::Value::Type::LITERALARRAY>(std::string_view {litArray.back().first}));
    }
}

void ETSEmitter::GenCustomAnnotationProp(const ir::ClassProperty *prop, std::string &baseName, pandasm::Record &record,
                                         bool external)
{
    auto field = pandasm::Field(Program()->lang);
    auto *type = prop->TsType();
    ES2PANDA_ASSERT(prop->Id() != nullptr);
    field.name = prop->Id()->Name().Mutf8();
    field.type = PandasmTypeWithRank(type);
    field.metadata->SetAccessFlags(TranslateModifierFlags(prop->Modifiers()));

    if (external) {
        field.metadata->SetAttribute(Signatures::EXTERNAL);
    } else if (type->IsETSEnumType()) {
        CreateEnumProp(prop, field);
    } else if (type->IsETSPrimitiveType() || type->IsETSStringType()) {
        EmitDefaultFieldValue(field, prop->Value());
    } else if (type->IsETSArrayType() || type->IsETSResizableArrayType()) {
        CreateLiteralArrayProp(prop, baseName, field);
    } else {
        ES2PANDA_UNREACHABLE();
    }
    record.fieldList.emplace_back(std::move(field));
}

void ETSEmitter::GenCustomAnnotationRecord(const ir::AnnotationDeclaration *annoDecl, std::string &baseName,
                                           bool external)
{
    auto annoRecord = pandasm::Record(annoDecl->InternalName().Mutf8(), Program()->lang);
    if (Program()->recordTable.find(annoRecord.name) != Program()->recordTable.end()) {
        return;
    }

    if (external) {
        annoRecord.metadata->SetAttribute(Signatures::EXTERNAL);
    }

    uint32_t accessFlags = ACC_PUBLIC | ACC_ABSTRACT | ACC_ANNOTATION;
    annoRecord.metadata->SetAccessFlags(accessFlags);
    annoRecord.sourceFile = std::string {Context()->parserProgram->VarBinder()->Program()->RelativeFilePath()};
    for (auto *it : annoDecl->Properties()) {
        GenCustomAnnotationProp(it->AsClassProperty(), baseName, annoRecord, external);
    }

    Program()->recordTable.emplace(annoRecord.name, std::move(annoRecord));
}

pandasm::AnnotationElement ETSEmitter::ProcessArrayType(const ir::ClassProperty *prop, std::string &baseName,
                                                        const ir::Expression *init)
{
    ES2PANDA_ASSERT(prop->Id() != nullptr);
    auto propName = prop->Id()->Name().Mutf8();
    std::string newBaseName = util::NameMangler::GetInstance()->AppendToAnnotationName(baseName, propName);
    auto litArrays = CreateLiteralArray(newBaseName, init);

    for (const auto &item : litArrays) {
        Program()->literalarrayTable.emplace(item.first, item.second);
    }

    return pandasm::AnnotationElement {
        propName, std::make_unique<pandasm::ScalarValue>(pandasm::ScalarValue::Create<pandasm::Value::Type::STRING>(
                      std::string_view {litArrays.back().first}))};
}

static pandasm::AnnotationElement ProcessETSEnumType(std::string &baseName, const ir::Expression *init,
                                                     const checker::Type *type)
{
    ES2PANDA_ASSERT(init->AsMemberExpression()->PropVar() != nullptr);
    auto declNode = init->AsMemberExpression()->PropVar()->Declaration()->Node();
    auto *initValue = declNode->AsClassProperty()->OriginEnumMember()->Init();
    if (type->IsETSIntEnumType()) {
        auto enumValue = static_cast<uint32_t>(initValue->AsNumberLiteral()->Number().GetInt());
        auto intEnumValue = pandasm::ScalarValue::Create<pandasm::Value::Type::I32>(enumValue);
        return pandasm::AnnotationElement {baseName, std::make_unique<pandasm::ScalarValue>(intEnumValue)};
    }
    ES2PANDA_ASSERT(type->IsETSStringEnumType());
    auto enumValue = initValue->AsStringLiteral()->Str().Mutf8();
    auto stringValue = pandasm::ScalarValue::Create<pandasm::Value::Type::STRING>(enumValue);
    return pandasm::AnnotationElement {baseName, std::make_unique<pandasm::ScalarValue>(stringValue)};
}

pandasm::AnnotationElement ETSEmitter::GenCustomAnnotationElement(const ir::ClassProperty *prop, std::string &baseName)
{
    const auto *init = prop->Value();
    const auto *type = init->TsType();
    if (type->IsETSArrayType() || type->IsETSResizableArrayType()) {
        return ProcessArrayType(prop, baseName, init);
    }
    if (type->IsETSEnumType()) {
        return ProcessETSEnumType(baseName, init, type);
    }
    if (init->IsLiteral()) {
        auto typeKind = checker::ETSChecker::TypeKind(type);
        ES2PANDA_ASSERT(prop->Id() != nullptr);
        auto propName = prop->Id()->Name().Mutf8();
        return pandasm::AnnotationElement {
            propName, std::make_unique<pandasm::ScalarValue>(CreateScalarValue(init->AsLiteral(), typeKind))};
    }
    ES2PANDA_UNREACHABLE();
}

pandasm::AnnotationData ETSEmitter::GenCustomAnnotation(ir::AnnotationUsage *anno, std::string &baseName)
{
    auto *annoDecl = anno->GetBaseName()->Variable()->Declaration()->Node()->AsAnnotationDeclaration();
    pandasm::AnnotationData annotation(annoDecl->InternalName().Mutf8());
    if (annoDecl->IsImportDeclaration()) {
        auto annoRecord = pandasm::Record(annoDecl->InternalName().Mutf8(), Program()->lang);
        annoRecord.metadata->SetAttribute(Signatures::EXTERNAL);
        uint32_t accessFlags = ACC_PUBLIC | ACC_ABSTRACT | ACC_ANNOTATION;
        annoRecord.metadata->SetAccessFlags(accessFlags);
        Program()->recordTable.emplace(annoRecord.name, std::move(annoRecord));
    }

    for (auto *prop : anno->Properties()) {
        annotation.AddElement(GenCustomAnnotationElement(prop->AsClassProperty(), baseName));
    }
    return annotation;
}

std::vector<pandasm::AnnotationData> ETSEmitter::GenCustomAnnotations(
    const ArenaVector<ir::AnnotationUsage *> &annotationUsages, const std::string &baseName)
{
    std::vector<pandasm::AnnotationData> annotations;
    for (auto *anno : annotationUsages) {
        auto *annoDecl = anno->GetBaseName()->Variable()->Declaration()->Node()->AsAnnotationDeclaration();
        if (!annoDecl->IsSourceRetention()) {
            std::string newBaseName = util::NameMangler::GetInstance()->CreateMangledNameForAnnotation(
                baseName, anno->GetBaseName()->Name().Mutf8());
            annotations.emplace_back(GenCustomAnnotation(anno, newBaseName));
        }
    }
    return annotations;
}

pandasm::AnnotationData ETSEmitter::GenAnnotationModule(const ir::ClassDefinition *classDef)
{
    std::vector<pandasm::ScalarValue> exportedClasses {};

    for (auto cls : classDef->ExportedClasses()) {
        exportedClasses.emplace_back(pandasm::ScalarValue::Create<pandasm::Value::Type::RECORD>(
            pandasm::Type::FromName(cls->Definition()->InternalName().Utf8(), true)));
    }

    GenAnnotationRecord(Signatures::ETS_ANNOTATION_MODULE);
    pandasm::AnnotationData moduleAnno(Signatures::ETS_ANNOTATION_MODULE);
    pandasm::AnnotationElement value(
        Signatures::ANNOTATION_KEY_EXPORTED,
        std::make_unique<pandasm::ArrayValue>(pandasm::Value::Type::RECORD, std::move(exportedClasses)));
    moduleAnno.AddElement(std::move(value));
    return moduleAnno;
}

pandasm::AnnotationData ETSEmitter::GenAnnotationFunctionOverload(const ArenaVector<ir::AstNode *> &body)
{
    GenAnnotationRecord(Signatures::ETS_ANNOTATION_FUNCTION_OVERLOAD);
    pandasm::AnnotationData overloadAnno(Signatures::ETS_ANNOTATION_FUNCTION_OVERLOAD);

    for (auto *node : body) {
        if (!node->IsOverloadDeclaration()) {
            continue;
        }
        std::vector<pandasm::ScalarValue> overloadDeclRecords {};

        for (auto *overloadedName : node->AsOverloadDeclaration()->OverloadedList()) {
            auto *methodDef = overloadedName->Variable()->Declaration()->Node()->AsMethodDefinition();
            overloadDeclRecords.emplace_back(pandasm::ScalarValue::Create<pandasm::Value::Type::METHOD>(
                methodDef->Function()->Scope()->InternalName().Mutf8()));
        }

        pandasm::AnnotationElement value(
            node->AsOverloadDeclaration()->Id()->Name().Mutf8(),
            std::make_unique<pandasm::ArrayValue>(pandasm::Value::Type::RECORD, std::move(overloadDeclRecords)));

        overloadAnno.AddElement(std::move(value));
    }
    return overloadAnno;
}

pandasm::AnnotationData ETSEmitter::GenAnnotationSignature(const ir::ClassDefinition *classDef)
{
    std::vector<pandasm::ScalarValue> parts {};
    const auto &typeParams = classDef->TypeParams()->Params();

    auto const addStringValue = [&parts](std::string_view str) {
        parts.emplace_back(pandasm::ScalarValue::Create<pandasm::Value::Type::STRING>(str));
    };

    if (!typeParams.empty()) {
        addStringValue(Signatures::GENERIC_BEGIN);

        for (const auto *param : typeParams) {
            addStringValue(Signatures::MANGLE_BEGIN);
            std::stringstream ss;
            param->Constraint()->TsType()->ToAssemblerTypeWithRank(ss);
            auto asmName = util::StringView(ss.str());
            addStringValue(checker::ETSObjectType::NameToDescriptor(asmName));
        }
        addStringValue(Signatures::GENERIC_END);
    }

    if (auto super = classDef->TsType()->AsETSObjectType()->SuperType(); super != nullptr) {
        addStringValue(checker::ETSObjectType::NameToDescriptor(util::StringView(super->AssemblerName().Mutf8())));
    } else {
        addStringValue(checker::ETSObjectType::NameToDescriptor(Signatures::BUILTIN_OBJECT));
    }

    GenAnnotationRecord(Signatures::ETS_ANNOTATION_SIGNATURE);
    pandasm::AnnotationData signature(Signatures::ETS_ANNOTATION_SIGNATURE);
    pandasm::AnnotationElement value(
        Signatures::ANNOTATION_KEY_VALUE,
        std::make_unique<pandasm::ArrayValue>(pandasm::Value::Type::STRING, std::move(parts)));
    signature.AddElement(std::move(value));
    return signature;
}

pandasm::AnnotationData ETSEmitter::GenAnnotationEnclosingMethod(const ir::MethodDefinition *methodDef)
{
    GenAnnotationRecord(Signatures::ETS_ANNOTATION_ENCLOSING_METHOD);
    pandasm::AnnotationData enclosingMethod(Signatures::ETS_ANNOTATION_ENCLOSING_METHOD);
    ES2PANDA_ASSERT(methodDef->Function() != nullptr);
    pandasm::AnnotationElement value(
        Signatures::ANNOTATION_KEY_VALUE,
        std::make_unique<pandasm::ScalarValue>(pandasm::ScalarValue::Create<pandasm::Value::Type::METHOD>(
            methodDef->Function()->Scope()->InternalName().Mutf8())));
    enclosingMethod.AddElement(std::move(value));
    return enclosingMethod;
}

pandasm::AnnotationData ETSEmitter::GenAnnotationFunctionalReference(const ir::ClassDefinition *classDef)
{
    GenAnnotationRecord(Signatures::ETS_ANNOTATION_FUNCTIONAL_REFERENCE);
    pandasm::AnnotationData functionalReference(Signatures::ETS_ANNOTATION_FUNCTIONAL_REFERENCE);
    bool isStatic = classDef->FunctionalReferenceReferencedMethod()->IsStatic();
    ES2PANDA_ASSERT(const_cast<ir::ClassDefinition *>(classDef) != nullptr);
    pandasm::AnnotationElement value(
        Signatures::ANNOTATION_KEY_VALUE,
        std::make_unique<pandasm::ScalarValue>(
            pandasm::ScalarValue::Create<pandasm::Value::Type::METHOD>(const_cast<ir::ClassDefinition *>(classDef)
                                                                           ->FunctionalReferenceReferencedMethod()
                                                                           ->Function()
                                                                           ->Scope()
                                                                           ->InternalName()
                                                                           .Mutf8(),
                                                                       isStatic)));
    functionalReference.AddElement(std::move(value));
    return functionalReference;
}

pandasm::AnnotationData ETSEmitter::GenAnnotationEnclosingClass(std::string_view className)
{
    GenAnnotationRecord(Signatures::ETS_ANNOTATION_ENCLOSING_CLASS);
    pandasm::AnnotationData enclosingClass(Signatures::ETS_ANNOTATION_ENCLOSING_CLASS);
    pandasm::AnnotationElement value(
        Signatures::ANNOTATION_KEY_VALUE,
        std::make_unique<pandasm::ScalarValue>(
            pandasm::ScalarValue::Create<pandasm::Value::Type::RECORD>(pandasm::Type::FromName(className, true))));
    enclosingClass.AddElement(std::move(value));
    return enclosingClass;
}

pandasm::AnnotationData ETSEmitter::GenAnnotationInnerClass(const ir::ClassDefinition *classDef,
                                                            const ir::AstNode *parent)
{
    GenAnnotationRecord(Signatures::ETS_ANNOTATION_INNER_CLASS);
    pandasm::AnnotationData innerClass(Signatures::ETS_ANNOTATION_INNER_CLASS);
    const bool isAnonymous = (classDef->Modifiers() & ir::ClassDefinitionModifiers::ANONYMOUS) != 0;
    pandasm::AnnotationElement name(Signatures::ANNOTATION_KEY_NAME,
                                    std::make_unique<pandasm::ScalarValue>(
                                        isAnonymous
                                            ? pandasm::ScalarValue::Create<pandasm::Value::Type::STRING_NULLPTR>(0)
                                            : pandasm::ScalarValue::Create<pandasm::Value::Type::STRING>(
                                                  classDef->TsType()->AsETSObjectType()->AssemblerName().Mutf8())));
    innerClass.AddElement(std::move(name));

    pandasm::AnnotationElement accessFlags(
        Signatures::ANNOTATION_KEY_ACCESS_FLAGS,
        std::make_unique<pandasm::ScalarValue>(
            pandasm::ScalarValue::Create<pandasm::Value::Type::I32>(TranslateModifierFlags(parent->Modifiers()))));
    innerClass.AddElement(std::move(accessFlags));
    return innerClass;
}

ir::MethodDefinition *ETSEmitter::FindAsyncImpl(ir::ScriptFunction *asyncFunc)
{
    std::string implName = checker::ETSChecker::GetAsyncImplName(asyncFunc->Id()->Name());
    ir::AstNode *ownerNode = asyncFunc->Signature()->Owner()->GetDeclNode();
    ES2PANDA_ASSERT(ownerNode != nullptr && ownerNode->IsClassDefinition());
    const ir::ClassDefinition *classDef = ownerNode->AsClassDefinition();
    ES2PANDA_ASSERT(classDef != nullptr);

    ir::MethodDefinition *method = nullptr;
    for (auto node : classDef->Body()) {
        if (!node->IsMethodDefinition()) {
            continue;
        }
        bool isSameName = node->AsMethodDefinition()->Id()->Name().Utf8() == implName;
        bool isBothStaticOrInstance =
            (node->Modifiers() & ir::ModifierFlags::STATIC) == (asyncFunc->Modifiers() & ir::ModifierFlags::STATIC);
        if (isSameName && isBothStaticOrInstance) {
            method = node->AsMethodDefinition();
            break;
        }
    }
    if (method == nullptr) {
        return nullptr;
    }

    if (asyncFunc->AsyncPairMethod() == method->Function()) {
        return method;
    }

    for (auto overload : method->Overloads()) {
        if (asyncFunc->AsyncPairMethod() == overload->Function()) {
            return overload;
        }
    }

    return nullptr;
}

pandasm::AnnotationData ETSEmitter::GenAnnotationAsync(ir::ScriptFunction *scriptFunc)
{
    GenAnnotationRecord(Signatures::ETS_COROUTINE_ASYNC);
    const ir::MethodDefinition *impl = FindAsyncImpl(scriptFunc);
    ES2PANDA_ASSERT(impl != nullptr);
    ES2PANDA_ASSERT(impl->Function() != nullptr);
    pandasm::AnnotationData ann(Signatures::ETS_COROUTINE_ASYNC);
    pandasm::AnnotationElement value(
        Signatures::ANNOTATION_KEY_VALUE,
        std::make_unique<pandasm::ScalarValue>(pandasm::ScalarValue::Create<pandasm::Value::Type::METHOD>(
            impl->Function()->Scope()->InternalName().Mutf8())));
    ann.AddElement(std::move(value));
    return ann;
}

pandasm::AnnotationData ETSEmitter::GenAnnotationDynamicCall(DynamicCallNamesMap &callNames)
{
    GenAnnotationRecord(Signatures::ETS_ANNOTATION_DYNAMIC_CALL);
    pandasm::AnnotationData dynamicCallSig(Signatures::ETS_ANNOTATION_DYNAMIC_CALL);
    std::vector<pandasm::ScalarValue> allParts {};
    for (auto &[parts, startIdx] : callNames) {
        startIdx = allParts.size();
        for (const auto &str : parts) {
            allParts.emplace_back(pandasm::ScalarValue::Create<pandasm::Value::Type::STRING>(str.Utf8()));
        }
    }
    pandasm::AnnotationElement value(
        Signatures::ANNOTATION_KEY_VALUE,
        std::make_unique<pandasm::ArrayValue>(pandasm::Value::Type::STRING, std::move(allParts)));
    dynamicCallSig.AddElement(std::move(value));
    return dynamicCallSig;
}

void ETSEmitter::GenAnnotationRecord(std::string_view recordNameView, bool isRuntime, bool isType)
{
    const std::string recordName(recordNameView);
    const auto recordIt = Program()->recordTable.find(recordName);
    if (recordIt == Program()->recordTable.end()) {
        pandasm::Record record(recordName, EXTENSION);
        record.metadata->SetAttribute(Signatures::EXTERNAL);
        record.metadata->SetAttribute(Signatures::ANNOTATION_ATTRIBUTE);
        if (isRuntime && isType) {
            record.metadata->SetAttributeValue(Signatures::ANNOTATION_ATTRIBUTE_TYPE,
                                               Signatures::RUNTIME_TYPE_ANNOTATION);
        } else if (isRuntime && !isType) {
            record.metadata->SetAttributeValue(Signatures::ANNOTATION_ATTRIBUTE_TYPE, Signatures::RUNTIME_ANNOTATION);
        } else if (!isRuntime && isType) {
            record.metadata->SetAttributeValue(Signatures::ANNOTATION_ATTRIBUTE_TYPE, Signatures::TYPE_ANNOTATION);
        }
        Program()->recordTable.emplace(record.name, std::move(record));
    }
}
}  // namespace ark::es2panda::compiler

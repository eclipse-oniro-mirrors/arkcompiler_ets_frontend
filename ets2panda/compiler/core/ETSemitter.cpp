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

#include <iostream>
#include <memory>
#include <ostream>
#include <set>
#include <string>
#include <unordered_set>
#include <utility>

#include "annotation.h"
#include "compiler/base/catchTable.h"
#include "compiler/core/ETSGen.h"
#include "util/es2pandaMacros.h"
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
#include "ir/ts/tsTypeParameter.h"
#include "ir/typeNode.h"
#include "parser/program/program.h"
#include "checker/checker.h"
#include "checker/types/signature.h"
#include "checker/ETSchecker.h"
#include "checker/types/type.h"
#include "checker/types/ets/types.h"
#include "checker/types/ets/etsPartialTypeParameter.h"
#include "public/public.h"
#include "util/nameMangler.h"

#include "assembly-program.h"

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
        // NOTE: torokg. Add ACC_INTERNAL access flag to libarkbase
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

namespace detail {

// #29438
// NOLINTNEXTLINE (fuchsia-statically-constructed-objects, cert-err58-cpp)
static const std::set<std::string> AOT_WORKAROUND_BLACKLIST {
    "std.core.String",   "std.core.String[]",      "std.core.Object",
    "std.core.Object[]", "std.core.StringBuilder", "std.core.StringBuilder.<get>stringLength:i32;",
};

class EmitterDependencies final {
public:
    explicit EmitterDependencies() = default;
    NO_COPY_SEMANTIC(EmitterDependencies);
    NO_MOVE_SEMANTIC(EmitterDependencies);

    inline const std::string &AddDependence(std::string const &str)
    {
        reachable_.insert(str);
        return str;
    }

    bool IsNotRequired(std::string const &str, bool isExternal = true)
    {
        if (isExternal) {
            return toEmit_.find(str) == toEmit_.end();
        }
        AddDependence(str);
        return false;
    }

    void ProcessToEmitExternal()
    {
        toEmit_ = reachable_;
    }

    void ProceedToEmitExternalDelta()
    {
        if (reachable_.size() == toEmit_.size()) {
            toEmit_.clear();
            return;
        }

        std::unordered_set<std::string> diff;

        for (auto &e : reachable_) {
            if (toEmit_.find(e) == toEmit_.end()) {
                diff.insert(e);
            }
        }
        std::swap(toEmit_, diff);
    }
    ~EmitterDependencies() = default;

private:
    std::unordered_set<std::string> reachable_ {
        AOT_WORKAROUND_BLACKLIST.begin(),
        AOT_WORKAROUND_BLACKLIST.end(),
    };
    std::unordered_set<std::string> toEmit_ {};
};

}  // namespace detail

static pandasm::Type PandasmTypeWithRank(ETSEmitter *emitter, checker::Type const *type)
{
    if (type->IsETSTypeParameter()) {
        return PandasmTypeWithRank(emitter, type->AsETSTypeParameter()->GetConstraintType());
    }
    if (type->IsETSNonNullishType()) {
        return PandasmTypeWithRank(emitter, type->AsETSNonNullishType()->GetUnderlying());
    }
    if (type->IsETSPartialTypeParameter()) {
        return PandasmTypeWithRank(emitter, type->AsETSPartialTypeParameter()->GetUnderlying());
    }

    auto asmType = type->ToAssemblerType();
    auto res = pandasm::Type(asmType, type->Rank());
    if (res.IsObject() && !(res.IsArray() || res.IsUnion())) {
        emitter->AddDependence(asmType);
    }
    return res;
}

static std::string ToAssemblerSignature(ir::ScriptFunction const *func)
{
    return func->Scope()->InternalName().Mutf8();
}

static std::string ToAssemblerType(ir::AstNode const *node)
{
    if (node->IsClassDefinition()) {
        return node->AsClassDefinition()->InternalName().Mutf8();
    }
    if (node->IsTSInterfaceDeclaration()) {
        return node->AsTSInterfaceDeclaration()->InternalName().Mutf8();
    }
    if (node->IsAnnotationDeclaration()) {
        return node->AsAnnotationDeclaration()->InternalName().Mutf8();
    }
    ES2PANDA_UNREACHABLE();
}

static uint32_t ComputeAccessFlags(const ir::ScriptFunction *scriptFunc)
{
    uint32_t accessFlags = 0;
    if (!scriptFunc->IsStaticBlock()) {
        const auto *methodDef = util::Helpers::GetContainingClassMethodDefinition(scriptFunc);
        ES2PANDA_ASSERT(methodDef != nullptr);
        accessFlags |= TranslateModifierFlags(methodDef->Modifiers());
    }
    if (scriptFunc->HasRestParameter()) {
        accessFlags |= ACC_VARARGS;
    }
    if (!scriptFunc->HasBody() && scriptFunc->Signature()->Owner()->HasObjectFlag(checker::ETSObjectFlags::INTERFACE)) {
        accessFlags |= ACC_ABSTRACT;
    }
    return accessFlags;
}

static pandasm::Function GenScriptFunction(const ir::ScriptFunction *scriptFunc, ETSEmitter *emitter, bool external)
{
    auto *paramScope = scriptFunc->Scope()->ParamScope();
    auto func = pandasm::Function(ToAssemblerSignature(scriptFunc), EXTENSION);
    func.params.reserve(paramScope->Params().size());

    for (const auto *var : paramScope->Params()) {
        func.params.emplace_back(PandasmTypeWithRank(emitter, var->TsType()), EXTENSION);
        if (!external && var->Declaration()->Node() != nullptr &&
            var->Declaration()->Node()->IsETSParameterExpression() &&
            var->Declaration()->Node()->AsETSParameterExpression()->HasAnnotations()) {
            func.params.back().GetOrCreateMetadata()->SetAnnotations(emitter->GenCustomAnnotations(
                var->Declaration()->Node()->AsETSParameterExpression()->Annotations(), var->Name().Mutf8()));
        }
    }

    if (scriptFunc->IsConstructor() || scriptFunc->IsStaticBlock()) {
        func.returnType = pandasm::Type(Signatures::PRIMITIVE_VOID, 0);
    } else {
        func.returnType = PandasmTypeWithRank(emitter, scriptFunc->Signature()->ReturnType());
    }

    func.metadata->SetAccessFlags(ComputeAccessFlags(scriptFunc));

    if (scriptFunc->IsConstructor()) {
        func.metadata->SetAttribute(Signatures::CONSTRUCTOR);
    }

    if (external) {
        func.metadata->SetAttribute(Signatures::EXTERNAL);
    } else {
        if (scriptFunc->HasAnnotations()) {
            auto annotations = emitter->GenCustomAnnotations(scriptFunc->Annotations(), func.name);
            func.metadata->SetAnnotations(std::move(annotations));
        }
        if (scriptFunc->IsAsyncFunc()) {
            // callee does not tolerate constness
            func.metadata->AddAnnotations({
                emitter->GenAnnotationAsync(const_cast<ir::ScriptFunction *>(scriptFunc)),
            });
        }
    }
    return func;
}

ETSEmitter::ETSEmitter(const public_lib::Context *context)
    : Emitter(context), dependencies_(std::make_unique<detail::EmitterDependencies>())
{
}

ETSEmitter::~ETSEmitter() = default;

std::string const &ETSEmitter::AddDependence(std::string const &str)
{
    return dependencies_->AddDependence(str);
}

pandasm::Function *ETSFunctionEmitter::GenFunctionSignature()
{
    auto *scriptFunc = Cg()->RootNode()->AsScriptFunction();
    if (scriptFunc->IsExternal() || scriptFunc->Signature()->Owner()->GetDeclNode()->IsDeclare()) {
        return nullptr;
    }
    auto *emitter = static_cast<ETSEmitter *>(Cg()->Context()->emitter);
    auto func = GenScriptFunction(scriptFunc, emitter, false);
    if (scriptFunc->IsExternal()) {  // why do we emit an external method?
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

void ETSFunctionEmitter::GenFunctionAnnotations([[maybe_unused]] pandasm::Function *func)
{
    auto emitter = static_cast<const ETSGen *>(Cg())->Emitter();
    for (const auto *catchBlock : Cg()->CatchList()) {
        emitter->AddDependence(catchBlock->Exception());
    }
}

static void FilterForSimultaneous(varbinder::ETSBinder *varbinder)
{
    ArenaSet<ir::ClassDefinition *> &classDefinitions = varbinder->GetGlobalRecordTable()->ClassDefinitions();
    for (auto it = classDefinitions.begin(); it != classDefinitions.end(); ++it) {
        if ((*it)->InternalName().Is(Signatures::ETS_GLOBAL)) {
            classDefinitions.erase(it);
            break;
        }
    }
    // obsolete if record itself will not be emitted
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

void ETSEmitter::GenFunction(ir::ScriptFunction const *scriptFunc, bool external)
{
    if (!external && scriptFunc->Body() != nullptr) {
        return;  // was already produced by codegen
    }
    auto name = ToAssemblerSignature(scriptFunc);
    if (dependencies_->IsNotRequired(name, external)) {
        return;
    }

    auto func = GenScriptFunction(scriptFunc, this, external || scriptFunc->IsDeclare());  // #28197
    if (scriptFunc->Signature()->HasSignatureFlag(checker::SignatureFlags::STATIC) &&
        Program()->functionStaticTable.find(name) != Program()->functionStaticTable.cend()) {
        return;
    }
    if (!scriptFunc->Signature()->HasSignatureFlag(checker::SignatureFlags::STATIC) &&
        Program()->functionInstanceTable.find(name) != Program()->functionInstanceTable.cend()) {
        return;
    }

    Program()->AddToFunctionTable(std::move(func));
}

void ETSEmitter::GenAnnotation()
{
    Program()->lang = EXTENSION;
    auto *varbinder = static_cast<varbinder::ETSBinder *>(Context()->parserProgram->VarBinder());

    if (Context()->config->options->GetCompilationMode() == CompilationMode::GEN_ABC_FOR_EXTERNAL_SOURCE) {
        FilterForSimultaneous(varbinder);
    }
    ES2PANDA_ASSERT(varbinder->GetRecordTable() == varbinder->GetGlobalRecordTable());

    auto const traverseRecords = [this, varbinder](bool traverseExternals) {
        EmitRecordTable(varbinder->GetGlobalRecordTable(), false, traverseExternals);
        auto *saveProgram = varbinder->Program();
        for (auto [extProg, recordTable] : varbinder->GetExternalRecordTable()) {
            if (recordTable == varbinder->GetGlobalRecordTable()) {
                continue;
            }
            bool programIsExternal = !(varbinder->IsGenStdLib() || recordTable->Program()->IsGenAbcForExternal());
            varbinder->SetProgram(extProg);
            EmitRecordTable(recordTable, programIsExternal, traverseExternals);
        }
        varbinder->SetProgram(saveProgram);
        if (traverseExternals) {
            const auto *checker = static_cast<checker::ETSChecker *>(Context()->GetChecker());
            for (auto [arrType, _] : checker->GlobalArrayTypes()) {
                GenGlobalArrayRecord(arrType);
            }
            for (auto unionType : checker->UnionAssemblerTypes()) {
                GenGlobalUnionRecord(unionType);
            }
        }
    };

    // compile non-external dependencies
    traverseRecords(false);
    dependencies_->ProcessToEmitExternal();
    // compile external dependencies
    traverseRecords(true);                        // initial pass, which contributes to the difference
    dependencies_->ProceedToEmitExternalDelta();  // compute the difference
    traverseRecords(true);                        // re-run the pass
    dependencies_.reset();
}

void ETSEmitter::EmitRecordTable(varbinder::RecordTable *table, bool programIsExternal, bool traverseExternals)
{
    // #28197: We should just bailout if programIsExternal is not equal to traverseExternals, but there are
    // some sources which have declare* entities inside non-declare programs
    // Also, IsDeclare() for entity implies the program is external!
    // after it is fixed, the parameter traverseExternals to be removed
    if (!traverseExternals && programIsExternal) {  // #28197
        return;
    }

    const auto *varbinder = static_cast<const varbinder::ETSBinder *>(Context()->parserProgram->VarBinder());

    auto baseName = varbinder->GetRecordTable()->RecordName().Mutf8();
    for (auto *annoDecl : table->AnnotationDeclarations()) {
        auto external = programIsExternal || annoDecl->IsDeclare();
        if (external != traverseExternals) {  // #28197
            continue;
        }
        std::string newBaseName = util::NameMangler::GetInstance()->CreateMangledNameForAnnotation(
            baseName, annoDecl->GetBaseName()->Name().Mutf8());
        GenCustomAnnotationRecord(annoDecl, newBaseName, external);
    }

    for (auto *classDecl : table->ClassDefinitions()) {
        auto external = programIsExternal || classDecl->IsDeclare();
        if (external != traverseExternals) {  // #28197
            continue;
        }
        GenClassRecord(classDecl, external);
    }

    for (auto *interfaceDecl : table->InterfaceDeclarations()) {
        auto external = programIsExternal || interfaceDecl->IsDeclare();
        if (external != traverseExternals) {  // #28197
            continue;
        }
        GenInterfaceRecord(interfaceDecl, external);
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

void ETSEmitter::GenClassField(const ir::ClassProperty *prop, pandasm::Record &classRecord, bool external)
{
    if (dependencies_->IsNotRequired(classRecord.name + '.' + prop->Id()->Name().Mutf8(), external)) {
        return;
    }

    auto field = pandasm::Field(Program()->lang);
    ES2PANDA_ASSERT(prop->Id() != nullptr);
    field.name = prop->Id()->Name().Mutf8();
    field.type = PandasmTypeWithRank(this, prop->TsType());
    field.metadata->SetAccessFlags(TranslateModifierFlags(prop->Modifiers()));

    if (!external && prop->HasAnnotations()) {
        field.metadata->SetAnnotations(GenCustomAnnotations(prop->Annotations(), field.name));
    }

    if (external || prop->IsDeclare()) {  // #28197
        field.metadata->SetAttribute(Signatures::EXTERNAL);
    } else if (prop->TsType()->IsETSPrimitiveType() || prop->TsType()->IsETSStringType()) {
        EmitDefaultFieldValue(field, prop->Value());
    }

    classRecord.fieldList.emplace_back(std::move(field));
}

void ETSEmitter::GenGlobalArrayRecord(const checker::ETSArrayType *arrayType)
{
    // escompat.taskpool.ThreadInfo[]. <ctor> : escompat.taskpool.ThreadInfo[]; i32; void;
    auto name = static_cast<const checker::Type *>(arrayType)->ToAssemblerTypeWithRank();
    if (dependencies_->IsNotRequired(name)) {
        return;
    }
    if (Program()->recordTable.find(name) != Program()->recordTable.end()) {
        return;
    }

    auto arrayRecord = pandasm::Record(name, Program()->lang);
    arrayRecord.metadata->SetAttribute(Signatures::EXTERNAL);
    Program()->AddToRecordTable(std::move(arrayRecord));
    Program()->arrayTypes.emplace(PandasmTypeWithRank(this, arrayType));

    std::vector<pandasm::Function::Parameter> params;
    auto singatureName = name + ".<ctor>:" + name + ";";
    params.emplace_back(pandasm::Type(name, 0), EXTENSION);
    for (size_t i = 0; i < arrayType->Rank(); ++i) {
        singatureName += "i32;";
        params.emplace_back(pandasm::Type("i32", 0), EXTENSION);
    }
    singatureName += "void;";

    auto ctor = pandasm::Function(singatureName, EXTENSION);
    ctor.params = std::move(params);
    ctor.returnType = pandasm::Type("void", 0);
    ctor.metadata->SetAttribute(Signatures::CONSTRUCTOR);
    ctor.metadata->SetAttribute(Signatures::EXTERNAL);
    Program()->AddToFunctionTable(std::move(ctor));
}

void ETSEmitter::GenGlobalUnionRecord(util::StringView assemblerType)
{
    std::string name = assemblerType.Mutf8();
    if (dependencies_->IsNotRequired(name)) {
        return;
    }
    if (Program()->recordTable.find(name) != Program()->recordTable.end()) {
        return;
    }
    auto unionRecord = pandasm::Record(name, Program()->lang);
    unionRecord.metadata->SetAttribute(Signatures::EXTERNAL);
    Program()->AddToRecordTable(std::move(unionRecord));
}

void ETSEmitter::GenMethodDefinition(ir::MethodDefinition const *method, bool external)
{
    GenFunction(method->Function(), external);
    for (auto *overload : method->Overloads()) {
        GenFunction(overload->Function(), external);
    }
}

void ETSEmitter::GenInterfaceRecord(const ir::TSInterfaceDeclaration *interfaceDecl, bool external)
{
    if (dependencies_->IsNotRequired(ToAssemblerType(interfaceDecl), external)) {
        return;
    }
    auto interfaceRecord = pandasm::Record(ToAssemblerType(interfaceDecl), Program()->lang);

    interfaceRecord.metadata->SetAccessFlags(ACC_PUBLIC | ACC_ABSTRACT | ACC_INTERFACE);
    interfaceRecord.sourceFile = std::string {Context()->parserProgram->VarBinder()->Program()->RelativeFilePath()};

    for (const auto *prop : interfaceDecl->Body()->Body()) {
        if (prop->IsMethodDefinition()) {
            GenMethodDefinition(prop->AsMethodDefinition(), external);
        }
    }

    if (external) {
        interfaceRecord.metadata->SetAttribute(Signatures::EXTERNAL);
        Program()->AddToRecordTable(std::move(interfaceRecord));
        return;
    }

    interfaceRecord.metadata->SetAttributeValue(Signatures::EXTENDS_ATTRIBUTE, Signatures::BUILTIN_OBJECT);

    for (auto *it : interfaceDecl->TsType()->AsETSObjectType()->Interfaces()) {
        auto *declNode = it->GetDeclNode();
        ES2PANDA_ASSERT(declNode->IsTSInterfaceDeclaration());
        interfaceRecord.metadata->SetAttributeValue(
            Signatures::IMPLEMENTS_ATTRIBUTE, AddDependence(ToAssemblerType(declNode->AsTSInterfaceDeclaration())));
    }

    if (interfaceDecl->HasAnnotations()) {
        interfaceRecord.metadata->SetAnnotations(
            GenCustomAnnotations(interfaceDecl->Annotations(), interfaceRecord.name));
    }
    if (std::any_of(interfaceDecl->Body()->Body().begin(), interfaceDecl->Body()->Body().end(),
                    [](const ir::AstNode *node) { return node->IsOverloadDeclaration(); })) {
        std::vector<pandasm::AnnotationData> annotations {};
        annotations.emplace_back(GenAnnotationFunctionOverload(interfaceDecl->Body()->Body()));
        interfaceRecord.metadata->AddAnnotations(annotations);
    }

    Program()->AddToRecordTable(std::move(interfaceRecord));
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
    if (dependencies_->IsNotRequired(ToAssemblerType(classDef), external)) {
        return;
    }
    auto classRecord = pandasm::Record(ToAssemblerType(classDef), Program()->lang);
    uint32_t accessFlags = GetAccessFlags(classDef);
    classRecord.metadata->SetAccessFlags(accessFlags);
    classRecord.sourceFile = std::string {Context()->parserProgram->VarBinder()->Program()->RelativeFilePath()};
    for (const auto *prop : classDef->Body()) {
        if (prop->IsClassProperty()) {
            GenClassField(prop->AsClassProperty(), classRecord, external);
        } else if (prop->IsMethodDefinition()) {
            GenMethodDefinition(prop->AsMethodDefinition(), external);
        }
    }

    if (external) {
        classRecord.metadata->SetAttribute(Signatures::EXTERNAL);
        Program()->AddToRecordTable(std::move(classRecord));
        return;
    }

    auto const type = classDef->TsType()->AsETSObjectType();
    if (type->SuperType() != nullptr) {
        classRecord.metadata->SetAttributeValue(Signatures::EXTENDS_ATTRIBUTE,
                                                AddDependence(ToAssemblerType(type->SuperType()->GetDeclNode())));
    }

    for (auto *it : type->Interfaces()) {
        classRecord.metadata->SetAttributeValue(
            Signatures::IMPLEMENTS_ATTRIBUTE,
            AddDependence(ToAssemblerType(it->GetDeclNode()->AsTSInterfaceDeclaration())));
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

    Program()->AddToRecordTable(std::move(classRecord));
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

LiteralArrayVector ETSEmitter::CreateLiteralArray(std::string &baseName, [[maybe_unused]] const ir::Expression *array)
{
    LiteralArrayVector result;
    std::vector<pandasm::LiteralArray::Literal> literals;
    ArenaVector<ir::Expression *> elements {array->AsArrayExpression()->Elements()};

    for (const auto *elem : elements) {
        ProcessArrayElement(elem, literals, baseName, result);
    }

    static uint32_t litArrayValueCount = 0;
    std::string litArrayName =
        util::NameMangler::GetInstance()->AppendToAnnotationName(baseName, std::to_string(litArrayValueCount++));
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
    field.type = pandasm::Type(AddDependence(elemType->ToAssemblerType()), rank);

    auto value = prop->Value();
    if (value != nullptr) {
        std::string newBaseName = util::NameMangler::GetInstance()->AppendToAnnotationName(baseName, field.name);
        auto litArray = CreateLiteralArray(newBaseName, value);
        auto const metaValue = litArray.back().first;

        for (auto &item : litArray) {
            Program()->AddToLiteralArrayTable(std::move(item.second), item.first);
        }
        field.metadata->SetValue(pandasm::ScalarValue::Create<pandasm::Value::Type::LITERALARRAY>(metaValue));
    }
}

void ETSEmitter::GenCustomAnnotationProp(const ir::ClassProperty *prop, std::string &baseName, pandasm::Record &record,
                                         bool external)
{
    auto field = pandasm::Field(Program()->lang);
    auto *type = prop->TsType();
    ES2PANDA_ASSERT(prop->Id() != nullptr);
    field.name = prop->Id()->Name().Mutf8();
    field.type = PandasmTypeWithRank(this, type);
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
    if (dependencies_->IsNotRequired(ToAssemblerType(annoDecl), external)) {
        return;
    }
    auto annoRecord = pandasm::Record(ToAssemblerType(annoDecl), Program()->lang);

    if (external) {
        annoRecord.metadata->SetAttribute(Signatures::EXTERNAL);
    }

    uint32_t accessFlags = ACC_PUBLIC | ACC_ABSTRACT | ACC_ANNOTATION;
    annoRecord.metadata->SetAccessFlags(accessFlags);
    annoRecord.sourceFile = std::string {Context()->parserProgram->VarBinder()->Program()->RelativeFilePath()};
    for (auto *it : annoDecl->Properties()) {
        GenCustomAnnotationProp(it->AsClassProperty(), baseName, annoRecord, external);
    }

    Program()->AddToRecordTable(std::move(annoRecord));
}

pandasm::AnnotationElement ETSEmitter::ProcessArrayType(const ir::ClassProperty *prop, std::string &baseName,
                                                        const ir::Expression *init)
{
    ES2PANDA_ASSERT(prop->Id() != nullptr);
    auto propName = prop->Id()->Name().Mutf8();
    std::string newBaseName = util::NameMangler::GetInstance()->AppendToAnnotationName(baseName, propName);
    auto litArrays = CreateLiteralArray(newBaseName, init);
    auto const value = litArrays.back().first;

    for (auto &item : litArrays) {
        Program()->AddToLiteralArrayTable(std::move(item.second), item.first);
    }

    return pandasm::AnnotationElement {
        propName,
        std::make_unique<pandasm::ScalarValue>(pandasm::ScalarValue::Create<pandasm::Value::Type::STRING>(value))};
}

static pandasm::AnnotationElement ChooseETSEnumType(const ir::Expression *initValue, const checker::Type *type,
                                                    std::string propName)
{
    auto *enumRef = type->AsETSNumericEnumType();
    auto *checker = public_lib::Context().GetChecker()->AsETSChecker();
    std::unique_ptr<pandasm::ScalarValue> numericEnumValue;
    if (enumRef->CheckBuiltInType(checker, checker::ETSObjectFlags::BUILTIN_BYTE)) {
        auto enumValue = static_cast<uint8_t>(initValue->AsNumberLiteral()->Number().GetByte());
        numericEnumValue =
            std::make_unique<pandasm::ScalarValue>(pandasm::ScalarValue::Create<pandasm::Value::Type::I8>(enumValue));
    } else if (enumRef->CheckBuiltInType(checker, checker::ETSObjectFlags::BUILTIN_SHORT)) {
        auto enumValue = static_cast<uint16_t>(initValue->AsNumberLiteral()->Number().GetShort());
        numericEnumValue =
            std::make_unique<pandasm::ScalarValue>(pandasm::ScalarValue::Create<pandasm::Value::Type::I16>(enumValue));
    } else if (enumRef->CheckBuiltInType(checker, checker::ETSObjectFlags::BUILTIN_INT)) {
        auto enumValue = static_cast<uint32_t>(initValue->AsNumberLiteral()->Number().GetInt());
        numericEnumValue =
            std::make_unique<pandasm::ScalarValue>(pandasm::ScalarValue::Create<pandasm::Value::Type::I32>(enumValue));
    } else if (enumRef->CheckBuiltInType(checker, checker::ETSObjectFlags::BUILTIN_FLOAT)) {
        auto enumValue = initValue->AsNumberLiteral()->Number().GetFloat();
        numericEnumValue =
            std::make_unique<pandasm::ScalarValue>(pandasm::ScalarValue::Create<pandasm::Value::Type::F32>(enumValue));
    } else if (enumRef->CheckBuiltInType(checker, checker::ETSObjectFlags::BUILTIN_LONG)) {
        auto enumValue = static_cast<uint64_t>(initValue->AsNumberLiteral()->Number().GetLong());
        numericEnumValue =
            std::make_unique<pandasm::ScalarValue>(pandasm::ScalarValue::Create<pandasm::Value::Type::I64>(enumValue));
    } else if (enumRef->CheckBuiltInType(checker, checker::ETSObjectFlags::BUILTIN_DOUBLE)) {
        auto enumValue = initValue->AsNumberLiteral()->Number().GetDouble();
        numericEnumValue =
            std::make_unique<pandasm::ScalarValue>(pandasm::ScalarValue::Create<pandasm::Value::Type::F64>(enumValue));
    }

    return pandasm::AnnotationElement {propName, std::move(numericEnumValue)};
}

static pandasm::AnnotationElement ProcessETSEnumType(const ir::ClassProperty *prop, const ir::Expression *init,
                                                     const checker::Type *type)
{
    ES2PANDA_ASSERT(init->AsMemberExpression()->PropVar() != nullptr);
    auto propName = prop->Id()->Name().Mutf8();
    auto declNode = init->AsMemberExpression()->PropVar()->Declaration()->Node();
    auto *initValue = declNode->AsClassProperty()->OriginEnumMember()->Init();
    if (type->IsETSNumericEnumType()) {
        if (type->AsETSNumericEnumType()->EnumAnnotedType() != nullptr) {
            return ChooseETSEnumType(initValue, type, propName);
        }
        if (type->AsETSNumericEnumType()->NonAnnotedHasDouble()) {
            auto enumValue = initValue->AsNumberLiteral()->Number().GetDouble();
            auto doubleEnumValue = pandasm::ScalarValue::Create<pandasm::Value::Type::F64>(enumValue);
            return pandasm::AnnotationElement {propName, std::make_unique<pandasm::ScalarValue>(doubleEnumValue)};
        } else {
            auto enumValue = static_cast<uint32_t>(initValue->AsNumberLiteral()->Number().GetInt());
            auto intEnumValue = pandasm::ScalarValue::Create<pandasm::Value::Type::I32>(enumValue);
            return pandasm::AnnotationElement {propName, std::make_unique<pandasm::ScalarValue>(intEnumValue)};
        }
    }
    ES2PANDA_ASSERT(type->IsETSStringEnumType());
    auto enumValue = initValue->AsStringLiteral()->Str().Mutf8();
    auto stringValue = pandasm::ScalarValue::Create<pandasm::Value::Type::STRING>(enumValue);
    return pandasm::AnnotationElement {propName, std::make_unique<pandasm::ScalarValue>(stringValue)};
}

pandasm::AnnotationElement ETSEmitter::GenCustomAnnotationElement(const ir::ClassProperty *prop, std::string &baseName)
{
    const auto *init = prop->Value();
    const auto *type = init->TsType();
    if (type->IsETSArrayType() || type->IsETSResizableArrayType()) {
        return ProcessArrayType(prop, baseName, init);
    }
    if (type->IsETSEnumType()) {
        return ProcessETSEnumType(prop, init, type);
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
    pandasm::AnnotationData annotation(AddDependence(ToAssemblerType(annoDecl)));
    if (annoDecl->IsImportDeclaration()) {
        auto annoRecord = pandasm::Record(ToAssemblerType(annoDecl), Program()->lang);
        annoRecord.metadata->SetAttribute(Signatures::EXTERNAL);
        uint32_t accessFlags = ACC_PUBLIC | ACC_ABSTRACT | ACC_ANNOTATION;
        annoRecord.metadata->SetAccessFlags(accessFlags);
        Program()->AddToRecordTable(std::move(annoRecord));
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
        if (cls->IsDeclare()) {  // #28197
            continue;            // AST inconsistency!
        }
        exportedClasses.emplace_back(pandasm::ScalarValue::Create<pandasm::Value::Type::RECORD>(
            pandasm::Type::FromName(AddDependence(ToAssemblerType(cls->Definition())), true)));
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
        std::make_unique<pandasm::ScalarValue>(
            pandasm::ScalarValue::Create<pandasm::Value::Type::METHOD>(ToAssemblerSignature(impl->Function()))));
    ann.AddElement(std::move(value));
    return ann;
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
        Program()->AddToRecordTable(std::move(record));
    }
}
}  // namespace ark::es2panda::compiler

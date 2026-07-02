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

#include "ETSBinder.h"

#include "es2panda.h"
#include "evaluate/scopedDebugInfoPlugin.h"
#include "public/public.h"
#include "compiler/lowering/util.h"
#include "util/helpers.h"
#include "util/nameMangler.h"
#include "varbinder/declaration.h"

namespace ark::es2panda::varbinder {

void ETSBinder::IdentifierAnalysis()
{
    ES2PANDA_ASSERT(Program()->Ast());
    ES2PANDA_ASSERT(GetScope() == TopScope());
    ES2PANDA_ASSERT(VarScope() == TopScope());

    recordTable_->SetProgram(Program());
    globalRecordTable_->SetClassDefinition(Program()->GlobalClass());

    BuildProgram();

    ES2PANDA_ASSERT(globalRecordTable_->ClassDefinition() == Program()->GlobalClass());
}

void ETSBinder::LookupTypeArgumentReferences(ir::ETSTypeReference *typeRef)
{
    auto *iter = typeRef->Part();
    if (typeRef->HasAnnotations()) {
        for (auto *anno : typeRef->Annotations()) {
            ResolveReference(anno);
        }
    }

    while (iter != nullptr) {
        if (iter->TypeParams() == nullptr) {
            iter = iter->Previous();
            continue;
        }

        ResolveReferences(iter->TypeParams());
        iter = iter->Previous();
    }
}

bool ETSBinder::IsSpecialName(const util::StringView &name)
{
    constexpr std::array SPECIAL_KEYWORDS = {compiler::Signatures::ANY_TYPE_NAME, compiler::Signatures::ANY,
                                             compiler::Signatures::UNDEFINED, compiler::Signatures::NULL_LITERAL};

    constexpr std::array UTILITY_TYPES = {
        compiler::Signatures::READONLY_TYPE_NAME,    compiler::Signatures::PARTIAL_TYPE_NAME,
        compiler::Signatures::REQUIRED_TYPE_NAME,    compiler::Signatures::FIXED_ARRAY_TYPE_NAME,
        compiler::Signatures::VALUE_ARRAY_TYPE_NAME, compiler::Signatures::AWAITED_TYPE_NAME,
        compiler::Signatures::RETURN_TYPE_TYPE_NAME};

    return std::find(SPECIAL_KEYWORDS.begin(), SPECIAL_KEYWORDS.end(), name.Utf8()) != SPECIAL_KEYWORDS.end() ||
           std::find(UTILITY_TYPES.begin(), UTILITY_TYPES.end(), name.Utf8()) != UTILITY_TYPES.end();
}

static util::StringView NormalizeReExportName(util::StringView name)
{
    return name.Is(compiler::Signatures::REEXPORT_DEFAULT_ANONYMOUSLY) ? util::StringView {"default"} : name;
}

static bool MatchesName(const ir::Identifier *id, util::StringView localName)
{
    return id != nullptr && (localName.Empty() || id->Name() == localName);
}

static const ir::Identifier *VariableDeclarationIdentifier(const ir::VariableDeclaration *declaration,
                                                           util::StringView localName)
{
    if (declaration == nullptr || localName.Empty()) {
        return nullptr;
    }
    auto *declarator = declaration->GetDeclaratorByName(localName);
    if (declarator == nullptr || !declarator->Id()->IsIdentifier()) {
        return nullptr;
    }
    return declarator->Id()->AsIdentifier();
}

static const ir::Identifier *TypeDeclarationIdentifier(const ir::AstNode *origin, util::StringView localName)
{
    if (origin->IsTSTypeAliasDeclaration()) {
        auto *ident = origin->AsTSTypeAliasDeclaration()->Id();
        return MatchesName(ident, localName) ? ident : nullptr;
    }

    if (origin->IsTSInterfaceDeclaration()) {
        auto *ident = origin->AsTSInterfaceDeclaration()->Id();
        return MatchesName(ident, localName) ? ident : nullptr;
    }

    if (origin->IsTSEnumDeclaration()) {
        auto *ident = origin->AsTSEnumDeclaration()->Key();
        return MatchesName(ident, localName) ? ident : nullptr;
    }

    return nullptr;
}

static const ir::Identifier *OtherDeclarationIdentifier(const ir::AstNode *origin, util::StringView localName)
{
    if (origin->IsAnnotationDeclaration()) {
        auto *ident = origin->AsAnnotationDeclaration()->GetBaseName();
        return MatchesName(ident, localName) ? ident : nullptr;
    }

    if (origin->IsETSModule()) {
        auto *ident = origin->AsETSModule()->Ident();
        return MatchesName(ident, localName) ? ident : nullptr;
    }

    return nullptr;
}

static const ir::Identifier *DeclarationIdentifier(const ir::AstNode *origin, util::StringView localName)
{
    if (origin == nullptr) {
        return nullptr;
    }

    if (origin->IsClassDeclaration()) {
        auto *definition = origin->AsClassDeclaration()->Definition();
        auto *ident = definition == nullptr ? nullptr : definition->Ident();
        return MatchesName(ident, localName) ? ident : nullptr;
    }

    if (origin->IsClassDefinition()) {
        auto *ident = origin->AsClassDefinition()->Ident();
        return MatchesName(ident, localName) ? ident : nullptr;
    }

    if (origin->IsVariableDeclaration()) {
        return VariableDeclarationIdentifier(origin->AsVariableDeclaration(), localName);
    }

    if (origin->IsFunctionDeclaration()) {
        auto *function = origin->AsFunctionDeclaration()->Function();
        auto *ident = function == nullptr ? nullptr : function->Id();
        return MatchesName(ident, localName) ? ident : nullptr;
    }

    if (origin->IsScriptFunction()) {
        auto *ident = origin->AsScriptFunction()->Id();
        return MatchesName(ident, localName) ? ident : nullptr;
    }

    if (auto *ident = TypeDeclarationIdentifier(origin, localName); ident != nullptr) {
        return ident;
    }

    return OtherDeclarationIdentifier(origin, localName);
}

static bool OriginDeclaresName(const ir::AstNode *origin, util::StringView localName)
{
    return DeclarationIdentifier(origin, localName) != nullptr;
}

static Variable *ResolveExportVariable(const ir::AstNode *origin, util::StringView localName = util::StringView())
{
    if (origin == nullptr) {
        return nullptr;
    }

    if (origin->IsImportSpecifier()) {
        auto *specifier = origin->AsImportSpecifier();
        if (specifier->Local() != nullptr && specifier->Local()->Variable() != nullptr) {
            return specifier->Local()->Variable();
        }

        if (specifier->Imported() != nullptr) {
            return specifier->Imported()->Variable();
        }

        return nullptr;
    }

    if (origin->IsImportNamespaceSpecifier()) {
        auto *specifier = origin->AsImportNamespaceSpecifier();
        return specifier->Local() == nullptr ? nullptr : specifier->Local()->Variable();
    }

    if (auto *ident = DeclarationIdentifier(origin, localName); ident != nullptr) {
        return ident->Variable();
    }

    if (origin->Variable() != nullptr) {
        return origin->Variable();
    }

    return nullptr;
}

static varbinder::Variable *FindProgramLocalVariable(parser::Program *program, util::StringView localName,
                                                     const varbinder::Scope::VariableMap &bindings)
{
    auto iter = bindings.find(localName);
    if (iter == bindings.end() || iter->second == nullptr || !iter->second->IsLocalVariable() ||
        iter->second->Declaration() == nullptr || iter->second->Declaration()->Node() == nullptr) {
        return nullptr;
    }

    return iter->second->Declaration()->Node()->Program() == program ? iter->second : nullptr;
}

static varbinder::Variable *FindProgramLocalVariable(parser::Program *program, util::StringView localName)
{
    if (program == nullptr || program->GlobalScope() == nullptr || program->GlobalClassScope() == nullptr) {
        return nullptr;
    }

    if (auto *variable =
            FindProgramLocalVariable(program, localName, program->GlobalClassScope()->StaticDeclScope()->Bindings());
        variable != nullptr) {
        return variable;
    }
    if (auto *variable =
            FindProgramLocalVariable(program, localName, program->GlobalClassScope()->StaticMethodScope()->Bindings());
        variable != nullptr) {
        return variable;
    }
    if (auto *variable = FindProgramLocalVariable(program, localName, program->GlobalScope()->Bindings());
        variable != nullptr) {
        return variable;
    }
    if (auto *variable =
            FindProgramLocalVariable(program, localName, program->GlobalClassScope()->StaticFieldScope()->Bindings());
        variable != nullptr) {
        return variable;
    }
    if (auto *variable =
            FindProgramLocalVariable(program, localName, program->GlobalClassScope()->TypeAliasScope()->Bindings());
        variable != nullptr) {
        return variable;
    }
    return nullptr;
}

static parser::Program *FindOwningProgram(const ir::AstNode *node)
{
    while (node != nullptr) {
        if (node->IsETSModule()) {
            return const_cast<parser::Program *>(node->AsETSModule()->Program());
        }
        node = node->Parent();
    }
    return nullptr;
}

static bool ShouldCollectLocalExportFact(parser::Program *program, varbinder::Variable *variable)
{
    if (variable == nullptr || !variable->IsLocalVariable() || variable->Declaration() == nullptr ||
        variable->Declaration()->Node() == nullptr) {
        return false;
    }

    const auto *node = variable->Declaration()->Node();
    if (node->Program() != program) {
        return false;
    }

    if (node->IsImportSpecifier() || node->IsImportNamespaceSpecifier() || node->IsImportDefaultSpecifier()) {
        return false;
    }

    return node->IsExported() || node->IsDefaultExported();
}

static void CollectLocalExportFacts(ExportFactStore *store, parser::Program *program,
                                    const varbinder::Scope::VariableMap &bindings)
{
    for (const auto &[name, variable] : bindings) {
        if (!ShouldCollectLocalExportFact(program, variable)) {
            continue;
        }

        const auto *node = variable->Declaration()->Node();
        if (node->IsExported()) {
            store->AddLocalExport(program, name, variable, node);
        }
        if (node->IsDefaultExported()) {
            store->AddLocalExport(program, util::StringView {"default"}, variable, node);
        }
    }
}

static void CollectDeclareNamespaceExportFacts(ExportFactStore *store, parser::Program *program)
{
    if (program == nullptr || program->Ast() == nullptr) {
        return;
    }

    for (auto *stmt : program->Ast()->Statements()) {
        if (!stmt->IsClassDeclaration()) {
            continue;
        }

        auto *classDef = stmt->AsClassDeclaration()->Definition();
        if (classDef == nullptr || !classDef->IsNamespaceTransformed() || !classDef->IsDeclare() ||
            (!stmt->IsExported() && !classDef->IsExported())) {
            continue;
        }

        auto *ident = classDef->Ident();
        if (ident == nullptr) {
            continue;
        }

        auto *variable = ident->Variable();
        if (variable == nullptr) {
            variable = FindProgramLocalVariable(program, ident->Name());
        }
        if (ShouldCollectLocalExportFact(program, variable)) {
            continue;
        }
        store->AddLocalExport(program, ident->Name(), variable, stmt);
    }
}

static void CollectPendingLocalExportAliases(ExportFactStore *store, parser::Program *program)
{
    for (const auto &alias : store->PendingLocalExportAliases(program)) {
        auto *variable = ResolveExportVariable(alias.origin, alias.localName);
        variable = variable != nullptr ? variable : FindProgramLocalVariable(program, alias.localName);
        if (variable != nullptr && variable->Declaration() != nullptr && variable->Declaration()->Node() != nullptr &&
            variable->Declaration()->Node()->IsTSEnumDeclaration()) {
            if (auto *currentVariable = FindProgramLocalVariable(program, alias.localName);
                currentVariable != nullptr) {
                variable = currentVariable;
            }
        }
        if (variable == nullptr) {
            continue;
        }
        if (alias.exportedName.Is("default") && variable->Declaration() != nullptr &&
            variable->Declaration()->Node() != nullptr) {
            auto *node = variable->Declaration()->Node();
            auto *modifierNode = node->IsClassDefinition() && node->Parent() != nullptr ? node->Parent() : node;
            modifierNode->AddModifier(ir::ModifierFlags::DEFAULT_EXPORT);
        }
        if (!alias.isTypeOnly && alias.exportedName == alias.localName &&
            ShouldCollectLocalExportFact(program, variable)) {
            continue;
        }
        if (alias.kind == LocalExportKind::DECLARATION) {
            store->AddLocalExport(program, alias.exportedName, alias.localName, variable, alias.origin);
            continue;
        }
        store->AddLocalExportAlias(program, alias.exportedName, alias.localName, variable, alias.origin,
                                   alias.isTypeOnly, alias.isInvalid);
    }
}

static void AddReExportFact(ExportFactStore *store, parser::Program *program, const ir::ETSImportDeclaration *import,
                            ir::AstNode *specifier)
{
    if (specifier->IsImportSpecifier()) {
        auto *importSpecifier = specifier->AsImportSpecifier();
        auto exportedName = NormalizeReExportName(importSpecifier->Local()->Name());
        auto importedName = NormalizeReExportName(importSpecifier->Imported()->Name());
        if (exportedName.Is("default") && !importedName.Is("default")) {
            return;
        }
        store->AddNamedReExport(program, import, exportedName, importedName, importSpecifier, import->IsTypeKind());
        return;
    }

    if (!specifier->IsImportNamespaceSpecifier()) {
        return;
    }

    auto *namespaceSpecifier = specifier->AsImportNamespaceSpecifier();
    if (namespaceSpecifier->Local() == nullptr || namespaceSpecifier->Local()->Name().Empty()) {
        store->AddStarExport(program, import, namespaceSpecifier, import->IsTypeKind());
        return;
    }

    store->AddNamespaceExport(program, import, namespaceSpecifier->Local()->Name(),
                              ResolveExportVariable(namespaceSpecifier), namespaceSpecifier, import->IsTypeKind());
}

void ETSBinder::CollectExportFactsForCurrentProgram()
{
    auto *program = Program();
    exportFactStore_->ResetProgram(program);

    if (program == nullptr || program->GlobalScope() == nullptr) {
        return;
    }

    exportFactStore_->RegisterProgramSurface(program);
    exportFactStore_->RegisterPackageSurface(program);

    CollectLocalExportFacts(exportFactStore_, program, program->GlobalScope()->Bindings());
    CollectLocalExportFacts(exportFactStore_, program, program->GlobalClassScope()->StaticMethodScope()->Bindings());
    CollectLocalExportFacts(exportFactStore_, program, program->GlobalClassScope()->StaticFieldScope()->Bindings());
    CollectLocalExportFacts(exportFactStore_, program, program->GlobalClassScope()->StaticDeclScope()->Bindings());
    CollectDeclareNamespaceExportFacts(exportFactStore_, program);
    CollectPendingLocalExportAliases(exportFactStore_, program);

    auto reExportsIt = ReExportImports().find(program);
    if (reExportsIt == ReExportImports().end()) {
        return;
    }

    for (auto *reExport : reExportsIt->second) {
        const auto *import = reExport->GetETSImportDeclarations();
        RegisterImportTarget(import);

        for (auto *specifier : import->Specifiers()) {
            AddReExportFact(exportFactStore_, program, import, specifier);
        }
    }
}

static bool IsAnyOrUnknown(ETSBinder *binder, const util::StringView &name, const lexer::SourcePosition &pos)
{
    if (name.Is("any") || name.Is("unknown")) {
        binder->ThrowError(pos, diagnostic::ANY_UNKNOWN_TYPES);
        return true;
    }
    return false;
}

bool ETSBinder::LookupInDebugInfoPlugin(ir::Identifier *ident)
{
    auto *checker = GetContext()->GetChecker()->AsETSChecker();
    auto *debugInfoPlugin = checker->GetDebugInfoPlugin();
    if (UNLIKELY(debugInfoPlugin)) {
        auto *var = debugInfoPlugin->FindClass(ident);
        if (var != nullptr) {
            ident->SetVariable(var);
            return true;
        }
    }
    // NOTE: search an imported module's name in case of 'import "file" as xxx'.
    return false;
}

//  Auxiliary method extracted from LookupTypeReference(...) to avoid too large size
static void CreateDummyVariable(ETSBinder *varBinder, ir::Identifier *ident)
{
    auto expressionCtx = varbinder::LexicalScope<varbinder::Scope>::Enter(varBinder, varBinder->VarScope());
    auto [decl, var] =
        varBinder->NewVarDecl<varbinder::LetDecl>(ident->Start(), compiler::GenName(varBinder->Allocator()).View());
    var->SetScope(varBinder->GetScope());
    ident->SetVariable(var);
    ident->SetTsType(var->SetTsType(varBinder->GetContext()->GetChecker()->AsETSChecker()->GlobalTypeError()));
    decl->BindNode(ident);
}

static bool IsInStaticMember(ir::AstNode *node)
{
    if (node == nullptr || node->IsClassDefinition()) {
        return false;
    }
    if (node->Parent() != nullptr && node->Parent()->IsClassDefinition()) {
        return ((node->Modifiers() & ir::ModifierFlags::STATIC) != 0);
    }
    return IsInStaticMember(node->Parent());
}

static void CheckAndSetVariableReference(ETSBinder *varBinder, ir::Identifier *ident,
                                         ark::es2panda::varbinder::Variable *resVar)
{
    bool isIdentInStaticMethod = IsInStaticMember(ident);
    bool isVarInStaticMethod = IsInStaticMember(resVar->Declaration()->Node());
    if (isIdentInStaticMethod && !isVarInStaticMethod) {
        varBinder->ThrowError(ident->Start(), diagnostic::STATIC_METHOD_CANNOT_REFERENCE_CLASS_TYPE, {ident->Name()});
    } else {
        ident->SetVariable(resVar);
    }
}

static bool TryBindTypeReferenceVariable(ETSBinder *varBinder, ir::Identifier *ident, varbinder::Variable *var)
{
    if (var->IsLocalVariable() && var->HasFlag(varbinder::VariableFlags::IMPORT_BINDING)) {
        ident->SetVariable(var);
        return true;
    }

    switch (var->Declaration()->Node()->Type()) {
        case ir::AstNodeType::CLASS_DECLARATION:
        case ir::AstNodeType::CLASS_DEFINITION:
        case ir::AstNodeType::STRUCT_DECLARATION:
        case ir::AstNodeType::TS_ENUM_DECLARATION:
        case ir::AstNodeType::TS_INTERFACE_DECLARATION:
        case ir::AstNodeType::TS_TYPE_ALIAS_DECLARATION:
        case ir::AstNodeType::ANNOTATION_DECLARATION:
        case ir::AstNodeType::IMPORT_NAMESPACE_SPECIFIER: {
            ident->SetVariable(var);
            return true;
        }
        case ir::AstNodeType::TS_TYPE_PARAMETER: {
            CheckAndSetVariableReference(varBinder, ident, var);
            return true;
        }
        default:
            return false;
    }
}

void ETSBinder::LookupTypeReference(ir::Identifier *ident)
{
    ES2PANDA_ASSERT(ident != nullptr);
    if (ident->Variable() != nullptr && ident->Variable()->Declaration()->Node() == ident) {
        return;
    }

    auto const &name = ident->Name();
    if (IsSpecialName(name)) {
        return;
    }

    if (ident->IsErrorPlaceHolder() || IsAnyOrUnknown(this, name, ident->Start())) {
        CreateDummyVariable(this, ident);
        return;
    }

    auto *scope = GetScope();
    while (scope != nullptr) {
        auto options = ResolveBindingOptions::DECLARATION | ResolveBindingOptions::TYPE_ALIASES |
                       ResolveBindingOptions::STATIC_DECLARATION;
        auto res = scope->Find(name, options);
        if (res.variable == nullptr) {
            break;
        }

        if (TryBindTypeReferenceVariable(this, ident, res.variable)) {
            return;
        }
        scope = scope->Parent();
    }

    if (LookupInDebugInfoPlugin(ident)) {
        return;
    }

    ThrowUnresolvableType(ident->Start(), name);
    CreateDummyVariable(this, ident);
}

void ETSBinder::ResolveReferencesForScope(ir::AstNode const *const parent, Scope *const scope)
{
    parent->Iterate([this, scope](auto *node) { ResolveReferenceForScope(node, scope); });
}

void ETSBinder::ResolveReferenceForScope(ir::AstNode *const node, Scope *const scope)
{
    switch (node->Type()) {
        case ir::AstNodeType::IDENTIFIER: {
            auto *ident = node->AsIdentifier();
            if (ident->Variable() != nullptr) {
                break;
            }
            if (auto const res = scope->Find(ident->Name(), ResolveBindingOptions::ALL); res.variable != nullptr) {
                ident->SetVariable(res.variable);
            }
            break;
        }
        case ir::AstNodeType::VARIABLE_DECLARATOR: {
            auto scopeCtx = LexicalScope<Scope>::Enter(this, scope);
            BuildVarDeclarator(node->AsVariableDeclarator());
            break;
        }
        /* Maybe will be used
        case ir::AstNodeType::BLOCK_STATEMENT: {
            auto scope_ctx = LexicalScope<Scope>::Enter(this, node->AsBlockStatement()->Scope());
            ResolveReferences(node);
            break;
        }
        */
        case ir::AstNodeType::BLOCK_EXPRESSION: {
            auto scopeCtx = LexicalScope<Scope>::Enter(this, node->AsBlockExpression()->Scope());
            ResolveReferences(node);
            break;
        }
        case ir::AstNodeType::SCRIPT_FUNCTION: {
            ResolveReferencesForScope(node, node->AsScriptFunction()->Scope());
            break;
        }
        default: {
            ResolveReferencesForScope(node, scope);
            break;
        }
    }
}

void ETSBinder::ResolveReferencesForScopeWithContext(ir::AstNode *node, Scope *scope)
{
    auto lexScope = LexicalScope<Scope>::Enter(this, scope);
    ResolveReference(node);
}

bool ETSBinder::AddSelectiveExportAlias(const SelectiveExportAlias &alias) noexcept
{
    ES2PANDA_ASSERT(alias.localIdent != nullptr);
    const auto localName = alias.localIdent->Name();
    const auto normalizedExportedName = NormalizeReExportName(alias.exportedName);
    const auto originDeclaresName = OriginDeclaresName(alias.decl, localName);
    const auto kind = originDeclaresName && !alias.isTypeOnly && normalizedExportedName == localName &&
                              !normalizedExportedName.Is("default")
                          ? LocalExportKind::DECLARATION
                          : LocalExportKind::ALIAS;
    return exportFactStore_->AddPendingLocalExportAlias(alias.program, normalizedExportedName, localName, alias.decl,
                                                        alias.exportDecl, alias.reportOrigin, originDeclaresName,
                                                        alias.isTypeOnly, kind);
}

void ETSBinder::LookupIdentReference(ir::Identifier *ident)
{
    if (ident->IsErrorPlaceHolder()) {
        return;
    }

    const auto &name = ident->Name();
    auto res = GetScope()->Find(name, ResolveBindingOptions::ALL);
    if (res.level != 0) {
        ES2PANDA_ASSERT(res.variable != nullptr);

        ES2PANDA_ASSERT(GetScope()->EnclosingVariableScope() != nullptr);
        auto *outerFunction = GetScope()->EnclosingVariableScope()->Node();

        if ((!outerFunction->IsScriptFunction() || !outerFunction->AsScriptFunction()->IsArrow()) &&
            !res.variable->IsGlobalVariable() && res.variable->HasFlag(VariableFlags::LOCAL) && res.level > 1) {
            ThrowInvalidCapture(ident->Start(), name);
        }
    }

    if (res.variable == nullptr) {
        return;
    }

    if (ident->IsReference(Extension()) && res.variable->Declaration()->IsLetOrConstDecl() &&
        !res.variable->HasFlag(VariableFlags::INITIALIZED) &&
        !res.variable->HasFlag(VariableFlags::INIT_IN_STATIC_BLOCK)) {
        ThrowTDZ(ident->Start(), name);
    }
}

void ETSBinder::BuildClassProperty(const ir::ClassProperty *prop)
{
    ResolveReferences(prop);
}

void ETSBinder::BuildETSTypeReference(ir::ETSTypeReference *typeRef)
{
    auto *baseName = typeRef->BaseName();
    ES2PANDA_ASSERT(baseName->IsReference(Extension()));

    // We allow to resolve following types in pure dynamic mode:
    // import * as I from "@dynamic"
    // let x : I.X.Y
    LookupTypeReference(baseName);
    LookupTypeArgumentReferences(typeRef);
}

void ETSBinder::BuildObjectExpression(ir::ObjectExpression *obj)
{
    // NOTE: when we try to resolve references for Object Expression
    // we visit properties, example:
    // class C { x : boolean }
    // let x: C = { x: true }
    //
    // However we visit Object Expression with _outer_ scope, not class scope.
    // That means that varbinder will try to resolve `x` as `x` from outer scope, _not from the class scope_.
    // The following code will skip resolving LHS of the property.
    // We can do it because currently LHS is still checked in the `ETSAnalyzer::CheckObjectExprProps` function.
    for (auto expr : obj->Properties()) {
        if (expr->IsProperty()) {
            auto *prop = expr->AsProperty();
            ResolveReference(prop->Value());
        }
    }

    if (obj->TypeAnnotation() != nullptr) {
        ResolveReference(obj->TypeAnnotation());
    }
}

void ETSBinder::InitializeInterfaceIdent(ir::TSInterfaceDeclaration *decl)
{
    auto res = GetScope()->Find(decl->Id()->Name());

    ES2PANDA_ASSERT(res.variable && res.variable->Declaration()->IsInterfaceDecl());
    res.variable->AddFlag(VariableFlags::INITIALIZED);
    decl->Id()->SetVariable(res.variable);
}

void ETSBinder::ResolveEnumDeclaration(ir::TSEnumDeclaration *enumDecl)
{
    auto enumScopeCtx = LexicalScope<LocalScope>::Enter(this, enumDecl->Scope());

    for (auto *member : enumDecl->Members()) {
        ResolveReference(member);
    }
}

void ETSBinder::ResolveInterfaceDeclaration(ir::TSInterfaceDeclaration *decl)
{
    auto boundCtx = BoundContext(recordTable_, decl);

    for (auto *extend : decl->Extends()) {
        ResolveReference(extend);
    }

    if (decl->HasAnnotations()) {
        for (auto *anno : decl->Annotations()) {
            ResolveReference(anno);
        }
    }

    auto scopeCtx = LexicalScope<ClassScope>::Enter(this, decl->Scope()->AsClassScope());

    for (auto *stmt : decl->Body()->Body()) {
        if (!stmt->IsClassProperty()) {
            continue;
        }

        ResolveReference(stmt);

        ES2PANDA_ASSERT(stmt->AsClassProperty()->Id() != nullptr);
        auto fieldVar =
            ResolvePropertyReference(stmt->AsClassProperty(), decl->Scope()->AsClassScope())
                ->FindLocal(stmt->AsClassProperty()->Id()->Name(), varbinder::ResolveBindingOptions::BINDINGS);
        ES2PANDA_ASSERT(fieldVar != nullptr);
        fieldVar->AddFlag(VariableFlags::INITIALIZED);
    }

    for (auto *stmt : decl->Body()->Body()) {
        if (stmt->IsClassProperty()) {
            continue;
        }
        ResolveReference(stmt);
    }
}

void ETSBinder::BuildInterfaceDeclaration(ir::TSInterfaceDeclaration *decl)
{
    if (decl->TypeParams() != nullptr) {
        auto typeParamScopeCtx = LexicalScope<LocalScope>::Enter(this, decl->TypeParams()->Scope());
        ResolveReferences(decl->TypeParams());
        ResolveInterfaceDeclaration(decl);
        return;
    }

    ResolveInterfaceDeclaration(decl);
}

void ETSBinder::BuildMethodDefinition(ir::MethodDefinition *methodDef)
{
    if (methodDef->BaseOverloadMethod() != nullptr &&
        methodDef->GetTopStatement()->AsETSModule()->Program() != Program() &&
        methodDef->BaseOverloadMethod()->GetTopStatement() != methodDef->GetTopStatement()) {
        return;
    }
    ES2PANDA_ASSERT(methodDef->Function() != nullptr);
    if (methodDef->Function()->TypeParams() != nullptr) {
        auto scopeCtx = LexicalScope<LocalScope>::Enter(this, methodDef->Function()->TypeParams()->Scope());
        ResolveReferences(methodDef->Function()->TypeParams());
    }
    ResolveMethodDefinition(methodDef);
}

void ETSBinder::BuildAnnotationDeclaration(ir::AnnotationDeclaration *annoDecl)
{
    auto boundCtx = BoundContext(recordTable_, annoDecl);
    if (annoDecl->Expr()->IsIdentifier()) {
        LookupTypeReference(annoDecl->AsAnnotationDeclaration()->Expr()->AsIdentifier());
    } else {
        ResolveReference(annoDecl->Expr());
    }
    auto scopeCtx = LexicalScope<LocalScope>::Enter(this, annoDecl->Scope());
    for (auto *property : annoDecl->Properties()) {
        ResolveReference(property);
    }
    if (annoDecl->HasAnnotations()) {
        for (auto *anno : annoDecl->Annotations()) {
            ResolveReference(anno);
        }
    }
}

void ETSBinder::BuildAnnotationUsage(ir::AnnotationUsage *annoUsage)
{
    if (annoUsage->Expr()->IsIdentifier()) {
        LookupTypeReference(annoUsage->AsAnnotationUsage()->Expr()->AsIdentifier());
    } else {
        ResolveReference(annoUsage->Expr());
    }

    for (auto *property : annoUsage->Properties()) {
        ResolveReference(property);
    }
}

void ETSBinder::ResolveMethodDefinition(ir::MethodDefinition *methodDef)
{
    methodDef->ResolveReferences([this](auto *childNode) { ResolveReference(childNode); });

    auto *func = methodDef->Function();
    ES2PANDA_ASSERT(func != nullptr);
    if (func->HasAnnotations()) {
        for (auto *anno : func->Annotations()) {
            ResolveReference(anno);
        }
    }
    if (methodDef->IsStatic() || func->IsStaticBlock()) {
        return;
    }

    auto paramScopeCtx = LexicalScope<FunctionParamScope>::Enter(this, func->Scope()->ParamScope());

    auto params = func->Scope()->ParamScope()->Params();
    if (!params.empty() && params.front()->Name() == MANDATORY_PARAM_THIS && !func->HasReceiver()) {
        return;  // Implicit this parameter is already inserted by ResolveReferences(), don't insert it twice.
    }

    auto *thisParam = AddMandatoryParam(MANDATORY_PARAM_THIS);
    ES2PANDA_ASSERT(thisParam != nullptr);
    thisParam->Declaration()->BindNode(thisParam_);
}

void ETSBinder::BuildOverloadDeclaration(ir::OverloadDeclaration *overloadDef)
{
    overloadDef->ResolveReferences([this](auto *childNode) { ResolveReference(childNode); });
}

void ETSBinder::BuildMemberExpression(ir::MemberExpression *memberExpr)
{
    ResolveReference(memberExpr->Object());

    if (memberExpr->Kind() == ir::MemberExpressionKind::ELEMENT_ACCESS) {
        ResolveReference(memberExpr->Property());
    }
}

void ETSBinder::BuildClassDefinition(ir::ClassDefinition *classDef)
{
    auto boundCtx = BoundContext(recordTable_, classDef);

    if (classDef->TypeParams() != nullptr) {
        auto scopeCtx = LexicalScope<LocalScope>::Enter(this, classDef->TypeParams()->Scope());
        ResolveReferences(classDef->TypeParams());
        BuildClassDefinitionImpl(classDef);
        return;
    }

    BuildClassDefinitionImpl(classDef);
}

LocalScope *ETSBinder::ResolvePropertyReference(ir::ClassProperty *prop, ClassScope *scope)
{
    ResolveReferences(prop);

    if (prop->IsStatic()) {
        return scope->StaticFieldScope();
    }

    return scope->InstanceFieldScope();
}

void ETSBinder::BuildClassDefinitionImpl(ir::ClassDefinition *classDef)
{
    auto classCtx = LexicalScope<ClassScope>::Enter(this, classDef->Scope()->AsClassScope());

    if (classDef->Super() != nullptr) {
        ResolveReference(classDef->Super());
    }

    for (auto *impl : classDef->Implements()) {
        ResolveReference(impl);
    }

    if (classDef->HasAnnotations()) {
        for (auto *anno : classDef->Annotations()) {
            ResolveReference(anno);
        }
    }

    for (auto *stmt : classDef->Body()) {
        if (!stmt->IsClassProperty()) {
            continue;
        }
        auto *const prop = stmt->AsClassProperty();

        auto fieldScope = ResolvePropertyReference(prop, classDef->Scope()->AsClassScope());
        ES2PANDA_ASSERT(prop->Id() != nullptr);
        auto fieldName = prop->Id()->Name();
        if (auto fieldVar = fieldScope->FindLocal(fieldName, varbinder::ResolveBindingOptions::BINDINGS);
            fieldVar != nullptr) {
            if (fieldVar->Declaration()->Node()->IsClassProperty() &&
                fieldVar->Declaration()->Node()->AsClassProperty()->NeedInitInStaticBlock()) {
                fieldVar->AddFlag(VariableFlags::INIT_IN_STATIC_BLOCK);
            } else if (!fieldVar->Declaration()->Node()->IsDefinite()) {
                fieldVar->AddFlag(VariableFlags::INITIALIZED);
            }

            if ((fieldVar->Declaration()->IsConstDecl() || fieldVar->Declaration()->IsReadonlyDecl()) &&
                prop->Value() == nullptr) {
                fieldVar->AddFlag(VariableFlags::EXPLICIT_INIT_REQUIRED);
            }
        } else {
            ES2PANDA_ASSERT(GetContext()->diagnosticEngine->IsAnyError());
            auto *checker = GetContext()->GetChecker()->AsETSChecker();
            prop->SetTsType(checker->GlobalTypeError());
            prop->Id()->SetTsType(checker->GlobalTypeError());
        }
    }

    for (auto *stmt : classDef->Body()) {
        if (stmt->IsClassProperty()) {
            continue;
        }
        ResolveReference(stmt);
    }
}

void ETSBinder::AddFunctionThisParam(ir::ScriptFunction *func)
{
    auto paramScopeCtx = LexicalScope<FunctionParamScope>::Enter(this, func->Scope()->ParamScope());
    auto *thisParam = AddMandatoryParam(MANDATORY_PARAM_THIS);
    ES2PANDA_ASSERT(thisParam != nullptr);
    thisParam->Declaration()->BindNode(thisParam_);
}

void ETSBinder::AddDynamicImport(ir::ETSImportDeclaration *import)
{
    ES2PANDA_ASSERT(import->Language().IsDynamic());
    if (std::find(dynamicImports_.begin(), dynamicImports_.end(), import) != dynamicImports_.end()) {
        return;
    }
    dynamicImports_.push_back(import);
}

void ETSBinder::InsertForeignBinding(const util::StringView &name, Variable *var)
{
    TopScope()->InsertForeignBinding(name, var);
}

void ETSBinder::InsertOrAssignForeignBinding(const util::StringView &name, Variable *var)
{
    TopScope()->InsertOrAssignForeignBinding(name, var);
}

void ETSBinder::ThrowRedeclarationError(const lexer::SourcePosition &pos, const Variable *const var,
                                        const Variable *const variable, util::StringView localName)
{
    const bool isNamespace = var->Declaration()->Node()->IsClassDefinition() &&
                             var->Declaration()->Node()->AsClassDefinition()->IsNamespaceTransformed();
    const auto type = isNamespace                                       ? "Namespace"
                      : var->Declaration()->Node()->IsClassDefinition() ? "Class"
                      : var->Declaration()->IsFunctionDecl()            ? "Function"
                                                                        : "Variable";

    if (variable->Declaration()->Type() == var->Declaration()->Type()) {
        ThrowError(pos, diagnostic::REDEFINITION, {type, localName});
    } else {
        ThrowError(pos, diagnostic::REDEFINITION_DIFF_TYPE, {type, localName});
    }
}

void AddOverloadFlag(ArenaAllocator *allocator, bool isStdLib, varbinder::Variable *importedVar,
                     varbinder::Variable *variable)
{
    auto *const currentNode = variable->Declaration()->Node()->AsMethodDefinition();
    auto *const method = importedVar->Declaration()->Node()->AsMethodDefinition();

    // Necessary because stdlib and escompat handled as same package, it can be removed after fixing package handling
    auto const getPackageName = [](Variable *var) {
        return var->GetScope()->Node()->GetTopStatement()->AsETSModule()->Program()->ModuleName();
    };
    if (isStdLib && (getPackageName(importedVar) != getPackageName(variable))) {
        return;
    }

    ES2PANDA_ASSERT(method->Function() != nullptr);
    if (!method->Overloads().empty() && !method->HasOverload(currentNode)) {
        method->AddOverload(currentNode);
        ES2PANDA_ASSERT(currentNode->Function() != nullptr);
        currentNode->Function()->Id()->SetVariable(importedVar);
        currentNode->Function()->AddFlag(ir::ScriptFunctionFlags::OVERLOAD);
        currentNode->Function()->AddFlag(ir::ScriptFunctionFlags::EXTERNAL_OVERLOAD);
        util::UString newInternalName(currentNode->Function()->Scope()->Name(), allocator);
        currentNode->Function()->Scope()->BindInternalName(newInternalName.View());
        return;
    }

    if (!currentNode->HasOverload(method)) {
        currentNode->AddOverload(method);
        if (method->Function()->Scope()->InternalName() == "") {
            method->Function()->Id()->SetVariable(variable);
            method->Function()->AddFlag(ir::ScriptFunctionFlags::OVERLOAD);
            method->Function()->AddFlag(ir::ScriptFunctionFlags::EXTERNAL_OVERLOAD);
            util::UString newInternalName(method->Function()->Scope()->Name(), allocator);
            method->Function()->Scope()->BindInternalName(newInternalName.View());
        }
    }
}

void ETSBinder::ImportAllForeignBindings(const parser::Program *const importedProgram)
{
    const auto *const importGlobalScope = importedProgram->GlobalScope();

    bool const isStdLib = util::Helpers::IsStdLib(Program());

    for (const auto [bindingName, var] : importGlobalScope->Bindings()) {
        if (!var->Declaration()->Node()->IsValidInCurrentPhase()) {
            continue;
        }
        if (util::Helpers::IsGlobalVar(var)) {
            continue;
        }
        if (!importGlobalScope->IsForeignBinding(bindingName) && !var->Declaration()->Node()->IsDefaultExported() &&
            (var->AsLocalVariable()->Declaration()->Node()->IsExported())) {
            auto variable = Program()->GlobalClassScope()->FindLocal(bindingName, ResolveBindingOptions::ALL);
            if (variable == nullptr || var == variable) {
                InsertForeignBinding(bindingName, var);
                continue;
            }

            if (variable->Declaration()->IsFunctionDecl() && var->Declaration()->IsFunctionDecl()) {
                AddOverloadFlag(Allocator(), isStdLib, var, variable);
                continue;
            }

            // It will be a redeclaration error, but the imported element has not been placed among the bindings yet
            if (TopScope()->FindLocal(bindingName, ResolveBindingOptions::ALL) == nullptr) {
                InsertForeignBinding(bindingName, var);
            }

            // redeclaration for builtin type,
            // need to erase the redeclaration one and make sure the builtin types initialized successfully.
            if (var->HasFlag(varbinder::VariableFlags::BUILTIN_TYPE)) {
                TopScope()->CorrectForeignBinding(bindingName, var, variable);
            }

            ThrowRedeclarationError(variable->Declaration()->Node()->Start(), var, variable, bindingName);
        }
    }

    for (const auto [bindingName, var] : importedProgram->GlobalClassScope()->StaticMethodScope()->Bindings()) {
        if (!var->Declaration()->Node()->IsDefaultExported()) {
            InsertForeignBinding(bindingName, var);
        }
    }

    for (const auto [bindingName, var] : importedProgram->GlobalClassScope()->StaticFieldScope()->Bindings()) {
        if (!var->Declaration()->Node()->IsDefaultExported()) {
            InsertForeignBinding(bindingName, var);
        }
    }
}

struct ImportBindingKey {
    const ir::ETSImportDeclaration *import {};
    util::StringView importedName {};
    util::StringView localName {};
    bool isTypeOnly {};
    ImportBindingKind kind {ImportBindingKind::NAMED};
};

static bool IsSameImportBinding(const ImportBindingInfo *bindingInfo, const ImportBindingKey &key);
static Variable *DropShadowableForeignBinding(GlobalScope *scope, util::StringView localName, Variable *previous);

void ETSBinder::AddImportNamespaceSpecifiersToTopBindings(parser::Program *const importedProgram,
                                                          ir::ImportNamespaceSpecifier *const namespaceSpecifier,
                                                          const ir::ETSImportDeclaration *const import)
{
    auto *local = namespaceSpecifier->Local();
    if (!local->Name().Empty()) {
        auto *previouslyImportedVariable = TopScope()->FindLocal(local->Name(), ResolveBindingOptions::ALL);
        previouslyImportedVariable =
            DropShadowableForeignBinding(TopScope(), local->Name(), previouslyImportedVariable);
        if (previouslyImportedVariable != nullptr && previouslyImportedVariable->IsLocalVariable() &&
            previouslyImportedVariable->HasFlag(varbinder::VariableFlags::IMPORT_BINDING)) {
            const ImportBindingKey key {import, local->Name(), local->Name(), import->IsTypeKind(),
                                        ImportBindingKind::NAMESPACE};
            if (IsSameImportBinding(previouslyImportedVariable->AsLocalVariable()->ImportBinding(), key)) {
                local->SetVariable(previouslyImportedVariable);
                return;
            }
        }

        if (previouslyImportedVariable != nullptr && previouslyImportedVariable->IsLocalVariable() &&
            previouslyImportedVariable->Declaration() != nullptr &&
            previouslyImportedVariable->Declaration()->Node()->IsImportNamespaceSpecifier()) {
            auto *bindingInfo = Allocator()->New<ImportBindingInfo>();
            bindingInfo->importDecl = import;
            bindingInfo->importedName = local->Name();
            bindingInfo->localName = local->Name();
            bindingInfo->origin = namespaceSpecifier;
            bindingInfo->kind = ImportBindingKind::NAMESPACE;
            bindingInfo->isTypeOnly = import->IsTypeKind();
            previouslyImportedVariable->AsLocalVariable()->SetImportBinding(bindingInfo);
            local->SetVariable(previouslyImportedVariable);
            return;
        }

        auto *var = CreateNamedImportBinding(local->Name(), local, import, ImportBindingKind::NAMESPACE);
        if (previouslyImportedVariable != nullptr) {
            var->ImportBinding()->conflictingLocalVariable = previouslyImportedVariable;
            TopScope()->InsertOrAssignBinding(local->Name(), var);
        }
        local->SetVariable(var);
        return;
    }

    if (importedProgram != nullptr) {
        ImportAllForeignBindings(importedProgram);
    }

    const auto &reexportImports = ReExportImports()[GetExternalProgram(import)];
    for (auto *reexp : reexportImports) {
        for (auto it : reexp->GetETSImportDeclarations()->Specifiers()) {
            if (it->IsImportNamespaceSpecifier() && !namespaceSpecifier->Local()->Name().Empty()) {
                continue;
            }

            AddSpecifiersToTopBindings(it, reexp->GetETSImportDeclarations());
        }
    }
}

util::StringView ETSBinder::GetAdjustedImportedName(ir::ImportSpecifier *const importSpecifier,
                                                    const ir::ETSImportDeclaration *const import)
{
    auto imported = importSpecifier->Imported()->Name();
    for (auto const item : import->Specifiers()) {
        if (item->IsImportSpecifier() && item->AsImportSpecifier()->Local()->Name().Is(imported.Mutf8()) &&
            !item->AsImportSpecifier()->Local()->Name().Is(item->AsImportSpecifier()->Imported()->Name().Mutf8())) {
            imported = item->AsImportSpecifier()->Imported()->Name();
        }
    }

    return imported;
}

LocalVariable *ETSBinder::CreateNamedImportBinding(util::StringView importedName, ir::Identifier *local,
                                                   const ir::ETSImportDeclaration *import, ImportBindingKind kind)
{
    auto *decl = Allocator()->New<ImportDecl>(importedName, local->Name(), local);
    auto *var = Allocator()->New<LocalVariable>(decl, VariableFlags::READONLY | VariableFlags::INITIALIZED);
    auto *bindingInfo = Allocator()->New<ImportBindingInfo>();
    bindingInfo->importDecl = import;
    bindingInfo->importedName = importedName;
    bindingInfo->localName = local->Name();
    bindingInfo->origin = local->Parent();
    bindingInfo->kind = kind;
    bindingInfo->isTypeOnly = import->IsTypeKind();
    var->SetImportBinding(bindingInfo);
    if (kind == ImportBindingKind::NAMESPACE) {
        var->AddFlag(VariableFlags::NAMESPACE);
    }
    var->SetScope(TopScope());
    TopScope()->InsertBinding(local->Name(), var);
    return var;
}

void ETSBinder::BindReExportSpecifierIdentifiers(ir::AstNode *specifier, const ir::ETSImportDeclaration *import)
{
    if (!specifier->IsImportSpecifier()) {
        return;
    }

    auto *importSpecifier = specifier->AsImportSpecifier();
    if (!importSpecifier->Imported()->IsIdentifier()) {
        return;
    }

    auto *local = importSpecifier->Local();
    auto imported = GetAdjustedImportedName(importSpecifier, import);
    auto *localDecl = Allocator()->New<ImportDecl>(imported, local->Name(), local);
    auto *localVar = Allocator()->New<LocalVariable>(localDecl, VariableFlags::READONLY | VariableFlags::INITIALIZED);
    auto *bindingInfo = Allocator()->New<ImportBindingInfo>();
    bindingInfo->importDecl = import;
    bindingInfo->importedName = imported;
    bindingInfo->localName = local->Name();
    bindingInfo->origin = importSpecifier;
    bindingInfo->kind = ImportBindingKind::NAMED;
    bindingInfo->isTypeOnly = import->IsTypeKind();
    localVar->SetImportBinding(bindingInfo);
    localVar->SetScope(TopScope());
    local->SetVariable(localVar);

    if (imported == local->Name()) {
        importSpecifier->Imported()->SetVariable(localVar);
        return;
    }

    auto *importedDecl = Allocator()->New<ImportDecl>(imported, imported, importSpecifier->Imported());
    auto *importedVar =
        Allocator()->New<LocalVariable>(importedDecl, VariableFlags::READONLY | VariableFlags::INITIALIZED);
    auto *importedBindingInfo = Allocator()->New<ImportBindingInfo>();
    importedBindingInfo->importDecl = import;
    importedBindingInfo->importedName = imported;
    importedBindingInfo->localName = local->Name();
    importedBindingInfo->origin = importSpecifier;
    importedBindingInfo->kind = ImportBindingKind::NAMED;
    importedBindingInfo->isTypeOnly = import->IsTypeKind();
    importedVar->SetImportBinding(importedBindingInfo);
    importedVar->SetScope(TopScope());
    importSpecifier->Imported()->SetVariable(importedVar);
}

static bool IsSameImportBinding(const ImportBindingInfo *bindingInfo, const ImportBindingKey &key)
{
    return bindingInfo != nullptr && bindingInfo->importDecl != nullptr && key.import != nullptr &&
           bindingInfo->importDecl->ImportInfo().Key() == key.import->ImportInfo().Key() &&
           bindingInfo->importedName == key.importedName && bindingInfo->localName == key.localName &&
           bindingInfo->isTypeOnly == key.isTypeOnly && bindingInfo->kind == key.kind;
}

static Variable *DropShadowableForeignBinding(GlobalScope *scope, util::StringView localName, Variable *previous)
{
    if (previous != nullptr && scope->IsForeignBinding(localName) &&
        !previous->HasFlag(varbinder::VariableFlags::BUILTIN_TYPE)) {
        scope->EraseBinding(localName);
        return nullptr;
    }
    return previous;
}

void ETSBinder::AddImportSpecifiersToTopBindings(ir::ImportSpecifier *const importSpecifier,
                                                 const ir::ETSImportDeclaration *const import)
{
    if (!importSpecifier->Imported()->IsIdentifier()) {
        return;
    }

    auto imported = GetAdjustedImportedName(importSpecifier, import);
    ir::Identifier *const local = importSpecifier->Local();
    const auto &localName = local->Name();
    auto previouslyImportedVariable = TopScope()->FindLocal(localName, ResolveBindingOptions::ALL);
    previouslyImportedVariable = DropShadowableForeignBinding(TopScope(), localName, previouslyImportedVariable);

    auto bindImportedIdentifier = [this, importSpecifier, imported, localName](Variable *localVar) {
        if (imported == localName) {
            importSpecifier->Imported()->SetVariable(localVar);
            return;
        }

        auto *decl = Allocator()->New<ImportDecl>(imported, imported, importSpecifier->Imported());
        auto *var = Allocator()->New<LocalVariable>(decl, VariableFlags::READONLY | VariableFlags::INITIALIZED);
        var->SetScope(TopScope());
        importSpecifier->Imported()->SetVariable(var);
    };

    if (previouslyImportedVariable != nullptr && previouslyImportedVariable->IsLocalVariable() &&
        previouslyImportedVariable->HasFlag(varbinder::VariableFlags::IMPORT_BINDING)) {
        const ImportBindingKey key {import, imported, localName, import->IsTypeKind(), ImportBindingKind::NAMED};
        if (IsSameImportBinding(previouslyImportedVariable->AsLocalVariable()->ImportBinding(), key)) {
            bindImportedIdentifier(previouslyImportedVariable);
            local->SetVariable(previouslyImportedVariable);
            return;
        }
    }

    auto *var = CreateNamedImportBinding(imported, local, import);
    if (previouslyImportedVariable != nullptr) {
        var->ImportBinding()->conflictingLocalVariable = previouslyImportedVariable;
    }
    bindImportedIdentifier(var);
    local->SetVariable(var);
}

void ETSBinder::AddImportDefaultSpecifiersToTopBindings(ir::ImportDefaultSpecifier *const importDefaultSpecifier,
                                                        const ir::ETSImportDeclaration *const import)
{
    ir::Identifier *const local = importDefaultSpecifier->Local();
    const auto &localName = local->Name();
    auto previouslyImportedVariable = TopScope()->FindLocal(localName, ResolveBindingOptions::ALL);
    previouslyImportedVariable = DropShadowableForeignBinding(TopScope(), localName, previouslyImportedVariable);
    auto varInGlobalClassScope = Program()->GlobalClassScope()->FindLocal(localName, ResolveBindingOptions::ALL);

    // Idempotent re-import: if the existing variable is also an import binding,
    // reuse it rather than reporting a redeclaration error.
    if (previouslyImportedVariable != nullptr && previouslyImportedVariable->IsLocalVariable() &&
        previouslyImportedVariable->HasFlag(varbinder::VariableFlags::IMPORT_BINDING)) {
        const ImportBindingKey key {import, util::StringView {"default"}, localName, import->IsTypeKind(),
                                    ImportBindingKind::DEFAULT};
        if (IsSameImportBinding(previouslyImportedVariable->AsLocalVariable()->ImportBinding(), key)) {
            local->SetVariable(previouslyImportedVariable);
            return;
        }
    }

    auto *var = CreateNamedImportBinding("default", local, import, ImportBindingKind::DEFAULT);
    if (varInGlobalClassScope != nullptr || previouslyImportedVariable != nullptr) {
        var->ImportBinding()->conflictingLocalVariable =
            varInGlobalClassScope != nullptr ? varInGlobalClassScope : previouslyImportedVariable;
    }
    local->SetVariable(var);
}

parser::Program *ETSBinder::GetExternalProgram(const ir::ETSImportDeclaration *import)
{
    auto importee = GetContext()->parser->GetImportPathManager()->SearchResolved(import->ImportInfo());
    if (importee == nullptr) {
        if (ark::os::file::File::IsDirectory(std::string(import->ResolvedSource()))) {
            ThrowError(import->Start(), diagnostic::MODULE_INDEX_MISSING, {import->ResolvedSource()});
        } else {
            ThrowError(import->Start(), diagnostic::IMPORT_NOT_FOUND_2, {import->ResolvedSource()});
        }
        return nullptr;
    }
    if (importee->Is<util::ModuleKind::PACKAGE>() &&
        importee->As<util::ModuleKind::PACKAGE>()->GetUnmergedPackagePrograms().empty()) {
        ThrowError(import->Start(), diagnostic::MODULE_INDEX_MISSING, {import->ResolvedSource()});
        return nullptr;
    }
    return importee;
}

parser::Program *ETSBinder::RegisterImportTarget(const ir::ETSImportDeclaration *import)
{
    if (import == nullptr || !import->IsValid()) {
        return nullptr;
    }

    auto *effectiveTargetProgram = GetExternalProgram(import);
    auto *targetProgram = effectiveTargetProgram;
    auto *exactTargetProgram = GetContext()->parser->GetImportPathManager()->SearchResolvedExact(import->ImportInfo());
    if (exactTargetProgram != nullptr) {
        targetProgram = exactTargetProgram;
    }
    auto *sourceProgram = FindOwningProgram(import);
    exportFactStore_->AddImportTarget(sourceProgram, import, targetProgram);
    exportFactStore_->AddEffectiveImportTarget(sourceProgram, import, effectiveTargetProgram);
    return targetProgram;
}

void ETSBinder::AddSpecifiersToTopBindings(ir::AstNode *const specifier, const ir::ETSImportDeclaration *const import)
{
    if (specifier->IsImportNamespaceSpecifier()) {
        auto *namespaceSpecifier = specifier->AsImportNamespaceSpecifier();
        parser::Program *importedProgram = nullptr;
        if (namespaceSpecifier->Local()->Name().Empty()) {
            importedProgram = GetExternalProgram(import);
            if (importedProgram == nullptr || importedProgram->Ast() == nullptr ||
                importedProgram->Ast()->Scope() == nullptr) {
                return;
            }
        }
        AddImportNamespaceSpecifiersToTopBindings(importedProgram, specifier->AsImportNamespaceSpecifier(), import);
    } else if (specifier->IsImportSpecifier()) {
        AddImportSpecifiersToTopBindings(specifier->AsImportSpecifier(), import);
    } else if (specifier->IsImportDefaultSpecifier()) {
        AddImportDefaultSpecifiersToTopBindings(specifier->AsImportDefaultSpecifier(), import);
    }
}

void ETSBinder::BuildReExportDeclaration(ir::ETSReExportDeclaration *reExportDecl)
{
    auto *import = reExportDecl->GetETSImportDeclarations();
    RegisterImportTarget(import);
    for (auto *specifier : import->Specifiers()) {
        BindReExportSpecifierIdentifiers(specifier, import);
    }
    ResolveReferences(reExportDecl);
}

void ETSBinder::HandleCustomNodes(ir::AstNode *childNode)
{
    switch (childNode->Type()) {
        case ir::AstNodeType::ETS_TYPE_REFERENCE: {
            return BuildETSTypeReference(childNode->AsETSTypeReference());
        }
        case ir::AstNodeType::TS_INTERFACE_DECLARATION: {
            return BuildInterfaceDeclaration(childNode->AsTSInterfaceDeclaration());
        }
        case ir::AstNodeType::TS_ENUM_DECLARATION: {
            return ResolveEnumDeclaration(childNode->AsTSEnumDeclaration());
        }
        case ir::AstNodeType::EXPORT_NAMED_DECLARATION: {
            break;
        }
        case ir::AstNodeType::ETS_IMPORT_DECLARATION: {
            return BuildImportDeclaration(childNode->AsETSImportDeclaration());
        }
        case ir::AstNodeType::REEXPORT_STATEMENT: {
            return BuildReExportDeclaration(childNode->AsETSReExportDeclaration());
        }
        case ir::AstNodeType::MEMBER_EXPRESSION: {
            return BuildMemberExpression(childNode->AsMemberExpression());
        }
        case ir::AstNodeType::METHOD_DEFINITION: {
            return BuildMethodDefinition(childNode->AsMethodDefinition());
        }
        case ir::AstNodeType::OVERLOAD_DECLARATION: {
            return BuildOverloadDeclaration(childNode->AsOverloadDeclaration());
        }
        case ir::AstNodeType::ETS_NEW_CLASS_INSTANCE_EXPRESSION: {
            return BuildETSNewClassInstanceExpression(childNode->AsETSNewClassInstanceExpression());
        }
        case ir::AstNodeType::ETS_FUNCTION_TYPE: {
            return BuildSignatureDeclarationBaseParams(childNode);
        }
        case ir::AstNodeType::OBJECT_EXPRESSION: {
            return BuildObjectExpression(childNode->AsObjectExpression());
        }
        case ir::AstNodeType::ANNOTATION_USAGE: {
            return BuildAnnotationUsage(childNode->AsAnnotationUsage());
        }
        case ir::AstNodeType::ANNOTATION_DECLARATION: {
            BuildAnnotationDeclaration(childNode->AsAnnotationDeclaration());
            break;
        }
        default: {
            return ResolveReferences(childNode);
        }
    }
}

bool ETSBinder::BuildInternalName(ir::ScriptFunction *scriptFunc)
{
    const bool isExternal = recordTable_->IsExternal();
    if (isExternal) {
        scriptFunc->AddFlag(ir::ScriptFunctionFlags::EXTERNAL);
    }

    if (scriptFunc->IsArrow()) {
        return true;
    }

    auto *funcScope = scriptFunc->Scope();
    funcScope->BindName(recordTable_->RecordName());

    return scriptFunc->Body() != nullptr && !isExternal;
}

bool ETSBinder::BuildInternalNameWithCustomRecordTable(ir::ScriptFunction *const scriptFunc,
                                                       RecordTable *const recordTable)
{
    const bool isExternal = recordTable->IsExternal();
    if (isExternal) {
        scriptFunc->AddFlag(ir::ScriptFunctionFlags::EXTERNAL);
    }

    if (scriptFunc->IsArrow()) {
        return true;
    }

    auto *const funcScope = scriptFunc->Scope();
    funcScope->BindName(recordTable->RecordName());

    return scriptFunc->Body() != nullptr && !isExternal;
}

void ETSBinder::AddCompilableFunction(ir::ScriptFunction *func)
{
    /*
     * NOTE(knazarov) Here it is important to leave IsAsyncFunc, since
     * for stackless we need to omit compilation of these 'native' funcs
     */
    if (!GetContext()->config->options->IsStacklessCoros() && func->IsAsyncFunc()) {
        return;
    }

    if (func->IsArrow()) {
        return;
    }

    if (GetContext()->config->options->GetCompilationMode() >= CompilationMode::SIMULTANEOUS &&
        func->Scope()->Name().Is(compiler::Signatures::ETS_GLOBAL)) {
        return;
    }

    AddCompilableFunctionScope(func->Scope());
}

void ETSBinder::BuildFunctionName(const ir::ScriptFunction *func) const
{
    auto *funcScope = func->Scope();

    std::stringstream ss;
    ES2PANDA_ASSERT(func->IsArrow() || !funcScope->Name().Empty());
    ss << (func->IsExternalOverload() ? funcScope->InternalName() : funcScope->Name())
       << compiler::Signatures::METHOD_SEPARATOR;

    const auto *signature = func->Signature();
    const auto funcName = util::Helpers::FunctionName(Allocator(), func);

    if (func->IsStaticBlock()) {
        ss << compiler::Signatures::CCTOR;
    } else if (func->IsConstructor() && funcName.Is(compiler::Signatures::CONSTRUCTOR_NAME)) {
        ss << compiler::Signatures::CTOR;
    } else {
        std::string newName;
        if (func->IsGetter()) {
            newName = util::NameMangler::GetInstance()->CreateMangledNameByTypeAndName(
                util::NameMangler::GET, util::Helpers::FunctionName(Allocator(), func));
            ss << newName;
        } else if (func->IsSetter()) {
            newName = util::NameMangler::GetInstance()->CreateMangledNameByTypeAndName(
                util::NameMangler::SET, util::Helpers::FunctionName(Allocator(), func));
            ss << newName;
        } else {
            ss << funcName;
        }
    }

    signature->ToAssemblerType(ss);
    auto newName = ss.str();
    if (funcScope->InternalName().Utf8() == newName) {
        return;
    }
    funcScope->BindInternalName(util::UString(newName, Allocator()).View());
}

void ETSBinder::InitImplicitThisParam()
{
    thisParam_ = Allocator()->New<ir::Identifier>("this", Allocator());
}

inline constexpr std::string_view STD_PREFIX = "std.";

static void TraverseAST(ETSBinder *binder, ArenaVector<ir::ETSImportDeclaration *> &defaultImports)
{
    for (auto *defaultImport : defaultImports) {
        binder->BuildImportDeclaration(defaultImport);
    }

    auto &stmts = binder->Program()->Ast()->StatementsForUpdates();
    if (binder->Program()->GetImportInfo().ModuleName() == compiler::Signatures::SIMULT_MODULE_NAME) {
        stmts.clear();
        return;
    }

    const auto etsGlobal = std::find_if(stmts.begin(), stmts.end(), [](const ir::Statement *stmt) {
        return stmt->IsClassDeclaration() && stmt->AsClassDeclaration()->Definition()->IsGlobal();
    });
    if (etsGlobal != stmts.end()) {
        const auto begin = std::find_if(stmts.rbegin(), stmts.rend(), [](const ir::Statement *stmt) {
                               return stmt->IsETSImportDeclaration() || stmt->IsETSPackageDeclaration();
                           }).base();

        const auto index = std::distance(begin, etsGlobal);
        std::rotate(begin, begin + index, begin + index + 1);
    }

    for (auto *stmt : stmts) {
        binder->ResolveReference(stmt);
    }
}

void ETSBinder::BuildProgram()
{
    // NOTE(dkofanov): remove from varbinder state:
    Program()->SetRecordTable(globalRecordTable_);
    // A tmp solution caused by #23877, needs to check stdlib first to avoid a bug in std/math/math.ets
    // After the bug fixed, we can merge these 2 loop.
    Program()->GetExternalDecls()->Visit([this](auto *extProg) {
        if (extProg->ModuleName().substr(0, STD_PREFIX.length()) == STD_PREFIX) {
            BuildExternalProgram(extProg);
        }
    });
    Program()->GetExternalDecls()->Visit([this](auto *extProg) {
        if (extProg->ModuleName().substr(0, STD_PREFIX.length()) != STD_PREFIX) {
            BuildExternalProgram(extProg);
        }
    });

    TraverseAST(this, defaultImports_);

    // NOTE(dkofanov): For some reason, reexports from all the programs are stored in a single container.
    ValidateReexports();
}

void ETSBinder::BuildExternalProgram(parser::Program *extProgram)
{
    if (extProgram->Is<util::ModuleKind::PACKAGE>() &&
        extProgram->As<util::ModuleKind::PACKAGE>()->GetUnmergedPackagePrograms().empty()) {
        return;
    }

    exportFactStore_->RegisterProgramSurface(extProgram);
    exportFactStore_->RegisterPackageSurface(extProgram);

    auto *savedProgram = Program();
    auto *savedRecordTable = recordTable_;
    auto *savedTopScope = TopScope();

    auto flags = Program()->VarBinder()->IsGenStdLib() || (extProgram->IsBuiltSimultaneously())
                     ? RecordTableFlags::NONE
                     : RecordTableFlags::EXTERNAL;
    auto *extRecordTable = Allocator()->New<RecordTable>(Allocator(), extProgram, flags);
    extRecordTable->SetClassDefinition(extProgram->GlobalClass());

    // NOTE(dkofanov): 'externalRecordTable_' should be eliminated, recordTables should be stored in programs
    // themselves.
    externalRecordTable_.insert({extProgram, extRecordTable});

    ResetTopScope(extProgram->GlobalScope());
    recordTable_ = extRecordTable;
    extProgram->SetRecordTable(extRecordTable);
    SetProgram(extProgram);

    if (extProgram->IsASTLowered() || !extProgram->IsProgramModified()) {
        extRecordTable->Merge(extProgram->VarBinder()->AsETSBinder()->GetExternalRecordTable().at(extProgram));
    } else {
        TraverseAST(this, defaultImports_);
    }

    ValidateReexports();

    SetProgram(savedProgram);
    recordTable_ = savedRecordTable;
    ResetTopScope(savedTopScope);
}

bool ETSBinder::CheckRecordTablesConsistency(parser::Program *program /* = nullptr */) const
{
    bool ok {true};
    auto mainProg = GetContext()->parserProgram;
    ok &= (GetExternalRecordTable().find(mainProg) == GetExternalRecordTable().cend());
    ok &= (mainProg->GetRecordTable() == GetGlobalRecordTable());

    if (program == nullptr) {
        mainProg->GetExternalDecls()->Visit([this, &ok](auto *extProgram) {
            ok &= (extProgram->GetRecordTable() == GetExternalRecordTable().find(extProgram)->second);
            ok &= (extProgram->GetRecordTable() != GetGlobalRecordTable());
        });
    } else if (program != mainProg) {
        ok &= (program->GetRecordTable() == GetExternalRecordTable().find(program)->second);
        ok &= (program->GetRecordTable() != GetGlobalRecordTable());
    }
    return ok;
}

void ETSBinder::BuildETSNewClassInstanceExpression(ir::ETSNewClassInstanceExpression *classInstance)
{
    ResolveReference(classInstance->GetTypeRef());

    for (auto *arg : classInstance->GetArguments()) {
        ResolveReference(arg);
    }
}

void ETSBinder::BuildImportDeclaration(ir::ETSImportDeclaration *decl)
{
    if (!decl->IsValid()) {
        return;
    }

    RegisterImportTarget(decl);
    const auto &specifiers = decl->Specifiers();
    for (auto specifier : specifiers) {
        AddSpecifiersToTopBindings(specifier, decl);
    }
}

void ETSBinder::ValidateReexports()
{
    CollectExportFactsForCurrentProgram();
}

void ETSBinder::ThrowError(const lexer::SourcePosition &pos, const diagnostic::DiagnosticKind &kind,
                           const util::DiagnosticMessageParams &params) const
{
    GetContext()->diagnosticEngine->LogDiagnostic(kind, params, pos);
}

bool ETSBinder::IsGlobalIdentifier([[maybe_unused]] const util::StringView &str) const
{
    return false;
}

}  // namespace ark::es2panda::varbinder

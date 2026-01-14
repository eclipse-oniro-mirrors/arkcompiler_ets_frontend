/**
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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
 * @file generate_override_methods.cpp
 * @brief Implements functionality to generate override methods for classes and interfaces in ArkTS.
 *
 * This file defines the GenerateOverrideMethods refactor action, which:
 *  - Detects classes and interfaces in the AST that have superclasses or extended/implemented interfaces.
 *  - Compares existing methods in the class/interface with those in superclasses/interfaces.
 *  - Generates missing override methods automatically.
 *
 * The override methods include proper access modifiers (public, protected), async/generator
 * handling, getter/setter keywords, parameter types, and return types.
 *
 * Main classes and functions:
 *  - ark::es2panda::lsp::GenerateOverrideMethods: Refactor action class for generating override methods.
 *  - GenerateAllOverrideMethods(): Generates all missing overrides and applies them using ChangeTracker.
 *  - GetRefactorEditsToOverrideMethods(): Returns the text edits for applying this refactor.
 *
 * Helper functions include:
 *  - IsInterface, GetSuperClassDefinition, GetImplementsDefinition
 *  - GetMethodsFromClassDecl, GetMethodsFromInterfaceDecl
 *  - GetSuperMethodsOfClass, GetSuperMethodsOfImpl, GetSuperMethodsOfInterface
 *  - ParamNameFor, ParamsText, ReturnTypeText, SignaturePrefix, CreateOverrideText, ToGenerateMethods
 *
 * Usage:
 *  - The refactor can be triggered in an IDE or editor supporting ArkTS LSP.
 *  - Inserts missing override method stubs at the appropriate position in the class or interface.
 *  - Ensures code formatting and syntax correctness.
 */

#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

#include "refactors/generate_override_methods.h"
#include "internal_api.h"
#include "ir/astNode.h"
#include "ir/base/methodDefinition.h"
#include "public/public.h"
#include "refactor_provider.h"
#include "quick_info.h"
#include "refactors/refactor_types.h"
#include "services/text_change/change_tracker.h"
#include "types.h"

namespace ark::es2panda::lsp {

GenerateOverrideMethods::GenerateOverrideMethods()
{
    AddKind(std::string(TO_GENERATE_OVERRIDE_METHODS_ACTION.kind));
}

std::vector<ApplicableRefactorInfo> GenerateOverrideMethods::GetAvailableActions(const RefactorContext &ctx) const
{
    (void)ctx;
    return {};
}

static bool IsInterface(const ir::AstNode *n)
{
    return n != nullptr && n->IsTSInterfaceDeclaration();
}

static ir::AstNode *GetSuperClassDefinition(ir::AstNode *node)
{
    if (node == nullptr || !node->IsClassDefinition()) {
        return nullptr;
    }
    auto *sup = node->AsClassDefinition()->Super();
    if (sup == nullptr || !sup->IsETSTypeReference()) {
        return nullptr;
    }
    auto *part = sup->AsETSTypeReference()->Part();
    if (!part->IsETSTypeReferencePart()) {
        return nullptr;
    }
    return part->Name();
}

static std::vector<ir::AstNode *> GetImplementsDefinition(ir::AstNode *node)
{
    std::vector<ir::AstNode *> out;
    if (!IsClass(node)) {
        return out;
    }
    for (auto const &impl : node->AsClassDefinition()->Implements()) {
        auto *exp = impl->Expr();
        if (exp == nullptr || !exp->IsETSTypeReference()) {
            continue;
        }
        auto *part = exp->AsETSTypeReference()->Part();
        if (!part->IsETSTypeReferencePart()) {
            continue;
        }
        out.push_back(part->Name());
    }
    return out;
}

static std::vector<const ir::AstNode *> GetSuperClassOfInterface(ir::AstNode *node)
{
    std::vector<const ir::AstNode *> out;
    if (!IsInterface(node)) {
        return out;
    }
    for (auto e : node->AsTSInterfaceDeclaration()->Extends()) {
        out.push_back(e->Expr()->AsETSTypeReference()->Part()->Name()->AsIdentifier());
    }
    return out;
}

static std::vector<ir::MethodDefinition *> GetMethodsFromClassDecl(const ir::AstNode *clsDecl)
{
    std::vector<ir::MethodDefinition *> out;
    if (clsDecl == nullptr || !clsDecl->IsClassDefinition()) {
        return out;
    }
    for (auto const &m : clsDecl->AsClassDefinition()->Body()) {
        if (m->IsMethodDefinition()) {
            out.push_back(m->AsMethodDefinition());
        }
    }
    return out;
}

static std::vector<ir::MethodDefinition *> GetMethodsFromInterfaceDecl(const ir::AstNode *ifaceDecl)
{
    std::vector<ir::MethodDefinition *> out;
    if (ifaceDecl == nullptr || !ifaceDecl->IsTSInterfaceDeclaration()) {
        return out;
    }
    for (auto const &m : ifaceDecl->AsTSInterfaceDeclaration()->Body()->Body()) {
        if (m->IsMethodDefinition()) {
            out.push_back(m->AsMethodDefinition());
        }
    }
    return out;
}

static std::vector<ir::MethodDefinition *> GetSuperMethodsOfClass(const ir::AstNode *superClassDecl,
                                                                  const public_lib::Context *ctx)
{
    if (superClassDecl == nullptr || !superClassDecl->IsIdentifier()) {
        return {};
    }
    auto name = superClassDecl->AsIdentifier()->Name().Utf8();
    auto *found = ctx->parserProgram->Ast()->FindChild([&name](ir::AstNode *n) {
        return n->IsClassDefinition() && n->AsClassDefinition()->Ident() != nullptr &&
               n->AsClassDefinition()->Ident()->Name().Utf8() == name;
    });
    return GetMethodsFromClassDecl(found);
}

static std::vector<ir::MethodDefinition *> GetSuperMethodsOfImpl(const ir::AstNode *implDecl,
                                                                 const public_lib::Context *ctx)
{
    std::vector<ir::MethodDefinition *> out;
    if (implDecl == nullptr || !implDecl->IsIdentifier()) {
        return out;
    }
    auto name = implDecl->AsIdentifier()->Name().Utf8();
    auto *found = ctx->parserProgram->Ast()->FindChild([&name](ir::AstNode *n) {
        return n->IsTSInterfaceDeclaration() && n->AsTSInterfaceDeclaration()->Id() != nullptr &&
               n->AsTSInterfaceDeclaration()->Id()->Name().Utf8() == name;
    });
    return GetMethodsFromInterfaceDecl(found);
}

static std::vector<ir::MethodDefinition *> GetSuperMethodsOfInterface(const ir::AstNode *superClass,
                                                                      ir::AstNode *&superClassDecl,
                                                                      const public_lib::Context *ctx)
{
    if (superClass == nullptr) {
        return {};
    }
    if (!superClass->IsIdentifier()) {
        return {};
    }
    auto name = superClass->AsIdentifier()->Name().Utf8();
    superClassDecl = ctx->parserProgram->Ast()->FindChild([&name](ir::AstNode *n) {
        return n->IsTSInterfaceDeclaration() && n->AsTSInterfaceDeclaration()->Id() != nullptr &&
               n->AsTSInterfaceDeclaration()->Id()->Name().Utf8() == name;
    });
    return GetMethodsFromInterfaceDecl(superClassDecl);
}

static std::string ParamNameFor(const ir::AstNode *param)
{
    if (param == nullptr) {
        return "param";
    }
    if (param->IsETSParameterExpression()) {
        auto *ident = param->AsETSParameterExpression()->Ident();
        if (ident != nullptr && ident->IsIdentifier()) {
            return ident->Name().Mutf8();
        }
    }
    if (param->IsAssignmentPattern()) {
        auto *left = param->AsAssignmentPattern()->Left();
        if (left != nullptr && left->IsIdentifier()) {
            return left->AsIdentifier()->Name().Mutf8();
        }
    }
    return "param";
}

static std::string ParamsText(ir::MethodDefinition *method)
{
    std::string out;
    bool first = true;
    for (auto const &p : method->Function()->Params()) {
        if (!p->IsETSParameterExpression()) {
            continue;
        }
        if (!first) {
            out += ", ";
        }
        first = false;
        out += ParamNameFor(p);
        if (auto *t = p->AsETSParameterExpression()->TypeAnnotation()) {
            out += ": " + GetNameForTypeNode(t);
        }
    }
    return out;
}

static std::string ReturnTypeText(ir::MethodDefinition *method)
{
    if (auto *r = method->Function()->ReturnTypeAnnotation()) {
        return ": " + GetNameForTypeNode(r) + " ";
    }
    return {};
}

static std::string SignaturePrefix(ir::MethodDefinition *m)
{
    if (m->Parent() != nullptr && m->Parent()->IsTSInterfaceBody()) {
        return {};
    }
    std::string out;
    if (m->IsPublic() || m->IsProtected()) {
        out += "public ";
    }
    out += "override ";
    if (m->IsGetter()) {
        out += "get ";
    } else if (m->IsSetter()) {
        out += "set ";
    }
    if (m->IsAsync()) {
        out += "async ";
    }
    if (m->Function()->IsGenerator()) {
        out += "*";
    }
    return out;
}

static std::string CreateOverrideText(ir::MethodDefinition *m)
{
    std::string t = "    ";
    t += SignaturePrefix(m);
    if (m->Key()->IsIdentifier()) {
        t += m->Key()->AsIdentifier()->Name().Utf8();
    }
    t += "(" + ParamsText(m) + ") " + ReturnTypeText(m);
    if (m->Parent() != nullptr && m->Parent()->IsTSInterfaceBody()) {
        return t + "{\n\n}" + "\n\n";
    }
    t += "{\n        ";
    if (m->Function()->AsScriptFunction()->ReturnTypeAnnotation() != nullptr) {
        t += "return ";
    }
    t += "super.";
    if (m->Key()->IsIdentifier()) {
        t += m->Key()->AsIdentifier()->Name().Utf8();
    }
    t += "(" + ParamsText(m) + ");\n    }\n\n";
    return t;
}

static std::vector<ir::MethodDefinition *> ToGenerateMethods(const std::vector<ir::MethodDefinition *> &superMethods,
                                                             const std::vector<ir::MethodDefinition *> &classMethods)
{
    std::vector<ir::MethodDefinition *> out;
    for (auto *sm : superMethods) {
        if (!sm->Key()->IsIdentifier()) {
            continue;
        }
        if (sm->IsStatic() || sm->IsPrivate()) {
            continue;
        }
        auto const &name = sm->Key()->AsIdentifier()->Name();
        if (name.Is(compiler::Signatures::INIT_METHOD)) {
            continue;
        }
        if (name.Is(compiler::Signatures::CONSTRUCTOR_NAME)) {
            continue;
        }
        bool found = false;
        for (auto *cm : classMethods) {
            if (cm->Key()->IsIdentifier() && cm->Key()->AsIdentifier()->Name() == name) {
                found = true;
                break;
            }
        }
        if (!found) {
            out.push_back(sm);
        }
    }
    return out;
}

static std::vector<ir::MethodDefinition *> FindMethodIfClass(ir::AstNode *cur, const public_lib::Context *ctx)
{
    std::vector<ir::MethodDefinition *> superMethods;
    auto *cls = cur->AsClassDefinition();
    if (auto *sup = GetSuperClassDefinition(cls)) {
        auto v = GetSuperMethodsOfClass(sup, ctx);
        superMethods.insert(superMethods.end(), v.begin(), v.end());
    }
    for (auto *impl : GetImplementsDefinition(cls)) {
        auto v = GetSuperMethodsOfImpl(impl, ctx);
        superMethods.insert(superMethods.end(), v.begin(), v.end());
    }
    return superMethods;
}

static std::vector<ir::MethodDefinition *> FindMethodIfInterface(ir::AstNode *cur, const public_lib::Context *ctx)
{
    std::vector<ir::MethodDefinition *> superMethods;
    auto *iface = cur->AsTSInterfaceDeclaration();
    for (auto *s : GetSuperClassOfInterface(iface)) {
        ir::AstNode *decl = nullptr;
        auto v = GetSuperMethodsOfInterface(s, decl, ctx);
        superMethods.insert(superMethods.end(), v.begin(), v.end());
    }
    return superMethods;
}

void GenerateAllOverrideMethods(const RefactorContext &context, ChangeTracker &tracker)
{
    auto *baseCtx = context.context;
    auto *cur = GetTouchingToken(baseCtx, context.span.pos, false);
    auto *ctx = reinterpret_cast<public_lib::Context *>(baseCtx);
    while (cur != nullptr && !IsClass(cur) && !IsInterface(cur)) {
        cur = cur->Parent();
    }
    if (cur == nullptr) {
        return;
    }
    std::vector<ir::MethodDefinition *> superMethods;
    std::vector<ir::MethodDefinition *> classMethods;
    if (IsClass(cur)) {
        superMethods = FindMethodIfClass(cur, ctx);
        auto *cls = cur->AsClassDefinition();
        for (auto const &m : cls->AsClassDefinition()->Body()) {
            if (m->IsMethodDefinition()) {
                classMethods.push_back(m->AsMethodDefinition());
            }
        }
    } else {
        superMethods = FindMethodIfInterface(cur, ctx);
        auto *iface = cur->AsTSInterfaceDeclaration();
        for (auto const &m : iface->Body()->Body()) {
            if (m->IsMethodDefinition()) {
                classMethods.push_back(m->AsMethodDefinition());
            }
        }
    }
    auto toOverride = ToGenerateMethods(superMethods, classMethods);
    if (toOverride.empty()) {
        return;
    }
    std::string insertText;
    for (auto *m : toOverride) {
        if (m->Key()->IsIdentifier()) {
            insertText += CreateOverrideText(m);
        }
    }
    if (insertText.empty()) {
        return;
    }
    tracker.InsertText(ctx->sourceFile, cur->Start().index + 1, "\n" + insertText);
}

RefactorEditInfo GetRefactorEditsToOverrideMethods(const RefactorContext &context)
{
    std::vector<FileTextChanges> edits;
    TextChangesContext textChangesContext = *context.textChangesContext;
    edits = ChangeTracker::With(textChangesContext,
                                [&](ChangeTracker &tracker) { GenerateAllOverrideMethods(context, tracker); });

    RefactorEditInfo refactorEdits(std::move(edits));
    return refactorEdits;
}

std::unique_ptr<RefactorEditInfo> GenerateOverrideMethods::GetEditsForAction(const RefactorContext &context,
                                                                             const std::string &actionName) const
{
    if ((context.context == nullptr) || (actionName != TO_GENERATE_OVERRIDE_METHODS_ACTION.name)) {
        return nullptr;
    }
    auto edits = GetRefactorEditsToOverrideMethods(context);
    if (!edits.GetFileTextChanges().empty()) {
        return std::make_unique<RefactorEditInfo>(std::move(edits));
    }
    return std::make_unique<RefactorEditInfo>();
}

// NOLINTNEXTLINE(fuchsia-statically-constructed-objects, cert-err58-cpp)
AutoRefactorRegister<GenerateOverrideMethods> g_generateOverrideMethodsRegister("GenerateOverrideMethods");

}  // namespace ark::es2panda::lsp

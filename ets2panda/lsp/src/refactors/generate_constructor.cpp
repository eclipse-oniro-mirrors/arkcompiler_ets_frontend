/**
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at*
 *
 * http://www.apache.org/licenses/LICENSE-2.0*
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "lsp/include/refactors/generate_constructor.h"
#include <iostream>
#include <string>
#include "internal_api.h"
#include "refactor_provider.h"
#include "services/text_change/change_tracker.h"
#include "quick_info.h"

#include "public/public.h"

namespace ark::es2panda::lsp {

GenerateConstructorRefactor::GenerateConstructorRefactor()
{
    AddKind(std::string(TO_GENERATE_CONSTRUCTOR_ACTION.kind));
}

struct PropertyInfo {
    PropertyInfo(std::string_view n, std::string_view t) : name {n}, type {t} {}

    const std::string &GetName() const
    {
        return name;
    }
    const std::string &GetType() const
    {
        return type;
    }
    void SetName(const std::string &n)
    {
        name = n;
    }
    void SetType(const std::string &t)
    {
        type = t;
    }

private:
    std::string name;
    std::string type;
};

static std::string GetParamName(std::string_view name)
{
    if (!name.empty() && name[0] == '_') {
        return std::string(name.substr(1));
    }
    return std::string(name);
}

static const ir::Identifier *GetSuperClassIdent(const ir::AstNode *node)
{
    if (node == nullptr || !node->IsClassDefinition()) {
        return nullptr;
    }

    const auto *cls = node->AsClassDefinition();
    const auto *sup = cls->Super();
    if (sup == nullptr || !sup->IsETSTypeReference()) {
        return nullptr;
    }

    const auto *typeRef = sup->AsETSTypeReference();
    const auto *part = typeRef->Part();
    if (part == nullptr || !part->IsETSTypeReferencePart()) {
        return nullptr;
    }

    const auto *name = part->AsETSTypeReferencePart()->Name();
    if (name == nullptr || !name->IsIdentifier()) {
        return nullptr;
    }

    return name->AsIdentifier();
}

static std::vector<PropertyInfo> CollectClassProperties(ir::AstNode *node)
{
    std::vector<PropertyInfo> properties;
    if (node == nullptr || !node->IsClassDefinition()) {
        return properties;
    }

    const auto *cls = node->AsClassDefinition();
    properties.reserve(cls->Body().size());

    for (const auto *member : cls->Body()) {
        if (member == nullptr || !member->IsClassProperty()) {
            continue;
        }
        if (member->IsStatic() || member->IsAbstract()) {
            continue;
        }

        const auto *prop = member->AsClassProperty();
        const auto *key = prop->Key();
        if (key == nullptr || !key->IsIdentifier()) {
            continue;
        }

        const auto *ann = prop->TypeAnnotation();
        if (ann == nullptr || !ann->IsETSTypeReference()) {
            continue;
        }

        const auto *typeRef = ann->AsETSTypeReference();
        const auto *part = typeRef->Part();
        if (part == nullptr || !part->IsETSTypeReferencePart()) {
            continue;
        }

        const auto *typeName = part->AsETSTypeReferencePart()->Name();
        if (typeName == nullptr || !typeName->IsIdentifier()) {
            continue;
        }

        properties.emplace_back(key->AsIdentifier()->Name().Utf8(), typeName->AsIdentifier()->Name().Utf8());
    }
    return properties;
}

static const ir::ClassDefinition *FindBaseClassByName(const public_lib::Context *ctx, std::string_view baseName)
{
    if (ctx == nullptr || ctx->parserProgram == nullptr) {
        return nullptr;
    }
    auto *found = ctx->parserProgram->Ast()->FindChild([&](ir::AstNode *n) {
        return n != nullptr && n->IsClassDefinition() && n->AsClassDefinition()->Ident() != nullptr &&
               n->AsClassDefinition()->Ident()->Name().Utf8() == baseName;
    });
    return (found != nullptr && found->IsClassDefinition()) ? found->AsClassDefinition() : nullptr;
}

static const ir::ScriptFunction *GetFirstCtorFunction(const ir::ClassDefinition *baseCls)
{
    if (baseCls == nullptr) {
        return nullptr;
    }
    for (const auto *member : baseCls->Body()) {
        if (member == nullptr || !member->IsConstructor()) {
            continue;
        }

        const auto *md = member->AsMethodDefinition();
        if (md == nullptr) {
            continue;
        }

        const auto *val = md->Value();
        if (val == nullptr || !val->IsFunctionExpression()) {
            continue;
        }

        return val->AsFunctionExpression()->Function();
    }
    return nullptr;
}

static bool TryExtractParam(const ir::ETSParameterExpression *p, std::string &outName, std::string &outType)
{
    if (p == nullptr) {
        return false;
    }

    const auto *ident = p->Ident();
    if (ident == nullptr || !ident->IsIdentifier()) {
        return false;
    }
    outName = ident->AsIdentifier()->Name().Utf8();

    outType = "any";
    const auto *t = p->TypeAnnotation();
    if (t != nullptr && t->IsETSTypeReference()) {
        const auto *tr = t->AsETSTypeReference();
        const auto *pr = tr->Part();
        if (pr != nullptr && pr->IsETSTypeReferencePart()) {
            const auto *tn = pr->AsETSTypeReferencePart()->Name();
            if (tn != nullptr && tn->IsIdentifier()) {
                outType = tn->AsIdentifier()->Name().Utf8();
            }
        }
    }
    return true;
}

static std::vector<PropertyInfo> CollectSuperClassConstructorParameters(const ir::AstNode *node,
                                                                        const public_lib::Context *ctx)
{
    std::vector<PropertyInfo> out;

    if (node == nullptr || !node->IsClassDefinition() || ctx == nullptr || ctx->parserProgram == nullptr) {
        return out;
    }

    const auto *superId = GetSuperClassIdent(node);
    if (superId == nullptr) {
        return out;
    }

    const std::string baseName = std::string(superId->Name().Utf8());
    const auto *baseCls = FindBaseClassByName(ctx, baseName);
    if (baseCls == nullptr) {
        return out;
    }

    const auto *fun = GetFirstCtorFunction(baseCls);
    if (fun == nullptr) {
        return out;
    }

    for (const auto *param : fun->Params()) {
        if (param == nullptr || !param->IsETSParameterExpression()) {
            continue;
        }
        const auto *pe = param->AsETSParameterExpression();

        std::string name {};
        std::string type {};
        if (!TryExtractParam(pe, name, type)) {
            continue;
        }

        out.emplace_back(name, type);
    }
    return out;
}

static std::string GenerateConstructorSignature(const std::vector<PropertyInfo> &superClassProps,
                                                const std::vector<PropertyInfo> &properties)
{
    std::ostringstream signature;
    signature << "constructor(";

    bool first = true;
    for (const auto &prop : superClassProps) {
        if (!first) {
            signature << ", ";
        }
        signature << GetParamName(prop.GetName()) << ": " << prop.GetType();
        first = false;
    }
    for (const auto &prop : properties) {
        if (!first) {
            signature << ", ";
        }
        signature << GetParamName(prop.GetName()) << ": " << prop.GetType();
        first = false;
    }

    signature << ")";
    return signature.str();
}

static std::string GenerateConstructorBody(const std::vector<PropertyInfo> &superClassProps,
                                           const std::vector<PropertyInfo> &properties, bool hasBaseClass)
{
    std::ostringstream body;

    if (hasBaseClass) {
        body << "        super(";
        for (size_t i = 0; i < superClassProps.size(); ++i) {
            body << GetParamName(superClassProps[i].GetName());
            if (i + 1 < superClassProps.size()) {
                body << ", ";
            }
        }
        body << ")\n";
    }

    for (const auto &prop : properties) {
        body << "        this." << prop.GetName() << " = " << GetParamName(prop.GetName()) << "\n";
    }

    return body.str();
}

void GenerateCtors(const RefactorContext &context, ChangeTracker &tracker)
{
    auto *baseCtx = context.context;
    if (baseCtx == nullptr || context.textChangesContext == nullptr) {
        return;
    }

    auto *ctx = reinterpret_cast<const public_lib::Context *>(baseCtx);
    if (ctx == nullptr || ctx->sourceFile == nullptr) {
        return;
    }

    auto *cur = GetTouchingToken(baseCtx, context.span.pos, false);
    while (cur != nullptr && !IsClass(cur)) {
        cur = cur->Parent();
    }
    if (cur == nullptr || !cur->IsClassDefinition()) {
        return;
    }

    auto *cls = cur->AsClassDefinition();

    std::vector<PropertyInfo> superClassProps;
    if (cls->Super() != nullptr) {
        superClassProps = CollectSuperClassConstructorParameters(cls, ctx);
    }

    auto classProps = CollectClassProperties(cls);
    const bool hasBaseClass = (cls->Super() != nullptr);

    std::ostringstream insertText {};
    insertText << GenerateConstructorSignature(superClassProps, classProps) << " {\n"
               << GenerateConstructorBody(superClassProps, classProps, hasBaseClass) << "    }\n";

    tracker.InsertText(ctx->sourceFile, cur->Start().index + 1, "\n    " + insertText.str());
}

std::vector<ApplicableRefactorInfo> GenerateConstructorRefactor::GetAvailableActions(
    const RefactorContext &context) const
{
    ApplicableRefactorInfo applicableRef;
    std::vector<ApplicableRefactorInfo> res;

    if (context.context == nullptr) {
        return {};
    }

    if (!context.kind.empty() && context.kind != TO_GENERATE_CONSTRUCTOR_ACTION.kind) {
        return {};
    }

    auto *cur = GetTouchingToken(context.context, context.span.pos, false);
    while (cur != nullptr && !IsClass(cur)) {
        cur = cur->Parent();
    }

    if (cur == nullptr || !cur->IsClassDefinition()) {
        return {};
    }

    auto *cls = cur->AsClassDefinition();

    const auto *ctx = reinterpret_cast<const public_lib::Context *>(context.context);
    if (!CollectClassProperties(cls).empty() ||
        (cls->Super() != nullptr && !CollectSuperClassConstructorParameters(cls, ctx).empty())) {
        applicableRef.name = refactor_name::GENERATE_CONSTRUCTOR_REFACTOR_NAME;
        applicableRef.description = refactor_description::GENERATE_CONSTRUCTOR_REFACTOR_DESC;
        applicableRef.action.kind = std::string(TO_GENERATE_CONSTRUCTOR_ACTION.kind);
        applicableRef.action.name = std::string(TO_GENERATE_CONSTRUCTOR_ACTION.name);
        applicableRef.action.description = std::string(TO_GENERATE_CONSTRUCTOR_ACTION.description);
        res.push_back(applicableRef);
        return res;
    }

    return {};
}

std::unique_ptr<RefactorEditInfo> GenerateConstructorRefactor::GetEditsForAction(const RefactorContext &context,
                                                                                 const std::string &actionName) const
{
    if (context.context == nullptr || context.textChangesContext == nullptr ||
        actionName != TO_GENERATE_CONSTRUCTOR_ACTION.name) {
        return nullptr;
    }

    std::vector<FileTextChanges> edits;
    TextChangesContext textChangesContext = *context.textChangesContext;
    edits = ChangeTracker::With(textChangesContext, [&](ChangeTracker &tracker) { GenerateCtors(context, tracker); });

    RefactorEditInfo refactorEdits(std::move(edits));
    if (!refactorEdits.GetFileTextChanges().empty()) {
        return std::make_unique<RefactorEditInfo>(std::move(refactorEdits));
    }
    return std::make_unique<RefactorEditInfo>();
}

// NOLINTNEXTLINE(fuchsia-statically-constructed-objects, cert-err58-cpp)
AutoRefactorRegister<GenerateConstructorRefactor> g_generateConstructorRegister("Generate constructor");

}  // namespace ark::es2panda::lsp
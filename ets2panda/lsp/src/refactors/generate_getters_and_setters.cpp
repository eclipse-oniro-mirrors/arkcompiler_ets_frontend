/**
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include <string>
#include <memory>
#include <utility>
#include "refactors/generate_getters_and_setters.h"
#include "refactors/refactor_types.h"
#include "services/text_change/change_tracker.h"
#include "services/text_change/text_change_context.h"
#include "refactor_provider.h"
#include "internal_api.h"
#include "parser/program/program.h"
#include "public/es2panda_lib.h"

namespace ark::es2panda::lsp {

// Check if the node is a class-like declaration
inline bool IsClassLike(ir::AstNode *n)
{
    return n != nullptr && (n->IsClassDeclaration() || n->IsClassDefinition() || n->IsClassExpression());
}

// Check if the node represents a class field
bool IsClassFieldNode(ir::AstNode *n)
{
    if (n == nullptr) {
        return false;
    }

    if (n->IsProperty() || n->IsClassProperty()) {
        return true;
    }

    if (n->IsIdentifier()) {
        auto *p = n->Parent();
        if (p != nullptr && (p->IsProperty() || p->IsClassProperty())) {
            return true;
        }
    }

    if (n->IsVariableDeclarator()) {
        auto *par = n->Parent();
        while (par != nullptr && !IsClassLike(par)) {
            par = par->Parent();
        }
        if (IsClassLike(par)) {
            return true;
        }
    }

    return false;
}

// Find the enclosing class node
inline ir::AstNode *FindEnclosingClass(ir::AstNode *n)
{
    return FindAncestor(n, [](ir::AstNode *a) { return IsClassLike(a); });
}

// Extract field name and type annotation (if available)
std::pair<std::string, std::string> ExtractFieldNameAndType(ir::AstNode *fieldOrChild)
{
    std::string name;
    std::string typeText;

    ir::AstNode *field = fieldOrChild;
    if (fieldOrChild->IsIdentifier() && fieldOrChild->Parent() != nullptr) {
        field = fieldOrChild->Parent();
    }

    bool gotName = field->FindChild([&](ir::AstNode *c) {  // NOLINT (readability-implicit-bool-conversion)
        if (c->IsIdentifier()) {
            name = c->AsIdentifier()->Name().Utf8();
            return true;
        }
        return false;
    });
    auto parent = field->Parent();
    if (gotName) {
        (void)parent->FindChild([&](ir::AstNode *c) {
            if (c->IsETSTypeReferencePart()) {
                typeText = ": " + std::string(c->AsETSTypeReferencePart()->GetIdent()->Name());
                return true;
            }
            return false;
        });
    }

    return {name, typeText};
}

// Position before the closing brace of the class body
inline size_t ClassBodyInsertPos(ir::AstNode *cls)
{
    return cls != nullptr ? (cls->End().index - 1) : 0;
}

// Plan getter and setter names to avoid clashes
struct AccessorPlan {
    std::string getterName;
    std::string setterName;
    std::string backingName;
    bool namesClashAvoided {false};
};

AccessorPlan PlanFor(const std::string &fieldName)
{
    AccessorPlan p;
    if (!fieldName.empty() && fieldName[0] == '_') {
        p.getterName = fieldName.substr(1);
        p.setterName = fieldName.substr(1);
        p.backingName = fieldName;
    } else {
        p.getterName = "get_" + fieldName;
        p.setterName = "set_" + fieldName;
        p.backingName = fieldName;
        p.namesClashAvoided = true;
    }
    return p;
}

// Build getter and setter code for ArkTS
std::string BuildAccessorsArkTS(const AccessorPlan &plan, const std::string &maybeType)
{
    std::string typeAnnRet;
    std::string typeAnnParam;
    if (!maybeType.empty()) {
        typeAnnRet = maybeType;
        typeAnnParam = maybeType;
    }

    std::string nl = "\n";
    std::string code;
    code += nl;
    code += "  get " + plan.getterName + "()" + typeAnnRet + " { return this." + plan.backingName + "; }" + nl;
    code += "  set " + plan.setterName + "(value" + typeAnnParam + ") { this." + plan.backingName + " = value; }" + nl;
    if (plan.namesClashAvoided) {
        code += "  // NOTE: Backing field and accessor names differ to avoid recursion/name clash." + nl;
    }
    return code;
}

GenerateGettersAndSettersRefactor::GenerateGettersAndSettersRefactor()
{
    AddKind(std::string(GENERATE_GETTERS_AND_SETTERS_ACTION.kind));
}

std::vector<ApplicableRefactorInfo> GenerateGettersAndSettersRefactor::GetAvailableActions(
    const RefactorContext &refContext) const
{
    std::vector<ApplicableRefactorInfo> res;

    if (!IsKind(refContext.kind)) {
        return res;
    }

    es2panda_Context *ctx = refContext.context;
    size_t pos = refContext.span.pos;

    auto *token = GetTouchingToken(ctx, pos, false);
    if (token == nullptr) {
        return res;
    }

    if (!IsClassFieldNode(token) && !((token->Parent() != nullptr) && IsClassFieldNode(token->Parent()))) {
        return res;
    }

    auto *cls = FindEnclosingClass(token);
    if (cls == nullptr) {
        return res;
    }

    ApplicableRefactorInfo info;
    info.name = refactor_name::GENERATE_GETTERS_AND_SETTERS_REFACTOR_NAME;
    info.description = std::string(GENERATE_GETTERS_AND_SETTERS_ACTION.description);
    info.action.kind = std::string(GENERATE_GETTERS_AND_SETTERS_ACTION.kind);
    info.action.name = std::string(GENERATE_GETTERS_AND_SETTERS_ACTION.name);
    info.action.description = std::string(GENERATE_GETTERS_AND_SETTERS_ACTION.description);

    res.push_back(std::move(info));

    return res;
}

std::unique_ptr<RefactorEditInfo> GenerateGettersAndSettersRefactor::GetEditsForAction(
    const RefactorContext &refContext, const std::string &actionName) const
{
    auto edits = std::make_unique<RefactorEditInfo>();

    if (actionName != GENERATE_GETTERS_AND_SETTERS_ACTION.name) {
        return edits;
    }

    es2panda_Context *raw = refContext.context;
    auto *ctxContent = reinterpret_cast<public_lib::Context *>(raw);
    if (ctxContent == nullptr) {
        return edits;
    }

    size_t pos = refContext.span.pos;
    auto *token = GetTouchingToken(raw, pos, false);
    if (token == nullptr) {
        return edits;
    }

    ir::AstNode *fieldNode =
        IsClassFieldNode(token)
            ? token
            : ((token->Parent() != nullptr) && IsClassFieldNode(token->Parent()) ? token->Parent() : nullptr);
    if (fieldNode == nullptr) {
        return edits;
    }
    auto *cls = FindEnclosingClass(fieldNode);
    if (cls == nullptr) {
        return edits;
    }

    auto [fieldName, typePlaceholder] = ExtractFieldNameAndType(fieldNode);
    if (fieldName.empty()) {
        return edits;
    }

    auto plan = PlanFor(fieldName);
    std::string accessors = BuildAccessorsArkTS(plan, typePlaceholder);

    TextChangesContext tcc = *refContext.textChangesContext;
    auto changes = ChangeTracker::With(tcc, [&](ChangeTracker &tracker) {
        size_t insertPos = ClassBodyInsertPos(cls);
        tracker.ReplaceRangeWithText(ctxContent->sourceFile, {insertPos, insertPos}, accessors);
    });

    edits->SetFileTextChanges(changes);
    return edits;
}

// NOLINTNEXTLINE(fuchsia-statically-constructed-objects, cert-err58-cpp, readability-identifier-naming)
AutoRefactorRegister<GenerateGettersAndSettersRefactor> g_GenerateGettersAndSettersRefactorRegister(
    "GenerateGettersAndSettersRefactor");

}  // namespace ark::es2panda::lsp

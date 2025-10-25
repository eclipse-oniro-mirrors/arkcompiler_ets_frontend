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

#include <string>
#include <vector>
#include <cctype>
#include <sstream>
#include <string_view>
#include "ir/astNode.h"
#include "ir/expression.h"
#include "public/es2panda_lib.h"
#include "lsp/include/internal_api.h"
#include "ir/expressions/identifier.h"
#include "ir/expressions/functionExpression.h"
#include "ir/expressions/arrowFunctionExpression.h"
#include "ir/statements/functionDeclaration.h"
#include "ir/statements/variableDeclaration.h"
#include "lsp/include/refactors/refactor_types.h"
#include "lsp/include/services/text_change/change_tracker.h"
#include "lsp/include/refactors/convert_function_to_class.h"
#include "refactor_provider.h"

/**
 * @file convert_function_to_class.cpp
 * @brief Implements the ConvertFunctionToClassRefactor.
 *
 * This refactor identifies function declarations, arrow functions,
 * and function expressions, then offers an LSP code action
 * `"Convert to Class"`. When applied, it replaces the function text
 * with a `class` declaration containing a constructor that reproduces
 * the original parameters and body.
 *
 * ### Supported patterns
 * - `function greet(x: number) { return x + 1; }`
 * - `const Foo = (a, b) => a + b;`
 * - `const Foo = () => { ... }`
 *
 * ### Not supported
 * - Anonymous function expressions without a variable binding
 * - Function expressions if the ArkTS parser rejects them
 *
 * @see ConvertFunctionToClassRefactor
 * @see ApplicableRefactorInfo
 */
namespace ark::es2panda::lsp {
namespace {

struct TargetInfo {
    ir::AstNode *node {nullptr};
    ir::Identifier *name {nullptr};
    bool isFromVarDecl {false};
};

struct FnShape {
    std::string name;
    std::string paramsText;
    std::string bodyText;
    bool bodyIsExpression {false};
};

static bool SelectionInsideIdent(size_t start, size_t end, ir::Identifier *ident)
{
    if (ident == nullptr) {
        return false;
    }
    const auto r = ident->Range();
    const size_t begin = r.start.index;
    const size_t finish = r.end.index;
    if (start < begin) {
        return false;
    }
    if (end > finish) {
        return false;
    }
    return true;
}

static ir::AstNode *FindEnclosingFunctionLike(ir::AstNode *n)
{
    ir::AstNode *cur = n;
    while (cur != nullptr) {
        if (cur->IsFunctionDeclaration() || cur->IsArrowFunctionExpression() || cur->IsFunctionExpression()) {
            break;
        }
        cur = cur->Parent();
    }
    return cur;
}

static ir::VariableDeclaration *FindEnclosingVarDecl(ir::AstNode *n)
{
    ir::AstNode *cur = n;
    while (cur != nullptr && !cur->IsVariableDeclaration()) {
        cur = cur->Parent();
    }
    if (cur == nullptr) {
        return nullptr;
    }
    return cur->AsVariableDeclaration();
}

static bool CanConvertToClass(ir::AstNode *fnNode)
{
    if (fnNode == nullptr) {
        return false;
    }
    if (fnNode->IsFunctionDeclaration()) {
        return true;
    }
    if (fnNode->IsFunctionExpression()) {
        return true;
    }
    if (fnNode->IsArrowFunctionExpression()) {
        return true;
    }
    return false;
}

static ir::Identifier *GetIdFromFunctionDecl(ir::AstNode *fnNode)
{
    auto *func = fnNode->AsFunctionDeclaration()->Function();
    if (func == nullptr || func->Id() == nullptr || !func->Id()->IsIdentifier()) {
        return nullptr;
    }
    return func->Id()->AsIdentifier();
}

static ir::Identifier *GetIdFromFunctionExpr(ir::AstNode *fnNode)
{
    auto *func = fnNode->AsFunctionExpression()->Function();
    if (func == nullptr) {
        return nullptr;
    }
    auto *id = func->Id();
    return (id != nullptr && id->IsIdentifier()) ? id->AsIdentifier() : nullptr;
}

static ir::Identifier *GetIdFromVariableBinding(ir::AstNode *fnNode)
{
    ir::AstNode *p = fnNode->Parent();
    while (p != nullptr && !p->IsVariableDeclaration()) {
        p = p->Parent();
    }
    if (p == nullptr) {
        return nullptr;
    }

    for (auto *d : p->AsVariableDeclaration()->Declarators()) {
        if (d != nullptr && d->Init() == fnNode) {
            auto *name = d->Id();
            if (name != nullptr && name->IsIdentifier()) {
                return name->AsIdentifier();
            }
            return nullptr;
        }
    }
    return nullptr;
}

static ir::Identifier *FindNameForFunctionLike(ir::AstNode *fnNode)
{
    if (fnNode == nullptr) {
        return nullptr;
    }

    if (fnNode->IsFunctionDeclaration()) {
        return GetIdFromFunctionDecl(fnNode);
    }

    if (fnNode->IsFunctionExpression()) {
        if (auto *id = GetIdFromFunctionExpr(fnNode)) {
            return id;
        }
        return GetIdFromVariableBinding(fnNode);
    }

    if (fnNode->IsArrowFunctionExpression()) {
        return GetIdFromVariableBinding(fnNode);
    }

    return nullptr;
}

static ir::Expression *TryGetFunctionInitFromVarDecl(ir::VariableDeclaration *vardecl, const TextRange &span,
                                                     ir::Identifier **outName)
{
    if (vardecl == nullptr) {
        return nullptr;
    }
    auto &decls = vardecl->Declarators();
    if (decls.empty()) {
        return nullptr;
    }

    for (auto *d : decls) {
        if (d == nullptr) {
            continue;
        }
        auto *id = d->Id();
        if (id == nullptr || !id->IsIdentifier()) {
            continue;
        }
        auto *ident = id->AsIdentifier();
        if (!SelectionInsideIdent(span.pos, span.end, ident)) {
            continue;
        }

        auto *init = d->Init();
        if (init == nullptr) {
            continue;
        }
        if (init->IsFunctionExpression() || init->IsArrowFunctionExpression()) {
            if (outName != nullptr) {
                *outName = ident;
            }
            return init->AsExpression();
        }
    }

    return nullptr;
}

static es2panda_AstNode *GetScriptFunction(es2panda_Context *ctx, ir::AstNode *fnNode)
{
    if (fnNode == nullptr) {
        return nullptr;
    }
    const auto impl = es2panda_GetImpl(ES2PANDA_LIB_VERSION);
    if (fnNode->IsArrowFunctionExpression()) {
        return impl->ArrowFunctionExpressionFunction(ctx, reinterpret_cast<es2panda_AstNode *>(fnNode));
    }
    if (fnNode->IsFunctionExpression()) {
        return reinterpret_cast<es2panda_AstNode *>(fnNode->AsFunctionExpression()->Function());
    }
    if (fnNode->IsFunctionDeclaration()) {
        return reinterpret_cast<es2panda_AstNode *>(fnNode->AsFunctionDeclaration()->Function());
    }
    return nullptr;
}

static std::string SafeNameFromIdent(ir::Identifier *nameIdent)
{
    if (nameIdent == nullptr) {
        return "ConvertedClass";
    }
    std::string_view sv = nameIdent->Name().Utf8();
    if (!sv.empty()) {
        return std::string(sv);
    }
    return "ConvertedClass";
}

static std::string BuildParamsText(es2panda_Context *ctx, es2panda_AstNode *scriptFunc)
{
    const auto impl = es2panda_GetImpl(ES2PANDA_LIB_VERSION);
    size_t paramCount = 0;
    auto **params = impl->ScriptFunctionParams(ctx, scriptFunc, &paramCount);
    std::ostringstream oss;
    for (size_t i = 0; i < paramCount; ++i) {
        if (params == nullptr || params[i] == nullptr) {
            continue;
        }
        auto *p = reinterpret_cast<ir::AstNode *>(params[i]);
        if (i != 0) {
            oss << ", ";
        }
        oss << p->DumpEtsSrc();
    }
    return oss.str();
}

static std::string TrimView(std::string_view v)
{
    const size_t first = v.find_first_not_of(" \t\r\n");
    const size_t last = v.find_last_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return std::string();
    }
    return std::string(v.substr(first, last - first + 1));
}

static bool SliceBlockInnerFromFile(const SourceFile *sf, const ir::AstNode *bodyNode, std::string *out)
{
    if (sf == nullptr || out == nullptr) {
        return false;
    }
    if (sf->source.empty()) {
        return false;
    }

    std::string_view fileText = sf->source;
    const size_t bs = bodyNode->Start().index;
    const size_t be = bodyNode->End().index;
    if (!(bs < be && be <= fileText.size())) {
        return false;
    }
    if (fileText[bs] != '{') {
        return false;
    }

    const size_t innerStart = bs + 1;
    const size_t innerEnd = be - 1;  // ‘}’ position
    const size_t len = (innerEnd > innerStart) ? (innerEnd - innerStart) : 0;
    std::string_view inner = fileText.substr(innerStart, len);
    *out = TrimView(inner);
    return true;
}

static bool ExtractBlockBodyText(es2panda_Context *ctx, es2panda_AstNode *body, std::string *out)
{
    if (out == nullptr) {
        return false;
    }

    auto *pub = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    const SourceFile *sf = (pub != nullptr) ? pub->sourceFile : nullptr;
    auto *bodyNode = reinterpret_cast<ir::AstNode *>(body);

    if (SliceBlockInnerFromFile(sf, bodyNode, out)) {
        return true;
    }

    // Fallback to DumpEtsSrc if direct slice failed
    constexpr size_t bracePair = 2;
    std::string dumped = bodyNode->DumpEtsSrc();
    if (!dumped.empty() && dumped.front() == '{' && dumped.back() == '}') {
        *out = TrimView(std::string_view(dumped).substr(1, dumped.size() - bracePair));
    } else {
        *out = TrimView(dumped);
    }
    return true;
}

static bool TryExtractBlockBody(es2panda_Context *ctx, es2panda_AstNode *body, std::string *out)
{
    if (out == nullptr) {
        return false;
    }

    const auto impl = es2panda_GetImpl(ES2PANDA_LIB_VERSION);
    if (!impl->IsBlockStatement(body)) {
        return false;
    }

    return ExtractBlockBodyText(ctx, body, out);
}

static bool TryExtractExprBody(es2panda_AstNode *body, std::string *out)
{
    if (out == nullptr) {
        return false;
    }

    const auto impl = es2panda_GetImpl(ES2PANDA_LIB_VERSION);
    if (!impl->IsExpression(body)) {
        return false;
    }

    auto *exprNode = reinterpret_cast<ir::AstNode *>(body);
    *out = exprNode->DumpEtsSrc();
    return true;
}

static FnShape ExtractFunctionShape(es2panda_Context *ctx, ir::AstNode *fnNode, ir::Identifier *nameIdent)
{
    FnShape out;
    out.name = SafeNameFromIdent(nameIdent);

    es2panda_AstNode *scriptFunc = GetScriptFunction(ctx, fnNode);
    if (scriptFunc == nullptr) {
        return out;
    }

    out.paramsText = BuildParamsText(ctx, scriptFunc);

    const auto impl = es2panda_GetImpl(ES2PANDA_LIB_VERSION);
    es2panda_AstNode *body = impl->ScriptFunctionBody(ctx, scriptFunc);
    if (body == nullptr) {
        return out;
    }

    std::string text;
    if (TryExtractBlockBody(ctx, body, &text)) {
        out.bodyText = text;
        out.bodyIsExpression = false;
        return out;
    }

    if (TryExtractExprBody(body, &text)) {
        out.bodyText = text;
        out.bodyIsExpression = true;
        return out;
    }

    return out;
}

static std::string BuildClassText(const FnShape &shape)
{
    const std::string &clsName = !shape.name.empty() ? shape.name : std::string("ConvertedClass");

    std::ostringstream oss;
    oss << "class " << clsName << " {\n";
    oss << "  constructor(" << shape.paramsText << ") {\n";
    if (shape.bodyIsExpression) {
        oss << "    return " << shape.bodyText << ";\n";
    } else {
        if (!shape.bodyText.empty()) {
            oss << shape.bodyText << "\n";
        }
    }
    oss << "  }\n";
    oss << "}\n";
    return oss.str();
}

static ir::AstNode *PickEditBoundary(ir::AstNode *n)
{
    if (n == nullptr) {
        return nullptr;
    }
    ir::AstNode *b = ChangeTracker::ToEditBoundary(n);
    if (b != nullptr) {
        return b;
    }
    return n;
}

static TargetInfo ResolveTarget(es2panda_Context *ctx, const TextRange &span)
{
    TargetInfo out {};
    ir::AstNode *token = GetTouchingToken(ctx, span.pos, false);
    if (token == nullptr) {
        return out;
    }

    ir::VariableDeclaration *vardecl = FindEnclosingVarDecl(token);
    if (vardecl != nullptr) {
        ir::Identifier *ident = nullptr;
        ir::Expression *init = TryGetFunctionInitFromVarDecl(vardecl, span, &ident);
        if (init != nullptr && ident != nullptr) {
            if (CanConvertToClass(init)) {
                out.node = init;
                out.name = ident;
                out.isFromVarDecl = true;
            }
        }
    }

    if (out.node == nullptr) {
        ir::AstNode *func = FindEnclosingFunctionLike(token);
        if (func != nullptr) {
            ir::Identifier *name = FindNameForFunctionLike(func);
            bool inside = (name != nullptr) && SelectionInsideIdent(span.pos, span.end, name);
            bool ok = CanConvertToClass(func);
            if (inside && ok) {
                out.node = func;
                out.name = name;
                out.isFromVarDecl = func->IsArrowFunctionExpression();
            }
        }
    }

    return out;
}

/**
 * @brief Core implementation that generates the text edits for `"Convert to Class"`.
 *
 * Given a resolved function-like node and its identifier, this routine:
 * 1. Extracts the function's "shape" (name, parameters, body).
 * 2. Builds a `class` declaration string with a constructor that reproduces
 *    the original signature and body.
 * 3. Replaces the entire AST node (or its edit boundary) with the new class text.
 *
 * ### Edit construction
 * - The edit span covers the full source range of the target node.
 * - The replacement text is the newly generated `class` declaration.
 *
 * @param context   Refactor context containing the es2panda context and user preferences.
 * @param fnNode    Function-like AST node (function declaration, arrow function, or function expression).
 * @param nameIdent Identifier node representing the function's name (used for class name).
 *
 * @return std::vector<FileTextChanges>
 *         A collection with one `FileTextChanges` entry containing a single `TextChange`
 *         that replaces the original function with the class.
 *         If input is invalid, the returned vector is empty.
 *
 * @see ExtractFunctionShape
 * @see BuildClassText
 * @see PickEditBoundary
 */
static std::vector<FileTextChanges> DoConvertToClassInternal(const RefactorContext &context, ir::AstNode *fnNode,
                                                             ir::Identifier *nameIdent)
{
    std::vector<FileTextChanges> out;
    if (fnNode == nullptr || nameIdent == nullptr || context.context == nullptr) {
        return out;
    }

    FnShape shape = ExtractFunctionShape(context.context, fnNode, nameIdent);
    const std::string classText = BuildClassText(shape);
    ir::AstNode *boundary = PickEditBoundary(fnNode);
    if (boundary == nullptr) {
        boundary = fnNode;
    }

    const size_t start = boundary->Start().index;
    const size_t end = boundary->End().index;
    const size_t length = (end > start) ? (end - start) : 0;
    TextSpan span(start, length);
    TextChange change(span, classText);

    FileTextChanges ftc;
    ftc.textChanges.push_back(std::move(change));
    out.push_back(std::move(ftc));

    return out;
}
}  // namespace

ConvertFunctionToClassRefactor::ConvertFunctionToClassRefactor()
{
    AddKind(std::string(CONVERT_TO_CLASS_ACTION.kind));
}

/**
 * @brief Determines whether the `"Convert to Class"` refactor is available.
 *
 * Examines the cursor/selection span in the given context. If it lies on the
 * identifier of a supported function-like construct, the action will be
 * offered. Otherwise, the result is empty.
 *
 * ### Offers action on
 * - Arrow functions assigned to a `const` variable (`const Foo = (x, y) => ...;`)
 * - Function declarations (`function greet(x: number) { ... }`)
 * - Identifier selections (single-caret or range covering the name)
 *
 * ### Does *not* offer action on
 * - Non-function variable initializers (`const Foo = 123;`)
 * - Selections inside the function/initializer body (`const Foo = () => 42;`)
 * - Whitespace or tokens outside identifiers
 *
 * @param refContext Refactor context including AST, cursor span, and kind filter.
 * @return ApplicableRefactorInfo describing the available action, or empty if not applicable.
 *
 * @see ConvertFunctionToClassRefactor::GetEditsForAction
 */
std::vector<ApplicableRefactorInfo> ConvertFunctionToClassRefactor::GetAvailableActions(
    const RefactorContext &refContext) const
{
    ApplicableRefactorInfo applicableRef;
    std::vector<ApplicableRefactorInfo> res;
    const auto target = ResolveTarget(refContext.context, refContext.span);
    if (target.node == nullptr || target.name == nullptr) {
        return res;
    }
    applicableRef.name = std::string(refactor_name::CONVERT_FUNCTION_TO_CLASS_NAME);
    applicableRef.description = std::string(refactor_description::CONVERT_FUNCTION_TO_CLASS_DESC);
    applicableRef.action.name = std::string(CONVERT_TO_CLASS_ACTION.name);
    applicableRef.action.description = std::string(CONVERT_TO_CLASS_ACTION.description);
    applicableRef.action.kind = std::string(CONVERT_TO_CLASS_ACTION.kind);
    res.push_back(applicableRef);
    return res;
}

/**
 * @brief Produces text edits to replace the selected function with a class declaration.
 *
 * If the action name matches `"Convert to Class"` and the context resolves to a valid
 * function-like node, this method generates edits that remove the function and insert
 * a `class` with a constructor reproducing its parameters and body.
 *
 * ### Conversion rules
 * - Concise arrow bodies → `return <expr>;` in the constructor
 * - Block arrow bodies → body contents copied verbatim (without outer braces)
 * - Function declarations → class named after the function
 *
 * @param context    Refactor context with cursor span and preferences.
 * @param actionName Action name; must match `"Convert to Class"` or be empty.
 * @return Non-null RefactorEditInfo containing one or more edits, or `nullptr` if no target was found.
 *
 * @see ConvertFunctionToClassRefactor::GetAvailableActions
 */
std::unique_ptr<RefactorEditInfo> ConvertFunctionToClassRefactor::GetEditsForAction(const RefactorContext &context,
                                                                                    const std::string &actionName) const
{
    if (!actionName.empty() && actionName != std::string(CONVERT_TO_CLASS_ACTION.name)) {
        return nullptr;
    }

    const auto target = ResolveTarget(context.context, context.span);
    if (target.node == nullptr || target.name == nullptr) {
        return nullptr;
    }

    auto edits = DoConvertToClassInternal(context, target.node, target.name);
    return std::make_unique<RefactorEditInfo>(std::move(edits));
}
// NOLINTNEXTLINE(fuchsia-statically-constructed-objects, cert-err58-cpp)
AutoRefactorRegister<ConvertFunctionToClassRefactor> g_ConvertToClassRefactorRegister("ConvertFunctionToClassRefactor");

}  // namespace ark::es2panda::lsp
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
 */

#include "lsp/include/register_code_fix/fix_spelling.h"
#include <cstddef>
#include <string>
#include <vector>
#include "generated/code_fix_register.h"
#include "lsp/include/code_fix_provider.h"
#include "lsp/include/internal_api.h"
#include "public/es2panda_lib.h"

namespace ark::es2panda::lsp {
using codefixes::FIX_SPELLING;

FixSpelling::FixSpelling()
{
    auto errorCodes = FIX_SPELLING.GetSupportedCodeNumbers();
    SetErrorCodes({errorCodes.begin(), errorCodes.end()});
    SetFixIds({FIX_SPELLING.GetFixId().data()});
}
double JaccardSimilarity(const std::string &a, const std::string &b)
{
    std::unordered_set<char> setA;
    std::unordered_set<char> setB;

    for (char ch : a) {
        if (isalpha(ch) != 0) {
            setA.insert(tolower(ch));
        }
    }

    for (char ch : b) {
        if (isalpha(ch) != 0) {
            setB.insert(tolower(ch));
        }
    }

    size_t intersectionSize = 0;
    for (char ch : setA) {
        if (setB.find(ch) != setB.end()) {
            intersectionSize++;
        }
    }

    size_t unionSize = setA.size() + setB.size() - intersectionSize;
    if (unionSize == 0) {
        return 0.0;
    }

    return static_cast<double>(intersectionSize) / unionSize;
}

std::string FindClosestWordJaccard(const ir::AstNode *astNode, const std::string &search)
{
    if (astNode == nullptr) {
        return "";
    }
    double maxSimilarity = -1.0;
    std::string closestWord;
    astNode->FindChild([&search, &maxSimilarity, &closestWord](const ir::AstNode *node) {
        if (node->IsIdentifier() && std::string(node->AsIdentifier()->Name().Utf8()) != search) {
            double similarity = JaccardSimilarity(std::string(node->AsIdentifier()->Name().Utf8()), search);
            if (similarity > maxSimilarity) {
                maxSimilarity = similarity;
                closestWord = std::string(node->AsIdentifier()->Name().Utf8());
            }
        }
        return false;
    });
    return closestWord;
}

void DoChanges(ChangeTracker &changes, es2panda_Context *context, ir::AstNode *node, const std::string &target)
{
    const auto impl = es2panda_GetImpl(ES2PANDA_LIB_VERSION);
    auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);

    if (node == nullptr) {
        return;
    }
    if (!node->IsIdentifier()) {
        std::vector<char> buffer(target.begin(), target.end());
        buffer.push_back('\0');
        auto newSource = impl->CreateStringLiteral1(context, buffer.data());
        auto changedNode = reinterpret_cast<ir::AstNode *>(newSource);
        changes.ReplaceNode(context, node, changedNode, {});
    } else if (node->IsIdentifier()) {
        auto newNode = node->Clone(ctx->Allocator(), node->Parent());
        newNode->AsIdentifier()->SetName(target.c_str());
        changes.ReplaceNode(context, node, newNode, {});
    }
}

std::vector<CodeFixAction> FixSpelling::GetCodeActions(const CodeFixContext &context)
{
    std::vector<CodeFixAction> returnedActions;
    if (context.errorCode != 0) {
        auto info = GetInfoSpelling(context.context, context.span.start);
        if (info.GetFindClosestWord().empty() || info.GetNode() == nullptr) {
            return returnedActions;
        }
        TextChangesContext textChangesContext {context.host, context.formatContext, context.preferences};
        auto changes = ChangeTracker::With(textChangesContext, [&](ChangeTracker &tracker) {
            DoChanges(tracker, context.context, info.GetNode(), info.GetFindClosestWord());
        });
        CodeFixAction action;
        action.fixName = FIX_SPELLING.GetFixId().data();
        action.description = "Fix spelling error";
        action.fixId = FIX_SPELLING.GetFixId().data();
        action.fixAllDescription = "Fix all spelling errors";
        action.changes.insert(action.changes.end(), changes.begin(), changes.end());
        returnedActions.push_back(action);

        return returnedActions;
    }

    return returnedActions;
}

CombinedCodeActions FixSpelling::GetAllCodeActions(const CodeFixAllContext &codeFixAll)
{
    CodeFixProvider provider;
    const auto changes = provider.CodeFixAll(
        codeFixAll, GetErrorCodes(), [&](ChangeTracker &tracker, const DiagnosticWithLocation &diag) {
            auto info = GetInfoSpelling(codeFixAll.context, diag.GetStart());
            DoChanges(tracker, codeFixAll.context, info.GetNode(), info.GetFindClosestWord());
        });

    CombinedCodeActions combinedCodeActions;
    combinedCodeActions.changes = changes.changes;
    combinedCodeActions.commands = changes.commands;
    return combinedCodeActions;
}

Info GetInfoSpelling(es2panda_Context *context, size_t position)
{
    const auto token = GetTouchingToken(context, position, false);
    if (token == nullptr) {
        return {"", nullptr};
    }
    auto parent = token->Parent();
    const auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
    const auto astNode = ctx->parserProgram->Ast();

    if (!parent->IsETSImportDeclaration() &&
        !(parent->IsImportSpecifier() || parent->IsImportDefaultSpecifier() || parent->IsImportNamespaceSpecifier())) {
        auto findClosestWord = FindClosestWordJaccard(astNode, std::string(token->AsIdentifier()->Name().Utf8()));
        if (!findClosestWord.empty()) {
            return {findClosestWord, token};
        }
    }
    if (parent->IsImportSpecifier() || parent->IsImportDefaultSpecifier() || parent->IsImportNamespaceSpecifier()) {
        parent = parent->Parent();
    }
    auto importDecl = parent->AsETSImportDeclaration();
    if (!importDecl->Specifiers().empty()) {
        Initializer initializer = Initializer();
        const auto path = importDecl->ResolvedSource();
        auto con = initializer.CreateContext(std::string(path).c_str(), ES2PANDA_STATE_CHECKED);
        auto cctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(con);
        const auto importAstNode = cctx->parserProgram->Ast();
        std::string findClosestWord;
        if (token->IsIdentifier()) {
            findClosestWord = FindClosestWordJaccard(importAstNode, std::string(token->AsIdentifier()->Name().Utf8()));
        } else if (token->IsStringLiteral()) {
            findClosestWord =
                FindClosestWordJaccard(importAstNode, std::string(token->AsStringLiteral()->Str().Utf8()));
        }
        return {findClosestWord, token};
    }

    return {"", token};
}

// NOLINTNEXTLINE(fuchsia-statically-constructed-objects, cert-err58-cpp)
AutoCodeFixRegister<FixSpelling> g_fixSpelling("FixSpelling");
}  // namespace ark::es2panda::lsp

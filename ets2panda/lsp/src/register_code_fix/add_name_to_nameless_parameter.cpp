/**
  Copyright (c) 2025-2026 Huawei Device Co., Ltd.
  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 */

#include <string>
#include <vector>
#include <algorithm>
#include "public/es2panda_lib.h"
#include "lsp/include/internal_api.h"
#include "generated/code_fix_register.h"
#include "lsp/include/code_fix_provider.h"
#include "lsp/include/register_code_fix/add_name_to_nameless_parameter.h"

namespace ark::es2panda::lsp {
using codefixes::ADD_NAME_TO_NAMELESS_PARAMETER;
namespace {

inline std::string ToString(std::string_view sv)
{
    return std::string(sv.data(), sv.size());
}

inline std::string_view Slice(std::string_view sv, size_t start, size_t length)
{
    return std::string_view(sv.data() + start, length);
}

// Count commas between the opening '(' and the parameter start (0-based index).
int ComputeParamIndexByText(const std::string &fullSource, size_t paramStart)
{
    if (fullSource.empty() || paramStart == 0 || paramStart > fullSource.size()) {
        return 0;
    }
    const size_t lparen = fullSource.rfind('(', paramStart);
    if (lparen == std::string::npos) {
        return 0;
    }
    int idx = 0;
    for (size_t i = lparen + 1; i < paramStart; ++i) {
        if (fullSource[i] == ',') {
            ++idx;
        }
    }
    return idx;
}

// Return the start index of the current parameter (right after the nearest '(' or ',' before tokenStart)
size_t ParamSliceStart(const std::string &src, size_t tokenStart)
{
    const size_t lp = src.rfind('(', tokenStart);
    const size_t cm = src.rfind(',', tokenStart);
    size_t start = std::max(lp == std::string::npos ? 0 : lp, cm == std::string::npos ? 0 : cm);
    if (start < src.size()) {
        ++start;
    }
    return start;
}

// true if there's a ':' between the start of *this* parameter and the token start
bool HasColonBeforeTokenInSameParam(const std::string &src, size_t tokenStart)
{
    if (tokenStart == 0 || tokenStart > src.size()) {
        return false;
    }
    const size_t start = ParamSliceStart(src, tokenStart);
    const size_t colon = src.find(':', start);
    return colon != std::string::npos && colon < tokenStart;
}

// true if there's a '=' between the start of *this* parameter and the token start
bool HasEqualsBeforeTokenInSameParam(const std::string &src, size_t tokenStart)
{
    if (tokenStart == 0 || tokenStart > src.size()) {
        return false;
    }
    const size_t start = ParamSliceStart(src, tokenStart);
    const size_t eq = src.find('=', start);
    return eq != std::string::npos && eq < tokenStart;
}

}  // namespace

AddNameToNamelessParameter::AddNameToNamelessParameter()
{
    auto errorCodes = ADD_NAME_TO_NAMELESS_PARAMETER.GetSupportedCodeNumbers();
    SetErrorCodes({errorCodes.begin(), errorCodes.end()});
    SetFixIds({ADD_NAME_TO_NAMELESS_PARAMETER.GetFixId().data()});
}

bool AddNameToNamelessParameter::IsIdentifier(const ir::AstNode *node)
{
    return node != nullptr && node->IsIdentifier();
}

const ir::AstNode *AddNameToNamelessParameter::FindParameterNode(const ir::AstNode *start)
{
    const ir::AstNode *n = start;
    while (n != nullptr) {
        if (n->Type() == ir::AstNodeType::ETS_PARAMETER_EXPRESSION) {
            return n;
        }
        n = n->Parent();
    }
    return (start != nullptr) ? start->Parent() : nullptr;
}

std::string AddNameToNamelessParameter::GetNodeText(std::string_view fullSource, const ir::AstNode *node)
{
    if (node == nullptr) {
        return {};
    }
    const size_t start = node->Range().start.index;
    const size_t end = node->Range().end.index;
    if (end < start) {
        return {};
    }
    return ToString(Slice(fullSource, start, end - start));
}

void AddNameToNamelessParameter::MakeChange(ChangeTracker &changeTracker, es2panda_Context *context, size_t pos,
                                            std::vector<ir::AstNode *> &fixedNodes)
{
    auto *token = GetTouchingToken(context, pos, false);
    if (token == nullptr || !IsIdentifier(token)) {
        return;
    }

    const ir::AstNode *paramNode = FindParameterNode(token);
    if (paramNode == nullptr) {
        return;
    }

    using ark::es2panda::public_lib::Context;
    auto *pub = reinterpret_cast<Context *>(context);
    if (pub == nullptr || pub->parserProgram == nullptr) {
        return;
    }

    const std::string fullSource = ToString(pub->parserProgram->SourceCode());
    const size_t tokenStart = token->Range().start.index;
    if (HasColonBeforeTokenInSameParam(fullSource, tokenStart) ||
        HasEqualsBeforeTokenInSameParam(fullSource, tokenStart)) {
        return;
    }

    const std::string typeText = GetNodeText(fullSource, token);
    const int idx = ComputeParamIndexByText(fullSource, tokenStart);
    const std::string nameText = "arg" + std::to_string(idx);
    const size_t tokenEnd = token->Range().end.index;
    if (tokenEnd < tokenStart) {
        return;
    }

    const TextSpan replaceSpan(tokenStart, tokenEnd - tokenStart);
    const std::string replacement = nameText + ": " + typeText;
    const std::string filePath = (pub->sourceFile != nullptr) ? ToString(pub->sourceFile->filePath) : std::string();
    TextChange tc(replaceSpan, replacement);
    FileTextChanges fc(filePath, {tc});
    const SourceFile *owner = pub->sourceFile;
    changeTracker.PushRaw(owner, fc);
    fixedNodes.push_back(const_cast<ir::AstNode *>(paramNode));
}

std::vector<FileTextChanges> AddNameToNamelessParameter::GetCodeActionsToFix(const CodeFixContext &context)
{
    TextChangesContext textChangesContext = {context.host, context.formatContext, context.preferences};
    std::vector<ir::AstNode *> fixedNodes;
    auto fileTextChanges = ChangeTracker::With(textChangesContext, [&](ChangeTracker &tracker) {
        MakeChange(tracker, context.context, context.span.start, fixedNodes);
    });

    return fileTextChanges;
}

std::vector<CodeFixAction> AddNameToNamelessParameter::GetCodeActions(const CodeFixContext &context)
{
    std::vector<CodeFixAction> actions;
    auto changes = GetCodeActionsToFix(context);
    if (!changes.empty()) {
        CodeFixAction action;
        action.fixName = ADD_NAME_TO_NAMELESS_PARAMETER.GetFixId().data();
        action.description = "Add parameter name";
        action.changes = changes;
        action.fixId = ADD_NAME_TO_NAMELESS_PARAMETER.GetFixId().data();
        actions.push_back(action);
    }
    return actions;
}

CombinedCodeActions AddNameToNamelessParameter::GetAllCodeActions([[maybe_unused]] const CodeFixAllContext & /*unused*/)
{
    return {};
}

// NOLINTNEXTLINE(fuchsia-statically-constructed-objects, cert-err58-cpp)
AutoCodeFixRegister<AddNameToNamelessParameter> g_addNameToNamelessParameter(
    ADD_NAME_TO_NAMELESS_PARAMETER.GetFixId().data());
}  // namespace ark::es2panda::lsp
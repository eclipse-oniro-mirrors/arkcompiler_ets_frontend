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

#include <memory>
#include <string>
#include <string_view>
#include "refactors/convert_template.h"
#include "ir/astNode.h"
#include "ir/expressions/literals/stringLiteral.h"
#include "public/es2panda_lib.h"
#include "public/public.h"
#include "refactor_provider.h"
#include "internal_api.h"
#include "refactors/refactor_types.h"
#include "types.h"

namespace ark::es2panda::lsp {

ConvertTemplateRefactor::ConvertTemplateRefactor()
{
    AddKind(std::string(TO_NAMED_TEMPLATE_ACTION.kind));
}

ApplicableRefactorInfo ConvertTemplateRefactor::GetAvailableActions(const RefactorContext &refContext) const
{
    es2panda_Context *context = refContext.context;
    size_t position = refContext.span.pos;

    ApplicableRefactorInfo res;

    if (!IsKind(refContext.kind)) {
        return res;
    }
    auto node = GetTouchingToken(context, position, false);
    if (node == nullptr) {
        return res;
    }

    if (node != nullptr && node->Parent() != nullptr &&
        (node->Parent()->IsExpression() && node->Parent()->IsBinaryExpression())) {
        res.name = refactor_name::CONVERT_TEMPLATE_REFACTOR_NAME;
        res.description = refactor_description::CONVERT_TEMPLATE_REFACTOR_DESC;
        res.action.kind = std::string(TO_NAMED_TEMPLATE_ACTION.kind);
        res.action.name = std::string(TO_NAMED_TEMPLATE_ACTION.name);
        res.action.description = std::string(TO_NAMED_TEMPLATE_ACTION.description);
    }

    return res;
}

void CollectConcatParts(const ir::AstNode *expr, std::vector<const ir::AstNode *> &parts)
{
    if (expr == nullptr) {
        return;
    }
    if (expr->IsBinaryExpression()) {
        const auto *bin = expr->AsBinaryExpression();
        CollectConcatParts(bin->Left(), parts);
        CollectConcatParts(bin->Right(), parts);
    } else {
        if (expr->IsStringLiteral()) {
            auto val = expr->AsStringLiteral()->Str().Utf8();
            if (!val.empty()) {
                parts.push_back(expr);
            }
        } else {
            parts.push_back(expr);
        }
    }
}

std::string ReviewText(const ir::StringLiteral *p, std::string &newText)
{
    auto s = p->AsStringLiteral()->Str().Utf8();
    for (char ch : s) {
        switch (ch) {
            case '`':
                newText += "\\`";
                break;
            case '$':
                newText += "\\$";
                break;
            case '\\':
                newText += "\\\\";
                break;
            default:
                newText += ch;
                break;
        }
    }
    return newText;
}

RefactorEditInfo ConvertToTemplateString(const ir::AstNode *node)
{
    RefactorEditInfo res;
    if (node == nullptr || !node->IsBinaryExpression()) {
        return res;
    }
    std::string newText;
    std::vector<const ir::AstNode *> parts;
    while (node->Parent() != nullptr && node->Parent()->IsBinaryExpression()) {
        node = node->Parent();
    }
    CollectConcatParts(node, parts);
    newText += '`';
    for (auto *p : parts) {
        if (p->IsStringLiteral()) {
            newText = ReviewText(p->AsStringLiteral(), newText);
        } else if (p->IsIdentifier()) {
            newText += "${";
            newText += p->AsIdentifier()->Name().Mutf8();
            newText += "}";
        }
    }
    newText += "`;";
    TextChange textChange {TextSpan(node->Start().index, node->End().index - node->Start().index), newText};
    FileTextChanges fileTextChanges;
    fileTextChanges.fileName = std::string(node->Range().start.Program()->SourceFile().GetAbsolutePath().Utf8());
    fileTextChanges.textChanges.push_back(textChange);
    res.GetFileTextChanges().push_back(fileTextChanges);
    return res;
}

std::unique_ptr<RefactorEditInfo> ConvertTemplateRefactor::GetEditsForAction(const RefactorContext &context,
                                                                             const std::string &actionName) const
{
    if (actionName == TO_NAMED_TEMPLATE_ACTION.name) {
        const auto ctx = context.context;
        const auto pbContext = reinterpret_cast<public_lib::Context *>(ctx);
        auto source = std::string(pbContext->sourceFile->source.begin(), pbContext->sourceFile->source.end());
        size_t position = context.span.pos;
        auto node = GetTouchingToken(context.context, position, false);
        if (node != nullptr && node->Parent() != nullptr &&
            (node->Parent()->IsExpression() && node->Parent()->IsBinaryExpression())) {
            auto edits = ConvertToTemplateString(node->Parent());
            return std::make_unique<RefactorEditInfo>(edits);
        }
    }
    return std::make_unique<RefactorEditInfo>();
}
// NOLINTNEXTLINE(fuchsia-statically-constructed-objects, cert-err58-cpp)
AutoRefactorRegister<ConvertTemplateRefactor> g_convertTemplateRefactorRegister("ConvertTemplateRefactor");

}  // namespace ark::es2panda::lsp

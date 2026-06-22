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

#ifndef ES2PANDA_LSP_INLAY_HINTS_H
#define ES2PANDA_LSP_INLAY_HINTS_H

#include "api.h"
#include "ir/astNode.h"
#include "cancellation_token.h"
#include <string>
#include <vector>
#include "user_preferences.h"

namespace ark::es2panda::lsp {

struct ParamInfoObj {
    std::string paramName;
    bool isFirstVariadicArgument;
};

int GetFullWidth(const ir::AstNode *node);
void AddEnumMemberValueHints(const std::string &text, const int position, InlayHintList *result);
void AddTypeHints(const std::string &text, const int position, InlayHintList *result);
void AddParameterHints(const std::string &text, const int position, const bool isFirstVariadicArgument,
                       InlayHintList *result);
bool DecodedTextSpanIntersectsWith(const int start1, const int length1, const int start2, const int length2);
bool TextSpanIntersectsWith(const TextSpan span, const int position, const int nodeSize);
bool IsExpressionWithTypeArguments(const ir::AstNode *node);
void GetVariableDeclarationTypeForHints(const ir::AstNode *decl, InlayHintList *result);
void GetCallExpTypeForHints(const ir::AstNode *expr, const ir::AstNode *parent, InlayHintList *result);
bool ShouldShowParameterNameHints(const UserPreferences &preferences);
int GetIndexForEnum(const ir::AstNode *mem, const std::string &assignName);
void SaveToList(const ir::Expression *mem, const ir::AstNode *asignNode, InlayHintList *list);
void GetEnumTypeForHints(const ir::AstNode *member, const ir::AstNode *parent, InlayHintList *result);
void GetFunctionReturnTypeForHints(const ir::AstNode *decl, InlayHintList *result);
void GetFunctionParameterTypeForHints(const ir::AstNode *node, InlayHintList *result);
bool IsSignatureSupportingReturnAnnotation(const ir::AstNode *node);
void Visitor(const ir::AstNode *node, const TextSpan *span, const ir::AstNode *parent,
             CancellationToken cancellationToken, InlayHintList *result);
InlayHintList ProvideInlayHintsImpl(es2panda_Context *context, const TextSpan *span,
                                    CancellationToken &cancellationToken, UserPreferences &preferences);

}  // namespace ark::es2panda::lsp

#endif

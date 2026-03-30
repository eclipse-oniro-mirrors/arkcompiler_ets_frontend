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

#ifndef ES2PANDA_LSP_INCLUDE_CLASSIFIER_H
#define ES2PANDA_LSP_INCLUDE_CLASSIFIER_H

#include <cstddef>
#include "lexer/lexer.h"
#include "lexer/token/token.h"
#include "public/es2panda_lib.h"

namespace ark::es2panda::lsp {

enum class ClassificationType {
    IDENTIFIER,
    KEYWORD,
    NUMERIC_LITERAL,
    STRING_LITERAL,
    BOOLEAN_LITERAL,
    NULL_LITERAL,
    PUNCTUATION,

    CLASS_NAME,
    ENUM_NAME,
    INTERFACE_NAME,
    TYPE_PARAMETER_NAME,
    TYPE_ALIAS_NAME,
    PARAMETER_NAME,
};

struct ClassifiedSpan {
    size_t start;
    size_t length;
    char const *name;
};

std::unique_ptr<lexer::Lexer> InitLexer(es2panda_Context *context);
ClassificationType GetClassificationType(const lexer::Token &token);
ClassificationType AstNodeTypeToClassificationType(ir::AstNodeType type);
char const *ClassificationTypeToString(ClassificationType type);
std::vector<ClassifiedSpan *> GetSyntacticClassifications(es2panda_Context *context, size_t startPos, size_t length);
std::vector<ClassifiedSpan *> GetSemanticClassifications(es2panda_Context *context, size_t startPos, size_t length);

}  // namespace ark::es2panda::lsp

#endif

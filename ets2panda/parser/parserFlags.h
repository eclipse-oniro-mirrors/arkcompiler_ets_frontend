/**
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

#ifndef ES2PANDA_PARSER_PARSER_FLAGS_H
#define ES2PANDA_PARSER_PARSER_FLAGS_H

#include <cstdint>
#include "util/enumbitops.h"

namespace ark::es2panda::parser {

using ENUMBITOPS_OPERATORS;

enum class LexicalScopeType {
    BLOCK,
    STRICT_BLOCK,
    CATCH,
    FUNCTION_PARAM,
    TS_TYPE_LITERAL,
};

enum class VariableParsingFlags : uint32_t {
    NO_OPTS = 0U,
    NO_SKIP_VAR_KIND = 1U << 0U,
    ACCEPT_CONST_NO_INIT = 1U << 1U,
    DISALLOW_INIT = 1U << 2U,
    VAR = 1U << 3U,
    LET = 1U << 4U,
    CONST = 1U << 5U,
    STOP_AT_IN = 1U << 6U,
    IN_FOR = 1U << 7U,
    FOR_OF = 1U << 8U
};

enum class ExpressionParseFlags : uint32_t {
    NO_OPTS = 0U,
    ACCEPT_COMMA = 1U << 0U,
    ACCEPT_REST = 1U << 1U,
    EXP_DISALLOW_AS = 1U << 2U,
    DISALLOW_ASSIGNMENT = 1U << 3U,
    DISALLOW_YIELD = 1U << 4U,
    STOP_AT_IN = 1U << 5U,
    MUST_BE_PATTERN = 1U << 6U,
    POTENTIALLY_IN_PATTERN = 1U << 7U,
    OBJECT_PATTERN = 1U << 8U,
    IN_REST = 1U << 9U,
    IMPORT = 1U << 10U,
    POTENTIAL_CLASS_LITERAL = 1U << 11U,
    IN_FOR = 1U << 12U,
    INSTANCEOF = 1U << 13U,
    POTENTIAL_NEW_ARRAY = 1U << 14U
};

enum class StatementParsingFlags : uint32_t {
    NONE = 0U,
    ALLOW_LEXICAL = 1U << 0U,
    GLOBAL = 1U << 1U,
    IF_ELSE = 1U << 2U,
    LABELLED = 1U << 3U,
    INIT_MODULE = 1U << 4U,

    STMT_LEXICAL_SCOPE_NEEDED = IF_ELSE | LABELLED,
    STMT_GLOBAL_LEXICAL = GLOBAL | ALLOW_LEXICAL,
};

enum class ForStatementKind {
    UPDATE,
    IN,
    OF,
};
}  // namespace ark::es2panda::parser

namespace enumbitops {

template <>
struct IsAllowedType<ark::es2panda::parser::LexicalScopeType> : std::true_type {
};

template <>
struct IsAllowedType<ark::es2panda::parser::VariableParsingFlags> : std::true_type {
};

template <>
struct IsAllowedType<ark::es2panda::parser::ExpressionParseFlags> : std::true_type {
};

template <>
struct IsAllowedType<ark::es2panda::parser::StatementParsingFlags> : std::true_type {
};

}  // namespace enumbitops

#endif

/*
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

#ifndef ES2PANDA_IR_AST_NODE_FLAGS_H
#define ES2PANDA_IR_AST_NODE_FLAGS_H

#include <cstdint>

#include "util/enumbitops.h"

namespace ark::es2panda::ir {

using ENUMBITOPS_OPERATORS;

enum class AstNodeFlags : uint16_t {
    NO_OPTS = 0,
    CHECKCAST = 1U << 0U,
    ALLOW_REQUIRED_INSTANTIATION = 1U << 2U,
    GENERATE_VALUE_OF = 1U << 4U,
    RECHECK = 1U << 5U,
    NOCLEANUP = 1U << 6U,
    RESIZABLE_REST = 1U << 7U,
    // Moved out of the ir::Expression
    IS_GROUPED = 1U << 8U,
    /* do not introduce new flags. all the existing to be removed */
};

enum class ModifierFlags : uint32_t {
    NONE = 0U,
    STATIC = 1U << 0U,
    ASYNC = 1U << 1U,
    PUBLIC = 1U << 2U,
    PROTECTED = 1U << 3U,
    PRIVATE = 1U << 4U,
    DECLARE = 1U << 5U,
    READONLY = 1U << 6U,
    OPTIONAL = 1U << 7U,
    DEFINITE = 1U << 8U,
    ABSTRACT = 1U << 9U,
    CONST = 1U << 10U,
    FINAL = 1U << 11U,
    NATIVE = 1U << 12U,
    OVERRIDE = 1U << 13U,
    CONSTRUCTOR = 1U << 14U,
    SYNCHRONIZED = 1U << 15U,
    FUNCTIONAL = 1U << 16U,
    IN = 1U << 17U,
    OUT = 1U << 18U,
    INTERNAL = 1U << 19U,
    EXPORT = 1U << 20U,
    GETTER = 1U << 21U,
    SETTER = 1U << 22U,
    DEFAULT_EXPORT = 1U << 23U,
    EXPORT_TYPE = 1U << 24U,
    SUPER_OWNER = 1U << 26U,
    ANNOTATION_DECLARATION = 1U << 27U,
    ANNOTATION_USAGE = 1U << 28U,
    READONLY_PARAMETER = 1U << 29U,
    EXPORT_WITH_ALIAS = 1U << 30U,
    DEFAULT = 1U << 31U,
    ACCESS = PUBLIC | PROTECTED | PRIVATE | INTERNAL,
    ALL = STATIC | ASYNC | ACCESS | DECLARE | READONLY | ABSTRACT,
    ALLOWED_IN_CTOR_PARAMETER = ACCESS | READONLY,
    INTERNAL_PROTECTED = INTERNAL | PROTECTED,
    ACCESSOR_MODIFIERS = ABSTRACT | FINAL,
    GETTER_SETTER = GETTER | SETTER,
    EXPORTED = EXPORT | DEFAULT_EXPORT | EXPORT_TYPE
};

enum class PrivateFieldKind {
    FIELD,
    METHOD,
    GET,
    SET,
    STATIC_FIELD,
    STATIC_METHOD,
    STATIC_GET,
    STATIC_SET,
    OVERLOAD,
    STATIC_OVERLOAD
};

enum class ScriptFunctionFlags : uint32_t {
    NONE = 0U,
    GENERATOR = 1U << 0U,
    ASYNC = 1U << 1U,
    ARROW = 1U << 2U,
    EXPRESSION = 1U << 3U,
    OVERLOAD = 1U << 4U,
    CONSTRUCTOR = 1U << 5U,
    METHOD = 1U << 6U,
    STATIC_BLOCK = 1U << 7U,
    HIDDEN = 1U << 8U,
    IMPLICIT_SUPER_CALL_NEEDED = 1U << 9U,
    ENUM = 1U << 10U,
    EXTERNAL = 1U << 11U,
    PROXY = 1U << 12U,
    THROWS = 1U << 13U,
    RETHROWS = 1U << 14U,
    GETTER = 1U << 15U,
    SETTER = 1U << 16U,
    ENTRY_POINT = 1U << 17U,
    HAS_RETURN = 1U << 18U,
    ASYNC_IMPL = 1U << 19U,
    EXTERNAL_OVERLOAD = 1U << 20U,
    HAS_THROW = 1U << 21U,
    IN_RECORD = 1U << 22U,
    TRAILING_LAMBDA = 1U << 23U,
    SYNTHETIC = 1U << 24U,
    RETURN_PROMISEVOID = 1U << 25U,
    EXPLICIT_THIS_CALL = 1U << 26U,
    EXPLICIT_SUPER_CALL = 1U << 27U,
};

enum class TSOperatorType { READONLY, KEYOF, UNIQUE };
enum class MappedOption { NO_OPTS, PLUS, MINUS };

}  // namespace ark::es2panda::ir

namespace enumbitops {

template <>
struct IsAllowedType<ark::es2panda::ir::AstNodeFlags> : std::true_type {
};

template <>
struct IsAllowedType<ark::es2panda::ir::ModifierFlags> : std::true_type {
};

template <>
struct IsAllowedType<ark::es2panda::ir::ScriptFunctionFlags> : std::true_type {
};

}  // namespace enumbitops

#endif

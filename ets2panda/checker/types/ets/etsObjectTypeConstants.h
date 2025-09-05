/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

#ifndef ES2PANDA_COMPILER_CHECKER_TYPES_ETS_OBJECT_TYPE_CONSTANTS_H
#define ES2PANDA_COMPILER_CHECKER_TYPES_ETS_OBJECT_TYPE_CONSTANTS_H

#include "checker/types/type.h"

namespace ark::es2panda::checker {

using ENUMBITOPS_OPERATORS;

enum class ETSObjectFlags : std::uint64_t {
    NO_OPTS = 0U,
    CLASS = 1U << 0U,
    INTERFACE = 1U << 1U,
    INSTANCE = 1U << 2U,
    ABSTRACT = 1U << 3U,
    GLOBAL = 1U << 4U,
    ENUM = 1U << 5U,
    FUNCTIONAL = 1U << 6U,
    RESOLVED_INTERFACES = 1U << 7U,
    RESOLVED_SUPER = 1U << 8U,
    RESOLVED_TYPE_PARAMS = 1U << 9U,
    STRING = 1U << 10U,
    INCOMPLETE_INSTANTIATION = 1U << 11U,
    INNER = 1U << 12U,
    GRADUAL = 1U << 13U,
    ASYNC_FUNC_RETURN_TYPE = 1U << 14U,
    CHECKED_INVOKE_LEGITIMACY = 1U << 15U,
    REQUIRED = 1U << 16U,
    READONLY = 1U << 17U,

    BUILTIN_BIGINT = 1U << 22U,
    BUILTIN_STRING = 1U << 23U,
    BUILTIN_BOOLEAN = 1U << 24U,
    BUILTIN_BYTE = 1U << 25U,
    BUILTIN_CHAR = 1U << 26U,
    BUILTIN_SHORT = 1U << 27U,
    BUILTIN_INT = 1U << 28U,
    BUILTIN_LONG = 1U << 29U,
    BUILTIN_FLOAT = 1U << 30U,
    BUILTIN_DOUBLE = 1U << 31U,
    BUILTIN_ARRAY = 1ULL << 32U,
    BUILTIN_READONLY_ARRAY = 1ULL << 33U,

    INT_ENUM_OBJECT = 1ULL << 34U,
    STRING_ENUM_OBJECT = 1ULL << 35U,

    EXTENSION_FUNCTION = 1ULL << 36U,
    FUNCTIONAL_REFERENCE = 1ULL << 37U,
    LAZY_IMPORT_OBJECT = 1ULL << 38U,
    DOUBLE_ENUM_OBJECT = 1ULL << 39U,

    ENUM_OBJECT = INT_ENUM_OBJECT | STRING_ENUM_OBJECT | DOUBLE_ENUM_OBJECT,

    BUILTIN_FLOATING_POINT = BUILTIN_DOUBLE | BUILTIN_FLOAT,
    BUILTIN_INTEGRAL = BUILTIN_BYTE | BUILTIN_SHORT | BUILTIN_INT | BUILTIN_LONG,

    BUILTIN_ARRAY_INDEX = BUILTIN_BYTE | BUILTIN_SHORT | BUILTIN_INT,
    BUILTIN_ARRAY_NUMERIC = BUILTIN_ARRAY_INDEX | BUILTIN_FLOATING_POINT,

    BUILTIN_NUMERIC = BUILTIN_INTEGRAL | BUILTIN_FLOATING_POINT,
    // Complete set includes null|undefined|Object
    VALUE_TYPED = BUILTIN_BOOLEAN | BUILTIN_CHAR | BUILTIN_NUMERIC | BUILTIN_BIGINT | STRING,
    UNBOXABLE_TYPE = BUILTIN_BOOLEAN | BUILTIN_CHAR | BUILTIN_NUMERIC,
    BUILTIN_TYPE = BUILTIN_STRING | BUILTIN_BIGINT | UNBOXABLE_TYPE,
    CONVERTIBLE_TO_NUMERIC = BUILTIN_NUMERIC | BUILTIN_CHAR | INT_ENUM_OBJECT,

    GLOBAL_CLASS = CLASS | GLOBAL,
    FUNCTIONAL_INTERFACE = INTERFACE | ABSTRACT | FUNCTIONAL,
    RESOLVED_HEADER = RESOLVED_INTERFACES | RESOLVED_SUPER | RESOLVED_TYPE_PARAMS,
};

// NOTE: Do not change the order of the first 7 flags (including NO_OPTS)!
// Because ETSChecker::ValidateResolvedProperty relies on the order of the flags.
enum class PropertySearchFlags : std::uint32_t {
    NO_OPTS = 0,
    SEARCH_INSTANCE_METHOD = 1U << 0U,
    SEARCH_INSTANCE_FIELD = 1U << 1U,
    SEARCH_INSTANCE_DECL = 1U << 2U,
    SEARCH_STATIC_METHOD = 1U << 3U,
    SEARCH_STATIC_FIELD = 1U << 4U,
    SEARCH_STATIC_DECL = 1U << 5U,

    SEARCH_IN_BASE = 1U << 6U,
    SEARCH_IN_INTERFACES = 1U << 7U,
    IGNORE_ABSTRACT = 1U << 8U,
    ALLOW_FUNCTIONAL_INTERFACE = 1U << 9U,
    DISALLOW_SYNTHETIC_METHOD_CREATION = 1U << 10U,
    IS_SETTER = 1U << 11U,
    IS_GETTER = 1U << 12U,
    IGNORE_OVERLOAD = 1U << 13U,

    SEARCH_INSTANCE = SEARCH_INSTANCE_FIELD | SEARCH_INSTANCE_METHOD | SEARCH_INSTANCE_DECL,
    SEARCH_STATIC = SEARCH_STATIC_FIELD | SEARCH_STATIC_METHOD | SEARCH_STATIC_DECL,

    SEARCH_METHOD = SEARCH_INSTANCE_METHOD | SEARCH_STATIC_METHOD,
    SEARCH_FIELD = SEARCH_INSTANCE_FIELD | SEARCH_STATIC_FIELD,
    SEARCH_DECL = SEARCH_INSTANCE_DECL | SEARCH_STATIC_DECL,
    SEARCH_ALL = SEARCH_METHOD | SEARCH_FIELD | SEARCH_DECL,
};

enum class PropertyType {
    INSTANCE_METHOD,
    INSTANCE_FIELD,
    INSTANCE_DECL,
    STATIC_METHOD,
    STATIC_FIELD,
    STATIC_DECL,
    COUNT,
};

}  // namespace ark::es2panda::checker

namespace enumbitops {

template <>
struct IsAllowedType<ark::es2panda::checker::ETSObjectFlags> : std::true_type {
};

template <>
struct IsAllowedType<ark::es2panda::checker::PropertySearchFlags> : std::true_type {
};

}  // namespace enumbitops

#endif /* ES2PANDA_COMPILER_CHECKER_TYPES_ETS_OBJECT_TYPE_CONSTANTS_H */

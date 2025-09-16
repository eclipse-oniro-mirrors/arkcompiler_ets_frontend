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

#ifndef ES2PANDA_COMPILER_SCOPES_VARIABLE_FLAGS_H
#define ES2PANDA_COMPILER_SCOPES_VARIABLE_FLAGS_H

#include <cstdint>
#include "util/enumbitops.h"

namespace ark::es2panda::varbinder {
// CC-OFFNXT(G.PRE.06) solid logic
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DECLARATION_KINDS(_)             \
    _(VAR, VarDecl)                      \
    _(LET, LetDecl)                      \
    _(CONST, ConstDecl)                  \
    _(LABEL, LabelDecl)                  \
    _(READONLY, ReadonlyDecl)            \
    _(FUNC, FunctionDecl)                \
    _(PARAM, ParameterDecl)              \
    _(IMPORT, ImportDecl)                \
    _(DYNAMIC_IMPORT, DynamicImportDecl) \
    _(EXPORT, ExportDecl)                \
    _(ANNOTATIONDECL, AnnotationDecl)    \
    _(ANNOTATIONUSAGE, AnnotationUsage)  \
    /* TS */                             \
    _(TYPE_ALIAS, TypeAliasDecl)         \
    _(NAMESPACE, NameSpaceDecl)          \
    _(INTERFACE, InterfaceDecl)          \
    _(ENUM_LITERAL, EnumLiteralDecl)     \
    _(TYPE_PARAMETER, TypeParameterDecl) \
    _(PROPERTY, PropertyDecl)            \
    _(CLASS, ClassDecl)                  \
    _(METHOD, MethodDecl)                \
    _(ENUM, EnumDecl)

enum class DeclType {
    NONE,
// CC-OFFNXT(G.PRE.02) name part
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DECLARE_TYPES(decl_kind, class_name) decl_kind,
    DECLARATION_KINDS(DECLARE_TYPES)
#undef DECLARE_TYPES
};

// CC-OFFNXT(G.PRE.06) solid logic
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define SCOPE_TYPES(_)                            \
    _(PARAM, ParamScope)                          \
    _(CATCH_PARAM, CatchParamScope)               \
    _(FUNCTION_PARAM, FunctionParamScope)         \
    _(CATCH, CatchScope)                          \
    _(CLASS, ClassScope)                          \
    _(ANNOTATION, AnnotationScope)                \
    _(ANNOTATIONPARAMSCOPE, AnnotationParamScope) \
    _(LOCAL, LocalScope)                          \
    /* Variable Scopes */                         \
    _(LOOP, LoopScope)                            \
    _(LOOP_DECL, LoopDeclarationScope)            \
    _(FUNCTION, FunctionScope)                    \
    _(GLOBAL, GlobalScope)                        \
    _(MODULE, ModuleScope)

enum class ScopeType {
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define GEN_SCOPE_TYPES(type, class_name) type,
    SCOPE_TYPES(GEN_SCOPE_TYPES)
#undef GEN_SCOPE_TYPES
};

using ENUMBITOPS_OPERATORS;

enum class ResolveBindingOptions : uint32_t {
    BINDINGS = 1U << 0U,
    INTERFACES = 1U << 1U,
    VARIABLES = 1U << 2U,
    METHODS = 1U << 3U,
    DECLARATION = 1U << 4U,
    STATIC_VARIABLES = 1U << 5U,
    STATIC_METHODS = 1U << 6U,
    STATIC_DECLARATION = 1U << 7U,
    TYPE_ALIASES = 1U << 8U,
    ALL_VARIABLES = VARIABLES | STATIC_VARIABLES,
    ALL_METHOD = METHODS | STATIC_METHODS,
    ALL_DECLARATION = DECLARATION | STATIC_DECLARATION,
    ALL_STATIC = STATIC_VARIABLES | STATIC_METHODS | STATIC_DECLARATION,
    ALL_NON_STATIC = VARIABLES | METHODS | DECLARATION,

    LAST = TYPE_ALIASES,
    ALL = (LAST << 1U) - 1U,
};

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define VARIABLE_TYPES(_)     \
    _(LOCAL, LocalVariable)   \
    _(GLOBAL, GlobalVariable) \
    _(MODULE, ModuleVariable) \
    _(ENUM, EnumVariable)

enum class VariableType {
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define GEN_VARIABLE_TYPES(type, class_name) type,
    VARIABLE_TYPES(GEN_VARIABLE_TYPES)
#undef GEN_VARIABLE_TYPES
};

enum class VariableKind {
    NONE,
    VAR,
    LEXICAL,
    FUNCTION,
    MODULE,
};

enum class VariableFlags : uint64_t {
    NONE = 0U,
    OPTIONAL = 1U << 0U,
    PROPERTY = 1U << 1U,
    METHOD = 1U << 2U,
    TYPE_ALIAS = 1U << 3U,
    INTERFACE = 1U << 4U,
    ENUM_LITERAL = 1U << 5U,
    READONLY = 1U << 6U,
    COMPUTED = 1U << 7U,
    COMPUTED_IDENT = 1U << 8U,  // Remove
    INFERRED_IN_PATTERN = 1U << 9U,
    REST_ARG = 1U << 10U,
    NUMERIC_NAME = 1U << 11U,
    TYPE = 1U << 12U,
    LOCAL_EXPORT = 1U << 13U,
    TYPE_PARAMETER = 1U << 14U,
    STATIC = 1U << 15U,
    CLASS = 1U << 16U,
    EXPLICIT_INIT_REQUIRED = 1U << 17U,
    PUBLIC = 1U << 18U,
    PROTECTED = 1U << 19U,
    PRIVATE = 1U << 20U,
    SYNTHETIC = 1U << 21U,
    LOCAL = 1U << 22U,

    LEXICAL = 1U << 23U,
    LOOP_DECL = 1U << 24U,
    PER_ITERATION = 1U << 25U,
    LEXICAL_VAR = 1U << 26U,
    HOIST = 1U << 27U,
    VAR = 1U << 28U,
    INITIALIZED = 1U << 29U,
    LEXICAL_BOUND = 1U << 30U,

    BUILTIN_TYPE = 1ULL << 31ULL,
    CAPTURED_MODIFIED = 1ULL << 32ULL,
    ANNOTATIONDECL = 1ULL << 33ULL,
    ANNOTATIONUSAGE = 1ULL << 34ULL,
    NAMESPACE = 1ULL << 35ULL,
    INIT_IN_STATIC_BLOCK = 1ULL << 36ULL,
    DYNAMIC = 1ULL << 37ULL,
    OVERLOAD = 1ULL << 38ULL,

    HOIST_VAR = HOIST | VAR,
    CLASS_OR_INTERFACE = CLASS | INTERFACE,
    CLASS_OR_INTERFACE_OR_ENUM = CLASS_OR_INTERFACE | ENUM_LITERAL,
};

enum class LetOrConstStatus {
    INITIALIZED,
    UNINITIALIZED,
};

enum class ScopeFlags : uint32_t {
    NONE = 0U,
    SET_LEXICAL_FUNCTION = 1U << 0U,
    USE_ARGS = 1U << 2U,
    USE_SUPER = 1U << 3U,
    INNER_ARROW = 1U << 4U,
    NO_REG_STORE = 1U << 5U,
    DECL_SCOPE = 1U << 6U,
    FIELD_SCOPE = 1U << 7U,
    METHOD_SCOPE = 1U << 8U,
    STATIC = 1U << 9U,
    TYPE_ALIAS = 1U << 10U,
    LOOP_SCOPE = 1U << 11U,

    STATIC_DECL_SCOPE = DECL_SCOPE | STATIC,
    STATIC_FIELD_SCOPE = FIELD_SCOPE | STATIC,
    STATIC_METHOD_SCOPE = METHOD_SCOPE | STATIC,
};

}  // namespace ark::es2panda::varbinder

namespace enumbitops {

template <>
struct IsAllowedType<ark::es2panda::varbinder::ResolveBindingOptions> : std::true_type {
};

template <>
struct IsAllowedType<ark::es2panda::varbinder::VariableFlags> : std::true_type {
};

template <>
struct IsAllowedType<ark::es2panda::varbinder::ScopeFlags> : std::true_type {
};

}  // namespace enumbitops

#endif

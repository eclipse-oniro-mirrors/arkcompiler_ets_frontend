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

#ifndef ES2PANDA_EVALUATE_HELPERS_H
#define ES2PANDA_EVALUATE_HELPERS_H

#include "checker/ETSchecker.h"
#include "evaluate/varbinderScopes.h"
#include "ir/astNodeFlags.h"
#include "varbinder/ETSBinder.h"

#include "libarkfile/field_data_accessor.h"
#include "libarkfile/method_data_accessor.h"
#include "libarkfile/class_data_accessor.h"
#include "libarkfile/file.h"
#include <libarkfile/include/type.h>

#include <optional>
#include <string>

namespace ark::es2panda::checker {
class Type;
}  // namespace ark::es2panda::checker

namespace ark::es2panda::ir {
class BlockStatement;
class Identifier;
class TypeNode;
class ETSTypeReference;
class ClassProperty;
}  // namespace ark::es2panda::ir

namespace ark::es2panda::parser {
class Program;
}  // namespace ark::es2panda::parser

namespace ark::es2panda::evaluate::helpers {

class SafeStateScope final {
public:
    explicit SafeStateScope(checker::ETSChecker *checker, varbinder::ETSBinder *varBinder);

    ~SafeStateScope();

    NO_COPY_SEMANTIC(SafeStateScope);
    NO_MOVE_SEMANTIC(SafeStateScope);

    void *operator new(size_t) = delete;
    void *operator new[](size_t) = delete;

private:
    checker::ETSChecker *checker_ {nullptr};
    varbinder::ETSBinder *varBinder_ {nullptr};
    varbinder::Scope *checkerScope_ {nullptr};
    varbinder::GlobalScope *binderTopScope_ {nullptr};
    varbinder::VariableScope *binderVarScope_ {nullptr};
    varbinder::Scope *binderScope_ {nullptr};
    parser::Program *binderProgram_ {nullptr};
    varbinder::RecordTable *recordTable_ {nullptr};
};

static inline constexpr std::string_view DEBUGGER_API_CLASS_NAME = "DebuggerAPI";

// CC-OFFNXT(G.PRE.02-CPP) code generation
// CC-OFFNXT(G.PRE.06) solid logic
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define TYPED_ACCESSOR_NAME_SWITCH(TYPE_NAME_BASE)     \
    do {                                               \
        switch (typeId) {                              \
            case panda_file::Type::TypeId::U1:         \
                /* CC-OFFNXT(G.PRE.05) function gen */ \
                return #TYPE_NAME_BASE "Boolean";      \
            case panda_file::Type::TypeId::I8:         \
                /* CC-OFFNXT(G.PRE.05) function gen */ \
                return #TYPE_NAME_BASE "Byte";         \
            case panda_file::Type::TypeId::U16:        \
                /* CC-OFFNXT(G.PRE.05) function gen */ \
                return #TYPE_NAME_BASE "Char";         \
            case panda_file::Type::TypeId::I16:        \
                /* CC-OFFNXT(G.PRE.05) function gen */ \
                return #TYPE_NAME_BASE "Short";        \
            case panda_file::Type::TypeId::I32:        \
                /* CC-OFFNXT(G.PRE.05) function gen */ \
                return #TYPE_NAME_BASE "Int";          \
            case panda_file::Type::TypeId::I64:        \
                /* CC-OFFNXT(G.PRE.05) function gen */ \
                return #TYPE_NAME_BASE "Long";         \
            case panda_file::Type::TypeId::F32:        \
                /* CC-OFFNXT(G.PRE.05) function gen */ \
                return #TYPE_NAME_BASE "Float";        \
            case panda_file::Type::TypeId::F64:        \
                /* CC-OFFNXT(G.PRE.05) function gen */ \
                return #TYPE_NAME_BASE "Double";       \
            case panda_file::Type::TypeId::REFERENCE:  \
                /* CC-OFFNXT(G.PRE.05) function gen */ \
                return #TYPE_NAME_BASE "Object";       \
            default:                                   \
                ES2PANDA_UNREACHABLE();                \
                /* CC-OFFNXT(G.PRE.05) function gen */ \
                return {};                             \
        }                                              \
        /* CC-OFFNXT(G.PRE.05) function gen */         \
        return {};                                     \
    } while (false)

constexpr inline std::string_view CreateGetterName(panda_file::Type::TypeId typeId)
{
    TYPED_ACCESSOR_NAME_SWITCH(getLocal);
}

constexpr inline std::string_view CreateSetterName(panda_file::Type::TypeId typeId)
{
    TYPED_ACCESSOR_NAME_SWITCH(setLocal);
}

#undef TYPED_ACCESSOR_NAME_SWITCH

template <typename F>
void DoScopedAction(checker::ETSChecker *checker, varbinder::ETSBinder *varBinder, parser::Program *program,
                    varbinder::Scope *scope, ir::AstNode *parentClass, F &&action)
{
    ES2PANDA_ASSERT(checker);
    ES2PANDA_ASSERT(varBinder);
    // Must enter either program global scope or a local scope.
    ES2PANDA_ASSERT(program != nullptr || scope != nullptr);

    SafeStateScope s(checker, varBinder);

    auto runInScope = [checker, varBinder, scope, parentClass](auto &&f) {
        RecordTableClassScope recordTableScope(varBinder, parentClass);
        if (scope != nullptr) {
            auto lexScope = varbinder::LexicalScope<varbinder::Scope>::Enter(varBinder, scope);
            checker::ScopeContext checkerScope(checker, scope);
            f();
        } else {
            f();
        }
    };

    if (program != nullptr && program != varBinder->Program()) {
        // Save checker scope because it can differ from VarBinder's scope.
        checker::ScopeContext savedCheckerScope(checker, checker->Scope());
        {
            ProgramScope rcScope(varBinder, program);
            checker->Initialize(varBinder);

            runInScope(std::forward<F>(action));
        }
        // Switch checker's state back after leaving another program's context.
        checker->Initialize(varBinder);
    } else {
        runInScope(std::forward<F>(action));
    }
}

ir::TypeNode *ToTypeNode(std::string_view typeSignature, checker::ETSChecker *checker);

ir::TypeNode *PandaTypeToTypeNode(const panda_file::File &pf, panda_file::FieldDataAccessor &fda,
                                  checker::ETSChecker *checker);

ir::TypeNode *PandaTypeToTypeNode(const panda_file::File &pf, panda_file::Type pandaType,
                                  panda_file::File::EntityId classId, checker::ETSChecker *checker);

std::optional<std::string> ToTypeName(std::string_view typeSignature, checker::GlobalTypesHolder *globalTypes);

panda_file::Type::TypeId GetTypeId(std::string_view typeSignature);

ir::BlockStatement *GetEnclosingBlock(ir::Identifier *ident);

template <typename AccessorType>
ir::ModifierFlags GetModifierFlags(AccessorType &da, bool forceAddPublicFlag)
{
    auto flags = ir::ModifierFlags::NONE;
    if (da.IsStatic()) {
        flags |= ir::ModifierFlags::STATIC;
    }
    if (da.IsFinal()) {
        flags |= ir::ModifierFlags::FINAL;
    }

    if (forceAddPublicFlag) {
        flags |= ir::ModifierFlags::PUBLIC;
    } else {
        if (da.IsPublic()) {
            flags |= ir::ModifierFlags::PUBLIC;
        }
        if (da.IsProtected()) {
            flags |= ir::ModifierFlags::PROTECTED;
        }
        if (da.IsPrivate()) {
            flags |= ir::ModifierFlags::PRIVATE;
        }
    }

    if constexpr (std::is_same_v<AccessorType, panda_file::FieldDataAccessor>) {
        if (da.IsReadonly()) {
            flags |= ir::ModifierFlags::READONLY;
        }
    } else if constexpr (std::is_same_v<AccessorType, panda_file::MethodDataAccessor>) {
        if (da.IsNative()) {
            flags |= ir::ModifierFlags::NATIVE;
        }
        if (da.IsAbstract()) {
            flags |= ir::ModifierFlags::ABSTRACT;
        }
    } else {
        LOG(FATAL, ES2PANDA) << "Should be passed only reference on FieldDataAccessor or MethodDataAccessor.";
    }

    return flags;
}

ir::ModifierFlags GetModifierFlags(panda_file::ClassDataAccessor &da);

// Adds `extProgram` into external programs list of the given `program`.
void AddExternalProgram(parser::Program *program, parser::Program *extProgram, std::string_view moduleName);

ir::ETSTypeReference *CreateETSTypeReference(checker::ETSChecker *checker, util::StringView name);

std::pair<std::string_view, std::string_view> SplitRecordName(std::string_view recordName);

ir::ClassProperty *CreateClassProperty(checker::ETSChecker *checker, std::string_view name, ir::TypeNode *type,
                                       ir::ModifierFlags modifiers);

}  // namespace ark::es2panda::evaluate::helpers

#endif  // ES2PANDA_EVALUATE_HELPERS_H

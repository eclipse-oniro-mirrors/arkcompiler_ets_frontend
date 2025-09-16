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

#ifndef ES2PANDA_COMPILER_ENUM_PRE_CHECK_LOWERING_H
#define ES2PANDA_COMPILER_ENUM_PRE_CHECK_LOWERING_H

#include <string>
#include <string_view>
#include "compiler/lowering/phase.h"
#include "checker/ETSchecker.h"

namespace ark::es2panda::compiler {

class EnumLoweringPhase : public PhaseForDeclarations {
public:
    static constexpr std::string_view STRING_REFERENCE_TYPE {"String"};
    static constexpr std::string_view IDENTIFIER_I {"i"};
    static constexpr std::string_view PARAM_NAME {"name"};
    static constexpr std::string_view PARAM_VALUE {"value"};
    static constexpr std::string_view PARAM_ORDINAL {"ordinal"};
    static constexpr std::string_view ITEMS_ARRAY_NAME {"#ItemsArray"};
    static constexpr std::string_view STRING_VALUES_ARRAY_NAME {checker::ETSEnumType::STRING_VALUES_ARRAY_NAME};
    static constexpr std::string_view NAMES_ARRAY_NAME {checker::ETSEnumType::NAMES_ARRAY_NAME};
    static constexpr std::string_view VALUES_ARRAY_NAME {checker::ETSEnumType::VALUES_ARRAY_NAME};
    static constexpr std::string_view BASE_CLASS_NAME {"BaseEnum"};
    static constexpr std::string_view ORDINAL_NAME {"#ordinal"};
    static constexpr auto ORDINAL_TYPE {ir::PrimitiveType::INT};

    enum EnumType { NOT_SPECIFIED = 0, INT = 1, LONG = 2, DOUBLE = 3, STRING = 4 };

    struct DeclarationFlags {
        // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
        bool isTopLevel;
        bool isLocal;
        bool isNamespace;
        // NOLINTEND(misc-non-private-member-variables-in-classes)

        [[nodiscard]] bool IsValid() const noexcept
        {
            return isTopLevel || isLocal || isNamespace;
        }
    };

    EnumLoweringPhase() noexcept = default;
    std::string_view Name() const override
    {
        return "EnumLoweringPhase";
    }
    bool PerformForModule(public_lib::Context *ctx, parser::Program *program) override;
    checker::ETSChecker *Checker()
    {
        return checker_;
    }

    varbinder::ETSBinder *Varbinder()
    {
        return varbinder_;
    }

private:
    struct FunctionInfo {
        ArenaVector<ir::Expression *> &&params;
        ArenaVector<ir::Statement *> &&body;
        ir::TypeNode *returnTypeAnnotation;
        const ir::TSEnumDeclaration *enumDecl;
        ir::ModifierFlags flags;
    };

    void LogError(const diagnostic::DiagnosticKind &diagnostic, const util::DiagnosticMessageParams &diagnosticParams,
                  const lexer::SourcePosition &pos);

    // clang-format off
    template <EnumLoweringPhase::EnumType TYPE_NODE>
    bool CheckEnumMemberType(const ArenaVector<ir::AstNode *> &enumMembers, bool &hasLoggedError,
                             bool &hasLongLiteral);
    // clang-format on

    [[nodiscard]] ir::ScriptFunction *MakeFunction(FunctionInfo &&functionInfo);
    ir::ClassDeclaration *CreateClass(ir::TSEnumDeclaration *const enumDecl, const DeclarationFlags flags,
                                      EnumType enumType);
    ir::ClassProperty *CreateOrdinalField(ir::ClassDefinition *const enumClass);
    ir::MemberExpression *CreateOrdinalAccessExpression();
    void CreateCCtorForEnumClass(ir::ClassDefinition *const enumClass);
    void CreateCtorForEnumClass(ir::ClassDefinition *const enumClass, EnumType enumType);
    ir::ScriptFunction *CreateFunctionForCtorOfEnumClass(ir::ClassDefinition *const enumClass, EnumType enumType);

    void ProcessEnumClassDeclaration(ir::TSEnumDeclaration *const enumDecl, const DeclarationFlags &flags,
                                     ir::ClassDeclaration *enumClassDecl);
    template <ir::PrimitiveType TYPE>
    ir::ClassDeclaration *CreateEnumIntClassFromEnumDeclaration(ir::TSEnumDeclaration *const enumDecl,
                                                                const DeclarationFlags flags);
    template <ir::PrimitiveType TYPE>
    ir::ClassDeclaration *CreateEnumFloatClassFromEnumDeclaration(ir::TSEnumDeclaration *const enumDecl,
                                                                  const DeclarationFlags flags);
    ir::ClassDeclaration *CreateEnumStringClassFromEnumDeclaration(ir::TSEnumDeclaration *const enumDecl,
                                                                   const DeclarationFlags flags);
    static void AppendParentNames(util::UString &qualifiedName, const ir::AstNode *const node);
    template <typename ElementMaker>
    [[nodiscard]] ir::Identifier *MakeArray(const ir::TSEnumDeclaration *const enumDecl, ir::ClassDefinition *enumClass,
                                            const util::StringView &name, ir::TypeNode *const typeAnnotation,
                                            ElementMaker &&elementMaker);
    void CreateEnumItemFields(const ir::TSEnumDeclaration *const enumDecl, ir::ClassDefinition *enumClass,
                              EnumType enumType);
    ir::Identifier *CreateEnumNamesArray(const ir::TSEnumDeclaration *const enumDecl, ir::ClassDefinition *enumClass);
    template <ir::PrimitiveType TYPE>
    ir::Identifier *CreateEnumValuesArray(const ir::TSEnumDeclaration *const enumDecl, ir::ClassDefinition *enumClass);
    ir::Identifier *CreateEnumStringValuesArray(const ir::TSEnumDeclaration *const enumDecl,
                                                ir::ClassDefinition *enumClass);
    ir::Identifier *CreateEnumItemsArray(const ir::TSEnumDeclaration *const enumDecl, ir::ClassDefinition *enumClass);
    void CreateEnumToStringMethod(ir::TSEnumDeclaration const *const enumDecl, ir::ClassDefinition *const enumClass,
                                  ir::Identifier *const stringValuesArrayIdent);
    void CreateEnumValueOfMethod(ir::TSEnumDeclaration const *const enumDecl, ir::ClassDefinition *const enumClass,
                                 ir::Identifier *const valuesArrayIdent,
                                 std::optional<ir::PrimitiveType> primitiveType);
    void CreateEnumGetNameMethod(ir::TSEnumDeclaration const *const enumDecl, ir::ClassDefinition *const enumClass,
                                 ir::Identifier *const namesArrayIdent);
    void CreateEnumGetValueOfMethod(ir::TSEnumDeclaration const *const enumDecl, ir::ClassDefinition *const enumClass,
                                    ir::Identifier *const namesArrayIdent, ir::Identifier *const itemsArrayIdent);
    void CreateEnumFromValueMethod(ir::TSEnumDeclaration const *const enumDecl, ir::ClassDefinition *const enumClass,
                                   ir::Identifier *const valuesArrayIdent, ir::Identifier *const itemsArrayIdent,
                                   std::optional<ir::PrimitiveType> primitiveType);
    void CreateEnumValuesMethod(ir::TSEnumDeclaration const *const enumDecl, ir::ClassDefinition *const enumClass,
                                ir::Identifier *const itemsArrayIdent);
    void CreateEnumGetOrdinalMethod(ir::TSEnumDeclaration const *const enumDecl, ir::ClassDefinition *const enumClass);
    void CreateEnumDollarGetMethod(ir::TSEnumDeclaration const *const enumDecl, ir::ClassDefinition *const enumClass);
    void SetDefaultPositionInUnfilledClassNodes(const ir::ClassDeclaration *enumClassDecl,
                                                ir::TSEnumDeclaration const *const enumDecl);
    ir::Expression *CheckEnumTypeForItemFields(EnumType enumType, ir::TSEnumMember *const member);
    checker::AstNodePtr TransformEnumChildrenRecursively(checker::AstNodePtr &ast);
    checker::AstNodePtr TransformAnnotedEnumChildrenRecursively(checker::AstNodePtr &ast);
    ArenaAllocator *Allocator();

    template <typename T, typename... Args>
    T *AllocNode(Args &&...args);

private:
    public_lib::Context *context_ {nullptr};
    checker::ETSChecker *checker_ {nullptr};
    parser::Program *program_ {nullptr};
    varbinder::ETSBinder *varbinder_ {nullptr};
};

}  // namespace ark::es2panda::compiler

#endif  // ES2PANDA_COMPILER_ENUM_PRE_CHECK_LOWERING_H

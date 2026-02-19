/**
 * Copyright (c) 2021-2026 Huawei Device Co., Ltd.
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

#ifndef ES2PANDA_COMPILER_CHECKER_TYPES_ETS_ENUM_TYPE_H
#define ES2PANDA_COMPILER_CHECKER_TYPES_ETS_ENUM_TYPE_H

#include "checker/types/ets/etsObjectType.h"
#include "checker/types/ets/etsObjectTypeConstants.h"
#include "checker/types/typeFlag.h"
#include "ir/base/classProperty.h"
#include "ir/expressions/arrayExpression.h"
#include "ir/expressions/literals/stringLiteral.h"
#include "ir/expressions/memberExpression.h"
#include "util/es2pandaMacros.h"

namespace ark::es2panda::checker {

class ETSEnumType : public ETSObjectType {
public:
    NO_COPY_SEMANTIC(ETSEnumType);
    NO_MOVE_SEMANTIC(ETSEnumType);

    ETSEnumType() = delete;
    ~ETSEnumType() override = default;

    static constexpr std::string_view TO_STRING_METHOD_NAME {"toString"};
    static constexpr std::string_view VALUE_OF_METHOD_NAME {"valueOf"};
    static constexpr std::string_view GET_NAME_METHOD_NAME {"getName"};
    static constexpr std::string_view GET_VALUE_OF_METHOD_NAME {"getValueOf"};
    static constexpr std::string_view FROM_VALUE_METHOD_NAME {"fromValue"};
    static constexpr std::string_view VALUES_METHOD_NAME {"values"};
    static constexpr std::string_view GET_ORDINAL_METHOD_NAME {"getOrdinal"};
    static constexpr std::string_view DOLLAR_GET_METHOD_NAME {"$_get"};
    static constexpr std::string_view ITEMS_ARRAY_NAME {"#ItemsArray"};

    auto GetOrdinalFromMemberName(std::string_view name) const
    {
        return memberNameToOrdinal_.at(name);
    }

    auto GetValueLiteralFromOrdinal(size_t ord) const
    {
        ES2PANDA_ASSERT(ord < membersValues_.size());
        return membersValues_[ord];
    }

    bool NodeIsEnumLiteral(ir::Expression *node) const
    {
        ES2PANDA_ASSERT(node->TsType() == this);
        if (!node->IsMemberExpression()) {
            return false;
        }

        auto mobj = node->AsMemberExpression()->Object();
        if (mobj->TsType() == this) {
            // No need to search properties since enum-literals are the only enum-type properties
            // NOTE(dkofanov): For some reason, 'enumLowering' changes 'CLASS' to 'ENUM_LITERAL', instead of 'ENUM'.
            ES2PANDA_ASSERT(GetDeclNode()->AsClassDefinition()->IsEnumTransformed());
            return true;
        }
        return false;
    }

    [[nodiscard]] Type *EnumAnnotedType() noexcept
    {
        return enumType_;
    }

    void SetEnumType(ir::TypeNode *typeNode, ETSChecker *checker) noexcept;

    [[nodiscard]] const Type *EnumAnnotedType() const noexcept
    {
        return enumType_;
    }

    virtual bool CheckBuiltInType(const ETSChecker *checker, ETSObjectFlags flag) const noexcept = 0;

    Type *Underlying() const;

protected:
    // CC-OFFNXT(G.FUN.01-CPP) solid logic
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    explicit ETSEnumType(ArenaAllocator *allocator, util::StringView name, util::StringView internalName,
                         ir::AstNode *declNode, TypeRelation *relation, ETSObjectFlags const flag)
        : ETSObjectType(allocator, name, internalName,
                        std::make_tuple(declNode, ETSObjectFlags::CLASS | flag, relation)),
          memberNameToOrdinal_(allocator->Adapter())
    {
        InitElementsShortcuts(declNode->AsClassDefinition());
    }

    Type *enumType_ {nullptr};  // NOLINT(misc-non-private-member-variables-in-classes)

private:
    void InitElementsShortcuts(ir::ClassDefinition *declNode)
    {
        ArenaVector<util::StringView> membersNames(Allocator()->Adapter());
        for (auto item : declNode->OrigEnumDecl()->Members()) {
            membersNames.push_back(item->AsTSEnumMember()->Name());
        }
        for (auto name : membersNames) {
            for (auto elem : declNode->Body()) {
                if (elem->AsClassElement()->Key()->AsIdentifier()->Name() == name) {
                    membersValues_.push_back(
                        elem->AsClassElement()->Value()->AsETSNewClassInstanceExpression()->GetArguments()[1]);
                    break;
                }
            }
        }
        auto membersValues = Span {membersValues_};
        ES2PANDA_ASSERT(membersValues.size() == membersNames.size());
        for (size_t i = 0; i < membersNames.size(); i++) {
            memberNameToOrdinal_.insert({membersNames[i], i});
            ES2PANDA_ASSERT(membersValues[i]->IsStringLiteral() || membersValues[i]->IsNumberLiteral());
        }
    }

private:
    ArenaMap<util::StringView, size_t> memberNameToOrdinal_;
    ArenaVector<ir::Expression *> membersValues_;
};

class ETSNumericEnumType : public ETSEnumType {
public:
    explicit ETSNumericEnumType(ArenaAllocator *allocator, util::StringView name, util::StringView internalName,
                                ir::AstNode *declNode, TypeRelation *relation)
        : ETSEnumType(allocator, name, internalName, declNode, relation, ETSObjectFlags::NUMERIC_ENUM_OBJECT)
    {
        AddTypeFlag(checker::TypeFlag::ETS_NUMERIC_ENUM);
    }

    NO_COPY_SEMANTIC(ETSNumericEnumType);
    NO_MOVE_SEMANTIC(ETSNumericEnumType);

    ETSNumericEnumType() = delete;
    ~ETSNumericEnumType() override = default;

    bool AssignmentSource(TypeRelation *relation, Type *target) override;
    void AssignmentTarget(TypeRelation *relation, Type *source) override;
    void Cast(TypeRelation *relation, Type *target) override;
    void CastTarget(TypeRelation *relation, Type *source) override;
    [[nodiscard]] bool CheckBuiltInType(const ETSChecker *checker, ETSObjectFlags flag) const noexcept override;

    [[nodiscard]] bool NonAnnotedHasDouble(ETSChecker *checker) const;
};

class ETSStringEnumType : public ETSEnumType {
public:
    explicit ETSStringEnumType(ArenaAllocator *allocator, util::StringView name, util::StringView internalName,
                               ir::AstNode *declNode, TypeRelation *relation)
        : ETSEnumType(allocator, name, internalName, declNode, relation, ETSObjectFlags::STRING_ENUM_OBJECT)
    {
        AddTypeFlag(checker::TypeFlag::ETS_STRING_ENUM);
    }

    NO_COPY_SEMANTIC(ETSStringEnumType);
    NO_MOVE_SEMANTIC(ETSStringEnumType);

    ETSStringEnumType() = delete;
    ~ETSStringEnumType() override = default;

    bool AssignmentSource(TypeRelation *relation, Type *target) override;
    void AssignmentTarget(TypeRelation *relation, Type *source) override;
    void Cast(TypeRelation *relation, Type *target) override;
    void CastTarget(TypeRelation *relation, Type *source) override;
    [[nodiscard]] bool CheckBuiltInType(const ETSChecker *checker, ETSObjectFlags flag) const noexcept override;
};

}  // namespace ark::es2panda::checker

#endif

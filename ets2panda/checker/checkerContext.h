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

#ifndef ES2PANDA_CHECKER_CHECKER_CONTEXT_H
#define ES2PANDA_CHECKER_CHECKER_CONTEXT_H

#include "es2panda.h"
#include "generated/tokenType.h"
#include "lexer/token/sourceLocation.h"
#include "util/enumbitops.h"

namespace ark::es2panda::ir {
class ArrowFunctionExpression;
class AstNode;
class LoopStatement;
class BlockStatement;
class Identifier;
class UnaryExpression;
class BreakStatement;
class Statement;
class BinaryExpression;
}  // namespace ark::es2panda::ir
namespace ark::es2panda::varbinder {
class Variable;
}  // namespace ark::es2panda::varbinder

namespace ark::es2panda::checker {

class ETSObjectType;
class Signature;
class Type;
class Checker;

using ENUMBITOPS_OPERATORS;

enum class CheckerStatus : uint32_t {
    NO_OPTS = 0U,
    FORCE_TUPLE = 1U << 0U,
    IN_CONST_CONTEXT = 1U << 1U,
    KEEP_LITERAL_TYPE = 1U << 2U,
    IN_PARAMETER = 1U << 3U,
    IN_CLASS = 1U << 4U,
    IN_INTERFACE = 1U << 5U,
    IN_ABSTRACT = 1U << 6U,
    IN_STATIC_CONTEXT = 1U << 7U,
    IN_CONSTRUCTOR = 1U << 8U,
    IN_STATIC_BLOCK = 1U << 9U,
    INNER_CLASS = 1U << 10U,
    IN_ENUM = 1U << 11U,
    BUILTINS_INITIALIZED = 1U << 12U,
    IN_LAMBDA = 1U << 13U,
    IGNORE_VISIBILITY = 1U << 14U,
    IN_EXTENSION_METHOD = 1U << 15U,
    IN_LOCAL_CLASS = 1U << 16U,
    IN_INSTANCEOF_CONTEXT = 1U << 17U,
    IN_TEST_EXPRESSION = 1U << 18U,
    IN_LOOP = 1U << 19U,
    MEET_RETURN = 1U << 20U,
    MEET_BREAK = 1U << 21U,
    MEET_CONTINUE = 1U << 22U,
    MEET_THROW = 1U << 23U,
    IN_EXTERNAL = 1U << 24U,
    IN_BRIDGE_TEST = 1U << 25U,
    IN_GETTER = 1U << 26U,
    IN_SETTER = 1U << 27U,
    IN_EXTENSION_ACCESSOR_CHECK = 1U << 28U,
    IN_TYPE_INFER = 1U << 29U,
    RESTRICTED_RETURN_IN_BLOCK = 1U << 30U
};

}  // namespace ark::es2panda::checker

template <>
struct enumbitops::IsAllowedType<ark::es2panda::checker::CheckerStatus> : std::true_type {
};

namespace ark::es2panda::checker {

using CapturedVarsMap = ArenaUnorderedMap<varbinder::Variable *, lexer::SourcePosition>;
using SmartCastMap = ArenaMap<varbinder::Variable const *, checker::Type *>;
using SmartCastArray = std::vector<std::pair<varbinder::Variable const *, checker::Type *>>;
using SmartCastTestMap = ArenaMap<varbinder::Variable const *, std::pair<checker::Type *, checker::Type *>>;
using SmartCastTuple = std::tuple<varbinder::Variable const *, checker::Type *, checker::Type *>;
using SmartCastTestArray = std::vector<SmartCastTuple>;
using PreservedSmartCastsMap = ArenaMultiMap<ir::AstNode const *, SmartCastArray>;

// ReassignedVariableMap
//  - key: a variable that is on the left side of an assignment
//  - value: a boolean that marks, if the variable was accessed after it has been reassigned
using ReassignedVariableMap = std::unordered_map<varbinder::Variable const *, bool>;

struct SmartCastCondition final {
    SmartCastCondition() = default;
    ~SmartCastCondition() = default;

    DEFAULT_COPY_SEMANTIC(SmartCastCondition);
    DEFAULT_MOVE_SEMANTIC(SmartCastCondition);

    // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
    varbinder::Variable const *variable = nullptr;
    checker::Type *testedType = nullptr;
    bool negate = false;
    bool strict = true;
    // NOLINTEND(misc-non-private-member-variables-in-classes)
};

using SmartCastTypes = std::optional<SmartCastTestArray>;

class CheckerContext final {
public:
    explicit CheckerContext(Checker *checker, CheckerStatus newStatus) : CheckerContext(checker, newStatus, nullptr) {}

    explicit CheckerContext(Checker *checker, CheckerStatus newStatus, const ETSObjectType *containingClass)
        : CheckerContext(checker, newStatus, containingClass, nullptr)
    {
    }

    explicit CheckerContext(Checker *checker, CheckerStatus newStatus, const ETSObjectType *containingClass,
                            Signature *containingSignature);

    CheckerContext() = delete;
    ~CheckerContext() = default;

    DEFAULT_COPY_SEMANTIC(CheckerContext);
    DEFAULT_MOVE_SEMANTIC(CheckerContext);

    [[nodiscard]] const CapturedVarsMap &CapturedVars() const noexcept
    {
        return capturedVars_;
    }

    [[nodiscard]] CapturedVarsMap &CapturedVars() noexcept
    {
        return capturedVars_;
    }

    [[nodiscard]] const CheckerStatus &Status() const noexcept
    {
        return status_;
    }

    [[nodiscard]] ETSObjectType *ContainingClass() const noexcept
    {
        return const_cast<ETSObjectType *>(containingClass_);
    }

    [[nodiscard]] Signature *ContainingSignature() const noexcept
    {
        return containingSignature_;
    }

    [[nodiscard]] CheckerStatus &Status() noexcept
    {
        return status_;
    }

    [[nodiscard]] varbinder::Variable *ContainingOverloadDeclaration() noexcept
    {
        return containingOverloadDeclaration_;
    }

    void SetContainingSignature(Signature *containingSignature) noexcept
    {
        containingSignature_ = containingSignature;
    }

    void SetContainingClass(ETSObjectType *containingClass) noexcept
    {
        containingClass_ = containingClass;
    }

    void SetContainingOverloadDeclaration(varbinder::Variable *containingOverloadDeclaration) noexcept
    {
        containingOverloadDeclaration_ = containingOverloadDeclaration;
    }

    void AddCapturedVar(varbinder::Variable *var, const lexer::SourcePosition &pos)
    {
        capturedVars_.emplace(var, pos);
    }
    [[nodiscard]] ir::ArrowFunctionExpression *ContainingLambda() const noexcept
    {
        return containingLambda_;
    }

    void SetContainingLambda(ir::ArrowFunctionExpression *containingLambda) noexcept
    {
        containingLambda_ = containingLambda;
    }

    void ClearSmartCasts() noexcept
    {
        smartCasts_.clear();
    }

    void RemoveSmartCast(varbinder::Variable const *const variable) noexcept
    {
        smartCasts_.erase(variable);
    }

    void SetSmartCast(varbinder::Variable const *const variable, checker::Type *const smartType) noexcept;

    [[nodiscard]] checker::Type *GetSmartCast(varbinder::Variable const *const variable) const noexcept;
    [[nodiscard]] SmartCastArray CloneSmartCasts(bool clearData = false) noexcept;
    void RestoreSmartCasts(SmartCastArray const &otherSmartCasts);
    void CombineSmartCasts(SmartCastArray const &otherSmartCasts);

    [[nodiscard]] SmartCastArray EnterTestExpression() noexcept
    {
        status_ |= CheckerStatus::IN_TEST_EXPRESSION;
        ClearTestSmartCasts();
        return CloneSmartCasts(false);
    }

    [[nodiscard]] bool IsInTestExpression() const noexcept
    {
        return (status_ & CheckerStatus::IN_TEST_EXPRESSION) != 0;
    }

    SmartCastTypes ExitTestExpression()
    {
        status_ &= ~CheckerStatus::IN_TEST_EXPRESSION;
        CheckTestSmartCastCondition(lexer::TokenType::EOS);
        return CloneTestSmartCasts(true);
    }

    [[nodiscard]] std::pair<SmartCastArray, bool> EnterLoop(const ir::LoopStatement &loop,
                                                            SmartCastTypes loopConditionSmartCasts) noexcept;

    [[nodiscard]] bool IsInLoop() const noexcept
    {
        return (status_ & CheckerStatus::IN_LOOP) != 0;
    }

    void ExitLoop(SmartCastArray &prevSmartCasts, bool clearFlag, ir::LoopStatement *loopStatement) noexcept;

    void EnterPath() noexcept
    {
        status_ &= ~(CheckerStatus::MEET_RETURN | CheckerStatus::MEET_BREAK | CheckerStatus::MEET_CONTINUE |
                     CheckerStatus::MEET_THROW);
    }

    [[nodiscard]] bool ExitPath() noexcept
    {
        auto const rc = (status_ & (CheckerStatus::MEET_RETURN | CheckerStatus::MEET_BREAK |
                                    CheckerStatus::MEET_CONTINUE | CheckerStatus::MEET_THROW)) != 0;
        status_ &= ~(CheckerStatus::MEET_RETURN | CheckerStatus::MEET_BREAK | CheckerStatus::MEET_CONTINUE |
                     CheckerStatus::MEET_THROW);
        return rc;
    }

    [[nodiscard]] SmartCastArray CheckTryBlock(ir::BlockStatement const &tryBlock) noexcept;

    checker::Type *GetIntersectionOfTypes(checker::Type *type1, checker::Type *type2) const noexcept;
    checker::Type *GetUnionOfTypes(checker::Type *type1, checker::Type *type2) const noexcept;
    void MergeSmartTypesForLogicalAnd(SmartCastTuple &newSmartCastTypes);
    void InvalidateNecessarySmartCastsInLogicalAnd(std::optional<SmartCastTuple> &newSmartCastTypes);
    ReassignedVariableMap GetReassignedVariablesInNode(const ir::AstNode *node) const;
    void CheckTestSmartCastCondition(lexer::TokenType operatorType);
    void CheckIdentifierSmartCastCondition(ir::Identifier const *identifier) noexcept;
    void CheckUnarySmartCastCondition(ir::UnaryExpression const *unaryExpression) noexcept;
    void CheckBinarySmartCastCondition(ir::BinaryExpression *binaryExpression) noexcept;

    void OnBreakStatement(ir::BreakStatement const *breakStatement);
    void AddBreakSmartCasts(ir::Statement const *targetStatement, SmartCastArray &&smartCasts);
    void CombineBreakSmartCasts(ir::Statement const *targetStatement);
    friend class SavedCheckerContextStatus;

private:
    Checker *parent_;
    CheckerStatus status_;
    CapturedVarsMap capturedVars_;
    SmartCastMap smartCasts_;
    const ETSObjectType *containingClass_ {nullptr};
    ir::ArrowFunctionExpression *containingLambda_ {nullptr};
    Signature *containingSignature_ {nullptr};
    varbinder::Variable *containingOverloadDeclaration_ {nullptr};

    lexer::TokenType operatorType_ = lexer::TokenType::EOS;
    SmartCastCondition testCondition_ {};
    SmartCastTestMap testSmartCasts_;

    PreservedSmartCastsMap breakSmartCasts_;

    void RemoveSmartCasts(SmartCastArray const &otherSmartCasts) noexcept;
    [[nodiscard]] checker::Type *CombineTypes(checker::Type *typeOne, checker::Type *typeTwo) const noexcept;
    [[nodiscard]] static bool IsInValidChain(ir::AstNode const *parent) noexcept;
    void CheckSmartCastEqualityCondition(ir::BinaryExpression *binaryExpression) noexcept;
    [[nodiscard]] SmartCastTypes CloneTestSmartCasts(bool clearData = true) noexcept;
    void ClearTestSmartCasts() noexcept;
    [[nodiscard]] std::optional<SmartCastTuple> ResolveSmartCastTypes();
    [[nodiscard]] bool CheckTestOrSmartCastCondition(SmartCastTuple const &types);
    void CheckAssignments(ir::AstNode const *node, ReassignedVariableMap &changedVariables) const noexcept;
};

class SavedCheckerContextStatus final {
public:
    explicit SavedCheckerContextStatus(CheckerContext *context, CheckerStatus status)
        : context_(context), storedStatus_(status), preStatus_(context_->status_)
    {
        context_->status_ = context_->status_ | storedStatus_;
    }

    ~SavedCheckerContextStatus()
    {
        context_->status_ = preStatus_;
    }

    NO_COPY_SEMANTIC(SavedCheckerContextStatus);
    DEFAULT_MOVE_SEMANTIC(SavedCheckerContextStatus);

private:
    CheckerContext *context_;
    CheckerStatus storedStatus_;
    CheckerStatus preStatus_;
};
}  // namespace ark::es2panda::checker

#endif

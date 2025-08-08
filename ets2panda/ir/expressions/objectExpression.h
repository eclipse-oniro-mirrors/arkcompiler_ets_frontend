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

#ifndef ES2PANDA_IR_EXPRESSION_OBJECT_EXPRESSION_H
#define ES2PANDA_IR_EXPRESSION_OBJECT_EXPRESSION_H

#include "varbinder/variable.h"
#include "ir/expression.h"
#include "ir/validationInfo.h"
namespace ark::es2panda::checker {
class ETSAnalyzer;
}  // namespace ark::es2panda::checker
namespace ark::es2panda::util {
class BitSet;
}  // namespace ark::es2panda::util

namespace ark::es2panda::ir {
class ObjectExpression : public AnnotatedExpression {
private:
    struct Tag {};

public:
    ObjectExpression() = delete;
    ~ObjectExpression() override = default;

    NO_COPY_SEMANTIC(ObjectExpression);
    NO_MOVE_SEMANTIC(ObjectExpression);

    explicit ObjectExpression(AstNodeType nodeType, [[maybe_unused]] ArenaAllocator *allocator,
                              ArenaVector<Expression *> &&properties, bool trailingComma)
        : AnnotatedExpression(nodeType), properties_(std::move(properties)), trailingComma_(trailingComma)
    {
    }
    explicit ObjectExpression(Tag tag, ObjectExpression const &other, ArenaAllocator *allocator);

    // NOTE (vivienvoros): these friend relationships can be removed once there are getters for private fields
    friend class checker::ETSAnalyzer;

    [[nodiscard]] const ArenaVector<Expression *> &Properties() const noexcept
    {
        return properties_;
    }

    [[nodiscard]] bool IsDeclaration() const noexcept
    {
        return isDeclaration_;
    }

    [[nodiscard]] bool IsOptional() const noexcept
    {
        return optional_;
    }

    [[nodiscard]] ObjectExpression *Clone(ArenaAllocator *allocator, AstNode *parent) override;

    [[nodiscard]] bool ConvertibleToObjectPattern();
    void SetDeclaration();
    void SetOptional(bool optional);
    void TransformChildren(const NodeTransformer &cb, std::string_view transformationName) override;
    void Iterate(const NodeTraverser &cb) const override;
    void Dump(ir::AstDumper *dumper) const override;
    void Dump(ir::SrcDumper *dumper) const override;
    void Compile(compiler::PandaGen *pg) const override;
    void Compile(compiler::ETSGen *etsg) const override;
    checker::Type *Check(checker::TSChecker *checker) override;
    checker::VerifiedType Check(checker::ETSChecker *checker) override;
    struct CheckPatternIsShorthandArgs {
        checker::TSChecker *checker;
        checker::Type *patternParamType;
        varbinder::Variable *bindingVar;
        ir::Property *prop;
        varbinder::LocalVariable *foundVar;
        bool isOptional;
    };
    checker::Type *CheckPattern(checker::TSChecker *checker);

    bool CheckAssignmentPattern(Property *prop, varbinder::Variable *&bindingVar, checker::Type *&patternParamType,
                                checker::TSChecker *checker, varbinder::LocalVariable *foundVar);

    void Accept(ASTVisitorT *v) override
    {
        v->Accept(this);
    }

    void CleanUp() override
    {
        AstNode::CleanUp();
        SetPreferredType(nullptr);
    }

    void CleanCheckInformation() override;

private:
    std::tuple<bool, varbinder::Variable *, checker::Type *, varbinder::LocalVariable *> CheckPatternIsShorthand(
        CheckPatternIsShorthandArgs *args);

    ArenaVector<Expression *> properties_;
    bool isDeclaration_ {};
    bool trailingComma_ {};
    bool optional_ {};
};
}  // namespace ark::es2panda::ir

#endif

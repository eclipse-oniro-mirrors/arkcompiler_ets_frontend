/**
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

#ifndef ES2PANDA_IR_STATEMENT_ANNOTATION_DECLARATION_H
#define ES2PANDA_IR_STATEMENT_ANNOTATION_DECLARATION_H

#include "ir/annotationAllowed.h"
#include "varbinder/scope.h"
#include "varbinder/variable.h"
#include "ir/statement.h"
#include "ir/astNode.h"
#include "ir/expressions/identifier.h"

namespace ark::es2panda::ir {

using ENUMBITOPS_OPERATORS;

enum class RetentionPolicy : uint8_t { SOURCE = 1U << 0U, BYTECODE = 1U << 1U, RUNTIME = 1U << 2U };

enum class AnnotationTargets : uint32_t {
    NONE = 0U,
    // Top-Level Declarations
    CLASS = 1U << 0U,
    ENUMERATION = 1U << 1U,
    FUNCTION = 1U << 2U,
    FUNCTION_WITH_RECEIVER = 1U << 3U,
    INTERFACE = 1U << 4U,
    NAMESPACE = 1U << 5U,
    TYPE_ALIAS = 1U << 6U,
    VARIABLE = 1U << 7U,

    // Class Members
    CLASS_FIELD = 1U << 8U,
    CLASS_METHOD = 1U << 9U,
    CLASS_GETTER = 1U << 10U,
    CLASS_SETTER = 1U << 11U,

    // Interface Members
    INTERFACE_PROPERTY = 1U << 12U,
    INTERFACE_METHOD = 1U << 13U,
    INTERFACE_GETTER = 1U << 14U,
    INTERFACE_SETTER = 1U << 15U,

    // Other targets
    LAMBDA = 1U << 16U,
    PARAMETER = 1U << 17U,
    STRUCT = 1U << 18U,
    TYPE = 1U << 19U
};
}  // namespace ark::es2panda::ir

template <>
struct enumbitops::IsAllowedType<ark::es2panda::ir::RetentionPolicy> : std::true_type {
};

template <>
struct enumbitops::IsAllowedType<ark::es2panda::ir::AnnotationTargets> : std::true_type {
};

namespace ark::es2panda::ir {
class AnnotationDeclaration : public AnnotationAllowed<Statement> {
public:
    explicit AnnotationDeclaration(Expression *expr, ArenaAllocator *allocator)
        : AnnotationAllowed<Statement>(AstNodeType::ANNOTATION_DECLARATION, allocator),
          expr_(expr),
          properties_(allocator->Adapter()),
          targets_(allocator->Adapter())
    {
        InitHistory();
    }
    explicit AnnotationDeclaration(Expression *expr, ArenaVector<AstNode *> &&properties, ArenaAllocator *allocator)
        : AnnotationAllowed<Statement>(AstNodeType::ANNOTATION_DECLARATION, allocator),
          expr_(expr),
          properties_(std::move(properties)),
          targets_(allocator->Adapter())
    {
        InitHistory();
    }

    const util::StringView &InternalName() const
    {
        return GetHistoryNodeAs<AnnotationDeclaration>()->internalName_;
    }

    void SetInternalName(util::StringView internalName);

    [[nodiscard]] const Expression *Expr() const noexcept
    {
        return GetHistoryNodeAs<AnnotationDeclaration>()->expr_;
    }

    [[nodiscard]] Expression *Expr() noexcept
    {
        return GetHistoryNodeAs<AnnotationDeclaration>()->expr_;
    }

    [[nodiscard]] const ArenaVector<AstNode *> &Properties();
    [[nodiscard]] ArenaVector<AstNode *> &PropertiesForUpdate();

    [[nodiscard]] const ArenaVector<AstNode *> &Properties() const noexcept
    {
        return GetHistoryNodeAs<AnnotationDeclaration>()->properties_;
    }

    [[nodiscard]] const ArenaVector<AstNode *> *PropertiesPtr() const
    {
        return &Properties();
    }

    void AddProperties(ArenaVector<AstNode *> &&properties)
    {
        auto newNode = reinterpret_cast<AnnotationDeclaration *>(this->GetOrCreateHistoryNode());
        newNode->properties_ = std::move(properties);
    }

    [[nodiscard]] bool IsSourceRetention() const noexcept
    {
        return (Policy() & RetentionPolicy::SOURCE) != 0;
    }

    [[nodiscard]] bool IsBytecodeRetention() const noexcept
    {
        return (Policy() & RetentionPolicy::BYTECODE) != 0;
    }

    [[nodiscard]] bool IsRuntimeRetention() const noexcept
    {
        return (Policy() & RetentionPolicy::RUNTIME) != 0;
    }

    void SetSourceRetention() noexcept
    {
        GetOrCreateHistoryNodeAs<AnnotationDeclaration>()->policy_ = RetentionPolicy::SOURCE;
    }

    void SetBytecodeRetention() noexcept
    {
        GetOrCreateHistoryNodeAs<AnnotationDeclaration>()->policy_ = RetentionPolicy::BYTECODE;
    }

    void SetRuntimeRetention() noexcept
    {
        GetOrCreateHistoryNodeAs<AnnotationDeclaration>()->policy_ = RetentionPolicy::RUNTIME;
    }

    [[nodiscard]] const ArenaVector<AnnotationTargets> &Targets() const noexcept
    {
        return GetHistoryNodeAs<AnnotationDeclaration>()->targets_;
    }

    void AddTargets(ArenaVector<AnnotationTargets> &&targets)
    {
        auto newNode = reinterpret_cast<AnnotationDeclaration *>(this->GetOrCreateHistoryNode());
        newNode->targets_ = std::move(targets);
    }

    void TransformChildren(const NodeTransformer &cb, std::string_view transformationName) override;
    void Iterate(const NodeTraverser &cb) const override;
    void Dump(ir::AstDumper *dumper) const override;
    void Dump(ir::SrcDumper *dumper) const override;
    void Compile(compiler::PandaGen *pg) const override;
    void Compile(compiler::ETSGen *etsg) const override;
    checker::Type *Check(checker::TSChecker *checker) override;
    checker::VerifiedType Check(checker::ETSChecker *checker) override;

    void Accept(ASTVisitorT *v) override
    {
        v->Accept(this);
    }

    [[nodiscard]] bool IsScopeBearer() const noexcept override
    {
        return true;
    }

    [[nodiscard]] varbinder::LocalScope *Scope() const noexcept override
    {
        return GetHistoryNodeAs<AnnotationDeclaration>()->scope_;
    }

    void SetScope(varbinder::LocalScope *scope)
    {
        ES2PANDA_ASSERT(scope_ == nullptr);
        GetOrCreateHistoryNodeAs<AnnotationDeclaration>()->scope_ = scope;
    }

    void ClearScope() noexcept override
    {
        GetOrCreateHistoryNodeAs<AnnotationDeclaration>()->scope_ = nullptr;
    }

    Identifier *GetBaseName() const;

    void EmplaceProperties(AstNode *properties);
    void ClearProperties();
    void SetValueProperties(AstNode *properties, size_t index);

    AnnotationDeclaration *Construct(ArenaAllocator *allocator) override;
    void CopyTo(AstNode *other) const override;

private:
    friend class SizeOfNodeTest;
    RetentionPolicy Policy() const
    {
        return GetHistoryNodeAs<AnnotationDeclaration>()->policy_;
    }

    void SetExpr(Expression *expr);

    util::StringView internalName_ {};
    varbinder::LocalScope *scope_ {};
    Expression *expr_;
    ArenaVector<AstNode *> properties_;
    ArenaVector<AnnotationTargets> targets_;
    RetentionPolicy policy_ = RetentionPolicy::BYTECODE;
};
}  // namespace ark::es2panda::ir

#endif

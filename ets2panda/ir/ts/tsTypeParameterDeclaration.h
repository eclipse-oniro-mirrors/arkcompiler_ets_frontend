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

#ifndef ES2PANDA_IR_TS_TYPE_PARAMETER_DECLARATION_H
#define ES2PANDA_IR_TS_TYPE_PARAMETER_DECLARATION_H

#include "varbinder/scope.h"
#include "ir/expression.h"
#include "ir/ts/tsTypeParameter.h"

namespace ark::es2panda::ir {

class TSTypeParameterDeclaration : public Expression {
public:
    explicit TSTypeParameterDeclaration(ArenaVector<TSTypeParameter *> &&params, size_t requiredParams)
        : Expression(AstNodeType::TS_TYPE_PARAMETER_DECLARATION),
          params_(std::move(params)),
          requiredParams_(requiredParams)
    {
        InitHistory();
    }

    explicit TSTypeParameterDeclaration(ArenaVector<TSTypeParameter *> &&params, size_t requiredParams,
                                        AstNodeHistory *history)
        : Expression(AstNodeType::TS_TYPE_PARAMETER_DECLARATION),
          params_(std::move(params)),
          requiredParams_(requiredParams)
    {
        if (history != nullptr) {
            history_ = history;
        } else {
            InitHistory();
        }
    }

    [[nodiscard]] bool IsScopeBearer() const noexcept override
    {
        return true;
    }

    [[nodiscard]] varbinder::LocalScope *Scope() const noexcept override
    {
        return GetHistoryNodeAs<TSTypeParameterDeclaration>()->scope_;
    }

    void SetScope(varbinder::LocalScope *source);

    void ClearScope() noexcept override
    {
        SetScope(nullptr);
    }

    const ArenaVector<TSTypeParameter *> &Params() const
    {
        return GetHistoryNodeAs<TSTypeParameterDeclaration>()->params_;
    }

    void SetParams(ArenaVector<TSTypeParameter *> &&paramsList)
    {
        auto newNode = GetOrCreateHistoryNodeAs<TSTypeParameterDeclaration>();
        auto &params = newNode->params_;
        params = std::move(paramsList);

        for (auto *param : params) {
            param->SetParent(newNode);
        }
    }

    void AddParam(TSTypeParameter *param)
    {
        ES2PANDA_ASSERT(param != nullptr);
        if (RequiredParams() == Params().size() && param->DefaultType() == nullptr) {
            SetRequiredParams(RequiredParams() + 1);
        }
        auto newNode = reinterpret_cast<TSTypeParameterDeclaration *>(this->GetOrCreateHistoryNode());
        newNode->params_.emplace_back(param);
    }

    void SetValueParams(TSTypeParameter *source, size_t index)
    {
        auto newNode = reinterpret_cast<TSTypeParameterDeclaration *>(this->GetOrCreateHistoryNode());
        auto &arenaVector = newNode->params_;
        ES2PANDA_ASSERT(arenaVector.size() > index);
        arenaVector[index] = source;
    }

    size_t RequiredParams() const
    {
        return GetHistoryNodeAs<TSTypeParameterDeclaration>()->requiredParams_;
    }

    void TransformChildren(const NodeTransformer &cb, std::string_view transformationName) override;
    void Iterate(const NodeTraverser &cb) const override;
    void Dump(ir::AstDumper *dumper) const override;
    void Dump(ir::SrcDumper *dumper) const override;
    void Compile([[maybe_unused]] compiler::PandaGen *pg) const override;
    void Compile(compiler::ETSGen *etsg) const override;
    checker::Type *Check([[maybe_unused]] checker::TSChecker *checker) override;
    checker::VerifiedType Check([[maybe_unused]] checker::ETSChecker *checker) override;

    void Accept(ASTVisitorT *v) override
    {
        v->Accept(this);
    }

    TSTypeParameterDeclaration *Construct(ArenaAllocator *allocator) override;
    void CopyTo(AstNode *other) const override;

private:
    friend class SizeOfNodeTest;

    void SetRequiredParams(size_t source);

    ArenaVector<TSTypeParameter *> params_;
    varbinder::LocalScope *scope_ {nullptr};
    size_t requiredParams_;
};
}  // namespace ark::es2panda::ir

#endif

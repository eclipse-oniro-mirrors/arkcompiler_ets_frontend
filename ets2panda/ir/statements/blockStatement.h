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

#ifndef ES2PANDA_IR_STATEMENT_BLOCK_STATEMENT_H
#define ES2PANDA_IR_STATEMENT_BLOCK_STATEMENT_H

#include "ir/statement.h"
#include "utils/arena_containers.h"

namespace ark::es2panda::checker {
class ETSAnalyzer;
}  // namespace ark::es2panda::checker

namespace ark::es2panda::ir {
class BlockStatement : public Statement {
public:
    explicit BlockStatement(ArenaAllocator *allocator, ArenaVector<Statement *> &&statementList)
        : Statement(AstNodeType::BLOCK_STATEMENT),
          statements_(std::move(statementList)),
          trailingBlocks_(allocator->Adapter())
    {
        InitHistory();
    }

    [[nodiscard]] bool IsScopeBearer() const noexcept override
    {
        return true;
    }

    [[nodiscard]] varbinder::Scope *Scope() const noexcept override
    {
        return AstNode::GetHistoryNodeAs<BlockStatement>()->scope_;
    }

    void SetScope(varbinder::Scope *scope) noexcept
    {
        if (Scope() != scope) {
            AstNode::GetOrCreateHistoryNodeAs<BlockStatement>()->scope_ = scope;
        }
    }

    void ClearScope() noexcept override
    {
        SetScope(nullptr);
    }

    ArenaVector<Statement *> &StatementsForUpdates()
    {
        return AstNode::GetOrCreateHistoryNodeAs<BlockStatement>()->statements_;
    }

    const ArenaVector<Statement *> &Statements()
    {
        return AstNode::GetHistoryNodeAs<BlockStatement>()->statements_;
    }

    const ArenaVector<Statement *> &Statements() const
    {
        return AstNode::GetHistoryNodeAs<BlockStatement>()->statements_;
    }

    void SetStatements(ArenaVector<Statement *> &&statementList)
    {
        auto &statements = AstNode::GetOrCreateHistoryNodeAs<BlockStatement>()->statements_;
        statements = std::move(statementList);

        for (auto *statement : Statements()) {
            statement->SetParent(this);
        }
    }

    void AddStatements(const ArenaVector<Statement *> &statementList)
    {
        auto &statements = AstNode::GetOrCreateHistoryNodeAs<BlockStatement>()->statements_;

        for (auto statement : statementList) {
            statement->SetParent(this);
            statements.emplace_back(statement);
        }
    }

    void ClearStatements()
    {
        auto &statements = AstNode::GetOrCreateHistoryNodeAs<BlockStatement>()->statements_;
        statements.clear();
    }

    void AddStatement(Statement *statement)
    {
        statement->SetParent(this);
        auto &statements = AstNode::GetOrCreateHistoryNodeAs<BlockStatement>()->statements_;
        statements.emplace_back(statement);
    }

    void AddStatement(std::size_t idx, Statement *statement)
    {
        statement->SetParent(this);
        auto &statements = AstNode::GetOrCreateHistoryNodeAs<BlockStatement>()->statements_;
        statements.emplace(std::next(statements.begin() + idx), statement);
    }

    void AddTrailingBlock(AstNode *stmt, BlockStatement *trailingBlock)
    {
        AstNode::GetOrCreateHistoryNodeAs<BlockStatement>()->trailingBlocks_.emplace(stmt, trailingBlock);
    }

    BlockStatement *SearchStatementInTrailingBlock(Statement *item)
    {
        auto &trailingBlock = AstNode::GetHistoryNodeAs<BlockStatement>()->trailingBlocks_;
        auto nowNode = item->GetHistoryNode();
        for (auto &it : trailingBlock) {
            if (it.first->GetHistoryNode() == nowNode) {
                return it.second;
            }
        }
        return nullptr;
    }

    void TransformChildren(const NodeTransformer &cb, std::string_view transformationName) override;

    AstNode *Clone(ArenaAllocator *const allocator, AstNode *const parent) override;

    void Iterate(const NodeTraverser &cb) const override;
    void Dump(ir::AstDumper *dumper) const override;
    void Dump(ir::SrcDumper *dumper) const override;
    void Compile([[maybe_unused]] compiler::PandaGen *pg) const override;
    void Compile([[maybe_unused]] compiler::ETSGen *etsg) const override;
    checker::Type *Check([[maybe_unused]] checker::TSChecker *checker) override;
    checker::VerifiedType Check([[maybe_unused]] checker::ETSChecker *checker) override;

    void Accept(ASTVisitorT *v) override
    {
        v->Accept(this);
    }

    BlockStatement *Construct(ArenaAllocator *allocator) override;
    void CopyTo(AstNode *other) const override;

private:
    friend class SizeOfNodeTest;
    varbinder::Scope *scope_ {};
    ArenaVector<Statement *> statements_;
    ArenaUnorderedMap<AstNode *, BlockStatement *> trailingBlocks_;
};
}  // namespace ark::es2panda::ir

#endif

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

#include "blockStatement.h"

#include "compiler/core/pandagen.h"
#include "compiler/core/regScope.h"
#include "compiler/core/ETSGen.h"
#include "checker/TSchecker.h"
#include "checker/ETSchecker.h"
#include "ir/astDump.h"
#include "ir/srcDump.h"

namespace ark::es2panda::ir {

BlockStatement::BlockStatement(ArenaAllocator *allocator, ArenaVector<Statement *> &&statementList)
    : Statement(AstNodeType::BLOCK_STATEMENT),
      statements_(std::move(statementList)),
      trailingBlocks_(allocator->Adapter())
{
    InitHistory();

    // NOTE(dkofanov): Some approximation to new node range.
    // There should be a generalized way to initialize node range from childs.
    if (!statements_.empty()) {
        SetStart(statements_.front()->Start());
        SetEnd(statements_.back()->End());
    }
}

void BlockStatement::TransformChildren(const NodeTransformer &cb, std::string_view const transformationName)
{
    // This will survive pushing element to the back of statements_ in the process
    auto const &constStatements = Statements();
    for (size_t index = 0; index < constStatements.size(); index++) {
        auto statement = constStatements[index];
        if (auto *transformedNode = cb(statement); statement != transformedNode) {
            statement->SetTransformedNode(transformationName, transformedNode);
            auto &statements = AstNode::GetOrCreateHistoryNodeAs<BlockStatement>()->statements_;
            statements[index] = transformedNode->AsStatement();
        }
    }
}

AstNode *BlockStatement::Clone(ArenaAllocator *const allocator, AstNode *const parent)
{
    ArenaVector<Statement *> statements(allocator->Adapter());

    for (auto *statement : Statements()) {
        statements.push_back(statement->Clone(allocator, parent)->AsStatement());
    }

    auto retVal = util::NodeAllocator::ForceSetParent<ir::BlockStatement>(allocator, allocator, std::move(statements));
    ES2PANDA_ASSERT(retVal != nullptr);
    retVal->SetParent(parent);

    return retVal;
}

void BlockStatement::Iterate(const NodeTraverser &cb) const
{
    // This will survive pushing element to the back of statements_ in the process
    auto const &statements = Statements();
    // NOLINTNEXTLINE(modernize-loop-convert)
    for (size_t ix = 0; ix < statements.size(); ix++) {
        cb(statements[ix]);
    }
}

void BlockStatement::Dump(ir::AstDumper *dumper) const
{
    dumper->Add({{"type", IsProgram() ? "Program" : "BlockStatement"}, {"statements", Statements()}});
}

void BlockStatement::Dump(ir::SrcDumper *dumper) const
{
    auto const &statements = Statements();
    // NOTE(nsizov): trailing blocks
    if (Parent() != nullptr && (Parent()->IsBlockStatement() || Parent()->IsCallExpression())) {
        dumper->Add("{");
        if (!statements.empty()) {
            dumper->IncrIndent();
            dumper->Endl();
        }
    }
    for (auto statement : statements) {
        statement->Dump(dumper);
        if (statement != statements.back()) {
            dumper->Endl();
        }
    }
    if (Parent() != nullptr && (Parent()->IsBlockStatement() || Parent()->IsCallExpression())) {
        if (!statements.empty()) {
            dumper->DecrIndent();
            dumper->Endl();
        }
        dumper->Add("}");
    }
}

void BlockStatement::Compile([[maybe_unused]] compiler::PandaGen *pg) const
{
    pg->GetAstCompiler()->Compile(this);
}

void BlockStatement::Compile([[maybe_unused]] compiler::ETSGen *etsg) const
{
    etsg->GetAstCompiler()->Compile(this);
}

checker::Type *BlockStatement::Check([[maybe_unused]] checker::TSChecker *checker)
{
    return checker->GetAnalyzer()->Check(this);
}

checker::VerifiedType BlockStatement::Check([[maybe_unused]] checker::ETSChecker *checker)
{
    return {this, checker->GetAnalyzer()->Check(this)};
}

BlockStatement *BlockStatement::Construct(ArenaAllocator *allocator)
{
    ArenaVector<Statement *> statementList(allocator->Adapter());
    return allocator->New<BlockStatement>(allocator, std::move(statementList));
}

void BlockStatement::CopyTo(AstNode *other) const
{
    auto otherImpl = static_cast<BlockStatement *>(other);

    otherImpl->scope_ = scope_;
    otherImpl->statements_ = statements_;
    otherImpl->trailingBlocks_ = trailingBlocks_;

    Statement::CopyTo(other);
}

}  // namespace ark::es2panda::ir

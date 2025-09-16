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

#include "CFG.h"
#include "public/public.h"
#include "util/helpers.h"

namespace ark::es2panda::compiler {

CFG::BasicBlock::BasicBlock(ArenaAllocator *allocator, size_t index) noexcept
    : index_(index), nodes_(allocator->Adapter()), succs_(allocator->Adapter()), preds_(allocator->Adapter())
{
}

size_t CFG::BasicBlock::AddNode(ir::AstNode *node)
{
    nodes_.push_back(node);
    return nodes_.size() - 1;
}

std::pair<size_t, size_t> CFG::BasicBlock::AddSuccessor(BasicBlock *successor)
{
    ES2PANDA_ASSERT(successor != nullptr);
    succs_.push_back(successor);
    ES2PANDA_ASSERT(successor != nullptr);
    successor->preds_.push_back(this);
    return std::make_pair(succs_.size() - 1, successor->preds_.size() - 1);
}
std::pair<size_t, size_t> CFG::BasicBlock::AddPredecessor(BasicBlock *predecessor)
{
    preds_.push_back(predecessor);
    predecessor->succs_.push_back(this);
    return std::make_pair(preds_.size() - 1, predecessor->succs_.size() - 1);
}

size_t CFG::BasicBlock::GetIndex() const
{
    return index_;
}

CFG::BasicBlockFlags CFG::BasicBlock::GetFlags() const
{
    return flags_;
}

size_t CFG::BasicBlock::GetSize() const
{
    return nodes_.size();
}

const ArenaVector<CFG::BasicBlock *> &CFG::BasicBlock::GetSuccessors() const
{
    return succs_;
}
const ArenaVector<CFG::BasicBlock *> &CFG::BasicBlock::GetPredecessors() const
{
    return preds_;
}

const ArenaVector<ir::AstNode *> &CFG::BasicBlock::GetNodes() const
{
    return nodes_;
}

ir::AstNode *CFG::BasicBlock::GetLastNode() const
{
    if (nodes_.empty()) {
        return nullptr;
    }
    return nodes_.back();
}

CFG::CFG(ArenaAllocator *allocator)
    : allocator_(allocator),
      blocks_(allocator_->Adapter()),
      nodeBBMap_(allocator_->Adapter()),
      functionNodeBBMap_(allocator_->Adapter()),
      bbAnnotationMap_(allocator_->Adapter()),
      switchCaseMap_(allocator_->Adapter()),
      bbSuccLabelMap_(allocator_->Adapter()),
      bbPredLabelMap_(allocator_->Adapter()),
      loopStmtJumpTargetMap_(allocator_->Adapter()),
      trueLabel_(true),
      falseLabel_(false)
{
}

void CFG::RedirectEmptyBBEdges(CFG::BasicBlock *bb) const
{
    for (auto pred : bb->preds_) {
        for (auto &succ : pred->succs_) {
            if (succ == bb) {
                succ = bb->succs_[0];
                break;
            }
        }
    }
}

void CFG::MergeEmptyBlocks()
{
    std::list<CFG::BasicBlock *> merged;

    for (auto bb : blocks_) {
        if (bb->nodes_.empty() && bb->succs_.empty() && bb->preds_.empty()) {
            merged.push_back(bb);
            continue;
        }

        if (!(bb->nodes_.empty() && bb->succs_.size() == 1 && bbSuccLabelMap_.find(bb) == bbSuccLabelMap_.end())) {
            continue;
        }

        RedirectEmptyBBEdges(bb);
        auto succ = bb->succs_[0];

        if (succ == bb) {
            continue;
        }

        if (bb->preds_.empty()) {
            merged.push_back(bb);
            continue;
        }
        // replace the previous predecessor with the first predecessor of the bb to be deleted if it exists
        for (size_t index = 0; index < succ->preds_.size(); ++index) {
            if (succ->preds_[index] == bb) {
                succ->preds_[index] = bb->preds_[0];
                SetBBPredEdgeLabel(succ, index, GetBBPredEdgeLabel(bb, 0));
                break;
            }
        }
        // add the remaining predecessors to the successor node
        for (size_t index = 1; index < bb->preds_.size(); ++index) {
            bb->succs_[0]->preds_.push_back(bb->preds_[index]);
            SetBBPredEdgeLabel(bb->succs_[0], bb->succs_[0]->preds_.size() - 1, GetBBPredEdgeLabel(bb, index));
        }
        merged.push_back(bb);
    }

    // remove the merged blocks
    for (auto bb : merged) {
        blocks_.erase(bb);
        bbAnnotationMap_.erase(bb);
        bbPredLabelMap_.erase(bb);
        bbSuccLabelMap_.erase(bb);
    }
}

const ir::AstNode *CFG::GetBBPredEdgeLabel(const BasicBlock *bb, size_t index) const
{
    const ir::AstNode *result = nullptr;
    if (auto predIt = bbPredLabelMap_.find(bb); predIt != bbPredLabelMap_.end()) {
        auto labelIt = predIt->second.find(index);
        if (labelIt != predIt->second.end()) {
            result = labelIt->second;
        }
    }
    return result;
}

/* pair<condition, label> */
std::pair<ir::AstNode *, const ir::AstNode *> CFG::GetBBPredCondition(const BasicBlock *bb, size_t index) const
{
    auto label = GetBBPredEdgeLabel(bb, index);
    if (label == nullptr) {
        return {nullptr, nullptr};
    }

    auto predBB = bb->preds_[index];
    return {predBB->nodes_[predBB->nodes_.size() - 1], label};
}

const ir::AstNode *CFG::GetBBSuccEdgeLabel(const BasicBlock *bb, size_t index) const
{
    const ir::AstNode *result = nullptr;
    if (auto succIt = bbSuccLabelMap_.find(bb); succIt != bbSuccLabelMap_.end()) {
        auto labelIt = succIt->second.find(index);
        if (labelIt != succIt->second.end()) {
            result = labelIt->second;
        }
    }
    return result;
}

CFG::BasicBlock *CFG::Build(ir::ScriptFunction *scriptFunctionNode)
{
    if (!scriptFunctionNode->HasBody()) {
        return nullptr;
    }

    BasicBlock *entryBB = CreateNewBB({});
    ES2PANDA_ASSERT(entryBB != nullptr);
    entryBB->SetFlag(BasicBlockFlags::ENTRY);
    functionNodeBBMap_[scriptFunctionNode] = entryBB;
    if (scriptFunctionNode->Id() != nullptr) {
        bbAnnotationMap_[entryBB] =
            util::Helpers::EscapeHTMLString(allocator_, scriptFunctionNode->Id()->Name().Mutf8());
    }
    ES2PANDA_ASSERT(scriptFunctionNode->Body()->IsBlockStatement());
    auto exitBB = Build(scriptFunctionNode->Body()->AsBlockStatement(), entryBB);
    ES2PANDA_ASSERT(exitBB != nullptr);
    exitBB->SetFlag(BasicBlockFlags::EXIT);
    return entryBB;
}

bool CFG::BasicBlock::HasFlag(BasicBlockFlags flag) const
{
    return (flags_ & flag) != 0;
}

void CFG::BasicBlock::SetFlag(BasicBlockFlags flag)
{
    flags_ |= flag;
}

void CFG::BasicBlock::ClearFlag(BasicBlockFlags flag)
{
    flags_ &= ~flag;
}

CFG::BasicBlock *CFG::CreateNewBB(const std::vector<BasicBlock *> &&preds,
                                  const std::vector<const ir::AstNode *> &&labels)
{
    auto bb = allocator_->New<BasicBlock>(allocator_, basicBlockIdx_++);
    ES2PANDA_ASSERT(bb != nullptr);
    if (inLoop_ > 0) {
        bb->SetFlag(BasicBlockFlags::LOOP);
    }
    for (size_t pi = 0; pi < preds.size(); ++pi) {
        if (preds[pi] == nullptr) {
            continue;
        }

        auto [predIndex, succIndex] = bb->AddPredecessor(preds[pi]);
        if (labels.size() > pi && labels[pi] != nullptr) {
            SetBBPredEdgeLabel(bb, predIndex, labels[pi]);
            SetBBSuccEdgeLabel(preds[pi], succIndex, labels[pi]);
            if (labels[pi] == &trueLabel_ || labels[pi] == &falseLabel_) {
                preds[pi]->SetFlag(BasicBlockFlags::CONDITION);
            }
        }
    }

    blocks_.insert(bb);
    return bb;
}

void CFG::SetBBPredEdgeLabel(const BasicBlock *bb, size_t index, const ir::AstNode *label)
{
    if (label == nullptr) {
        return;
    }

    auto insertPred = bbPredLabelMap_.emplace(bb, allocator_->Adapter());
    insertPred.first->second.emplace(index, label);
}
void CFG::SetBBSuccEdgeLabel(const BasicBlock *bb, size_t index, const ir::AstNode *label)
{
    if (label == nullptr) {
        return;
    }

    auto insertSucc = bbSuccLabelMap_.emplace(bb, allocator_->Adapter());
    insertSucc.first->second.emplace(index, label);
}

CFG::BasicBlock *CFG::BuildExpressions(ir::AstNode *node, CFG::BasicBlock *bb)
{
    switch (node->Type()) {
        case ir::AstNodeType::ARRAY_EXPRESSION:
            return Build(node->AsArrayExpression(), bb);
        case ir::AstNodeType::ARROW_FUNCTION_EXPRESSION:
            return Build(node->AsArrowFunctionExpression(), bb);
        case ir::AstNodeType::ASSIGNMENT_EXPRESSION:
            return Build(node->AsAssignmentExpression(), bb);
        case ir::AstNodeType::BINARY_EXPRESSION:
            return Build(node->AsBinaryExpression(), bb);
        case ir::AstNodeType::BLOCK_EXPRESSION:  // The expression itself is excluded
            return Build(node->AsBlockExpression(), bb);
        case ir::AstNodeType::CALL_EXPRESSION:  // NOTE: Call edge is missing
            return Build(node->AsCallExpression(), bb);
        case ir::AstNodeType::CATCH_CLAUSE:
            return Build(node->AsCatchClause(), bb);
        case ir::AstNodeType::CHAIN_EXPRESSION:
            return Build(node->AsChainExpression(), bb);
        case ir::AstNodeType::CONDITIONAL_EXPRESSION:  // The expression itself is excluded
            return Build(node->AsConditionalExpression(), bb);
        case ir::AstNodeType::MEMBER_EXPRESSION:
            return Build(node->AsMemberExpression(), bb);
        case ir::AstNodeType::TYPEOF_EXPRESSION:
            return Build(node->AsTypeofExpression(), bb);
        case ir::AstNodeType::UNARY_EXPRESSION:
            return Build(node->AsUnaryExpression(), bb);
        case ir::AstNodeType::UPDATE_EXPRESSION:
            return Build(node->AsUpdateExpression(), bb);
        default:
            return nullptr;
    }
}

CFG::BasicBlock *CFG::BuildETSExpressions(ir::AstNode *node, CFG::BasicBlock *bb)
{
    switch (node->Type()) {
        case ir::AstNodeType::ETS_NEW_CLASS_INSTANCE_EXPRESSION:  // NOTE: Call edge to the constructor is missing
            return Build(node->AsETSNewClassInstanceExpression(), bb);
        case ir::AstNodeType::ETS_TYPE_REFERENCE:
            return bb;
        case ir::AstNodeType::TS_AS_EXPRESSION:
            return Build(node->AsTSAsExpression(), bb);
        case ir::AstNodeType::TS_NON_NULL_EXPRESSION:
            return Build(node->AsTSNonNullExpression(), bb);
        default:
            return nullptr;
    }
}

CFG::BasicBlock *CFG::BuildStatements(ir::AstNode *node, CFG::BasicBlock *bb)
{
    switch (node->Type()) {
        case ir::AstNodeType::BLOCK_STATEMENT:
            return Build(node->AsBlockStatement(), bb);
        case ir::AstNodeType::BREAK_STATEMENT:
            return Build(node->AsBreakStatement(), bb);
        case ir::AstNodeType::CONTINUE_STATEMENT:
            return Build(node->AsContinueStatement(), bb);
        case ir::AstNodeType::EMPTY_STATEMENT:
            return bb;
        case ir::AstNodeType::EXPRESSION_STATEMENT:
            return Build(node->AsExpressionStatement(), bb);
        case ir::AstNodeType::FOR_OF_STATEMENT:
            return Build(node->AsForOfStatement(), bb);
        case ir::AstNodeType::FOR_UPDATE_STATEMENT:
            return Build(node->AsForUpdateStatement(), bb);
        case ir::AstNodeType::IF_STATEMENT:
            return Build(node->AsIfStatement(), bb);
        case ir::AstNodeType::LABELLED_STATEMENT:
            return Build(node->AsLabelledStatement(), bb);
        case ir::AstNodeType::RETURN_STATEMENT:
            return Build(node->AsReturnStatement(), bb);
        case ir::AstNodeType::SWITCH_STATEMENT:
            return Build(node->AsSwitchStatement(), bb);
        case ir::AstNodeType::THROW_STATEMENT:
            return Build(node->AsThrowStatement(), bb);
        case ir::AstNodeType::TRY_STATEMENT:
            return Build(node->AsTryStatement(), bb);
        case ir::AstNodeType::VARIABLE_DECLARATION:
            return Build(node->AsVariableDeclaration(), bb);
        case ir::AstNodeType::WHILE_STATEMENT:
            return Build(node->AsWhileStatement(), bb);
        case ir::AstNodeType::DO_WHILE_STATEMENT:
            return Build(node->AsDoWhileStatement(), bb);
        default:
            return nullptr;
    }
}

CFG::BasicBlock *CFG::Build(ir::AstNode *node, CFG::BasicBlock *bb)
{
    if (node == nullptr) {
        return bb;
    }

    if (bb == nullptr) {
        return nullptr;
    }

    BasicBlock *stmtBB = BuildStatements(node, bb);
    if (stmtBB != nullptr) {
        return stmtBB;
    }

    BasicBlock *exprBB = BuildExpressions(node, bb);
    if (exprBB != nullptr) {
        return exprBB;
    }

    BasicBlock *etsExprBB = BuildETSExpressions(node, bb);
    if (etsExprBB != nullptr) {
        return etsExprBB;
    }

    switch (node->Type()) {
        case ir::AstNodeType::BIGINT_LITERAL:
        case ir::AstNodeType::BOOLEAN_LITERAL:
        case ir::AstNodeType::CHAR_LITERAL:
        case ir::AstNodeType::IDENTIFIER:
        case ir::AstNodeType::NULL_LITERAL:
        case ir::AstNodeType::NUMBER_LITERAL:
        case ir::AstNodeType::STRING_LITERAL:
        case ir::AstNodeType::TEMPLATE_LITERAL:
        case ir::AstNodeType::THIS_EXPRESSION:
        case ir::AstNodeType::UNDEFINED_LITERAL:
            AddNodeToBB(node, bb);
            return bb;
        default:
            AddNodeToBB(node, bb);
            return bb;
    }
}

CFG::BasicBlock *CFG::Build(ir::CatchClause *catchClauseNode, BasicBlock *bb)
{
    // NOTE: Implement this
    AddNodeToBB(catchClauseNode, bb);
    return bb;
}

CFG::BasicBlock *CFG::Build(ir::ThrowStatement *throwStatementNode, BasicBlock *bb)
{
    // NOTE: Implement this
    AddNodeToBB(throwStatementNode, bb);
    return bb;
}

CFG::BasicBlock *CFG::Build(ir::TryStatement *tryStatementNode, BasicBlock *bb)
{
    // NOTE: Implement this
    AddNodeToBB(tryStatementNode, bb);
    return bb;
}

CFG::BasicBlock *CFG::Build(ir::ArrayExpression *arrayExpressionNode, BasicBlock *bb)
{
    for (auto element : arrayExpressionNode->Elements()) {
        bb = Build(element, bb);
    }
    AddNodeToBB(arrayExpressionNode, bb);
    return bb;
}

CFG::BasicBlock *CFG::Build(ir::ArrowFunctionExpression *arrowFunctionExpressionNode, BasicBlock *bb)
{
    AddNodeToBB(arrowFunctionExpressionNode, bb);
    return bb;
}

CFG::BasicBlock *CFG::Build(ir::UnaryExpression *unaryExpressionNode, BasicBlock *bb)
{
    bb = Build(unaryExpressionNode->Argument(), bb);
    AddNodeToBB(unaryExpressionNode, bb);
    return bb;
}

CFG::BasicBlock *CFG::Build(ir::TSNonNullExpression *tsNonNullExpressionNode, BasicBlock *bb)
{
    bb = Build(tsNonNullExpressionNode->Expr(), bb);
    AddNodeToBB(tsNonNullExpressionNode, bb);
    return bb;
}

CFG::BasicBlock *CFG::Build(ir::ChainExpression *chainExpressionNode, BasicBlock *bb)
{
    // NOTE: Implement this
    AddNodeToBB(chainExpressionNode, bb);
    return bb;
}

CFG::BasicBlock *CFG::Build(ir::ETSNewClassInstanceExpression *etsNewClassInstanceExpressionNode, BasicBlock *bb)
{
    for (auto argumentExpression : etsNewClassInstanceExpressionNode->GetArguments()) {
        bb = Build(argumentExpression, bb);
    }
    AddNodeToBB(etsNewClassInstanceExpressionNode, bb);
    return bb;
}

CFG::BasicBlock *CFG::Build(ir::ConditionalExpression *conditionalExpressionNode, BasicBlock *bb)
{
    auto conditionBB = Build(conditionalExpressionNode->Test(), bb);

    auto trueBB = CreateNewBB({conditionBB}, {&trueLabel_});
    trueBB = Build(conditionalExpressionNode->Consequent(), trueBB);
    auto falseBB = CreateNewBB({conditionBB}, {&falseLabel_});
    falseBB = Build(conditionalExpressionNode->Alternate(), falseBB);
    auto nextBB = CreateNewBB({trueBB, falseBB});
    return nextBB;
}

CFG::BasicBlock *CFG::Build(ir::TypeofExpression *typeofExpressionNode, BasicBlock *bb)
{
    bb = Build(typeofExpressionNode->Argument(), bb);
    AddNodeToBB(typeofExpressionNode, bb);
    return bb;
}

CFG::BasicBlock *CFG::Build(ir::MemberExpression *memberExpressionNode, BasicBlock *bb)
{
    bb = Build(memberExpressionNode->Object(), bb);
    bb = Build(memberExpressionNode->Property(), bb);
    AddNodeToBB(memberExpressionNode, bb);

    return bb;
}

CFG::BasicBlock *CFG::Build(ir::WhileStatement *whileStatementNode, BasicBlock *bb)
{
    ++inLoop_;
    auto testBB = CreateNewBB({bb});
    ES2PANDA_ASSERT(testBB != nullptr);
    testBB->SetFlag(BasicBlockFlags::CONDITION);
    --inLoop_;
    auto falseBB = CreateNewBB({testBB}, {&falseLabel_});
    ++inLoop_;
    loopStmtJumpTargetMap_[whileStatementNode] = std::make_pair(testBB, falseBB);
    bb = Build(whileStatementNode->Test(), testBB);
    auto trueBB = CreateNewBB({bb}, {&trueLabel_});
    trueBB = Build(whileStatementNode->Body(), trueBB);
    ES2PANDA_ASSERT(trueBB != nullptr);
    trueBB->AddSuccessor(testBB);
    --inLoop_;
    return falseBB;
}

CFG::BasicBlock *CFG::Build(ir::DoWhileStatement *doWhileStatementNode, BasicBlock *bb)
{
    ++inLoop_;
    auto bodyBB = CreateNewBB({bb});
    auto testBB = CreateNewBB({});
    ES2PANDA_ASSERT(testBB != nullptr);
    testBB->SetFlag(BasicBlockFlags::CONDITION);
    --inLoop_;
    auto falseBB = CreateNewBB({testBB}, {&falseLabel_});
    ++inLoop_;
    loopStmtJumpTargetMap_[doWhileStatementNode] = std::make_pair(testBB, falseBB);
    bb = Build(doWhileStatementNode->Body(), bodyBB);
    ES2PANDA_ASSERT(bb != nullptr);
    testBB = Build(doWhileStatementNode->Test(), testBB);
    bb->AddSuccessor(testBB);
    AddBBEdge(testBB, bodyBB, &trueLabel_);
    --inLoop_;
    return falseBB;
}

CFG::BasicBlock *CFG::Build(ir::SwitchStatement *switchStatementNode, BasicBlock *bb)
{
    bb = Build(switchStatementNode->Discriminant(), bb);
    auto afterBB = CreateNewBB({});
    loopStmtJumpTargetMap_[switchStatementNode] = std::make_pair(nullptr, afterBB);
    BasicBlock *fallThrough = nullptr;
    for (auto switchCase : switchStatementNode->Cases()) {
        bb = CreateNewBB({bb});
        bb = Build(switchCase->Test(), bb);
        if (switchCase->Test() != nullptr) {
            switchCaseMap_[switchCase->Test()] = switchStatementNode->Discriminant();
        }

        auto caseBB = CreateNewBB({bb, fallThrough}, {switchCase->Test()});
        for (auto consStatement : switchCase->Consequent()) {
            caseBB = Build(consStatement, caseBB);
        }
        fallThrough = caseBB;
    }

    if (fallThrough != nullptr) {
        afterBB->AddPredecessor(fallThrough);
    }

    return afterBB;
}

CFG::BasicBlock *CFG::Build(ir::CallExpression *callExpressionNode, BasicBlock *bb)
{
    bb = Build(callExpressionNode->Callee(), bb);
    for (auto argument : callExpressionNode->Arguments()) {
        bb = Build(argument, bb);
    }
    AddNodeToBB(callExpressionNode, bb);
    return bb;
}

CFG::BasicBlock *CFG::Build(ir::LabelledStatement *labelledStatementNode, BasicBlock *bb)
{
    bb = Build(labelledStatementNode->Body(), bb);
    return bb;
}

CFG::BasicBlock *CFG::Build(ir::BreakStatement *breakStatementNode, BasicBlock *bb)
{
    auto target = breakStatementNode->Target();
    if (target == nullptr) {
        return bb;
    }

    if (target->IsLabelledStatement()) {
        target = target->AsLabelledStatement()->Body();
    }

    if (const auto targetIt = loopStmtJumpTargetMap_.find(target); targetIt != loopStmtJumpTargetMap_.end()) {
        bb->AddSuccessor(targetIt->second.second);
    }
    return CreateNewBB({});
}

CFG::BasicBlock *CFG::Build(ir::ContinueStatement *continueStatementNode, BasicBlock *bb)
{
    auto target = continueStatementNode->Target();
    if (target == nullptr) {
        return bb;
    }

    if (target->IsLabelledStatement()) {
        target = target->AsLabelledStatement()->Body();
    }
    if (const auto targetIt = loopStmtJumpTargetMap_.find(target); targetIt != loopStmtJumpTargetMap_.end()) {
        bb->AddSuccessor(targetIt->second.first);
    }
    return CreateNewBB({});
}

CFG::BasicBlock *CFG::Build(ir::UpdateExpression *updateExpressionNode, BasicBlock *bb)
{
    if (updateExpressionNode->IsPrefix()) {
        AddNodeToBB(updateExpressionNode, bb);
        bb = Build(updateExpressionNode->Argument(), bb);
    } else {
        bb = Build(updateExpressionNode->Argument(), bb);
        AddNodeToBB(updateExpressionNode, bb);
    }
    return bb;
}

CFG::BasicBlock *CFG::Build(ir::ForOfStatement *forOfStatementNode, BasicBlock *bb)
{
    auto rightExprBB = Build(forOfStatementNode->Right(), bb);
    auto nextBB = CreateNewBB({rightExprBB});
    ++inLoop_;
    auto loopBB = CreateNewBB({rightExprBB});
    bb = Build(forOfStatementNode->Left(), loopBB);
    loopStmtJumpTargetMap_[forOfStatementNode] = std::make_pair(bb, nextBB);
    bb = Build(forOfStatementNode->Body(), bb);
    --inLoop_;
    ES2PANDA_ASSERT(bb != nullptr);
    bb->AddSuccessor(loopBB);
    bb->AddSuccessor(nextBB);
    return nextBB;
}

CFG::BasicBlock *CFG::Build(ir::ForUpdateStatement *forUpdateStatementNode, BasicBlock *bb)
{
    bb = Build(forUpdateStatementNode->Init(), bb);

    if (forUpdateStatementNode->Test() != nullptr) {
        ++inLoop_;
        auto testBB = CreateNewBB({bb});
        --inLoop_;
        auto falseBB = CreateNewBB({testBB}, {&falseLabel_});
        ++inLoop_;
        auto trueBB = CreateNewBB({testBB}, {&trueLabel_});
        loopStmtJumpTargetMap_[forUpdateStatementNode] = std::make_pair(testBB, falseBB);
        testBB = Build(forUpdateStatementNode->Test(), testBB);
        auto bodyBB = Build(forUpdateStatementNode->Body(), trueBB);
        bodyBB = Build(forUpdateStatementNode->Update(), bodyBB);
        ES2PANDA_ASSERT(bodyBB != nullptr);
        bodyBB->AddSuccessor(testBB);
        --inLoop_;
        return falseBB;
    }

    auto nextBB = CreateNewBB({});
    ++inLoop_;
    auto bodyStartBB = CreateNewBB({bb});
    loopStmtJumpTargetMap_[forUpdateStatementNode] = std::make_pair(bodyStartBB, nextBB);
    auto bodyBB = Build(forUpdateStatementNode->Body(), bodyStartBB);
    bodyBB = Build(forUpdateStatementNode->Update(), bodyBB);
    ES2PANDA_ASSERT(bodyBB != nullptr);
    bodyBB->AddSuccessor(bodyStartBB);
    --inLoop_;
    return nextBB;
}

CFG::BasicBlock *CFG::Build(ir::ReturnStatement *returnStatementNode, BasicBlock *bb)
{
    bb = Build(returnStatementNode->Argument(), bb);
    AddNodeToBB(returnStatementNode, bb);
    bb->SetFlag(BasicBlockFlags::EXIT);
    return CreateNewBB({});
}

CFG::BasicBlock *CFG::Build(ir::TSAsExpression *asExpressionNode, BasicBlock *bb)
{
    bb = Build(asExpressionNode->Expr(), bb);
    AddNodeToBB(asExpressionNode, bb);
    return bb;
}

CFG::BasicBlock *CFG::Build(ir::AssignmentExpression *assignmentExpressionNode, BasicBlock *bb)
{
    bb = Build(assignmentExpressionNode->Right(), bb);
    bb = Build(assignmentExpressionNode->Left(), bb);
    AddNodeToBB(assignmentExpressionNode, bb);
    return bb;
}

CFG::BasicBlock *CFG::Build(ir::ExpressionStatement *expressionStatementNode, BasicBlock *bb)
{
    return Build(expressionStatementNode->GetExpression(), bb);
}

CFG::BasicBlock *CFG::Build(ir::BinaryExpression *binaryExpressionNode, BasicBlock *bb)
{
    if (binaryExpressionNode->OperatorType() == lexer::TokenType::PUNCTUATOR_LOGICAL_AND) {
        auto leftBB = Build(binaryExpressionNode->Left(), bb);
        auto leftTrueBB = CreateNewBB({leftBB}, {&trueLabel_});
        leftTrueBB = Build(binaryExpressionNode->Right(), leftTrueBB);
        bb = CreateNewBB({leftBB, leftTrueBB}, {&falseLabel_});
    } else if (binaryExpressionNode->OperatorType() == lexer::TokenType::PUNCTUATOR_LOGICAL_OR) {
        auto leftBB = Build(binaryExpressionNode->Left(), bb);
        auto leftFalseBB = CreateNewBB({leftBB}, {&falseLabel_});
        leftFalseBB = Build(binaryExpressionNode->Right(), leftFalseBB);
        bb = CreateNewBB({leftFalseBB, leftBB}, {nullptr, &trueLabel_});
    } else {  // NOTE: Implement the rest of it if any
        bb = Build(binaryExpressionNode->Left(), bb);
        bb = Build(binaryExpressionNode->Right(), bb);
    }
    AddNodeToBB(binaryExpressionNode, bb);
    return bb;
}

CFG::BasicBlock *CFG::Build(ir::BlockStatement *blockStatementNode, BasicBlock *bb)
{
    for (const auto statement : blockStatementNode->Statements()) {
        bb = Build(statement, bb);
    }
    return bb;
}

CFG::BasicBlock *CFG::Build(ir::BlockExpression *blockExpressionNode, BasicBlock *bb)
{
    for (const auto statement : blockExpressionNode->Statements()) {
        bb = Build(statement, bb);
    }
    return bb;
}

CFG::BasicBlock *CFG::Build(ir::IfStatement *ifStatementNode, BasicBlock *bb)
{
    auto conditionBB = Build(ifStatementNode->Test(), bb);
    auto trueBB = CreateNewBB({conditionBB}, {&trueLabel_});
    trueBB = Build(ifStatementNode->Consequent(), trueBB);

    BasicBlock *falseBB = conditionBB;
    BasicBlock *newBB = nullptr;
    if (ifStatementNode->Alternate() != nullptr) {
        falseBB = CreateNewBB({conditionBB}, {&falseLabel_});
        falseBB = Build(ifStatementNode->Alternate(), falseBB);
        newBB = CreateNewBB({trueBB, falseBB});
    } else {
        newBB = CreateNewBB({trueBB, falseBB}, {nullptr, &falseLabel_});
    }

    return newBB;
}

void CFG::DumpSuccessorEdges(std::ofstream &ofs, BasicBlock *const bb) const
{
    for (size_t index = 0; index < bb->succs_.size(); ++index) {
        ofs << "  BB" << bb->index_ << " -> BB" << bb->succs_[index]->index_ << " [color=\"blue\"";
        if (auto labelsMapIt = bbSuccLabelMap_.find(bb); labelsMapIt != bbSuccLabelMap_.end()) {
            if (auto indexMapIt = labelsMapIt->second.find(index); indexMapIt != labelsMapIt->second.end()) {
                ofs << " label=\""
                    << util::Helpers::EscapeHTMLString(allocator_, indexMapIt->second->DumpEtsSrc()).View().Mutf8()
                    << "\"";
            }
        }
        ofs << "];\n";
    }
}

void CFG::DumpPredecessorEdges(std::ofstream &ofs, BasicBlock *const bb) const
{
    for (size_t index = 0; index < bb->preds_.size(); ++index) {
        ofs << "  BB" << bb->index_ << " -> BB" << bb->preds_[index]->index_ << " [color=\"red\"";
        if (auto labelsMapIt = bbPredLabelMap_.find(bb); labelsMapIt != bbPredLabelMap_.end()) {
            if (auto indexMapIt = labelsMapIt->second.find(index); indexMapIt != labelsMapIt->second.end()) {
                ofs << " label=\""
                    << util::Helpers::EscapeHTMLString(allocator_, indexMapIt->second->DumpEtsSrc()).View().Mutf8()
                    << "\"";
            }
        }
        ofs << "];\n";
    }
}

bool CFG::DumpDot(const char *filename) const
{
    std::ofstream out(filename);
    if (!out) {
        return false;
    }

    out << "digraph CFG {\n"
           "  node[shape = plaintext]\n";
    for (const auto bb : blocks_) {
        std::string color = "gray";
        if (bb->HasFlag(BasicBlockFlags::CONDITION)) {
            color = "green";
        }
        out << "  BB" << bb->index_ << R"([label=<<TABLE BORDER="0" CELLBORDER="1" CELLSPACING="0">)"
            << "<TR><TD BGCOLOR=\"" << color << R"(" COLSPAN="3"><B>)" << bb->index_ << "</B>";

        if (const auto labelIt = bbAnnotationMap_.find(bb); labelIt != bbAnnotationMap_.end()) {
            out << "(" << labelIt->second.View().Mutf8() << ")";
        }

        out << "</TD></TR>";
        for (size_t i = 0; i < bb->nodes_.size(); ++i) {
            out << "<TR><TD PORT=\"f" << i << "\">" << i << "</TD><TD>" << ToString(bb->nodes_[i]->Type())
                << "</TD><TD>"
                << util::Helpers::EscapeHTMLString(allocator_, bb->nodes_[i]->DumpEtsSrc()).View().Mutf8()
                << "</TD></TR>";
        }
        out << "</TABLE>>];\n";

        DumpSuccessorEdges(out, bb);
    }

    for (auto switchCase : switchCaseMap_) {
        auto from = nodeBBMap_.find(switchCase.first);
        auto to = nodeBBMap_.find(switchCase.second);
        out << "  BB" << from->second.first->index_ << ":f" << from->second.second << " -> BB"
            << to->second.first->index_ << ":f" << to->second.second << " [color=\"green\" label=\"case\"];\n";
    }
    out << "}\n";
    return true;
}

CFG::BasicBlock *CFG::Build(ir::VariableDeclaration *variableDeclarationNode, BasicBlock *bb)
{
    for (auto declarator : variableDeclarationNode->Declarators()) {
        bb = Build(declarator->Init(), bb);
        bb = Build(declarator->Id(), bb);
    }
    AddNodeToBB(variableDeclarationNode, bb);
    return bb;
}

size_t CFG::AddNodeToBB(ir::AstNode *node, BasicBlock *bb)
{
    if (bb == nullptr) {
        bb = CreateNewBB({});
    }
    ES2PANDA_ASSERT(bb != nullptr);
    size_t index = bb->AddNode(node);
    nodeBBMap_[node] = std::make_pair(bb, index);
    return index;
}

bool CFG::VisitDepthFirst(const CFG::BBTraverser &traverser, const CFG::BasicBlock *bb,
                          ArenaSet<const CFG::BasicBlock *> &visitedBlocks,
                          ArenaVector<BasicBlock *> CFG::BasicBlock::*edges, bool allPath) const
{
    if (visitedBlocks.find(bb) != visitedBlocks.end()) {
        return true;
    }

    if (!traverser(bb)) {
        return false;
    }
    visitedBlocks.insert(bb);

    for (const auto nextBB : bb->*edges) {
        if (!VisitDepthFirst(traverser, nextBB, visitedBlocks, edges, allPath)) {
            visitedBlocks.erase(bb);
            return false;
        }
    }
    visitedBlocks.erase(bb);
    return true;
}

void CFG::IterateForwardDepthFirst(const BBTraverser &traverser, const BasicBlock *start, bool allPath) const
{
    ArenaSet<const BasicBlock *> visitedBlocks(allocator_->Adapter());
    if (start != nullptr) {
        VisitDepthFirst(traverser, start, visitedBlocks, &CFG::BasicBlock::succs_, allPath);
    } else {
        for (auto functions : functionNodeBBMap_) {
            VisitDepthFirst(traverser, functions.second, visitedBlocks, &CFG::BasicBlock::succs_, allPath);
        }
    }
}

ArenaList<const CFG::BasicBlock *> CFG::GetForwardDepthFirstOrder(const BasicBlock *start, bool allPath) const
{
    ArenaList<const CFG::BasicBlock *> orderedList(allocator_->Adapter());
    BBTraverser recorder = [&orderedList](const CFG::BasicBlock *bb) {
        orderedList.push_back(bb);
        return true;
    };
    IterateForwardDepthFirst(recorder, start, allPath);
    return orderedList;
}

void CFG::IterateBackwardDepthFirst(const BBTraverser &traverser, const BasicBlock *start, bool allPath) const
{
    ArenaSet<const BasicBlock *> visitedBlocks(allocator_->Adapter());
    if (start != nullptr) {
        VisitDepthFirst(traverser, start, visitedBlocks, &CFG::BasicBlock::preds_, allPath);
    }
}

ArenaList<const CFG::BasicBlock *> CFG::GetBackwardDepthFirstOrder(const BasicBlock *start, bool allPath) const
{
    ArenaList<const CFG::BasicBlock *> orderedList(allocator_->Adapter());
    BBTraverser recorder = [&orderedList](const CFG::BasicBlock *bb) {
        orderedList.push_back(bb);
        return true;
    };
    IterateBackwardDepthFirst(recorder, start, allPath);
    return orderedList;
}

std::pair<CFG::BasicBlock *, size_t> CFG::FindBasicBlock(ir::AstNode *node) const
{
    auto bbIt = nodeBBMap_.find(node);
    if (bbIt == nodeBBMap_.end()) {
        return {nullptr, 0};
    }
    return bbIt->second;
}

ArenaAllocator *CFG::Allocator() const
{
    return allocator_;
}

CFG::BasicBlock *CFG::FindEntryBasicBlock(ir::ScriptFunction *function) const
{
    auto bbIt = functionNodeBBMap_.find(function);
    if (bbIt == functionNodeBBMap_.end()) {
        return nullptr;
    }

    return bbIt->second;
}

void CFG::IterateForwardTopologicalOrder(const BBTraverser &traverser, const BasicBlock *start, bool closeLoop) const
{
    ArenaList<const BasicBlock *> orderedList = GetForwardTopologicalOrder(start, closeLoop);
    for (auto bb : orderedList) {
        if (!traverser(bb)) {
            break;
        }
    }
}

ArenaList<const CFG::BasicBlock *> CFG::GetForwardTopologicalOrder(const CFG::BasicBlock *start, bool closeLoop) const
{
    ArenaSet<const BasicBlock *> visitedBlocks(allocator_->Adapter());
    ArenaSet<const BasicBlock *> markedBlocks(allocator_->Adapter());
    ArenaList<const BasicBlock *> orderedList(allocator_->Adapter());
    VisitResult visitResult {orderedList, markedBlocks, visitedBlocks};

    if (start != nullptr) {
        VisitTopOrder(visitResult, start, &CFG::BasicBlock::succs_, closeLoop);
    } else {
        for (auto functions : functionNodeBBMap_) {
            VisitTopOrder(visitResult, functions.second, &CFG::BasicBlock::succs_, closeLoop);
        }
    }
    return orderedList;
}

void CFG::IterateBackwardTopologicalOrder(const BBTraverser &traverser, const BasicBlock *start, bool closeLoop) const
{
    ArenaList<const BasicBlock *> orderedList = GetBackwardTopologicalOrder(start, closeLoop);
    for (auto bb : orderedList) {
        if (!traverser(bb)) {
            break;
        }
    }
}

ArenaList<const CFG::BasicBlock *> CFG::GetBackwardTopologicalOrder(const CFG::BasicBlock *start, bool closeLoop) const
{
    ArenaSet<const BasicBlock *> visitedBlocks(allocator_->Adapter());
    ArenaSet<const BasicBlock *> markedBlocks(allocator_->Adapter());
    ArenaList<const BasicBlock *> orderedList(allocator_->Adapter());
    VisitResult visitResult {orderedList, markedBlocks, visitedBlocks};

    if (start != nullptr) {
        VisitTopOrder(visitResult, start, &CFG::BasicBlock::preds_, closeLoop);
    } else {
        for (auto functions : functionNodeBBMap_) {
            VisitTopOrder(visitResult, functions.second, &CFG::BasicBlock::preds_, closeLoop);
        }
    }
    return orderedList;
}

void CFG::VisitTopOrder(VisitResult &visitResult, const CFG::BasicBlock *bb,
                        ArenaVector<BasicBlock *> CFG::BasicBlock::*edges, bool closeLoop) const
{
    if (visitResult.visitedBlocks.find(bb) != visitResult.visitedBlocks.end()) {
        return;
    }

    if (visitResult.markedBlocks.find(bb) != visitResult.markedBlocks.end()) {
        if (closeLoop) {
            visitResult.orderedList.push_front(bb);
        }
        return;
    }

    visitResult.markedBlocks.insert(bb);

    for (const auto nextBB : bb->*edges) {
        VisitTopOrder(visitResult, nextBB, edges, closeLoop);
    }

    visitResult.markedBlocks.erase(bb);
    visitResult.visitedBlocks.insert(bb);
    visitResult.orderedList.push_front(bb);
}

const ArenaUnorderedMap<ir::ScriptFunction *, CFG::BasicBlock *> &CFG::GetFunctionEntries() const
{
    return functionNodeBBMap_;
}

const util::UString *CFG::GetAnnotation(const BasicBlock *bb) const
{
    if (const auto annotationlIt = bbAnnotationMap_.find(bb); annotationlIt != bbAnnotationMap_.end()) {
        return &annotationlIt->second;
    }
    return nullptr;
}

const ArenaUnorderedMap<ir::AstNode *, ir::AstNode *> &CFG::GetSwitchCaseMap() const
{
    return switchCaseMap_;
}

const ArenaSet<CFG::BasicBlock *> &CFG::GetBasicBlocks() const
{
    return blocks_;
}

void CFG::AddBBEdge(BasicBlock *from, BasicBlock *to, const ir::AstNode *label)
{
    auto [succEdgeIndex, prevEdgeIndex] = from->AddSuccessor(to);
    if (label != nullptr) {
        SetBBSuccEdgeLabel(from, succEdgeIndex, label);
        SetBBPredEdgeLabel(to, prevEdgeIndex, label);
    }
}

}  // namespace ark::es2panda::compiler

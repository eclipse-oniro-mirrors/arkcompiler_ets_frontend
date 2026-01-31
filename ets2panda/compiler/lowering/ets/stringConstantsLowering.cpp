/**
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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

#include "stringConstantsLowering.h"
#include "checker/ETSchecker.h"

namespace ark::es2panda::compiler {

enum class ChildLocation { LEFT_CHILD, RIGHT_CHILD };

static ChildLocation IsLeftOrRight(ir::Expression *const child)
{
    ES2PANDA_ASSERT(child->Parent()->IsBinaryExpression());
    auto const binOpParent = child->Parent()->AsBinaryExpression();
    auto const leftExprParent = binOpParent->Left();
    return (leftExprParent == child) ? ChildLocation::LEFT_CHILD : ChildLocation::RIGHT_CHILD;
}

static bool IsBinaryExpressionPlus(ir::AstNode *const node)
{
    if (node->IsBinaryExpression()) {
        auto const binOp = node->AsBinaryExpression();
        return binOp->OperatorType() == lexer::TokenType::PUNCTUATOR_PLUS;
    }
    return false;
}

/*
<---AST before (input to optimization)--->
        BinaryExpression
                +
        /              \
 StringLiteral   StringLiteral

<---AST after (output from optimization)--->
 StringLiteral (combined from two)
*/
static ir::AstNode *FoldStringLiterals(public_lib::Context *ctx, ir::StringLiteral *const leftStrLit,
                                       ir::StringLiteral *const rightStrLit)
{
    auto const resStr = util::UString(leftStrLit->Str().Mutf8() + rightStrLit->Str().Mutf8(), ctx->allocator).View();

    auto resNode = util::NodeAllocator::Alloc<ir::StringLiteral>(ctx->allocator, resStr);
    ES2PANDA_ASSERT(resNode != nullptr);
    resNode->SetRange({leftStrLit->Range().start, rightStrLit->Range().end});
    return resNode;
}

/*
<---AST before (input to optimization)--->
             BinaryExpression
                     +
            /                \
    BinaryExpression   StringLiteral
            +
    /              \
Identifier   StringLiteral

<---AST after (output from optimization)--->
     BinaryExpression
             +
    /                \
Identifier    BinaryExpression
                      +
              /              \
       StringLiteral   StringLiteral
*/
static ir::AstNode *ReorderConcat(public_lib::Context *ctx, ir::StringLiteral *leftStrLit,
                                  ir::StringLiteral *rightStrLit)
{
    auto const parentLeftSL = leftStrLit->Parent()->AsBinaryExpression();
    auto const parentRightSL = rightStrLit->Parent()->AsBinaryExpression();
    if (parentLeftSL->Parent()->IsBinaryExpression()) {
        auto const parentParentLeftSL = parentLeftSL->Parent()->AsBinaryExpression();
        auto const leftExprParentSL = parentLeftSL->Left();

        const ChildLocation childLoc1 = IsLeftOrRight(parentLeftSL);
        const ChildLocation childLoc2 = IsLeftOrRight(rightStrLit);

        (childLoc1 == ChildLocation::LEFT_CHILD) ? parentParentLeftSL->SetLeft(leftExprParentSL)
                                                 : parentParentLeftSL->SetRight(leftExprParentSL);
        auto foldedNode = FoldStringLiterals(ctx, leftStrLit, rightStrLit);
        (childLoc2 == ChildLocation::LEFT_CHILD) ? parentRightSL->SetLeft(foldedNode->AsStringLiteral())
                                                 : parentRightSL->SetRight(foldedNode->AsStringLiteral());
        return foldedNode;
    }
    return rightStrLit;
}

bool StringConstantsLowering::PerformForProgram(parser::Program *program)
{
    ir::StringLiteral *firstFoundSL = nullptr;
    program->Ast()->TransformChildrenRecursivelyPostorder(
        [&firstFoundSL, ctx = Context()](checker::AstNodePtr const node) -> checker::AstNodePtr {
            if (node->IsStringLiteral() && IsBinaryExpressionPlus(node->AsStringLiteral()->Parent())) {
                auto currNodeSL = node->AsStringLiteral();
                if (firstFoundSL != nullptr && firstFoundSL->Parent() != currNodeSL->Parent()) {
                    auto resNode = ReorderConcat(ctx, firstFoundSL, currNodeSL);
                    firstFoundSL = resNode->AsStringLiteral();
                    return resNode;
                }

                firstFoundSL = currNodeSL;
                return node;
            }
            if (firstFoundSL != nullptr && !IsBinaryExpressionPlus(node)) {
                firstFoundSL = nullptr;
            }

            return node;
        },
        Name());
    return true;
}

}  // namespace ark::es2panda::compiler

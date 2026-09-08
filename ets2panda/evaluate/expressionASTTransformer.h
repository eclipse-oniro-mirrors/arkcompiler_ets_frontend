/**
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef ES2PANDA_EVALUATE_EXPRESSION_AST_TRANSFORMER_H
#define ES2PANDA_EVALUATE_EXPRESSION_AST_TRANSFORMER_H

#include "util/ustring.h"
#include "generated/tokenType.h"

namespace ark::es2panda::ir {
class Expression;
class Statement;
class BlockStatement;
class BinaryExpression;
class CallExpression;
class ConditionalExpression;
class AssignmentExpression;
class UnaryExpression;
class UpdateExpression;
class SequenceExpression;
class TypeofExpression;
class Identifier;
class MemberExpression;
class StringLiteral;
class NumberLiteral;
class TSAsExpression;
class TSUnionType;
class TypeNode;
class ArrayExpression;
class TemplateLiteral;
class ETSNewArrayInstanceExpression;
class ETSNewClassInstanceExpression;
}  // namespace ark::es2panda::ir

namespace ark::es2panda::checker {
class ETSChecker;
}  // namespace ark::es2panda::checker

namespace ark::es2panda::evaluate {

// Independent Identifier nodes for one generated temp variable, so that the
// declaration, the write and the return positions never share AST nodes.
struct EvalTempIdents {
    ir::Identifier *decl;
    ir::Identifier *set;
    ir::Identifier *ret;
};

class ExpressionASTTransformer {
public:
    explicit ExpressionASTTransformer(checker::ETSChecker *checker);

    ir::Statement *Transform(ir::Expression *expression);

private:
    ir::Expression *TransformExpression(ir::Expression *node, ArenaVector<ir::Statement *> *stmts = nullptr);

    // Dispatch layers: values and operators (TransformExpression), accesses /
    // calls and type operations (TransformCompoundExpression), aggregates and
    // construction (TransformAggregateExpression).
    ir::Expression *TransformCompoundExpression(ir::Expression *node, ArenaVector<ir::Statement *> *stmts);
    ir::Expression *TransformAggregateExpression(ir::Expression *node, ArenaVector<ir::Statement *> *stmts);

    // Leaf handlers
    ir::Expression *TransformLiteral(ir::Expression *node);
    ir::Expression *TransformIdentifier(ir::Identifier *node);
    ir::Expression *TransformThisExpression(ArenaVector<ir::Statement *> *stmts);
    ir::Expression *TransformSequenceExpression(ir::SequenceExpression *node, ArenaVector<ir::Statement *> *stmts);
    ir::Expression *TransformTypeofExpression(ir::TypeofExpression *node, ArenaVector<ir::Statement *> *stmts);

    // Compound handlers
    ir::Expression *TransformBinary(ir::BinaryExpression *node, ArenaVector<ir::Statement *> *stmts = nullptr);
    ir::Expression *TransformNullishCoalescing(ir::BinaryExpression *node, ArenaVector<ir::Statement *> *stmts);
    ir::Expression *TransformInstanceof(ir::BinaryExpression *node, ArenaVector<ir::Statement *> *stmts);
    ir::Expression *TransformArithmeticBinary(lexer::TokenType op, ir::Expression *left, ir::Expression *right);
    ir::Expression *TransformUnary(ir::UnaryExpression *node, ArenaVector<ir::Statement *> *stmts = nullptr);
    ir::Expression *TransformUpdateExpression(ir::UpdateExpression *node, ArenaVector<ir::Statement *> *stmts);
    ir::Expression *TransformMember(ir::MemberExpression *node, ArenaVector<ir::Statement *> *stmts = nullptr);
    ir::Expression *TransformElementAccess(ir::MemberExpression *node, ArenaVector<ir::Statement *> *stmts);
    ir::Expression *BuildMemberChain(ir::MemberExpression *node, ArenaVector<ir::Statement *> *stmts = nullptr);
    ir::Expression *TransformCall(ir::CallExpression *node, ArenaVector<ir::Statement *> *stmts = nullptr);
    ir::Expression *TransformMemberCalleeCall(ir::MemberExpression *memberExpr, ir::CallExpression *node,
                                              ArenaVector<ir::Statement *> *stmts);
    ir::Expression *TransformSuperCall(ir::Expression *methodName, ir::CallExpression *node,
                                       ArenaVector<ir::Statement *> *stmts);
    ir::Expression *TransformOptionalChain(ir::Expression *node, ArenaVector<ir::Statement *> *stmts = nullptr);
    ir::Expression *TransformOptionalMember(ir::MemberExpression *me, ArenaVector<ir::Statement *> *stmts);
    ir::Expression *TransformOptionalCall(ir::CallExpression *call, ArenaVector<ir::Statement *> *stmts);
    ir::Expression *BuildChainMemberAccess(ir::MemberExpression *me, ir::Expression *target,
                                           ArenaVector<ir::Statement *> *stmts);
    ir::Expression *BuildChainInvokeValue(ir::Expression *receiver, ArenaVector<ir::Expression *> &&args);
    ir::Expression *DeclareNullCheckedTemp(ir::Expression *value, util::StringView prefix, ir::Identifier **tempRef,
                                           ArenaVector<ir::Statement *> *stmts);
    ir::Expression *TransformCallArgument(ir::Expression *arg, ArenaVector<ir::Statement *> *stmts);

    checker::ETSChecker *checker_;
    ir::Expression *TransformConditional(ir::ConditionalExpression *node,
                                         ArenaVector<ir::Statement *> *stmts = nullptr);
    ir::Expression *LiftConditionalBranches(ir::Expression *test, ir::Expression *consequent,
                                            ArenaVector<ir::Statement *> &&consStmts, ir::Expression *alternate,
                                            ArenaVector<ir::Statement *> &&altStmts,
                                            ArenaVector<ir::Statement *> *stmts);
    ir::BlockStatement *BuildConditionalBranch(ArenaVector<ir::Statement *> &&branchStmts, ir::Expression *branchValue,
                                               ir::Expression *target);
    ir::Statement *TransformAssignment(ir::AssignmentExpression *node);

    // Assignment helpers
    ir::Statement *HandleSubscriptAssignment(ir::MemberExpression *me, ir::AssignmentExpression *node,
                                             lexer::TokenType op, ArenaVector<ir::Statement *> &&stmts);
    ir::Statement *HandleSubscriptStringAssignment(ir::MemberExpression *me, ir::AssignmentExpression *node,
                                                   ir::Expression *arrExpr, ArenaVector<ir::Statement *> &&stmts);
    ir::Statement *EmitSubscriptWriteback(ir::Expression *arrExpr, ir::Expression *idxExpr,
                                          ir::AssignmentExpression *node, lexer::TokenType op,
                                          ArenaVector<ir::Statement *> &&stmts);
    ir::Expression *BuildSubscriptIndex(ir::Expression *idxExpr, ArenaVector<ir::Statement *> *stmts);
    ir::Expression *BuildSubscriptNewValue(ir::Expression *arrProxy, ir::Expression *idxExpr,
                                           ir::AssignmentExpression *node, lexer::TokenType op,
                                           ir::Expression **idxForGet, ArenaVector<ir::Statement *> *stmts);
    ir::Expression *BuildSubscriptSetCall(ir::Expression *arrExpr, ir::Expression *arrProxy, ir::Expression *idxForGet,
                                          ir::Expression *idxExpr, ir::Identifier *valueIdent,
                                          ArenaVector<ir::Statement *> *stmts);
    ir::Statement *BuildAssignWriteback(ir::Expression *lhsProxy, ir::Expression *rawForArith, ir::Expression *left,
                                        ir::AssignmentExpression *node, lexer::TokenType op,
                                        ArenaVector<ir::Statement *> &&stmts);
    ir::Expression *BuildCompoundRhs(ir::AssignmentExpression *node, ir::Expression *curValue,
                                     ArenaVector<ir::Statement *> *stmts);
    EvalTempIdents DeclareEvalTemp(util::StringView prefix, ir::Expression *rhsValue,
                                   ArenaVector<ir::Statement *> *stmts);
    std::string_view GetCompoundOpMethod(ir::AssignmentExpression *node);
    ir::BlockStatement *BuildAssignReturnBlock(ArenaVector<ir::Statement *> &&stmts);
    ir::Expression *BuildIdentifierLHS(ir::Identifier *ident);
    ir::Expression *BuildMemberLHS(ir::MemberExpression *memberExpr, ArenaVector<ir::Statement *> *stmts);

    // Aggregate / construction handlers
    ir::Expression *TransformArrayLiteral(ir::ArrayExpression *node, ArenaVector<ir::Statement *> *stmts);
    ir::Expression *TransformTemplateLiteral(ir::TemplateLiteral *node, ArenaVector<ir::Statement *> *stmts);
    ir::Expression *TransformAsExpression(ir::TSAsExpression *node, ArenaVector<ir::Statement *> *stmts);
    // Returns the erased `as Array<Object>` form for a cast with an unbindable
    // element type, or nullptr when the cast should keep the native path
    // (bindable element) / be rejected (readonly + unbindable element).
    ir::Expression *EraseArrayCast(ir::TSAsExpression *node, ir::Expression *transformedExpr);
    ir::Expression *TransformNewArrayInstance(ir::ETSNewArrayInstanceExpression *node,
                                              ArenaVector<ir::Statement *> *stmts);
    ir::Expression *BridgeValueByElementType(ir::TypeNode *elemType, ir::Expression *value);
    ir::Expression *TransformNewClassInstance(ir::ETSNewClassInstanceExpression *node,
                                              ArenaVector<ir::Statement *> *stmts);

    // AST builder helpers
    ir::CallExpression *MakeDebuggerAPIStaticCall(util::StringView methodName, ArenaVector<ir::Expression *> &&args);
    ir::CallExpression *MakeProxyInstanceCall(ir::Expression *receiver, util::StringView methodName,
                                              ArenaVector<ir::Expression *> &&args);
    ir::MemberExpression *MakeStaticMemberAccess(util::StringView className, util::StringView methodName);
    ir::Identifier *MakeIdentifier(util::StringView name);
    ir::StringLiteral *MakeStringLiteral(util::StringView value);

    // Type helpers
    ir::TSAsExpression *MakeArrayOfObjectType(ir::Expression *arrProxy);
    ir::Expression *WrapInValueCall(ir::Expression *expr);
    ir::Expression *WrapInWrap(ir::Expression *expr);
};

}  // namespace ark::es2panda::evaluate

#endif  // ES2PANDA_EVALUATE_EXPRESSION_AST_TRANSFORMER_H

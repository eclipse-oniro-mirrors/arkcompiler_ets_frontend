/*
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

//
//  desc:   For-of-loop syntax is translated to the while-loop syntax by calling of special method
//          providing predefined 'iterator' interface:
//  for (let x of c) {    // c is an object of 'iterable' class
//    <body>
//  }
//  ...
//  let_ci_=_c.$_iterator()
//  let_it_=_ci.next()
//  while_(!it.done)_{
//    x_=_it.value!
//    <body>
//    it_=_ci.next()
//  }
//

#include "objectIterator.h"

#include "generated/signatures.h"
#include "macros.h"
#include "parser/ETSparser.h"
#include "compiler/lowering/util.h"
#include "compiler/lowering/scopesInit/scopesInitPhase.h"
#include "checker/ETSchecker.h"
#include "util/options.h"

namespace ark::es2panda::compiler {

static constexpr std::size_t const WHILE_LOOP_POSITION = 1U;
static constexpr std::size_t const WHILE_LOOP_SIZE = 3U;

std::string_view ObjectIteratorLowering::Name() const
{
    static std::string const NAME = "ObjectIteratorLowering";
    return NAME;
}

void ObjectIteratorLowering::TransferForOfLoopBody(ir::Statement *const forBody,
                                                   ir::BlockStatement *const whileBody) const noexcept
{
    ES2PANDA_ASSERT(forBody != nullptr && whileBody != nullptr);
    auto &whileStatements = whileBody->Statements();

    //  Currently while loop body consists of 2 statements: 'x = it.value!' and 'it = ci.next()'
    //  We need to insert the body of original for-of-loop between them, change their parent and
    //  probably clean types for expressions and variables for identifier for subsequent re-check.
    if (forBody->IsBlockStatement()) {
        auto &forStatements = forBody->AsBlockStatement()->Statements();
        std::size_t const forSize = forStatements.size();

        whileStatements.resize(WHILE_LOOP_SIZE + forSize);

        for (std::size_t i = 0U; i < forSize; ++i) {
            auto &statement = forStatements[i];
            statement->SetParent(whileBody);
            ClearTypesVariablesAndScopes(statement);
            whileStatements[WHILE_LOOP_SIZE + i] = statement;
        }
    } else {
        whileStatements.resize(WHILE_LOOP_SIZE + 1U);

        forBody->SetParent(whileBody);
        ClearTypesVariablesAndScopes(forBody);
        whileStatements[WHILE_LOOP_SIZE] = forBody;
    }
}

// interface Iterator<T> maybe implements by other classes
// we need the instantiated <T>
// so to find in interface and super
checker::Type *FindInstantiatedTypeParamFromIterator(checker::ETSObjectType *itor)
{
    if (itor == nullptr) {
        return nullptr;
    }
    if (itor->Name() == compiler::Signatures::ITERATOR_CLASS) {
        return itor->TypeArguments().front();
    }
    for (auto interface : itor->Interfaces()) {
        if (auto type = FindInstantiatedTypeParamFromIterator(interface); type != nullptr) {
            return type;
        }
    }
    if (auto type = FindInstantiatedTypeParamFromIterator(itor->SuperType()); type != nullptr) {
        return type;
    }
    return nullptr;
}

static ir::OpaqueTypeNode *FindIterValueType(checker::ETSObjectType *type, ArenaAllocator *allocator)
{
    auto *itor = type->GetProperty(compiler::Signatures::ITERATOR_METHOD,
                                   checker::PropertySearchFlags::SEARCH_INSTANCE_METHOD |
                                       checker::PropertySearchFlags::SEARCH_IN_INTERFACES |
                                       checker::PropertySearchFlags::SEARCH_IN_BASE);
    ES2PANDA_ASSERT(itor != nullptr);
    auto const &sigs = itor->TsType()->AsETSFunctionType()->CallSignatures();
    checker::ETSObjectType *itorReturnType = nullptr;
    for (auto &sig : sigs) {
        if (sig->Params().empty()) {
            itorReturnType = sig->ReturnType()->AsETSObjectType();
            break;
        }
    }
    ES2PANDA_ASSERT(itorReturnType);
    auto *valueType = FindInstantiatedTypeParamFromIterator(itorReturnType);
    return allocator->New<ir::OpaqueTypeNode>(valueType, allocator);
}

ir::Statement *ObjectIteratorLowering::ProcessObjectIterator(public_lib::Context *ctx,
                                                             ir::ForOfStatement *forOfStatement) const
{
    //  Note! We assume that parser, varbinder and checker phases have been already passed correctly, thus the
    //  class has required accessible iterator method and all the types and scopes are properly resolved.
    auto *const allocator = ctx->Allocator();

    auto *const varbinder = ctx->checker->VarBinder()->AsETSBinder();
    ES2PANDA_ASSERT(varbinder != nullptr);
    auto statementScope = varbinder::LexicalScope<varbinder::Scope>::Enter(varbinder, NearestScope(forOfStatement));

    ir::Identifier *const iterIdent = Gensym(allocator);
    ir::Identifier *const nextIdent = Gensym(allocator);
    ir::Identifier *loopVariableIdent = nullptr;

    // find $_iterator->ReturnType->Iterator<number>->number
    // we cannot simply use next().value! , because value itself maybe undefined or null
    ir::AstNode *typeNode;
    auto exprType = forOfStatement->Right()->TsType();
    if (!exprType->IsETSObjectType()) {
        return forOfStatement;
    }
    typeNode = FindIterValueType(exprType->AsETSObjectType(), allocator);

    //  Replace the for-of loop with the while loop using the provided iterator interface
    std::string whileStatement = "let @@I1 = (@@E2)." + std::string {compiler::Signatures::ITERATOR_METHOD} + "(); ";
    whileStatement += "while (true) { ";
    whileStatement += "let @@I3 = @@I4.next(); ";
    whileStatement += "if (@@I5.done) break;";

    if (auto *const left = forOfStatement->Left(); left->IsVariableDeclaration()) {
        auto *const declaration = left->AsVariableDeclaration();
        whileStatement +=
            declaration->Kind() != ir::VariableDeclaration::VariableDeclarationKind::CONST ? "let " : "const ";
        loopVariableIdent = declaration->Declarators().at(0U)->Id()->AsIdentifier()->Clone(allocator, nullptr);
    } else if (left->IsIdentifier()) {
        loopVariableIdent = Gensym(allocator);
        loopVariableIdent->SetName(left->AsIdentifier()->Name());
    } else {
        ES2PANDA_UNREACHABLE();
    }
    whileStatement += "@@I6 = (@@I7.value as @@T8);}; ";

    // Parse ArkTS code string and create corresponding AST nodes
    auto *const parser = ctx->parser->AsETSParser();
    ES2PANDA_ASSERT(parser != nullptr);

    auto *const loweringResult = parser->CreateFormattedStatement(
        whileStatement, iterIdent, forOfStatement->Right(), nextIdent, iterIdent->Clone(allocator, nullptr),
        nextIdent->Clone(allocator, nullptr), loopVariableIdent, nextIdent->Clone(allocator, nullptr), typeNode);
    ES2PANDA_ASSERT(loweringResult != nullptr);
    loweringResult->SetParent(forOfStatement->Parent());
    loweringResult->SetRange(forOfStatement->Range());

    TransferForOfLoopBody(forOfStatement->Body(), loweringResult->AsBlockStatement()
                                                      ->Statements()[WHILE_LOOP_POSITION]
                                                      ->AsWhileStatement()
                                                      ->Body()
                                                      ->AsBlockStatement());

    auto *const checker = ctx->checker->AsETSChecker();
    ES2PANDA_ASSERT(checker != nullptr);
    CheckLoweredNode(varbinder, checker, loweringResult);

    return loweringResult;
}

bool ObjectIteratorLowering::PerformForModule(public_lib::Context *ctx, parser::Program *program)
{
    auto hasIterator = [](checker::Type const *const exprType) -> bool {
        return exprType != nullptr && (exprType->IsETSObjectType() || exprType->IsETSTypeParameter());
    };

    program->Ast()->TransformChildrenRecursively(
        // clang-format off
        [this, ctx, &hasIterator](ir::AstNode *ast) -> ir::AstNode* {
            // clang-format on
            if (ast->IsForOfStatement()) {
                if (auto const *const exprType = ast->AsForOfStatement()->Right()->TsType();
                    hasIterator(exprType) || (exprType != nullptr && exprType->IsETSUnionType() &&
                                              exprType->AsETSUnionType()->AllOfConstituentTypes(hasIterator))) {
                    return ProcessObjectIterator(ctx, ast->AsForOfStatement());
                }
            }
            return ast;
        },
        Name());

    return true;
}
}  // namespace ark::es2panda::compiler

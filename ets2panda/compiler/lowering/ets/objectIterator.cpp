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
#include "libarkbase/macros.h"
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
    auto &whileStatements = whileBody->StatementsForUpdates();

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

static checker::ETSObjectType *GetIteratorMethodReturnType(const checker::ETSObjectType *type)
{
    auto *const itor = type->GetProperty(compiler::Signatures::ITERATOR_METHOD,
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
    return itorReturnType;
}

static std::vector<checker::Type *> CollectUnionIteratorTypes(checker::ETSChecker *checker,
                                                              const checker::ETSUnionType *unionType)
{
    std::vector<checker::Type *> iteratorTypes;
    for (auto &constituentType : unionType->ConstituentTypes()) {
        if (constituentType->IsETSUnionType()) {
            auto nestedTypes = CollectUnionIteratorTypes(checker, constituentType->AsETSUnionType());
            iteratorTypes.insert(iteratorTypes.end(), nestedTypes.begin(), nestedTypes.end());
            continue;
        }

        if (constituentType->IsETSObjectType()) {
            auto *const methodReturnType = GetIteratorMethodReturnType(constituentType->AsETSObjectType());
            auto *const iterValueType = FindInstantiatedTypeParamFromIterator(methodReturnType);
            iteratorTypes.emplace_back(iterValueType);
        }
    }
    return iteratorTypes;
}

static checker::Type *ResolveIteratorValueType(checker::ETSChecker *checker, checker::Type const *const exprType)
{
    if (exprType == nullptr) {
        return nullptr;
    }
    if (exprType->IsETSObjectType()) {
        auto *const methodReturnType = GetIteratorMethodReturnType(exprType->AsETSObjectType());
        return FindInstantiatedTypeParamFromIterator(methodReturnType);
    }
    if (exprType->IsETSUnionType()) {
        auto iteratorTypes = CollectUnionIteratorTypes(checker, exprType->AsETSUnionType());
        ArenaVector<checker::Type *> types(iteratorTypes.begin(), iteratorTypes.end(), checker->Allocator()->Adapter());
        return checker->CreateETSUnionType(Span<checker::Type *const>(types));
    }
    return nullptr;
}

static ir::Identifier *GetLoopVariable(ArenaAllocator *allocator, ir::ForOfStatement *forOfStatement)
{
    if (auto *const left = forOfStatement->Left(); left->IsVariableDeclaration()) {
        auto *const declaration = left->AsVariableDeclaration();
        return declaration->Declarators().at(0U)->Id()->AsIdentifier()->Clone(allocator, nullptr);
    }

    if (auto *const left = forOfStatement->Left(); left->IsIdentifier()) {
        auto *loopVariableIdent = Gensym(allocator);
        ES2PANDA_ASSERT(loopVariableIdent != nullptr);
        loopVariableIdent->SetName(left->AsIdentifier()->Name());
        return loopVariableIdent;
    }

    ES2PANDA_UNREACHABLE();
}

static std::string GetDeclarationPrefix(ir::ForOfStatement *forOfStatement)
{
    if (auto *const left = forOfStatement->Left(); left->IsVariableDeclaration()) {
        return left->AsVariableDeclaration()->Kind() != ir::VariableDeclaration::VariableDeclarationKind::CONST
                   ? "let "
                   : "const ";
    }
    return "";
}

ir::Statement *ObjectIteratorLowering::GenerateLoweredStatement(parser::ETSParser *parser,
                                                                ir::ForOfStatement *forOfStatement,
                                                                ir::AstNode *typeNode) const
{
    auto *const allocator = Context()->Allocator();
    auto *iterIdent = Gensym(allocator);
    auto *nextIdent = Gensym(allocator);
    auto *loopVariableIdent = GetLoopVariable(allocator, forOfStatement);
    auto exprType = forOfStatement->Right()->TsType();
    std::string declPrefix = GetDeclarationPrefix(forOfStatement);

    if (exprType->IsETSUnionType()) {
        std::string const unionWhile = "let @@I1 = (@@E2 as Iterable<@@T3>)." +
                                       std::string {compiler::Signatures::ITERATOR_METHOD} +
                                       "(); "
                                       "while (true) { "
                                       "let @@I4 = @@I5.next(); "
                                       "if (@@I6.done) break; " +
                                       declPrefix + "@@I7 = (@@I8.value as @@T9); }";

        return parser->CreateFormattedStatement(
            unionWhile, iterIdent, forOfStatement->Right(), typeNode, nextIdent, iterIdent->Clone(allocator, nullptr),
            nextIdent->Clone(allocator, nullptr), loopVariableIdent, nextIdent->Clone(allocator, nullptr),
            typeNode->Clone(allocator, nullptr));
    }

    std::string const stdWhile = "let @@I1 = (@@E2)." + std::string {compiler::Signatures::ITERATOR_METHOD} +
                                 "(); "
                                 "while (true) { "
                                 "let @@I3 = @@I4.next(); "
                                 "if (@@I5.done) break; " +
                                 declPrefix + "@@I6 = (@@I7.value as @@T8); }";

    return parser->CreateFormattedStatement(stdWhile, iterIdent, forOfStatement->Right(), nextIdent,
                                            iterIdent->Clone(allocator, nullptr), nextIdent->Clone(allocator, nullptr),
                                            loopVariableIdent, nextIdent->Clone(allocator, nullptr), typeNode);
}

ir::Statement *ObjectIteratorLowering::ProcessObjectIterator(ir::ForOfStatement *forOfStatement) const
{
    //  Note! We assume that parser, varbinder and checker phases have been already passed correctly, thus the
    //  class has required accessible iterator method and all the types and scopes are properly resolved.
    auto *const allocator = Context()->Allocator();
    auto *const checker = Context()->GetChecker()->AsETSChecker();

    auto *const varbinder = Context()->GetChecker()->VarBinder()->AsETSBinder();
    ES2PANDA_ASSERT(varbinder != nullptr);
    auto statementScope = varbinder::LexicalScope<varbinder::Scope>::Enter(varbinder, NearestScope(forOfStatement));

    // find $_iterator->ReturnType->Iterator<number>->number
    // we cannot simply use next().value! , because value itself maybe undefined or null
    auto exprType = forOfStatement->Right()->TsType();
    auto returnType = ResolveIteratorValueType(checker, exprType);
    if (returnType == nullptr) {
        return forOfStatement;
    }
    auto *typeNode = allocator->New<ir::OpaqueTypeNode>(returnType, allocator);

    auto *const parser = Context()->parser->AsETSParser();
    ES2PANDA_ASSERT(parser != nullptr);

    ir::Statement *loweringResult = GenerateLoweredStatement(parser, forOfStatement, typeNode);

    ES2PANDA_ASSERT(loweringResult != nullptr);
    loweringResult->SetParent(forOfStatement->Parent());
    loweringResult->SetRange(forOfStatement->Range());

    auto loweredWhile = loweringResult->AsBlockStatement()->Statements()[WHILE_LOOP_POSITION]->AsWhileStatement();
    auto whileBody = loweredWhile->Body()->AsBlockStatement();
    TransferForOfLoopBody(forOfStatement->Body(), whileBody);

    ES2PANDA_ASSERT(checker != nullptr);
    CheckLoweredNode(varbinder, checker, loweringResult);

    if (loweringResult->Parent()->IsLabelledStatement()) {
        loweringResult->Parent()->AsLabelledStatement()->Ident()->Variable()->GetScope()->BindNode(loweringResult);
    }
    return loweringResult;
}

bool ObjectIteratorLowering::PerformForProgram(parser::Program *program)
{
    auto hasIterator = [](checker::Type const *const exprType) -> bool {
        return exprType != nullptr && (exprType->IsETSObjectType() || exprType->IsETSTypeParameter());
    };

    program->Ast()->TransformChildrenRecursively(
        // clang-format off
        [this, &hasIterator](ir::AstNode *ast) -> ir::AstNode* {
            // clang-format on
            if (ast->IsForOfStatement()) {
                if (auto const *const exprType = ast->AsForOfStatement()->Right()->TsType();
                    hasIterator(exprType) || (exprType != nullptr && exprType->IsETSUnionType() &&
                                              exprType->AsETSUnionType()->AllOfConstituentTypes(hasIterator))) {
                    return ProcessObjectIterator(ast->AsForOfStatement());
                }
            }
            return ast;
        },
        Name());

    return true;
}
}  // namespace ark::es2panda::compiler

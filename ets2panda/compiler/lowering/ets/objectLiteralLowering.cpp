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

#include "objectLiteralLowering.h"
#include "checker/ETSchecker.h"
#include "compiler/lowering/scopesInit/scopesInitPhase.h"
#include "compiler/lowering/util.h"

namespace ark::es2panda::compiler {

std::string_view ObjectLiteralLowering::Name() const
{
    return "ObjectLiteralLowering";
}

static void MaybeAllowConstAssign(checker::Type *targetType, ArenaVector<ir::Statement *> &statements)
{
    if (!targetType->IsETSObjectType()) {
        return;
    }
    for (auto *const stmt : statements) {
        if (!stmt->IsExpressionStatement() ||
            !stmt->AsExpressionStatement()->GetExpression()->IsAssignmentExpression()) {
            continue;
        }

        auto *const assignmentExpr = stmt->AsExpressionStatement()->GetExpression()->AsAssignmentExpression();
        assignmentExpr->SetIgnoreConstAssign();
    }
}

static constexpr std::string_view NESTED_BLOCK_EXPRESSION = "_$NESTED_BLOCK_EXPRESSION$_";

static void RestoreNestedBlockExpression(const ArenaVector<ir::Statement *> &statements,
                                         std::deque<ir::BlockExpression *> &nestedBlckExprs, varbinder::Scope *scope)
{
    if (!nestedBlckExprs.empty()) {
        for (auto stmt : statements) {
            if (!stmt->IsExpressionStatement() ||
                !stmt->AsExpressionStatement()->GetExpression()->IsAssignmentExpression()) {
                continue;
            }

            auto *assign = stmt->AsExpressionStatement()->GetExpression()->AsAssignmentExpression();

            if (assign->Right()->IsStringLiteral() &&
                assign->Right()->AsStringLiteral()->Str().Is(NESTED_BLOCK_EXPRESSION)) {
                auto nestedBlckExpr = nestedBlckExprs.front();
                nestedBlckExprs.pop_front();
                nestedBlckExpr->Scope()->SetParent(scope);
                assign->SetRight(nestedBlckExpr);
            }
        }
        // All nested block expressions should be restored
        ES2PANDA_ASSERT(nestedBlckExprs.empty());
    }
}

static void AllowRequiredTypeInstantiation(const ir::Expression *const loweringResult)
{
    if (!loweringResult->IsBlockExpression()) {
        return;
    }

    const auto *const blockExpression = loweringResult->AsBlockExpression();
    const auto *const firstStatement = blockExpression->Statements().front();
    if (!firstStatement->IsVariableDeclaration() ||
        !firstStatement->AsVariableDeclaration()->Declarators().front()->Init()->IsETSNewClassInstanceExpression()) {
        return;
    }

    const auto *const varDecl = firstStatement->AsVariableDeclaration()->Declarators().front()->Init();

    varDecl->AddAstNodeFlags(ir::AstNodeFlags::ALLOW_REQUIRED_INSTANTIATION);

    for (auto *const stmt : blockExpression->Statements()) {
        if (!stmt->IsExpressionStatement() ||
            !stmt->AsExpressionStatement()->GetExpression()->IsAssignmentExpression() ||
            !stmt->AsExpressionStatement()->GetExpression()->AsAssignmentExpression()->Right()->IsBlockExpression()) {
            continue;
        }

        AllowRequiredTypeInstantiation(
            stmt->AsExpressionStatement()->GetExpression()->AsAssignmentExpression()->Right()->AsBlockExpression());
    }
}

static bool CheckReadonlyAndUpdateCtorArgs(const ir::Identifier *key, ir::Expression *value,
                                           std::map<util::StringView, ir::Expression *> &ctorArgumentsMap)
{
    ES2PANDA_ASSERT(key != nullptr);
    auto varType = (key->Variable() != nullptr) ? key->Variable()->TsType() : nullptr;
    if (varType == nullptr || varType->HasTypeFlag(checker::TypeFlag::SETTER)) {
        return false;
    }

    if (ctorArgumentsMap.find(key->Name()) == ctorArgumentsMap.end()) {
        return false;
    }

    ctorArgumentsMap[key->Name()] = value;
    return true;
}

static void PopulateCtorArgumentsFromMap(public_lib::Context *ctx, ir::ObjectExpression *objExpr,
                                         ArenaVector<ir::Expression *> &ctorArguments,
                                         std::map<util::StringView, ir::Expression *> &ctorArgumentsMap)
{
    if (ctorArgumentsMap.empty()) {
        return;
    }
    auto *const allocator = ctx->Allocator();
    auto *const classType = objExpr->TsType()->AsETSObjectType();

    for (auto param : classType->ConstructSignatures().front()->Params()) {
        auto ctorArgument = ctorArgumentsMap[param->Declaration()->Name()];
        if (ctorArgument == nullptr && objExpr->PreferredType()->IsETSObjectType() &&
            objExpr->PreferredType()->AsETSObjectType()->IsPartial()) {
            ctorArguments.push_back(allocator->New<ir::UndefinedLiteral>());
            continue;
        }
        if (ctorArgument == nullptr && param->TsType()->PossiblyETSUndefined()) {
            ctorArguments.push_back(allocator->New<ir::UndefinedLiteral>());
            continue;
        }
        ES2PANDA_ASSERT(ctorArgument != nullptr);
        ctorArguments.push_back(ctorArgument);
    }
}

static void SetInstanceArguments(ArenaVector<ir::Statement *> &statements, ArenaVector<ir::Expression *> &ctorArguments)
{
    if (statements.empty() || ctorArguments.empty()) {
        return;
    }

    const auto *const firstStatement = statements.front();
    if (!firstStatement->IsVariableDeclaration()) {
        return;
    }

    auto declarator = firstStatement->AsVariableDeclaration()->Declarators().front();
    auto *initExpression = declarator->Init();
    if (initExpression == nullptr || !initExpression->IsETSNewClassInstanceExpression()) {
        return;
    }

    auto *instance = initExpression->AsETSNewClassInstanceExpression();
    for (auto *arg : ctorArguments) {
        arg->SetParent(instance);
    }
    instance->SetArguments(std::move(ctorArguments));
}

static void GenerateArgsForAnonymousClassType(const checker::ETSObjectType *classType, const bool &isAnonymous,
                                              std::map<util::StringView, ir::Expression *> &ctorArgumentsMap)
{
    if (isAnonymous) {
        checker::Signature *sig = classType->ConstructSignatures().front();
        for (auto param : sig->Params()) {
            ES2PANDA_ASSERT(param->Declaration() != nullptr);
            ctorArgumentsMap.emplace(param->Declaration()->Name(), nullptr);
        }
    }
}

static void GenerateNewStatements(public_lib::Context *ctx, ir::ObjectExpression *objExpr, std::stringstream &ss,
                                  std::vector<ir::AstNode *> &newStmts,
                                  std::deque<ir::BlockExpression *> &nestedBlckExprs,
                                  ArenaVector<ir::Expression *> &ctorArguments)
{
    auto *const allocator = ctx->Allocator();

    auto *const classType = objExpr->TsType()->AsETSObjectType();

    auto addNode = [&newStmts](ir::AstNode *node) -> int {
        newStmts.emplace_back(node);
        return newStmts.size();
    };

    // Generating: let <genSym>: <TsType> = new <TsType>();
    auto *genSymIdent = Gensym(allocator);
    auto *type = ctx->AllocNode<ir::OpaqueTypeNode>(classType, allocator);
    ES2PANDA_ASSERT(genSymIdent != nullptr && type != nullptr);
    ss << "let @@I" << addNode(genSymIdent) << ": @@T" << addNode(type) << " = new @@T"
       << addNode(type->Clone(allocator, nullptr)) << "();" << std::endl;

    // Generating: <genSym>.key_i = value_i      ( i <= [0, object_literal.properties.size) )
    bool isAnonymous = IsAnonymousClassType(classType);

    std::map<util::StringView, ir::Expression *> ctorArgumentsMap;
    GenerateArgsForAnonymousClassType(classType, isAnonymous, ctorArgumentsMap);

    for (auto *propExpr : objExpr->Properties()) {
        //  Skip possibly invalid properties:
        if (!propExpr->IsProperty()) {
            ES2PANDA_ASSERT(ctx->GetChecker()->AsETSChecker()->IsAnyError());
            continue;
        }

        auto *prop = propExpr->AsProperty();
        ir::Expression *key = prop->Key();
        ir::Expression *value = prop->Value();

        //  Processing of possible invalid property key
        ir::Identifier *keyIdent;
        if (key->IsStringLiteral()) {
            keyIdent = ctx->AllocNode<ir::Identifier>(key->AsStringLiteral()->Str(), allocator);
        } else if (key->IsIdentifier()) {
            keyIdent = key->AsIdentifier();
        } else {
            continue;
        }

        if (isAnonymous && CheckReadonlyAndUpdateCtorArgs(keyIdent, value, ctorArgumentsMap)) {
            continue;
        }
        ss << "@@I" << addNode(genSymIdent->Clone(allocator, nullptr)) << ".@@I" << addNode(keyIdent);

        if (value->IsBlockExpression()) {
            // Case of nested object literal (all nested object literals has already been processed)
            // Corresponding nested block expressions should be stored somewhere and restored after ScopesPhase
            // Because it has already processed them
            // Predefined String Literal acts as placeholder
            ss << " = \"" << NESTED_BLOCK_EXPRESSION << "\";" << std::endl;
            nestedBlckExprs.emplace_back(value->AsBlockExpression());
        } else {
            ss << " = @@E" << addNode(value) << ";" << std::endl;
        }
    }

    PopulateCtorArgumentsFromMap(ctx, objExpr, ctorArguments, ctorArgumentsMap);

    ss << "(@@I" << addNode(genSymIdent->Clone(allocator, nullptr)) << ");" << std::endl;
}

static ir::AstNode *HandleObjectLiteralLowering(public_lib::Context *ctx, ir::ObjectExpression *objExpr)
{
    /*
     * For given object literal of class type generates following block expression:
     *
     *  ({
     *     let <genSym>: <TsType> = new <TsType>();
     *     <genSym>.key_i = value_i      ( i <= [0, object_literal.properties.size) )
     *     <genSym>;                     <-- NOTE: result of block expression
     *  })
     */

    if (objExpr->TsType() == nullptr) {
        return objExpr;
    }

    auto *const checker = ctx->GetChecker()->AsETSChecker();
    auto *const parser = ctx->parser->AsETSParser();
    auto *const varbinder = ctx->GetChecker()->VarBinder()->AsETSBinder();

    checker->CheckObjectLiteralKeys(objExpr->Properties());

    std::stringstream ss;
    // Double-ended queue for storing nested block expressions that have already been processed earlier
    std::deque<ir::BlockExpression *> nestedBlckExprs;
    std::vector<ir::AstNode *> newStmts;
    ArenaVector<ir::Expression *> ctorArguments(checker->Allocator()->Adapter());

    GenerateNewStatements(ctx, objExpr, ss, newStmts, nestedBlckExprs, ctorArguments);

    auto *loweringResult = parser->CreateFormattedExpression(ss.str(), newStmts);
    ES2PANDA_ASSERT(loweringResult != nullptr);
    SetInstanceArguments(loweringResult->AsBlockExpression()->Statements(), ctorArguments);

    loweringResult->SetParent(objExpr->Parent());

    MaybeAllowConstAssign(objExpr->TsType(), loweringResult->AsBlockExpression()->Statements());

    auto scopeCtx = varbinder::LexicalScope<varbinder::Scope>::Enter(varbinder, NearestScope(objExpr));
    InitScopesPhaseETS::RunExternalNode(loweringResult, varbinder);

    // Restoring nested block expressions
    RestoreNestedBlockExpression(loweringResult->AsBlockExpression()->Statements(), nestedBlckExprs,
                                 loweringResult->Scope());

    varbinder->ResolveReferencesForScope(loweringResult, NearestScope(loweringResult));

    AllowRequiredTypeInstantiation(loweringResult);

    checker::SavedCheckerContext scc {checker, checker::CheckerStatus::IGNORE_VISIBILITY};
    loweringResult->Check(checker);

    return loweringResult;
}

bool ObjectLiteralLowering::PerformForProgram(parser::Program *program)
{
    program->Ast()->TransformChildrenRecursively(
        // CC-OFFNXT(G.FMT.14-CPP) project code style
        [ctx = Context()](ir::AstNode *ast) -> ir::AstNode * {
            // Skip processing invalid and dynamic objects
            if (ast->IsObjectExpression()) {
                auto *exprType = ast->AsObjectExpression()->TsType();
                if (exprType == nullptr) {
                    return ast;
                }
                if (exprType->IsETSObjectType()) {
                    return HandleObjectLiteralLowering(ctx, ast->AsObjectExpression());
                }
            }
            return ast;
        },
        Name());

    return true;
}

bool ObjectLiteralLowering::PostconditionForProgram(const parser::Program *program)
{
    // In all object literal contexts (except dynamic) a substitution should take place
    return !program->Ast()->IsAnyChild([](const ir::AstNode *ast) -> bool {
        return ast->IsObjectExpression() && ast->AsObjectExpression()->TsType()->IsETSObjectType();
    });
}

}  // namespace ark::es2panda::compiler

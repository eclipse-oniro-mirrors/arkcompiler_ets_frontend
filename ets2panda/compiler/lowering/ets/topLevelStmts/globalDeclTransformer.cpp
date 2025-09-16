/*
 * Copyright (c) 2023 - 2025 Huawei Device Co., Ltd.
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

#include "compiler/lowering/ets/topLevelStmts/globalDeclTransformer.h"
#include "compiler/lowering/util.h"

namespace ark::es2panda::compiler {

void GlobalDeclTransformer::FilterDeclarations(ArenaVector<ir::Statement *> &stmts)
{
    const auto isDeclCb = [&types = typeDecl_](const ir::AstNode *node) {
        return types.count(node->Type()) == 0U || (node->IsExportNamedDeclaration());
    };
    stmts.erase(std::remove_if(stmts.begin(), stmts.end(), isDeclCb), stmts.end());
}

GlobalDeclTransformer::ResultT GlobalDeclTransformer::TransformStatements(const ArenaVector<ir::Statement *> &stmts)
{
    result_.classProperties.clear();
    result_.immediateInit.clear();
    result_.initializerBlocks.clear();
    for (auto stmt : stmts) {
        stmt->Accept(this);
    }
    return std::move(result_);
}

void GlobalDeclTransformer::VisitExpressionStatement(ir::ExpressionStatement *exprStmt)
{
    if (exprStmt->GetExpression()->IsCallExpression()) {
        auto *callExpr = exprStmt->GetExpression()->AsCallExpression();
        if (callExpr->Callee()->IsIdentifier() &&
            callExpr->Callee()->AsIdentifier()->Name() == compiler::Signatures::INIT_MODULE_METHOD) {
            return;  // skip initModule call
        }
    }
    result_.immediateInit.emplace_back(exprStmt);
}

void GlobalDeclTransformer::VisitOverloadDeclaration(ir::OverloadDeclaration *overloadDeclaration)
{
    result_.classProperties.emplace_back(overloadDeclaration);
}

void GlobalDeclTransformer::VisitFunctionDeclaration(ir::FunctionDeclaration *funcDecl)
{
    auto *funcExpr = util::NodeAllocator::ForceSetParent<ir::FunctionExpression>(allocator_, funcDecl->Function());
    ES2PANDA_ASSERT(funcExpr != nullptr);
    funcDecl->Function()->SetStart(funcDecl->Function()->Id()->Start());
    funcExpr->SetRange(funcDecl->Function()->Range());
    ir::MethodDefinitionKind methodKind;
    if (funcDecl->Function()->HasReceiver()) {
        if (funcDecl->Function()->IsGetter()) {
            methodKind = ir::MethodDefinitionKind::EXTENSION_GET;
        } else if (funcDecl->Function()->IsSetter()) {
            methodKind = ir::MethodDefinitionKind::EXTENSION_SET;
        } else {
            methodKind = ir::MethodDefinitionKind::EXTENSION_METHOD;
        }
    } else {
        methodKind = ir::MethodDefinitionKind::METHOD;
    }

    auto *method = util::NodeAllocator::ForceSetParent<ir::MethodDefinition>(
        allocator_, methodKind, funcDecl->Function()->Id()->Clone(allocator_, nullptr), funcExpr,
        funcDecl->Function()->Modifiers(), allocator_, false);
    ES2PANDA_ASSERT(method != nullptr && method->Function() != nullptr);
    method->SetRange(funcDecl->Range());
    method->Function()->SetAnnotations(funcDecl->Annotations());

    if (funcDecl->Function()->HasExportAlias()) {
        method->AddAstNodeFlags(ir::AstNodeFlags::HAS_EXPORT_ALIAS);
    }

    result_.classProperties.emplace_back(method);
}

void GlobalDeclTransformer::VisitVariableDeclaration(ir::VariableDeclaration *varDecl)
{
    for (auto declarator : varDecl->Declarators()) {
        auto id = declarator->Id()->AsIdentifier();
        auto typeAnn = id->TypeAnnotation();
        id->SetTsTypeAnnotation(nullptr);
        auto modifiers = varDecl->Modifiers() | declarator->Modifiers();
        auto *field = util::NodeAllocator::ForceSetParent<ir::ClassProperty>(
            allocator_, id->Clone(allocator_, nullptr), declarator->Init(), typeAnn, modifiers, allocator_, false);
        ES2PANDA_ASSERT(field != nullptr);
        if (declarator->Init() != nullptr) {
            field->SetIsImmediateInit();
        } else if ((modifiers & ir::ModifierFlags::CONST) != 0 &&
                   currentModule_->AsETSModule()->Program()->IsPackage()) {
            field->SetNeedInitInStaticBlock();
        }
        field->SetRange(declarator->Range());

        if (!varDecl->Annotations().empty()) {
            ArenaVector<ir::AnnotationUsage *> propAnnotations(allocator_->Adapter());
            for (auto *annotationUsage : varDecl->Annotations()) {
                ES2PANDA_ASSERT(annotationUsage != nullptr);
                auto annotationUsageClone = annotationUsage->Clone(allocator_, field);
                ES2PANDA_ASSERT(annotationUsageClone != nullptr);
                propAnnotations.push_back(annotationUsageClone->AsAnnotationUsage());
            }
            field->SetAnnotations(std::move(propAnnotations));
        }

        if (varDecl->HasExportAlias() || declarator->HasExportAlias()) {
            field->AddAstNodeFlags(ir::AstNodeFlags::HAS_EXPORT_ALIAS);
        }

        result_.classProperties.emplace_back(field);
        if (auto stmt = InitTopLevelProperty(field); stmt != nullptr) {
            result_.immediateInit.emplace_back(stmt);
        }
    }
}

static bool IsFinalBlockOfTryStatement(ir::AstNode const *node)
{
    if (!node->IsBlockStatement() || node->Parent() == nullptr) {
        return false;
    }

    auto parent = node->Parent();
    return parent->IsTryStatement() && (parent->AsTryStatement()->FinallyBlock() == node);
}

// Note: Extract the expressions from ClassStaticBlock to Initializer block.
void GlobalDeclTransformer::VisitClassStaticBlock(ir::ClassStaticBlock *classStaticBlock)
{
    auto containUnhandledThrow = [&](ir::AstNode *ast) {
        if (!ast->IsThrowStatement()) {
            return;
        }

        auto const *parent = ast->Parent();
        while (parent != nullptr) {
            if (parent->IsTryStatement() && !parent->AsTryStatement()->CatchClauses().empty()) {
                return;
            }

            if (parent->IsCatchClause() || IsFinalBlockOfTryStatement(parent)) {
                break;
            }
            parent = parent->Parent();
        }
        parser_->LogError(diagnostic::UNHANDLED_THROW_IN_INITIALIZER, {}, ast->Start());
    };

    auto *staticBlock = classStaticBlock->Function();
    ES2PANDA_ASSERT((staticBlock->Flags() & ir::ScriptFunctionFlags::STATIC_BLOCK) != 0);
    classStaticBlock->IterateRecursivelyPostorder(containUnhandledThrow);
    auto &initStatements = staticBlock->Body()->AsBlockStatement()->Statements();
    ArenaVector<ir::Statement *> initializerBlock(allocator_->Adapter());
    initializerBlock.insert(initializerBlock.begin(), initStatements.begin(), initStatements.end());
    result_.initializerBlocks.emplace_back(std::move(initializerBlock));
    ++initializerBlockCount_;
}

ir::Identifier *GlobalDeclTransformer::RefIdent(const util::StringView &name)
{
    auto *const callee = util::NodeAllocator::Alloc<ir::Identifier>(allocator_, name, allocator_);
    return callee;
}

ir::ExpressionStatement *GlobalDeclTransformer::CreateAssignmentStatement(ir::ClassProperty *classProperty,
                                                                          ir::Expression *initializer)
{
    auto const ident = RefIdent(classProperty->Id()->Name());
    ES2PANDA_ASSERT(ident != nullptr);
    ident->SetRange(classProperty->Id()->Range());

    initializer->SetParent(nullptr);
    auto *assignmentExpression = util::NodeAllocator::Alloc<ir::AssignmentExpression>(
        allocator_, ident, initializer, lexer::TokenType::PUNCTUATOR_SUBSTITUTION);
    ES2PANDA_ASSERT(assignmentExpression != nullptr);
    assignmentExpression->SetRange({ident->Start(), initializer->End()});
    assignmentExpression->SetTsType(initializer->TsType());

    auto expressionStatement = util::NodeAllocator::Alloc<ir::ExpressionStatement>(allocator_, assignmentExpression);
    ES2PANDA_ASSERT(expressionStatement != nullptr);
    expressionStatement->SetRange(classProperty->Range());
    assignmentExpression->SetIgnoreConstAssign();

    return expressionStatement;
}

ir::ExpressionStatement *GlobalDeclTransformer::InitTopLevelProperty(ir::ClassProperty *classProperty)
{
    ES2PANDA_ASSERT(classProperty->Id() != nullptr);
    if (classProperty->Value() == nullptr) {
        classProperty->SetStart(classProperty->Id()->Start());
        return nullptr;
    }

    ir::Expression *initializer = nullptr;
    if (classProperty->TypeAnnotation() != nullptr && !classProperty->IsConst()) {
        initializer = classProperty->Value();
        classProperty->SetEnd(classProperty->Value()->Start());
        classProperty->SetValue(nullptr);
        return CreateAssignmentStatement(classProperty, initializer);
    }

    // Code will be ignored, but checker is going to deduce the type.
    initializer = classProperty->Value()->Clone(allocator_, classProperty)->AsExpression();
    return CreateAssignmentStatement(classProperty, initializer);
}

void GlobalDeclTransformer::HandleNode(ir::AstNode *node)
{
    ES2PANDA_ASSERT(node->IsStatement());
    if (typeDecl_.count(node->Type()) == 0U) {
        ES2PANDA_ASSERT(!propertiesDecl_.count(node->Type()));
        result_.immediateInit.emplace_back(node->AsStatement());
    }
}

}  // namespace ark::es2panda::compiler

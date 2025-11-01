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

#include <utility>
#include "checker/ETSchecker.h"

#include "compiler/lowering/util.h"
#include "varbinder/declaration.h"
#include "varbinder/varbinder.h"
#include "varbinder/ETSBinder.h"
#include "compiler/lowering/scopesInit/scopesInitPhase.h"
#include "ir/base/classProperty.h"
#include "ir/base/classStaticBlock.h"
#include "ir/base/methodDefinition.h"
#include "ir/base/scriptFunction.h"
#include "ir/expressions/assignmentExpression.h"
#include "ir/expressions/callExpression.h"
#include "ir/expressions/functionExpression.h"
#include "ir/expressions/identifier.h"
#include "ir/expressions/thisExpression.h"
#include "ir/expressions/memberExpression.h"
#include "ir/ets/etsPrimitiveType.h"
#include "ir/ts/tsAsExpression.h"
#include "ir/statements/blockStatement.h"
#include "ir/statements/classDeclaration.h"
#include "ir/statements/expressionStatement.h"
#include "ir/statements/returnStatement.h"
#include "parser/program/program.h"
#include "util/helpers.h"
#include "util/language.h"
#include "generated/signatures.h"
#include "ir/ets/etsParameterExpression.h"

namespace ark::es2panda::checker {

ir::ETSParameterExpression *ETSChecker::AddParam(util::StringView name, ir::TypeNode *type)
{
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto *paramIdent = ProgramAllocNode<ir::Identifier>(name, ProgramAllocator());
    if (type != nullptr) {
        paramIdent->SetTsTypeAnnotation(type);
        type->SetParent(paramIdent);
    }
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    return ProgramAllocNode<ir::ETSParameterExpression>(paramIdent, false, ProgramAllocator());
}

std::pair<ir::ScriptFunction *, ir::Identifier *> ETSChecker::CreateStaticScriptFunction(
    ClassInitializerBuilder const &builder)
{
    ArenaVector<ir::Statement *> statements(ProgramAllocator()->Adapter());
    ArenaVector<ir::Expression *> params(ProgramAllocator()->Adapter());

    ir::ScriptFunction *func;
    ir::Identifier *id;

    builder(&statements, nullptr);
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto *body = ProgramAllocNode<ir::BlockStatement>(ProgramAllocator(), std::move(statements));
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    id = ProgramAllocNode<ir::Identifier>(compiler::Signatures::CCTOR, ProgramAllocator());
    auto signature = ir::FunctionSignature(nullptr, std::move(params), nullptr);
    // clang-format off
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    func = ProgramAllocNode<ir::ScriptFunction>(
        ProgramAllocator(), ir::ScriptFunction::ScriptFunctionData {
                        body,
                        std::move(signature),
                        ir::ScriptFunctionFlags::STATIC_BLOCK | ir::ScriptFunctionFlags::EXPRESSION,
                        ir::ModifierFlags::STATIC,
                     });
    // clang-format on
    ES2PANDA_ASSERT(func != nullptr);
    func->SetIdent(id);

    return std::make_pair(func, id);
}

std::pair<ir::ScriptFunction *, ir::Identifier *> ETSChecker::CreateScriptFunction(
    ClassInitializerBuilder const &builder)
{
    ArenaVector<ir::Statement *> statements(ProgramAllocator()->Adapter());
    ArenaVector<ir::Expression *> params(ProgramAllocator()->Adapter());

    ir::ScriptFunction *func;
    ir::Identifier *id;

    builder(&statements, &params);
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto *body = ProgramAllocNode<ir::BlockStatement>(ProgramAllocator(), std::move(statements));
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    id = ProgramAllocNode<ir::Identifier>(compiler::Signatures::CTOR, ProgramAllocator());
    auto funcSignature = ir::FunctionSignature(nullptr, std::move(params), nullptr);
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    func = ProgramAllocNode<ir::ScriptFunction>(
        ProgramAllocator(), ir::ScriptFunction::ScriptFunctionData {body, std::move(funcSignature),
                                                                    ir::ScriptFunctionFlags::CONSTRUCTOR |
                                                                        ir::ScriptFunctionFlags::EXPRESSION,
                                                                    ir::ModifierFlags::PUBLIC});
    ES2PANDA_ASSERT(func != nullptr);
    func->SetIdent(id);

    return std::make_pair(func, id);
}

ir::ClassStaticBlock *ETSChecker::CreateClassStaticInitializer(const ClassInitializerBuilder &builder,
                                                               [[maybe_unused]] ETSObjectType *type)
{
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto [func, id] = CreateStaticScriptFunction(builder);

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto *funcExpr = ProgramAllocNode<ir::FunctionExpression>(func);

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto *staticBlock = ProgramAllocNode<ir::ClassStaticBlock>(funcExpr, ProgramAllocator());
    ES2PANDA_ASSERT(staticBlock != nullptr);
    staticBlock->AddModifier(ir::ModifierFlags::STATIC);

    return staticBlock;
}

ir::MethodDefinition *ETSChecker::CreateClassInstanceInitializer(const ClassInitializerBuilder &builder,
                                                                 [[maybe_unused]] ETSObjectType *type)
{
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto [func, id] = CreateScriptFunction(builder);

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto *funcExpr = ProgramAllocNode<ir::FunctionExpression>(func);

    auto *ctor =
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        ProgramAllocNode<ir::MethodDefinition>(ir::MethodDefinitionKind::CONSTRUCTOR,
                                               // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
                                               id->Clone(ProgramAllocator(), nullptr), funcExpr,
                                               ir::ModifierFlags::NONE, ProgramAllocator(), false);
    return ctor;
}

ir::ClassDeclaration *ETSChecker::BuildClass(util::StringView name, const ClassBuilder &builder)
{
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto *classId = ProgramAllocNode<ir::Identifier>(name, ProgramAllocator());

    auto *classDef =
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        ProgramAllocNode<ir::ClassDefinition>(ProgramAllocator(), classId, ir::ClassDefinitionModifiers::CLASS_DECL,
                                              ir::ModifierFlags::NONE, Language(Language::Id::ETS));

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto *classDecl = ProgramAllocNode<ir::ClassDeclaration>(classDef, ProgramAllocator());

    auto *const varBinder = VarBinder()->AsETSBinder();
    auto *const program = varBinder->Program();

    program->Ast()->AddStatement(classDecl);
    classDecl->SetParent(program->Ast());

    bool isExternal = program != varBinder->GetGlobalRecordTable()->Program();
    auto recordTable = isExternal ? varBinder->GetExternalRecordTable().at(program) : varBinder->GetGlobalRecordTable();
    varbinder::BoundContext boundCtx(recordTable, classDef);

    ArenaVector<ir::AstNode *> classBody(ProgramAllocator()->Adapter());

    builder(classBody);

    classDef->AddProperties(std::move(classBody));

    compiler::InitScopesPhaseETS::RunExternalNode(classDecl, varBinder);
    varBinder->ResolveReference(classDecl);

    classDecl->Check(this);

    return classDecl;
}

ir::MethodDefinition *ETSChecker::CreateClassMethod(const std::string_view name, ir::ScriptFunctionFlags funcFlags,
                                                    ir::ModifierFlags modifierFlags, const MethodBuilder &builder)
{
    ArenaVector<ir::Expression *> params(ProgramAllocator()->Adapter());
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto *id = ProgramAllocNode<ir::Identifier>(name, ProgramAllocator());

    ArenaVector<ir::Statement *> statements(ProgramAllocator()->Adapter());
    Type *returnType = nullptr;

    builder(&statements, &params, &returnType);

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto *body = ProgramAllocNode<ir::BlockStatement>(ProgramAllocator(), std::move(statements));
    auto funcSignature = ir::FunctionSignature(
        nullptr, std::move(params),
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        returnType == nullptr ? nullptr : ProgramAllocNode<ir::OpaqueTypeNode>(returnType, ProgramAllocator()));
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto *func = ProgramAllocNode<ir::ScriptFunction>(
        ProgramAllocator(),
        ir::ScriptFunction::ScriptFunctionData {body, std::move(funcSignature), funcFlags, modifierFlags});

    func->SetIdent(id);

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto *funcExpr = ProgramAllocNode<ir::FunctionExpression>(func);
    auto *method =
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        ProgramAllocNode<ir::MethodDefinition>(ir::MethodDefinitionKind::METHOD,
                                               // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
                                               func->Id()->Clone(ProgramAllocator(), nullptr), funcExpr, modifierFlags,
                                               ProgramAllocator(), false);

    return method;
}

}  // namespace ark::es2panda::checker

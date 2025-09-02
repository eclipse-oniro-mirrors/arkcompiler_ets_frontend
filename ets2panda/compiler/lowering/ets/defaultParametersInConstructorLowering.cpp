/**
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

#include "defaultParametersInConstructorLowering.h"
#include "ir/expression.h"
#include "ir/expressions/literals/undefinedLiteral.h"
#include "ir/ets/etsUnionType.h"

namespace ark::es2panda::compiler {

// #23080 code was moved to lowering for no reason
static bool HasDefaultParameters(const ir::ScriptFunction *function, util::DiagnosticEngine &diagnosticEngine)
{
    bool hasDefaultParameter = false;
    bool hasRestParameter = false;

    for (auto *const it : function->Params()) {
        if (it->IsBrokenExpression()) {
            continue;
        }

        auto const *const param = it->AsETSParameterExpression();

        if (param->IsRestParameter()) {
            hasRestParameter = true;
            continue;
        }

        if (hasRestParameter) {
            // NOTE(pronai): This seems to be covered in parser. As the comment above says, it's unclear why the
            // lowering is needed.
            ES2PANDA_UNREACHABLE_POS(param->Start());
            diagnosticEngine.LogDiagnostic(diagnostic::REST_PARAM_NOT_LAST, util::DiagnosticMessageParams {},
                                           param->Start());
        }

        if (param->IsOptional()) {
            hasDefaultParameter = true;
            continue;
        }

        if (hasDefaultParameter) {
            util::DiagnosticMessageParams diagnosticParams = {};
            diagnosticEngine.LogDiagnostic(diagnostic::REQUIRED_PARAM_AFTER_DEFAULT, std::move(diagnosticParams),
                                           param->Start());
        }
    }

    if (hasDefaultParameter && hasRestParameter) {
        util::DiagnosticMessageParams diagnosticParams = {};
        diagnosticEngine.LogDiagnostic(diagnostic::REST_AND_DEFAULT_SAME_TIME, std::move(diagnosticParams),
                                       function->Start());
    }

    return hasDefaultParameter;
}

static ir::TSTypeParameterDeclaration *CreateParameterDeclaraion(ir::MethodDefinition *method, public_lib::Context *ctx)
{
    auto const allocator = ctx->allocator;
    ES2PANDA_ASSERT(method->Function());
    if (method->Function()->TypeParams() == nullptr || method->Function()->TypeParams()->Params().empty()) {
        return nullptr;
    }

    ArenaVector<ir::TSTypeParameter *> typeParams(allocator->Adapter());

    auto parentParams = method->Function()->TypeParams()->Params();
    std::for_each(parentParams.begin(), parentParams.end(), [&typeParams, allocator](ir::TSTypeParameter *par) {
        ir::Identifier *ident = par->Name()->Clone(allocator, nullptr)->AsIdentifier();
        auto *constraint =
            par->Constraint() != nullptr ? par->Constraint()->Clone(allocator, nullptr)->AsTypeNode() : nullptr;
        auto *defaultType =
            par->DefaultType() != nullptr ? par->DefaultType()->Clone(allocator, nullptr)->AsTypeNode() : nullptr;
        auto *typeParam = util::NodeAllocator::ForceSetParent<ir::TSTypeParameter>(allocator, ident, constraint,
                                                                                   defaultType, allocator);
        typeParams.push_back(typeParam);
    });
    return util::NodeAllocator::ForceSetParent<ir::TSTypeParameterDeclaration>(allocator, std::move(typeParams),
                                                                               typeParams.size());
}

static ir::FunctionSignature CreateFunctionSignature(ir::MethodDefinition *method,
                                                     ArenaVector<ir::Expression *> funcParam, public_lib::Context *ctx)
{
    auto const allocator = ctx->allocator;

    ir::TSTypeParameterDeclaration *typeParamDecl = CreateParameterDeclaraion(method, ctx);
    ES2PANDA_ASSERT(method->Function());
    auto *returnTypeAnnotation =
        method->Function()->ReturnTypeAnnotation() != nullptr
            ? method->Function()->ReturnTypeAnnotation()->Clone(allocator, nullptr)->AsTypeNode()
            : nullptr;

    return ir::FunctionSignature(typeParamDecl, std::move(funcParam), returnTypeAnnotation);
}

static ir::TSTypeParameterInstantiation *CreateTypeParameterInstantiation(ir::MethodDefinition *method,
                                                                          public_lib::Context *ctx)
{
    auto const allocator = ctx->allocator;

    ES2PANDA_ASSERT(method->Function());
    if (method->Function()->TypeParams() == nullptr || method->Function()->TypeParams()->Params().empty()) {
        return nullptr;
    }
    ArenaVector<ir::TypeNode *> selfParams(allocator->Adapter());
    ir::ETSTypeReferencePart *referencePart = nullptr;

    for (const auto &param : method->Function()->TypeParams()->Params()) {
        auto *identRef = util::NodeAllocator::ForceSetParent<ir::Identifier>(
            allocator, param->AsTSTypeParameter()->Name()->Name(), allocator);

        referencePart = util::NodeAllocator::ForceSetParent<ir::ETSTypeReferencePart>(allocator, identRef, nullptr,
                                                                                      nullptr, allocator);

        auto *typeReference =
            util::NodeAllocator::ForceSetParent<ir::ETSTypeReference>(allocator, referencePart, allocator);

        selfParams.push_back(typeReference);
    }

    return util::NodeAllocator::ForceSetParent<ir::TSTypeParameterInstantiation>(allocator, std::move(selfParams));
}

static ir::BlockStatement *CreateFunctionBody(ir::MethodDefinition *method, public_lib::Context *ctx,
                                              ArenaVector<ir::Expression *> funcCallArgs)
{
    auto const allocator = ctx->allocator;
    ArenaVector<ir::Statement *> funcStatements(allocator->Adapter());

    ES2PANDA_ASSERT(method->Id() != nullptr);
    auto *const callee =
        util::NodeAllocator::ForceSetParent<ir::Identifier>(allocator, method->Id()->Name(), allocator);

    ir::Expression *accessor = util::NodeAllocator::ForceSetParent<ir::ThisExpression>(allocator);
    auto *paramInst = CreateTypeParameterInstantiation(method, ctx);
    auto *callExpression = util::NodeAllocator::ForceSetParent<ir::CallExpression>(
        allocator, accessor != nullptr ? accessor : callee, std::move(funcCallArgs), paramInst, false, false);
    ES2PANDA_ASSERT(callExpression != nullptr);
    callExpression->SetRange(method->Range());  // NOTE: Used to locate the original node when an error occurs
    funcStatements.push_back(util::NodeAllocator::ForceSetParent<ir::ExpressionStatement>(allocator, callExpression));

    return util::NodeAllocator::ForceSetParent<ir::BlockStatement>(allocator, allocator, std::move(funcStatements));
}

static ir::FunctionExpression *CreateFunctionExpression(ir::MethodDefinition *method, public_lib::Context *ctx,
                                                        ArenaVector<ir::Expression *> funcDefinitionArgs,
                                                        ArenaVector<ir::Expression *> funcCallArgs)
{
    lexer::SourcePosition startLoc(method->Start());
    lexer::SourcePosition endLoc = startLoc;
    ir::FunctionSignature signature = CreateFunctionSignature(method, std::move(funcDefinitionArgs), ctx);

    auto const allocator = ctx->allocator;
    ir::Identifier *id = nullptr;

    ir::BlockStatement *body = nullptr;
    if (!(method->IsNative() || method->IsDeclare() || method->IsAbstract())) {
        body = CreateFunctionBody(method, ctx, std::move(funcCallArgs));
    }
    auto *funcNode = util::NodeAllocator::ForceSetParent<ir::ScriptFunction>(
        allocator, allocator,
        ir::ScriptFunction::ScriptFunctionData {
            body, std::move(signature), method->Function()->Flags(), {}, method->Function()->Language()});
    ES2PANDA_ASSERT(method->Function());
    funcNode->AddModifier(method->Function()->Modifiers());
    funcNode->SetRange({startLoc, endLoc});

    id = method->Id()->Clone(allocator, nullptr)->AsIdentifier();
    funcNode->SetIdent(id);
    return util::NodeAllocator::ForceSetParent<ir::FunctionExpression>(allocator, funcNode);
}

static void CreateFunctionOverload(ir::MethodDefinition *method, ArenaVector<ir::Expression *> &&funcCallArgs,
                                   ArenaVector<ir::Expression *> &&funcDefinitionArgs, public_lib::Context *ctx)
{
    auto const allocator = ctx->allocator;
    auto *funcExpression =
        CreateFunctionExpression(method, ctx, std::move(funcDefinitionArgs), std::move(funcCallArgs));
    ES2PANDA_ASSERT(funcExpression != nullptr);
    auto *ident = funcExpression->Function()->Id()->Clone(allocator, nullptr);
    auto *const overloadMethod = util::NodeAllocator::ForceSetParent<ir::MethodDefinition>(
        allocator, method->Kind(), ident, funcExpression, method->Modifiers(), allocator, false);

    ES2PANDA_ASSERT(overloadMethod != nullptr && overloadMethod->Function() != nullptr);
    overloadMethod->Function()->AddFlag(ir::ScriptFunctionFlags::OVERLOAD | ir::ScriptFunctionFlags::SYNTHETIC);
    overloadMethod->SetRange(funcExpression->Range());

    if (!method->IsDeclare() && method->Parent()->IsTSInterfaceBody()) {
        overloadMethod->Function()->Body()->AsBlockStatement()->ClearStatements();
    }

    method->AddOverload(overloadMethod);
    overloadMethod->SetStart(method->Start());  // NOTE: Used to locate the original node when an error occurs
    overloadMethod->SetParent(method);          // NOTE(aleksisch): It's incorrect and don't exist in class body
}

static void ExpandOptionalParameterAnnotationsToUnions(public_lib::Context *ctx, ir::ScriptFunction *function)
{
    ES2PANDA_ASSERT(function);
    auto allocator = ctx->allocator;

    for (auto p : function->Params()) {
        auto param = p->AsETSParameterExpression();
        if (param->IsOptional() && param->Initializer() == nullptr) {
            ArenaVector<ir::TypeNode *> typeNodes(allocator->Adapter());
            if (param->TypeAnnotation() != nullptr) {
                typeNodes.emplace_back(param->TypeAnnotation());
            }
            typeNodes.emplace_back(allocator->New<ir::ETSUndefinedType>(allocator));
            param->SetTypeAnnotation(
                util::NodeAllocator::ForceSetParent<ir::ETSUnionType>(allocator, std::move(typeNodes), allocator));
            param->TypeAnnotation()->SetParent(param->Ident());
        }
    }
}

static void ClearOptionalParameters(public_lib::Context *ctx, ir::ScriptFunction *function)
{
    auto allocator = ctx->allocator;

    auto const &params = function->Params();
    bool hasOptional = std::any_of(params.cbegin(), params.cend(),
                                   [](ir::Expression *p) { return p->AsETSParameterExpression()->IsOptional(); });
    if (!hasOptional) {
        return;
    }

    auto &paramsToChange = function->ParamsForUpdate();
    for (auto *&param : paramsToChange) {
        auto oldParam = param->AsETSParameterExpression();
        if (oldParam->IsOptional()) {
            param = util::NodeAllocator::ForceSetParent<ir::ETSParameterExpression>(allocator, oldParam->Ident(), false,
                                                                                    allocator);
            ES2PANDA_ASSERT(param);
            param->SetParent(function);
        }
        ES2PANDA_ASSERT(!param->AsETSParameterExpression()->IsOptional());
    }
}

static void ProcessGlobalFunctionDefinition(ir::MethodDefinition *method, public_lib::Context *ctx)
{
    ES2PANDA_ASSERT(method->Function() != nullptr);
    auto allocator = ctx->allocator;
    ExpandOptionalParameterAnnotationsToUnions(ctx, method->Function());
    auto const &params = method->Function()->Params();

    for (size_t paramsToCut = 1;; paramsToCut++) {
        if (paramsToCut > params.size() ||
            !params[params.size() - paramsToCut]->AsETSParameterExpression()->IsOptional()) {
            break;
        }

        ArenaVector<ir::Expression *> functionParams(allocator->Adapter());
        ArenaVector<ir::Expression *> callArgs(allocator->Adapter());
        functionParams.reserve(params.size() - paramsToCut);
        callArgs.reserve(params.size());

        for (size_t i = 0; i < params.size() - paramsToCut; ++i) {
            auto param = params[i]->AsETSParameterExpression();
            auto *cloneRef = param->Ident()->CloneReference(allocator, nullptr);
            auto *clone = param->Ident()->Clone(allocator, nullptr);
            ES2PANDA_ASSERT(cloneRef != nullptr && clone != nullptr);
            callArgs.push_back(cloneRef->AsIdentifier());
            functionParams.push_back(
                allocator->New<ir::ETSParameterExpression>(clone->AsIdentifier(), false, allocator));
        }

        for (size_t i = params.size() - paramsToCut; i < params.size(); ++i) {
            auto param = params[i]->AsETSParameterExpression();
            ir::Expression *init;
            if (param->Initializer() == nullptr) {
                ES2PANDA_ASSERT(param->IsOptional());
                init = allocator->New<ir::UndefinedLiteral>();
            } else {
                init = param->Initializer()->Clone(allocator, nullptr)->AsExpression();
            }
            callArgs.push_back(init);
        }
        CreateFunctionOverload(method, std::move(callArgs), std::move(functionParams), ctx);
    }

    ClearOptionalParameters(ctx, method->Function());
}

bool DefaultParametersInConstructorLowering::PerformForModule(public_lib::Context *ctx, parser::Program *program)
{
    util::DiagnosticEngine *logger = ctx->diagnosticEngine;
    std::vector<ir::MethodDefinition *> foundNodes {};

    program->Ast()->IterateRecursively([&foundNodes, logger](ir::AstNode *ast) {
        if (ast->IsMethodDefinition() && ast->AsMethodDefinition()->IsConstructor()) {
            if (HasDefaultParameters(ast->AsMethodDefinition()->Function(), *logger)) {
                // store all nodes (which is function definition with default/optional parameters)
                // to specific list, to process them later, as for now we can't modify AST in the
                // middle of walking through it
                foundNodes.push_back(ast->AsMethodDefinition());
            }
        }
    });

    for (auto &it : foundNodes) {
        ProcessGlobalFunctionDefinition(it, ctx);
    }
    return true;
}

bool DefaultParametersInConstructorLowering::PostconditionForModule([[maybe_unused]] public_lib::Context *ctx,
                                                                    parser::Program const *program)
{
    return !program->Ast()->IsAnyChild([](ir::AstNode const *node) {
        if (!node->IsMethodDefinition() || !node->AsMethodDefinition()->IsConstructor()) {
            return false;
        }
        for (auto *const it : node->AsMethodDefinition()->Function()->Params()) {
            if (it->IsBrokenExpression()) {
                continue;
            }
            auto const *const param = it->AsETSParameterExpression();
            if (param->IsOptional()) {
                return true;
            }
        }
        return false;
    });
}

}  // namespace ark::es2panda::compiler

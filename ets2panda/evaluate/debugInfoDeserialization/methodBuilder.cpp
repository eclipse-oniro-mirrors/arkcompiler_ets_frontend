/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

#include "evaluate/debugInfoDeserialization/methodBuilder.h"
#include "checker/ETSchecker.h"
#include "libarkfile/file-inl.h"
#include "libarkfile/method_data_accessor-inl.h"
#include "libarkfile/proto_data_accessor-inl.h"
#include "evaluate/helpers.h"

namespace ark::es2panda::evaluate {

namespace {

std::string GetFieldName(size_t fieldIdx)
{
    std::stringstream sstream;
    sstream << "field" << fieldIdx;
    return sstream.str();
}

ArenaVector<ir::TypeNode *> GetFunctionParameters(checker::ETSChecker *checker, panda_file::MethodDataAccessor &mda)
{
    const auto &pf = mda.GetPandaFile();
    ArenaVector<ir::TypeNode *> parameters(checker->Allocator()->Adapter());
    mda.EnumerateTypesInProto(
        [checker, &parameters, &pf = std::as_const(pf)](panda_file::Type type, panda_file::File::EntityId classId) {
            auto *typeNode = helpers::PandaTypeToTypeNode(pf, type, classId, checker);
            ES2PANDA_ASSERT(typeNode);
            parameters.push_back(typeNode);
        },
        true);  // true -- skip `this` parameter

    return parameters;
}

ir::ReturnStatement *CreateTypedReturnStatement(checker::ETSChecker *checker, ir::TypeNode *type)
{
    ES2PANDA_ASSERT(type);

    if (type->IsETSPrimitiveType() && type->AsETSPrimitiveType()->GetPrimitiveType() == ir::PrimitiveType::VOID) {
        return checker->AllocNode<ir::ReturnStatement>();
    }

    // Hack for correct validation. This function call won't be executed in compiled code,
    // as the whole class declaration only mimics the real code loaded into runtime.

    auto *allocator = checker->Allocator();
    auto *apiClass = checker->AllocNode<ir::Identifier>(helpers::DEBUGGER_API_CLASS_NAME, allocator);
    auto *prop =
        checker->AllocNode<ir::Identifier>(helpers::CreateGetterName(panda_file::Type::TypeId::REFERENCE), allocator);
    auto *callee = checker->AllocNode<ir::MemberExpression>(apiClass, prop, ir::MemberExpressionKind::PROPERTY_ACCESS,
                                                            false, false);

    ArenaVector<ir::Expression *> args(1, checker->AllocNode<ir::NumberLiteral>(lexer::Number((int64_t)0)),
                                       allocator->Adapter());
    auto *callExpression = checker->AllocNode<ir::CallExpression>(callee, std::move(args), nullptr, false);

    auto *asExpression = checker->AllocNode<ir::TSAsExpression>(callExpression, type->Clone(allocator, nullptr), false);
    return checker->AllocNode<ir::ReturnStatement>(asExpression);
}

}  // namespace

MethodBuilder::MethodBuilder(checker::ETSChecker *checker, panda_file::MethodDataAccessor &mda,
                             ir::ModifierFlags classModifierFlags)
    : checker_(checker), mda_(mda), params_(checker_->Allocator()->Adapter()), classModifierFlags_(classModifierFlags)
{
    methodName_ = util::UString(mda_.GetFullName(), checker_->Allocator()).View();
    modifierFlags_ = ir::ModifierFlags::EXPORT | helpers::GetModifierFlags(mda_, true);
}

ir::AstNode *MethodBuilder::Build() &&
{
    ArenaVector<ir::Statement *> statements(checker_->Allocator()->Adapter());

    CollectParametersAndReturnType();

    bool isCtor = IsConstructor();
    bool isCctor = IsStaticConstructor();

    if (isCtor) {
        auto *superConstructorCallStatement = CreateSuperConstructorExpressionCall();
        statements.push_back(superConstructorCallStatement);
    }

    auto *retStatement = CreateTypedReturnStatement(checker_, returnType_);
    statements.push_back(retStatement);

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto *id = checker_->AllocNode<ir::Identifier>(methodName_, checker_->Allocator());
    auto *body = CreateBody(std::move(statements));

    if (isCtor) {
        return CreateIrConstructor<false>(id, body);
    }
    if (isCctor) {
        return CreateIrConstructor<true>(id, body);
    }
    return CreateIrMethod(id, body);
}

void MethodBuilder::CollectParametersAndReturnType()
{
    auto parameters = GetFunctionParameters(checker_, mda_);
    auto *allocator = checker_->Allocator();

    // Start from 1, because 0 is return type
    for (size_t idx = 1U; idx < parameters.size(); ++idx) {
        util::UString paramName(GetFieldName(idx), allocator);
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        auto *paramIdent = checker_->AllocNode<ir::Identifier>(paramName.View(), parameters[idx], allocator);
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        auto *param = checker_->AllocNode<ir::ETSParameterExpression>(paramIdent, false, allocator);
        params_.push_back(param);
    }

    returnType_ = parameters[0];
}

ir::BlockStatement *MethodBuilder::CreateBody(ArenaVector<ir::Statement *> statements)
{
    bool isAbstractClass = ((classModifierFlags_ & ir::ModifierFlags::ABSTRACT) != 0);
    bool isAbstractMethod = ((modifierFlags_ & ir::ModifierFlags::ABSTRACT) != 0);
    bool needToCreateBody = !(isAbstractClass && isAbstractMethod);

    ir::BlockStatement *body = nullptr;
    if (needToCreateBody) {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        body = checker_->AllocNode<ir::BlockStatement>(checker_->Allocator(), std::move(statements));
    }

    return body;
}

ir::ExpressionStatement *MethodBuilder::CreateSuperConstructorExpressionCall()
{
    // NOTE: in future it may be necessary to pass non empty call args,
    // but currently frontend do not check args number.
    ArenaVector<ir::Expression *> callArguments(checker_->Allocator()->Adapter());

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto *callee = checker_->AllocNode<ir::SuperExpression>();
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto *superConstructorCall =
        checker_->AllocNode<ir::CallExpression>(callee, std::move(callArguments), nullptr, false);
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    return checker_->AllocNode<ir::ExpressionStatement>(superConstructorCall);
}

template <bool IS_STATIC>
ir::AstNode *MethodBuilder::CreateIrConstructor(ir::Identifier *id, ir::BlockStatement *body)
{
    auto scriptFuncFlags = ir::ScriptFunctionFlags::EXPRESSION |
                           (IS_STATIC ? ir::ScriptFunctionFlags::STATIC_BLOCK : ir::ScriptFunctionFlags::CONSTRUCTOR);

    auto *funcExpr = CreateFunctionExpression(id, body, scriptFuncFlags);
    auto *allocator = checker_->Allocator();

    if constexpr (IS_STATIC) {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        auto *staticBlock = checker_->AllocNode<ir::ClassStaticBlock>(funcExpr, allocator);
        ES2PANDA_ASSERT(staticBlock != nullptr);
        staticBlock->AddModifier(ir::ModifierFlags::STATIC);
        return staticBlock;
    }
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    return checker_->AllocNode<ir::MethodDefinition>(ir::MethodDefinitionKind::CONSTRUCTOR,
                                                     id->Clone(allocator, nullptr), funcExpr, ir::ModifierFlags::NONE,
                                                     allocator, false);
}

ir::MethodDefinition *MethodBuilder::CreateIrMethod(ir::Identifier *id, ir::BlockStatement *body)
{
    auto *allocator = checker_->Allocator();
    auto *funcExpr = CreateFunctionExpression(id, body, ir::ScriptFunctionFlags::METHOD);
    ES2PANDA_ASSERT(funcExpr != nullptr);
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto *method = checker_->AllocNode<ir::MethodDefinition>(ir::MethodDefinitionKind::METHOD,
                                                             funcExpr->Function()->Id()->Clone(allocator, nullptr),
                                                             funcExpr, modifierFlags_, allocator, false);
    return method;
}

ir::FunctionExpression *MethodBuilder::CreateFunctionExpression(ir::Identifier *id, ir::BlockStatement *body,
                                                                ir::ScriptFunctionFlags scriptFuncFlags)
{
    auto funcSignature = ir::FunctionSignature(nullptr, std::move(params_), nullptr);

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto *func = checker_->AllocNode<ir::ScriptFunction>(
        checker_->Allocator(),
        ir::ScriptFunction::ScriptFunctionData {body, std::move(funcSignature), scriptFuncFlags, modifierFlags_});
    ES2PANDA_ASSERT(func != nullptr);

    ES2PANDA_ASSERT(func != nullptr);
    func->SetIdent(id);
    func->SetReturnTypeAnnotation(returnType_);

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    return checker_->AllocNode<ir::FunctionExpression>(func);
}

}  // namespace ark::es2panda::evaluate

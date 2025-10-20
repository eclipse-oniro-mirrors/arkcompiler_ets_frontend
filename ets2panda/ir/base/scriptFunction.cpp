/**
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

#include "scriptFunction.h"

#include "checker/TSchecker.h"
#include "compiler/core/ETSGen.h"
#include "compiler/core/pandagen.h"
#include "ir/astDump.h"
#include "ir/srcDump.h"
#include "mem/arena_allocator.h"
#include "utils/arena_containers.h"

namespace ark::es2panda::ir {

void ScriptFunction::SetBody(AstNode *body)
{
    this->GetOrCreateHistoryNodeAs<ScriptFunction>()->body_ = body;
}

void ScriptFunction::SetSignature(checker::Signature *signature)
{
    this->GetOrCreateHistoryNodeAs<ScriptFunction>()->signature_ = signature;
}

void ScriptFunction::SetScope(varbinder::FunctionScope *scope)
{
    this->GetOrCreateHistoryNodeAs<ScriptFunction>()->scope_ = scope;
}

void ScriptFunction::SetPreferredReturnType(checker::Type *preferredReturnType)
{
    this->GetOrCreateHistoryNodeAs<ScriptFunction>()->preferredReturnType_ = preferredReturnType;
}

void ScriptFunction::EmplaceParams(Expression *params)
{
    auto newNode = this->GetOrCreateHistoryNodeAs<ScriptFunction>();
    newNode->irSignature_.Params().emplace_back(params);
}

void ScriptFunction::SetParams(ArenaVector<Expression *> &&paramsList)
{
    auto &params = this->GetOrCreateHistoryNodeAs<ScriptFunction>()->irSignature_.Params();
    params = std::move(paramsList);

    for (auto *param : params) {
        param->SetParent(this);
    }
}

void ScriptFunction::ClearParams()
{
    auto newNode = this->GetOrCreateHistoryNodeAs<ScriptFunction>();
    newNode->irSignature_.Params().clear();
}

void ScriptFunction::SetValueParams(Expression *params, size_t index)
{
    auto newNode = this->GetOrCreateHistoryNodeAs<ScriptFunction>();
    auto &arenaVector = newNode->irSignature_.Params();
    ES2PANDA_ASSERT(arenaVector.size() > index);
    arenaVector[index] = params;
}

[[nodiscard]] const ArenaVector<Expression *> &ScriptFunction::Params()
{
    auto newNode = this->GetHistoryNodeAs<ScriptFunction>();
    return newNode->irSignature_.Params();
}

[[nodiscard]] ArenaVector<Expression *> &ScriptFunction::ParamsForUpdate()
{
    auto newNode = this->GetOrCreateHistoryNodeAs<ScriptFunction>();
    return newNode->irSignature_.Params();
}

void ScriptFunction::EmplaceReturnStatements(ReturnStatement *returnStatements)
{
    auto newNode = this->GetOrCreateHistoryNodeAs<ScriptFunction>();
    newNode->returnStatements_.emplace_back(returnStatements);
}

void ScriptFunction::ClearReturnStatements()
{
    auto newNode = this->GetOrCreateHistoryNodeAs<ScriptFunction>();
    newNode->returnStatements_.clear();
}

void ScriptFunction::SetValueReturnStatements(ReturnStatement *returnStatements, size_t index)
{
    auto newNode = this->GetOrCreateHistoryNodeAs<ScriptFunction>();
    auto &arenaVector = newNode->returnStatements_;
    ES2PANDA_ASSERT(arenaVector.size() > index);
    arenaVector[index] = returnStatements;
}

[[nodiscard]] const ArenaVector<ReturnStatement *> &ScriptFunction::ReturnStatements()
{
    auto newNode = this->GetHistoryNodeAs<ScriptFunction>();
    return newNode->returnStatements_;
}

[[nodiscard]] ArenaVector<ReturnStatement *> &ScriptFunction::ReturnStatementsForUpdate()
{
    auto newNode = this->GetOrCreateHistoryNodeAs<ScriptFunction>();
    return newNode->returnStatements_;
}

ScriptFunction::ScriptFunction(ArenaAllocator *allocator, ScriptFunctionData &&data)
    : AnnotationAllowed<AstNode>(AstNodeType::SCRIPT_FUNCTION, data.flags, allocator),
      irSignature_(std::move(data.signature)),
      body_(data.body),
      funcFlags_(data.funcFlags),
      lang_(data.lang),
      returnStatements_(allocator->Adapter()),
      asyncPairFunction_(nullptr)
{
    for (auto *param : irSignature_.Params()) {
        param->SetParent(this);
    }

    if (auto *returnType = irSignature_.ReturnType(); returnType != nullptr) {
        returnType->SetParent(this);
    }

    if (auto *typeParams = irSignature_.TypeParams(); typeParams != nullptr) {
        typeParams->SetParent(this);
    }
    InitHistory();
}

ScriptFunction::ScriptFunction(ArenaAllocator *allocator, ScriptFunctionData &&data, AstNodeHistory *history)
    : AnnotationAllowed<AstNode>(AstNodeType::SCRIPT_FUNCTION, data.flags, allocator),
      irSignature_(std::move(data.signature)),
      body_(data.body),
      funcFlags_(data.funcFlags),
      lang_(data.lang),
      returnStatements_(allocator->Adapter()),
      asyncPairFunction_(nullptr)
{
    for (auto *param : irSignature_.Params()) {
        param->SetParent(this);
    }

    if (auto *returnType = irSignature_.ReturnType(); returnType != nullptr) {
        returnType->SetParent(this);
    }

    if (auto *typeParams = irSignature_.TypeParams(); typeParams != nullptr) {
        typeParams->SetParent(this);
    }
    if (history != nullptr) {
        history_ = history;
    } else {
        InitHistory();
    }
}

std::size_t ScriptFunction::FormalParamsLength() const noexcept
{
    std::size_t length = 0U;

    for (const auto *param : Params()) {
        if (param->IsRestElement() || param->IsAssignmentPattern()) {
            break;
        }

        ++length;
    }

    return length;
}

void ScriptFunction::SetIdent(Identifier *id) noexcept
{
    this->GetOrCreateHistoryNodeAs<ScriptFunction>()->id_ = id;
    id->SetParent(this);
}

ScriptFunction *ScriptFunction::Clone(ArenaAllocator *allocator, AstNode *parent)
{
    ArenaVector<ir::Expression *> params {allocator->Adapter()};
    ArenaVector<AnnotationUsage *> annotationUsages {allocator->Adapter()};
    for (auto *param : Params()) {
        params.push_back(param->Clone(allocator, nullptr)->AsExpression());
    }

    auto *clone = util::NodeAllocator::ForceSetParent<ScriptFunction>(
        allocator, allocator,
        ScriptFunctionData {
            Body() != nullptr ? Body()->Clone(allocator, nullptr) : nullptr,
            FunctionSignature {
                TypeParams() != nullptr ? TypeParams()->Clone(allocator, nullptr)->AsTSTypeParameterDeclaration()
                                        : nullptr,
                std::move(params),
                ReturnTypeAnnotation() != nullptr ? ReturnTypeAnnotation()->Clone(allocator, nullptr)->AsTypeNode()
                                                  : nullptr,
                HasReceiver()},
            Flags(), Modifiers(), Language()});

    if (parent != nullptr) {
        clone->SetParent(parent);
    }

    // Clone annotations if any
    if (HasAnnotations()) {
        clone->SetAnnotations(Annotations());
    }

    return clone;
}

void ScriptFunction::TransformChildren(const NodeTransformer &cb, std::string_view const transformationName)
{
    auto const id = Id();
    if (id != nullptr) {
        if (auto *transformedNode = cb(id); id != transformedNode) {
            id->SetTransformedNode(transformationName, transformedNode);
            SetIdent(transformedNode->AsIdentifier());
        }
    }

    GetOrCreateHistoryNode()->AsScriptFunction()->irSignature_.TransformChildren(cb, transformationName);

    auto const &body = Body();
    if (body != nullptr) {
        if (auto *transformedNode = cb(body); body != transformedNode) {
            body->SetTransformedNode(transformationName, transformedNode);
            SetBody(transformedNode);
        }
    }

    TransformAnnotations(cb, transformationName);
}

void ScriptFunction::Iterate(const NodeTraverser &cb) const
{
    auto id = GetHistoryNode()->AsScriptFunction()->id_;
    if (id != nullptr) {
        cb(id);
    }
    GetHistoryNode()->AsScriptFunction()->irSignature_.Iterate(cb);

    auto body = GetHistoryNode()->AsScriptFunction()->body_;
    if (body != nullptr) {
        cb(body);
    }

    IterateAnnotations(cb);
}

void ScriptFunction::SetReturnTypeAnnotation(TypeNode *node) noexcept
{
    auto newNode = GetOrCreateHistoryNode()->AsScriptFunction();
    newNode->irSignature_.SetReturnType(node);
    if (node != nullptr) {
        node->SetParent(this);
    }
}

void ScriptFunction::Dump(ir::AstDumper *dumper) const
{
    dumper->Add({{"type", "ScriptFunction"},
                 {"id", AstDumper::Nullish(Id())},
                 {"generator", IsGenerator()},
                 {"async", IsAsyncFunc()},
                 {"expression", ((Flags() & ir::ScriptFunctionFlags::EXPRESSION) != 0)},
                 {"params", Params()},
                 {"returnType", AstDumper::Optional(ReturnTypeAnnotation())},
                 {"typeParameters", AstDumper::Optional(TypeParams())},
                 {"declare", AstDumper::Optional(IsDeclare())},
                 {"body", AstDumper::Optional(Body())},
                 {"annotations", AstDumper::Optional(Annotations())}});
}

void ScriptFunction::DumpCheckerTypeForDeclGen(ir::SrcDumper *dumper) const
{
    if (!dumper->IsDeclgen()) {
        return;
    }

    if (IsConstructor()) {
        return;
    }

    if (IsSetter()) {
        return;
    }

    if (Signature() == nullptr) {
        return;
    }

    if (Signature()->ReturnType() == nullptr) {
        return;
    }

    auto typeStr = Signature()->ReturnType()->ToString();
    dumper->Add(": ");
    dumper->Add(typeStr);

    dumper->PushTask([dumper, typeStr] { dumper->DumpNode(typeStr); });
}

void ScriptFunction::Dump(ir::SrcDumper *dumper) const
{
    if (TypeParams() != nullptr) {
        dumper->Add("<");
        TypeParams()->Dump(dumper);
        dumper->Add(">");
    }
    dumper->Add("(");
    for (auto param : Params()) {
        param->Dump(dumper);
        if (param != Params().back()) {
            dumper->Add(", ");
        }
    }
    dumper->Add(")");
    if (ReturnTypeAnnotation() != nullptr && !dumper->IsDeclgen()) {
        dumper->Add(": ");
        ReturnTypeAnnotation()->Dump(dumper);
    }
    DumpCheckerTypeForDeclGen(dumper);
    if (dumper->IsDeclgen()) {
        dumper->Add(";");
        dumper->Endl();
        return;
    }
    DumpBody(dumper);
}

void ScriptFunction::DumpBody(ir::SrcDumper *dumper) const
{
    if (!HasBody()) {
        dumper->Endl();
        return;
    }

    if (IsArrow()) {
        dumper->Add(" =>");
    }

    if (body_->IsBlockStatement()) {
        dumper->Add(" {");
        const auto &statements = body_->AsBlockStatement()->Statements();
        if (!statements.empty()) {
            dumper->IncrIndent();
            dumper->Endl();
            body_->Dump(dumper);
            dumper->DecrIndent();
            dumper->Endl();
        }
        dumper->Add("}");
    } else {
        dumper->Add(" ");
        body_->Dump(dumper);
    }

    if (!IsArrow()) {
        dumper->Endl();
    }
}

void ScriptFunction::Compile(compiler::PandaGen *pg) const
{
    pg->GetAstCompiler()->Compile(this);
}
void ScriptFunction::Compile(compiler::ETSGen *etsg) const
{
    etsg->GetAstCompiler()->Compile(this);
}

checker::Type *ScriptFunction::Check(checker::TSChecker *checker)
{
    return checker->GetAnalyzer()->Check(this);
}

checker::VerifiedType ScriptFunction::Check(checker::ETSChecker *checker)
{
    return {this, checker->GetAnalyzer()->Check(this)};
}

ScriptFunction *ScriptFunction::Construct(ArenaAllocator *allocator)
{
    auto adapter = allocator->Adapter();
    return allocator->New<ScriptFunction>(
        allocator,
        ScriptFunctionData {nullptr, FunctionSignature(nullptr, ArenaVector<Expression *>(adapter), nullptr)});
}

void ScriptFunction::CopyTo(AstNode *other) const
{
    auto otherImpl = other->AsScriptFunction();

    otherImpl->id_ = id_;

    otherImpl->irSignature_.CopyFrom(irSignature_);

    otherImpl->body_ = body_;
    otherImpl->scope_ = scope_;
    otherImpl->funcFlags_ = funcFlags_;
    otherImpl->signature_ = signature_;
    otherImpl->preferredReturnType_ = preferredReturnType_;
    otherImpl->lang_ = lang_;
    otherImpl->returnStatements_ = returnStatements_;

    AnnotationAllowed<AstNode>::CopyTo(other);
}

}  // namespace ark::es2panda::ir

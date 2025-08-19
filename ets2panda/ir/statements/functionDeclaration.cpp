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

#include "functionDeclaration.h"

#include "varbinder/variable.h"
#include "compiler/core/ETSGen.h"
#include "checker/TSchecker.h"
#include "ir/astDump.h"
#include "ir/srcDump.h"
#include "compiler/core/pandagen.h"

namespace ark::es2panda::ir {

void FunctionDeclaration::SetFunction(ScriptFunction *func)
{
    this->GetOrCreateHistoryNodeAs<FunctionDeclaration>()->func_ = func;
}

void FunctionDeclaration::TransformChildren(const NodeTransformer &cb, std::string_view transformationName)
{
    TransformAnnotations(cb, transformationName);

    auto const func = Function();
    if (auto *transformedNode = cb(func); func != transformedNode) {
        func->SetTransformedNode(transformationName, transformedNode);
        SetFunction(transformedNode->AsScriptFunction());
    }
}

void FunctionDeclaration::Iterate(const NodeTraverser &cb) const
{
    IterateAnnotations(cb);

    auto func = GetHistoryNode()->AsFunctionDeclaration()->func_;
    cb(func);
}

void FunctionDeclaration::Dump(ir::AstDumper *dumper) const
{
    dumper->Add({{"type", Function()->IsOverload() ? "TSDeclareFunction" : "FunctionDeclaration"},
                 {"annotations", AstDumper::Optional(Annotations())},
                 {"function", Function()}});
}

void FunctionDeclaration::Dump(ir::SrcDumper *dumper) const
{
    DumpAnnotations(dumper);
    auto func = Function();
    if (func->IsNative()) {
        dumper->Add("native ");
    }
    if (IsExported()) {
        dumper->Add("export ");
    } else if (IsDefaultExported()) {
        dumper->Add("export default ");
    }
    if (func->IsDeclare() && !(parent_ != nullptr && parent_->IsDeclare())) {
        dumper->Add("declare ");
    }
    if (func->IsAsyncFunc()) {
        dumper->Add("async ");
    }
    dumper->Add("function ");

    func->Id()->Dump(dumper);
    func->Dump(dumper);
}

void FunctionDeclaration::Compile(compiler::PandaGen *pg) const
{
    pg->GetAstCompiler()->Compile(this);
}

void FunctionDeclaration::Compile(compiler::ETSGen *etsg) const
{
    etsg->GetAstCompiler()->Compile(this);
}

checker::Type *FunctionDeclaration::Check(checker::TSChecker *checker)
{
    return checker->GetAnalyzer()->Check(this);
}

checker::VerifiedType FunctionDeclaration::Check(checker::ETSChecker *checker)
{
    return {this, checker->GetAnalyzer()->Check(this)};
}

FunctionDeclaration *FunctionDeclaration::Construct(ArenaAllocator *allocator)
{
    return allocator->New<FunctionDeclaration>(allocator, nullptr);
}

void FunctionDeclaration::CopyTo(AstNode *other) const
{
    auto otherImpl = other->AsFunctionDeclaration();

    otherImpl->func_ = func_;
    otherImpl->isAnonymous_ = isAnonymous_;

    AnnotationAllowed<Statement>::CopyTo(other);
}

}  // namespace ark::es2panda::ir

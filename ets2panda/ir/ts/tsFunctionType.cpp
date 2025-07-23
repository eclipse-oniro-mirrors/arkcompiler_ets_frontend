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

#include "tsFunctionType.h"

#include "checker/TSchecker.h"
#include "checker/ETSchecker.h"
#include "checker/types/signature.h"
#include "compiler/core/ETSGen.h"
#include "compiler/core/pandagen.h"
#include "ir/astDump.h"
#include "ir/srcDump.h"

namespace ark::es2panda::ir {
void TSFunctionType::TransformChildren(const NodeTransformer &cb, std::string_view const transformationName)
{
    signature_.TransformChildren(cb, transformationName);
    TransformAnnotations(cb, transformationName);
}

void TSFunctionType::Iterate(const NodeTraverser &cb) const
{
    signature_.Iterate(cb);
}

void TSFunctionType::Dump(ir::AstDumper *dumper) const
{
    dumper->Add({{"type", "TSFunctionType"},
                 {"params", signature_.Params()},
                 {"typeParameters", AstDumper::Optional(signature_.TypeParams())},
                 {"returnType", signature_.ReturnType()},
                 {"isNullable", AstDumper::Optional(nullable_)},
                 {"annotations", AstDumper::Optional(Annotations())}});
}

void TSFunctionType::Dump(ir::SrcDumper *dumper) const
{
    DumpAnnotations(dumper);
    dumper->Add("TSFunctionType");
}

void TSFunctionType::Compile([[maybe_unused]] compiler::PandaGen *pg) const
{
    pg->GetAstCompiler()->Compile(this);
}
void TSFunctionType::Compile(compiler::ETSGen *etsg) const
{
    etsg->GetAstCompiler()->Compile(this);
}

checker::Type *TSFunctionType::Check(checker::TSChecker *checker)
{
    return checker->GetAnalyzer()->Check(this);
}

checker::Type *TSFunctionType::GetType(checker::TSChecker *checker)
{
    return checker->CheckTypeCached(this);
}

checker::VerifiedType TSFunctionType::Check([[maybe_unused]] checker::ETSChecker *checker)
{
    return {this, checker->GetAnalyzer()->Check(this)};
}

checker::Type *TSFunctionType::GetType([[maybe_unused]] checker::ETSChecker *checker)
{
    return nullptr;
}

TSFunctionType *TSFunctionType::Clone(ArenaAllocator *allocator, AstNode *parent)
{
    // Clone the function signature
    FunctionSignature clonedSignature = signature_.Clone(allocator);

    auto *clone = allocator->New<TSFunctionType>(std::move(clonedSignature), allocator);

    // Set parent relationships for cloned signature components
    if (clone->signature_.TypeParams() != nullptr) {
        clone->signature_.TypeParams()->SetParent(clone);
    }
    for (auto *param : clone->signature_.Params()) {
        param->SetParent(clone);
    }
    if (clone->signature_.ReturnType() != nullptr) {
        clone->signature_.ReturnType()->SetParent(clone);
    }

    clone->SetNullable(nullable_);

    if (parent != nullptr) {
        clone->SetParent(parent);
    }

    clone->SetRange(Range());

    // Clone annotations if any
    if (HasAnnotations()) {
        clone->SetAnnotations(Annotations());
    }

    return clone;
}
}  // namespace ark::es2panda::ir

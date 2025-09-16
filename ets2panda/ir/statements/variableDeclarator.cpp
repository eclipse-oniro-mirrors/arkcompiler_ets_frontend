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

#include "variableDeclarator.h"

#include "compiler/core/pandagen.h"
#include "compiler/core/ETSGen.h"
#include "checker/TSchecker.h"
#include "checker/ETSchecker.h"

namespace ark::es2panda::ir {
void VariableDeclarator::TransformChildren(const NodeTransformer &cb, std::string_view transformationName)
{
    if (auto *transformedNode = cb(id_); id_ != transformedNode) {
        id_->SetTransformedNode(transformationName, transformedNode);
        id_ = transformedNode->AsExpression();
    }

    if (init_ != nullptr) {
        if (auto *transformedNode = cb(init_); init_ != transformedNode) {
            init_->SetTransformedNode(transformationName, transformedNode);
            init_ = transformedNode->AsExpression();
        }
    }
}

void VariableDeclarator::Iterate(const NodeTraverser &cb) const
{
    cb(id_);

    if (init_ != nullptr) {
        cb(init_);
    }
}

void VariableDeclarator::Dump(ir::AstDumper *dumper) const
{
    dumper->Add({{"type", "VariableDeclarator"}, {"id", id_}, {"init", AstDumper::Nullish(init_)}});
}

void VariableDeclarator::Dump(ir::SrcDumper *dumper) const
{
    if (id_ != nullptr) {
        id_->Dump(dumper);
        if (id_->IsOptionalDeclaration()) {
            dumper->Add("?");
        }
        if (id_->IsAnnotatedExpression()) {
            auto *type = id_->AsAnnotatedExpression()->TypeAnnotation();
            if (type != nullptr) {
                dumper->Add(": ");
                type->Dump(dumper);
            }
        }
    }
    if (init_ != nullptr) {
        dumper->Add(" = ");
        init_->Dump(dumper);
    }
}

VariableDeclarator *VariableDeclarator::Clone(ArenaAllocator *const allocator, AstNode *const parent)
{
    auto *const id = id_ != nullptr ? id_->Clone(allocator, nullptr)->AsExpression() : nullptr;
    auto *const init = init_ != nullptr ? init_->Clone(allocator, nullptr)->AsExpression() : nullptr;
    auto *const clone = allocator->New<VariableDeclarator>(flag_, id, init);
    ES2PANDA_ASSERT(clone != nullptr);

    if (id != nullptr) {
        id->SetParent(clone);
    }

    if (init != nullptr) {
        init->SetParent(clone);
    }

    if (parent != nullptr) {
        clone->SetParent(parent);
    }

    clone->SetRange(range_.GetRange());
    return clone;
}

void VariableDeclarator::Compile([[maybe_unused]] compiler::PandaGen *pg) const
{
    pg->GetAstCompiler()->Compile(this);
}

void VariableDeclarator::Compile(compiler::ETSGen *etsg) const
{
    etsg->GetAstCompiler()->Compile(this);
}

checker::Type *VariableDeclarator::Check([[maybe_unused]] checker::TSChecker *checker)
{
    return checker->GetAnalyzer()->Check(this);
}

checker::VerifiedType VariableDeclarator::Check(checker::ETSChecker *checker)
{
    return {this, checker->GetAnalyzer()->Check(this)};
}
}  // namespace ark::es2panda::ir

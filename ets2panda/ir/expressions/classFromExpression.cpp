/**
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "ir/expressions/classFromExpression.h"

#include "checker/ETSchecker.h"
#include "checker/TSchecker.h"
#include "compiler/core/ETSGen.h"
#include "compiler/core/pandagen.h"

namespace ark::es2panda::ir {
void ClassFromExpression::TransformChildren(const NodeTransformer &cb, std::string_view const transformationName)
{
    TypeAnnotation()->TransformChildren(cb, transformationName);
}

void ClassFromExpression::Iterate(const NodeTraverser &cb) const
{
    if (TypeAnnotation() != nullptr) {
        cb(TypeAnnotation());
    }
}

void ClassFromExpression::Dump(ir::AstDumper *dumper) const
{
    dumper->Add({
        {"type", "ClassFromExpression"},
        {"typeAnnotation", AstDumper::Optional(TypeAnnotation())},
    });
}

void ClassFromExpression::Dump(ir::SrcDumper *dumper) const
{
    dumper->Add("Class.from<");
    if (TypeAnnotation() != nullptr) {
        TypeAnnotation()->Dump(dumper);
    }
    dumper->Add(">()");
}

void ClassFromExpression::Compile(compiler::PandaGen *pg) const
{
    pg->GetAstCompiler()->Compile(this);
}

void ClassFromExpression::Compile(compiler::ETSGen *etsg) const
{
    etsg->GetAstCompiler()->Compile(this);
}

checker::Type *ClassFromExpression::Check(checker::TSChecker *checker)
{
    return checker->GetAnalyzer()->Check(this);
}

checker::VerifiedType ClassFromExpression::Check(checker::ETSChecker *checker)
{
    return {this, checker->GetAnalyzer()->Check(this)};
}

ClassFromExpression *ClassFromExpression::Clone(ArenaAllocator *const allocator, AstNode *const parent)
{
    auto *const clone = allocator->New<ClassFromExpression>();
    ES2PANDA_ASSERT(clone != nullptr);

    clone->SetTsType(TsType());
    if (parent != nullptr) {
        clone->SetParent(parent);
    }
    if (TypeAnnotation() != nullptr) {
        clone->SetTsTypeAnnotation(TypeAnnotation()->Clone(allocator, this));
    }

    clone->SetRange(Range());
    return clone;
}
}  // namespace ark::es2panda::ir

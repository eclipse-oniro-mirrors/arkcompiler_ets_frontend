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

#include "tsNumberKeyword.h"

#include "checker/TSchecker.h"
#include "compiler/core/ETSGen.h"
#include "compiler/core/pandagen.h"
#include "ir/astDump.h"
#include "ir/srcDump.h"

namespace ark::es2panda::ir {
void TSNumberKeyword::TransformChildren([[maybe_unused]] const NodeTransformer &cb,
                                        [[maybe_unused]] std::string_view const transformationName)
{
    TransformAnnotations(cb, transformationName);
}

void TSNumberKeyword::Iterate(const NodeTraverser &cb) const
{
    IterateAnnotations(cb);
}

void TSNumberKeyword::Dump(ir::AstDumper *dumper) const
{
    dumper->Add({{"type", "TSNumberKeyword"}, {"annotations", AstDumper::Optional(Annotations())}});
}

void TSNumberKeyword::Dump(ir::SrcDumper *dumper) const
{
    DumpAnnotations(dumper);
    dumper->Add("TSNumberKeyword");
}

void TSNumberKeyword::Compile(compiler::PandaGen *pg) const
{
    pg->GetAstCompiler()->Compile(this);
}

void TSNumberKeyword::Compile(compiler::ETSGen *etsg) const
{
    etsg->GetAstCompiler()->Compile(this);
}

checker::Type *TSNumberKeyword::Check(checker::TSChecker *checker)
{
    return checker->GetAnalyzer()->Check(this);
}

checker::Type *TSNumberKeyword::GetType([[maybe_unused]] checker::TSChecker *checker)
{
    return checker->GlobalNumberType();
}

checker::VerifiedType TSNumberKeyword::Check(checker::ETSChecker *checker)
{
    return {this, checker->GetAnalyzer()->Check(this)};
}

TSNumberKeyword *TSNumberKeyword::Clone(ArenaAllocator *allocator, AstNode *parent)
{
    auto *clone = allocator->New<TSNumberKeyword>(allocator);

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

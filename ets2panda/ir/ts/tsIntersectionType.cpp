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

#include "tsIntersectionType.h"

#include "checker/TSchecker.h"
#include "compiler/core/ETSGen.h"
#include "compiler/core/pandagen.h"
#include "ir/astDump.h"
#include "ir/srcDump.h"
#include "checker/ETSchecker.h"

namespace ark::es2panda::ir {
void TSIntersectionType::TransformChildren(const NodeTransformer &cb, std::string_view const transformationName)
{
    for (auto *&it : VectorIterationGuard(types_)) {
        if (auto *transformedNode = cb(it); it != transformedNode) {
            it->SetTransformedNode(transformationName, transformedNode);
            it = transformedNode->AsExpression();
        }
    }

    TransformAnnotations(cb, transformationName);
}

void TSIntersectionType::Iterate(const NodeTraverser &cb) const
{
    for (auto *it : VectorIterationGuard(types_)) {
        cb(it);
    }

    IterateAnnotations(cb);
}

void TSIntersectionType::Dump(ir::AstDumper *dumper) const
{
    dumper->Add(
        {{"type", "TSIntersectionType"}, {"types", types_}, {"annotations", AstDumper::Optional(Annotations())}});
}

void TSIntersectionType::Dump(ir::SrcDumper *dumper) const
{
    DumpAnnotations(dumper);
    dumper->Add("TSIntersectionType");
}

void TSIntersectionType::Compile([[maybe_unused]] compiler::PandaGen *pg) const
{
    pg->GetAstCompiler()->Compile(this);
}
void TSIntersectionType::Compile(compiler::ETSGen *etsg) const
{
    etsg->GetAstCompiler()->Compile(this);
}

checker::Type *TSIntersectionType::Check([[maybe_unused]] checker::TSChecker *checker)
{
    return checker->GetAnalyzer()->Check(this);
}

checker::Type *TSIntersectionType::GetType([[maybe_unused]] checker::TSChecker *checker)
{
    return nullptr;
}

checker::Type *TSIntersectionType::GetType([[maybe_unused]] checker::ETSChecker *checker)
{
    // NOTE: validate
    return checker->GlobalETSObjectType();
}

checker::VerifiedType TSIntersectionType::Check([[maybe_unused]] checker::ETSChecker *checker)
{
    return {this, checker->GetAnalyzer()->Check(this)};
}
}  // namespace ark::es2panda::ir

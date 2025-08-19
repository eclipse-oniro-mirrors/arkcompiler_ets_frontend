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

#include "tsInferType.h"

#include "checker/TSchecker.h"
#include "compiler/core/ETSGen.h"
#include "compiler/core/pandagen.h"
#include "ir/astDump.h"
#include "ir/srcDump.h"
#include "ir/ts/tsTypeParameter.h"

namespace ark::es2panda::ir {
void TSInferType::TransformChildren(const NodeTransformer &cb, std::string_view transformationName)
{
    if (auto *transformedNode = cb(typeParam_); typeParam_ != transformedNode) {
        typeParam_->SetTransformedNode(transformationName, transformedNode);
        typeParam_ = transformedNode->AsTSTypeParameter();
    }

    TransformAnnotations(cb, transformationName);
}

void TSInferType::Iterate(const NodeTraverser &cb) const
{
    cb(typeParam_);
    IterateAnnotations(cb);
}

void TSInferType::Dump(ir::AstDumper *dumper) const
{
    dumper->Add(
        {{"type", "TSInferType"}, {"typeParameter", typeParam_}, {"annotations", AstDumper::Optional(Annotations())}});
}

void TSInferType::Dump(ir::SrcDumper *dumper) const
{
    DumpAnnotations(dumper);
    dumper->Add("TSInferType");
}

void TSInferType::Compile([[maybe_unused]] compiler::PandaGen *pg) const
{
    pg->GetAstCompiler()->Compile(this);
}
void TSInferType::Compile(compiler::ETSGen *etsg) const
{
    etsg->GetAstCompiler()->Compile(this);
}

checker::Type *TSInferType::Check([[maybe_unused]] checker::TSChecker *checker)
{
    return checker->GetAnalyzer()->Check(this);
}

checker::Type *TSInferType::GetType([[maybe_unused]] checker::TSChecker *checker)
{
    return nullptr;
}

checker::VerifiedType TSInferType::Check([[maybe_unused]] checker::ETSChecker *checker)
{
    return {this, checker->GetAnalyzer()->Check(this)};
}
}  // namespace ark::es2panda::ir

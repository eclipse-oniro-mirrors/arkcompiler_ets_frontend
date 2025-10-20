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

#include "tsUnknownKeyword.h"

#include "checker/TSchecker.h"
#include "compiler/core/ETSGen.h"
#include "compiler/core/pandagen.h"
#include "ir/astDump.h"
#include "ir/srcDump.h"

namespace ark::es2panda::ir {
void TSUnknownKeyword::TransformChildren([[maybe_unused]] const NodeTransformer &cb,
                                         [[maybe_unused]] std::string_view const transformationName)
{
    TransformAnnotations(cb, transformationName);
}

void TSUnknownKeyword::Iterate(const NodeTraverser &cb) const
{
    IterateAnnotations(cb);
}

void TSUnknownKeyword::Dump(ir::AstDumper *dumper) const
{
    dumper->Add({{"type", "TSUnknownKeyword"}, {"annotations", AstDumper::Optional(Annotations())}});
}

void TSUnknownKeyword::Dump(ir::SrcDumper *dumper) const
{
    DumpAnnotations(dumper);
    dumper->Add("TSUnknownKeyword");
}

void TSUnknownKeyword::Compile(compiler::PandaGen *pg) const
{
    pg->GetAstCompiler()->Compile(this);
}

void TSUnknownKeyword::Compile(compiler::ETSGen *etsg) const
{
    etsg->GetAstCompiler()->Compile(this);
}

checker::Type *TSUnknownKeyword::Check(checker::TSChecker *checker)
{
    return checker->GetAnalyzer()->Check(this);
}

checker::Type *TSUnknownKeyword::GetType([[maybe_unused]] checker::TSChecker *checker)
{
    return checker->GlobalUnknownType();
}

checker::VerifiedType TSUnknownKeyword::Check(checker::ETSChecker *checker)
{
    return {this, checker->GetAnalyzer()->Check(this)};
}
}  // namespace ark::es2panda::ir

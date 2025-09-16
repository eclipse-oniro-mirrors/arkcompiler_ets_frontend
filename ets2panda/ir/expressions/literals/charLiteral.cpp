/*
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

#include "charLiteral.h"

#include "checker/TSchecker.h"
#include "compiler/core/pandagen.h"
#include "compiler/core/ETSGen.h"
#include "checker/ETSchecker.h"

namespace ark::es2panda::ir {
void CharLiteral::TransformChildren([[maybe_unused]] const NodeTransformer &cb,
                                    [[maybe_unused]] std::string_view const transformationName)
{
}

void CharLiteral::Iterate([[maybe_unused]] const NodeTraverser &cb) const {}

void CharLiteral::Dump(ir::AstDumper *dumper) const
{
    dumper->Add({{"type", "CharLiteral"}, {"value", char_}});
}

void CharLiteral::Dump(ir::SrcDumper *dumper) const
{
    dumper->Add("c\'" + ToString() + "\'");
}

void CharLiteral::Compile([[maybe_unused]] compiler::PandaGen *pg) const
{
    pg->GetAstCompiler()->Compile(this);
}

void CharLiteral::Compile([[maybe_unused]] compiler::ETSGen *etsg) const
{
    etsg->GetAstCompiler()->Compile(this);
}

checker::Type *CharLiteral::Check([[maybe_unused]] checker::TSChecker *checker)
{
    return checker->GetAnalyzer()->Check(this);
}

checker::VerifiedType CharLiteral::Check([[maybe_unused]] checker::ETSChecker *checker)
{
    return {this, checker->GetAnalyzer()->Check(this)};
}

CharLiteral *CharLiteral::Clone(ArenaAllocator *const allocator, AstNode *const parent)
{
    auto *const clone = allocator->New<CharLiteral>(char_);
    ES2PANDA_ASSERT(clone != nullptr);
    if (parent != nullptr) {
        clone->SetParent(parent);
    }

    clone->SetRange(Range());
    clone->SetFolded(IsFolded());
    return clone;
}

std::string CharLiteral::ToString() const
{
    std::string utf8Str = util::Helpers::UTF16toUTF8(char_);
    if (UNLIKELY(utf8Str.empty())) {
        return std::to_string(char_);
    }
    return util::Helpers::CreateEscapedString(utf8Str);
}

}  // namespace ark::es2panda::ir

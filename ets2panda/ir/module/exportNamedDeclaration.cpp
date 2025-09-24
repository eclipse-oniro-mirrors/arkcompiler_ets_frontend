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

#include "exportNamedDeclaration.h"

#include "checker/TSchecker.h"
#include "compiler/core/ETSGen.h"
#include "compiler/core/pandagen.h"

namespace ark::es2panda::ir {
void ExportNamedDeclaration::TransformChildren(const NodeTransformer &cb, std::string_view transformationName)
{
    if (decl_ != nullptr) {
        if (auto *transformedNode = cb(decl_); decl_ != transformedNode) {
            decl_->SetTransformedNode(transformationName, transformedNode);
            decl_ = transformedNode;
        }
    } else {
        if (source_ != nullptr) {
            if (auto *transformedNode = cb(source_); source_ != transformedNode) {
                source_->SetTransformedNode(transformationName, transformedNode);
                source_ = transformedNode->AsStringLiteral();
            }
        }

        for (auto *&it : VectorIterationGuard(specifiers_)) {
            if (auto *transformedNode = cb(it); it != transformedNode) {
                it->SetTransformedNode(transformationName, transformedNode);
                it = transformedNode->AsExportSpecifier();
            }
        }
    }
}

void ExportNamedDeclaration::Iterate(const NodeTraverser &cb) const
{
    if (decl_ != nullptr) {
        cb(decl_);
    } else {
        if (source_ != nullptr) {
            cb(source_);
        }

        for (auto *it : VectorIterationGuard(specifiers_)) {
            cb(it);
        }
    }
}

void ExportNamedDeclaration::Dump(ir::AstDumper *dumper) const
{
    dumper->Add({{"type", "ExportNamedDeclaration"},
                 {"declaration", AstDumper::Nullish(decl_)},
                 {"source", AstDumper::Nullish(source_)},
                 {"specifiers", specifiers_}});
}

bool ExportNamedDeclaration::HasDumpData(bool const hasDefaultExport) const noexcept
{
    if (!specifiers_.empty()) {
        if (!hasDefaultExport) {
            return true;
        }

        for (const auto *spec : specifiers_) {
            if (!spec->IsDefaultExport() || spec->GetConstantExpression() != nullptr) {
                return true;
            }
        }
    }
    return false;
}

void ExportNamedDeclaration::Dump(ir::SrcDumper *dumper) const
{
    if (!HasDumpData(dumper->HasDefaultExport())) {
        return;
    }

    dumper->Add("export { ");
    for (const auto *spec : specifiers_) {
        if (spec->IsDefaultExport() && dumper->HasDefaultExport()) {
            continue;
        }

        spec->Dump(dumper);

        if (spec != specifiers_.back()) {
            dumper->Add(", ");
        }
    }
    dumper->Add(" }");
}

void ExportNamedDeclaration::Compile(compiler::PandaGen *pg) const
{
    pg->GetAstCompiler()->Compile(this);
}

void ExportNamedDeclaration::Compile(compiler::ETSGen *etsg) const
{
    etsg->GetAstCompiler()->Compile(this);
}

checker::Type *ExportNamedDeclaration::Check(checker::TSChecker *checker)
{
    return checker->GetAnalyzer()->Check(this);
}

checker::VerifiedType ExportNamedDeclaration::Check(checker::ETSChecker *checker)
{
    return {this, checker->GetAnalyzer()->Check(this)};
}
}  // namespace ark::es2panda::ir

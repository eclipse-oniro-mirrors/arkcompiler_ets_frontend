/**
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

#include "ir/ets/etsReExportDeclaration.h"
#include "ir/astDump.h"
#include "checker/checker.h"

namespace ark::es2panda::ir {

void ETSReExportDeclaration::SetETSImportDeclarations(ETSImportDeclaration *etsImportDeclarations)
{
    this->GetOrCreateHistoryNodeAs<ETSReExportDeclaration>()->etsImportDeclarations_ = etsImportDeclarations;
}

ETSReExportDeclaration::ETSReExportDeclaration(ETSImportDeclaration *etsImportDeclarations,
                                               const std::vector<std::string> &userPaths, util::StringView programPath,
                                               ArenaAllocator *allocator)
    : Statement(AstNodeType::REEXPORT_STATEMENT),
      etsImportDeclarations_(etsImportDeclarations),
      userPaths_(allocator->Adapter()),
      programPath_(programPath)
{
    for (const auto &path : userPaths) {
        userPaths_.emplace_back(util::UString(path, allocator).View());
    }
    InitHistory();
}

void ETSReExportDeclaration::TransformChildren(const NodeTransformer &cb, std::string_view const transformationName)
{
    auto const etsImportDecl = GetETSImportDeclarations();
    if (etsImportDecl != nullptr) {
        if (auto *transformedNode = cb(etsImportDecl); etsImportDecl != transformedNode) {
            etsImportDecl->SetTransformedNode(transformationName, transformedNode);
            SetETSImportDeclarations(transformedNode->AsETSImportDeclaration());
        }
    }
    InitHistory();
}

void ETSReExportDeclaration::Iterate(const NodeTraverser &cb) const
{
    GetETSImportDeclarations()->Iterate(cb);
}

void ETSReExportDeclaration::Dump(ir::AstDumper *dumper) const
{
    dumper->Add({{"type", "ETSReExportDeclaration"}, {"ets_import_declarations", GetETSImportDeclarations()}});
}

void ETSReExportDeclaration::Dump([[maybe_unused]] ir::SrcDumper *dumper) const
{
    dumper->DumpJsdocBeforeTargetNode(this);
    auto importDeclaration = GetETSImportDeclarations();
    const auto &specifiers = importDeclaration->Specifiers();
    dumper->Add("export ");
    if (specifiers.size() == 1 &&
        (specifiers[0]->IsImportNamespaceSpecifier() || specifiers[0]->IsImportDefaultSpecifier())) {
        specifiers[0]->Dump(dumper);
    } else {
        dumper->Add("{ ");
        for (auto specifier : specifiers) {
            specifier->Dump(dumper);
            if (specifier != specifiers.back()) {
                dumper->Add(", ");
            }
        }
        dumper->Add(" }");
    }

    dumper->Add(" from ");
    importDeclaration->Source()->Dump(dumper);
    dumper->Add(";");
    dumper->Endl();
}

checker::VerifiedType ETSReExportDeclaration::Check(checker::ETSChecker * /*checker*/)
{
    return {this, nullptr};
}

ETSReExportDeclaration *ETSReExportDeclaration::Construct(ArenaAllocator *allocator)
{
    std::vector<std::string> userPaths;
    return allocator->New<ETSReExportDeclaration>(nullptr, std::move(userPaths),
                                                  util::UString(std::string(), allocator).View(), allocator);
}

void ETSReExportDeclaration::CopyTo(AstNode *other) const
{
    auto otherImpl = reinterpret_cast<ETSReExportDeclaration *>(other);

    otherImpl->etsImportDeclarations_ = etsImportDeclarations_;
    otherImpl->userPaths_ = userPaths_;
    otherImpl->programPath_ = programPath_;

    Statement::CopyTo(other);
}
}  // namespace ark::es2panda::ir

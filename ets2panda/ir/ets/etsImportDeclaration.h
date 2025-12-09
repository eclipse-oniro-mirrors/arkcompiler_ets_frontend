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

#ifndef ES2PANDA_IR_ETS_IMPORT_DECLARATION_H
#define ES2PANDA_IR_ETS_IMPORT_DECLARATION_H

#include "ir/module/importDeclaration.h"
#include "ir/expressions/literals/stringLiteral.h"
#include "util/importPathManager.h"
#include "util/language.h"
#include "util/ustring.h"

namespace ark::es2panda::ir {
class StringLiteral;

class ETSImportDeclaration : public ImportDeclaration {
public:
    ETSImportDeclaration(ir::StringLiteral *importPath, util::ImportPathManager::ImportMetadata &&importMetadata,
                         ArenaVector<AstNode *> &&specifiers, const ImportKinds importKinds = ImportKinds::ALL)
        : ImportDeclaration(importPath, std::move(specifiers), importKinds), importMetadata_(importMetadata)
    {
        SetType(AstNodeType::ETS_IMPORT_DECLARATION);
    }

    ETSImportDeclaration(ir::StringLiteral *importPath, ArenaVector<AstNode *> &&specifiers,
                         const ImportKinds importKinds = ImportKinds::ALL)
        : ETSImportDeclaration(importPath, util::ImportPathManager::ImportMetadata {}, std::move(specifiers),
                               importKinds)
    {
    }

    void SetImportMetadata(util::ImportFlags importFlags, Language::Id lang, std::string_view resolvedSource,
                           std::string_view declPath, std::string_view ohmUrl)
    {
        auto node = GetOrCreateHistoryNode()->AsETSImportDeclaration();
        node->importMetadata_.importFlags = importFlags;
        node->importMetadata_.lang = lang;
        node->importMetadata_.resolvedSource = resolvedSource;
        node->importMetadata_.declPath = declPath;
        node->importMetadata_.ohmUrl = ohmUrl;
    }

    es2panda::Language Language() const
    {
        return es2panda::Language {ImportMetadata().lang};
    }

    std::string_view DeclPath() const
    {
        return ImportMetadata().declPath;
    }

    std::string_view OhmUrl() const
    {
        return ImportMetadata().ohmUrl;
    }

    bool IsValid() const
    {
        return (Source()->Str() != ERROR_LITERAL) && ImportMetadata().IsValid();
    }

    bool IsPureDynamic() const
    {
        return IsValid() && !DeclPath().empty() && Language().IsDynamic();
    }

    void SetAssemblerName(util::StringView assemblerName)
    {
        GetOrCreateHistoryNode()->AsETSImportDeclaration()->assemblerName_ = assemblerName;
    }

    const util::StringView &AssemblerName() const
    {
        return GetHistoryNode()->AsETSImportDeclaration()->assemblerName_;
    }

    std::string_view ResolvedSource() const
    {
        return ImportMetadata().resolvedSource;
    }

    const util::ImportPathManager::ImportMetadata &ImportMetadata() const
    {
        return GetHistoryNode()->AsETSImportDeclaration()->importMetadata_;
    }

    void Accept(ASTVisitorT *v) override
    {
        v->Accept(this);
    }

    ETSImportDeclaration *Construct(ArenaAllocator *allocator) override
    {
        ArenaVector<AstNode *> specifiers(allocator->Adapter());
        return allocator->New<ETSImportDeclaration>(nullptr, std::move(specifiers));
    }

    void CopyTo(AstNode *other) const override
    {
        auto otherImpl = other->AsETSImportDeclaration();

        otherImpl->importMetadata_ = importMetadata_;
        otherImpl->assemblerName_ = assemblerName_;

        ImportDeclaration::CopyTo(other);
    };

private:
    util::ImportPathManager::ImportMetadata importMetadata_;
    util::StringView assemblerName_ {};
};
}  // namespace ark::es2panda::ir

#endif

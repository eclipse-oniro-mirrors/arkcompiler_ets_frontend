/**
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at*
 *
 * http://www.apache.org/licenses/LICENSE-2.0*
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <ctime>
#include <string>

#include "compiler/lowering/util.h"
#include "find_rename_locations.h"
#include "find_references.h"
#include "internal_api.h"
#include "public/public.h"

namespace ark::es2panda::lsp {

bool IsImported(ir::AstNode *node)
{
    if (node == nullptr || !node->IsIdentifier()) {
        return false;
    }
    auto parent = node->Parent();
    if (parent == nullptr) {
        return false;
    }
    return parent->IsImportSpecifier();
}

bool NeedsCrossFileRename(es2panda_Context *context, size_t position)
{
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    auto touchingToken = GetTouchingToken(context, position, false);
    if (touchingToken == nullptr || !touchingToken->IsIdentifier()) {
        return false;
    }
    auto decl = compiler::DeclarationFromIdentifier(touchingToken->AsIdentifier());
    if (decl == nullptr || decl->Range().start.Program() == nullptr) {
        return false;
    }
    auto declFilePath = decl->Range().start.Program()->SourceFilePath();
    if (!declFilePath.Is(ctx->sourceFile->filePath)) {  // the declaration is in a different file
        auto parent = touchingToken->Parent();
        if (parent != nullptr && parent->IsMemberExpression()) {
            auto property = parent->AsMemberExpression()->Property();
            if (property != nullptr && compiler::DeclarationFromIdentifier(property->AsIdentifier()) == decl) {
                return true;
            }
        }
    } else {  // the declaration is in the same file
        auto isExported = [](ir::AstNode *node) {
            return node != nullptr && (node->IsExported() || node->IsDefaultExported() || node->IsExportedType());
        };
        auto exported = isExported(decl);
        if (exported) {
            return true;
        }
        if (decl->IsMethodDefinition() && decl->Parent() != nullptr && decl->Parent()->IsClassDefinition()) {
            return decl->IsPublic() && isExported(decl->Parent());
        }
        if (decl->IsClassProperty()) {
            return decl->IsPublic() && isExported(decl->Parent());
        }
    }
    return false;
}

std::set<RenameLocation> FindRenameLocations(CancellationToken *tkn,
                                             const std::vector<es2panda_Context *> &fileContexts,
                                             es2panda_Context *context, size_t position)
{
    auto references = FindReferences(tkn, fileContexts, context, position);
    std::set<RenameLocation> res;

    for (const auto &ref : references) {
        res.emplace(ref.filePath, ref.start, ref.end, ref.line);
    }

    return res;
}

std::set<RenameLocation> FindRenameLocations(CancellationToken *tkn, es2panda_Context *context, size_t position)
{
    auto references = FindReferences(tkn, {context}, context, position);
    std::set<RenameLocation> res;
    if (references.empty()) {
        return res;
    }
    auto it = references.cbegin();
    if (auto touchingToken = GetTouchingToken(context, it->start, false); IsImported(touchingToken)) {
        auto importSpecifier = touchingToken->Parent()->AsImportSpecifier();
        if (!(importSpecifier->Local()->Range() != importSpecifier->Imported()->Range())) {
            // this case: `import { SourceFile } from 'module';`
            // rename result: `import { SourceFile as sf } from 'module';`
            // so we need to add the prefix text `SourceFile as`
            res.emplace(it->filePath, it->start, it->end, it->line,
                        std::string(touchingToken->AsIdentifier()->Name()) + " as ");
            ++it;
        }
    }
    for (; it != references.end(); ++it) {
        res.emplace(it->filePath, it->start, it->end, it->line);
    }
    return res;
}

std::set<RenameLocation> FindRenameLocations(const std::vector<es2panda_Context *> &fileContexts,
                                             es2panda_Context *context, size_t position)
{
    time_t tmp = 0;
    CancellationToken cancellationToken {tmp, nullptr};
    return FindRenameLocations(&cancellationToken, fileContexts, context, position);
}

std::set<RenameLocation> FindRenameLocationsInCurrentFile(es2panda_Context *context, size_t position)
{
    time_t tmp = 0;
    CancellationToken cancellationToken {tmp, nullptr};
    return FindRenameLocations(&cancellationToken, context, position);
}
}  // namespace ark::es2panda::lsp
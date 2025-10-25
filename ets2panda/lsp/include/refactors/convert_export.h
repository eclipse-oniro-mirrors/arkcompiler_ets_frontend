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

#ifndef CONVERT_EXPORT_H
#define CONVERT_EXPORT_H

#include "refactor_types.h"
#include "services/text_change/change_tracker.h"

namespace ark::es2panda::lsp {

// Refactor action definitions
constexpr RefactorActionView TO_NAMED_EXPORT_ACTION {
    "ConvertExportRefactor",                   // name
    "Convert default export to named export",  // description
    "refactor.rewrite.export.named"            // kind
};

constexpr RefactorActionView TO_DEFAULT_EXPORT_ACTION {
    "ConvertExportRefactor",                   // name
    "Convert named export to default export",  // description
    "refactor.rewrite.export.default"          // kind
};

class ConvertExportRefactor : public Refactor {
public:
    ConvertExportRefactor();

    struct ImportSite {
        std::string filePath;  // file where the import is located
        size_t start;          // start offset of the import statement
        size_t end;            // end offset of the import statement
    };

    // Discover applicable actions for a given context
    std::vector<ApplicableRefactorInfo> GetAvailableActions(const RefactorContext &context) const override;

    void HandleToNamedExport(ChangeTracker &tracker, public_lib::Context *ctxContent,
                             ir::AstNode *exportIdentifier) const;

    void HandleToDefaultExport(ChangeTracker &tracker, public_lib::Context *ctxContent,
                               ir::AstNode *exportIdentifier) const;

    void ApplyExportEdits(ChangeTracker &tracker, public_lib::Context *ctxContent, ir::AstNode *ancestor,
                          ir::AstNode *exportIdentifier, const std::string &actionName) const;

    void ReplaceWithDefaultImport(ChangeTracker &tracker, public_lib::Context *importContent, const ReferenceInfo &ref,
                                  const std::string &exportName, const std::string &exportFile) const;

    void ReplaceWithNamedImport(ChangeTracker &tracker, public_lib::Context *importContent, const ReferenceInfo &ref,
                                const std::string &exportName, const std::string &exportFile) const;

    void HandleToDefaultImport(ChangeTracker &tracker, public_lib::Context *importContent, const ReferenceInfo &ref,
                               const std::string &exportName, const std::string &exportFile) const;

    void HandleToNamedImport(ChangeTracker &tracker, public_lib::Context *importContent, const ReferenceInfo &ref,
                             const std::string &exportName, const std::string &exportFile) const;

    void ProcessReference(ChangeTracker &tracker, public_lib::Context *importContent, const ReferenceInfo &ref,
                          ir::AstNode *ancestor, const std::string &actionName, const std::string &exportName,
                          const std::string &exportFile) const;

    void ApplyImportEdits(ChangeTracker &tracker, const std::vector<std::string> &importers,
                          const std::string &exportFile, const std::string &exportName, ir::AstNode *ancestor,
                          const std::string &actionName) const;

    // Generate edits for the selected action
    std::unique_ptr<RefactorEditInfo> GetEditsForAction(const RefactorContext &context,
                                                        const std::string &actionName) const override;

    // Find all import sites referencing a given export file
    std::vector<ImportSite> FindAllImportSites(const std::vector<ir::AstNode *> &allRoots,
                                               const std::string &targetPath) const;
};

}  // namespace ark::es2panda::lsp

#endif  // CONVERT_EXPORT_H
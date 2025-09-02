/**
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef ES2PANDA_LSP_INCLUDE_RENAME_H
#define ES2PANDA_LSP_INCLUDE_RENAME_H

#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <cstddef>

#include "types.h"
#include "ir/astNode.h"
#include "public/es2panda_lib.h"
#include "checker/types/type.h"
#include "parser/program/program.h"
#include "util/helpers.h"

namespace ark::es2panda::lsp {

struct RenameInfoSuccess {
private:
    bool canRenameSuccess_;
    std::string fileToRename_;
    std::string kind_;
    std::string displayName_;
    std::string fullDisplayName_;
    std::string kindModifiers_;
    TextSpan triggerSpan_;

public:
    RenameInfoSuccess(bool canRename, std::string fileToRename, std::string kind, std::string displayName,
                      std::string fullDisplayName, std::string kindModifiers, TextSpan triggerSpan)
        : canRenameSuccess_(canRename),
          fileToRename_(std::move(fileToRename)),
          kind_(std::move(kind)),
          displayName_(std::move(displayName)),
          fullDisplayName_(std::move(fullDisplayName)),
          kindModifiers_(std::move(kindModifiers)),
          triggerSpan_(triggerSpan)
    {
    }
    bool GetCanRenameSuccess() const
    {
        return canRenameSuccess_;
    }
    const std::string &GetFileToRename() const
    {
        return fileToRename_;
    }
    const std::string &GetKind() const
    {
        return kind_;
    }
    const std::string &GetDisplayName() const
    {
        return displayName_;
    }
    const std::string &GetFullDisplayName() const
    {
        return fullDisplayName_;
    }
    const std::string &GetKindModifiers() const
    {
        return kindModifiers_;
    }
    const TextSpan &GetTriggerSpan() const
    {
        return triggerSpan_;
    }
};

struct RenameInfoFailure {
private:
    bool canRenameFailure_;
    std::string localizedErrorMessage_;

public:
    RenameInfoFailure(bool canRename, std::string localizedErrorMessage)
        : canRenameFailure_(canRename), localizedErrorMessage_(std::move(localizedErrorMessage))
    {
    }

    bool GetCanRenameFailure() const
    {
        return canRenameFailure_;
    }
    const std::string &GetLocalizedErrorMessage() const
    {
        return localizedErrorMessage_;
    }
};

using RenameInfoType = std::variant<RenameInfoSuccess, RenameInfoFailure>;

RenameInfoType GetRenameInfo(es2panda_Context *context, size_t pos, const std::string &pandaLibPath);
std::optional<RenameInfoType> GetRenameInfoForNode(ir::AstNode *node, checker::ETSChecker *checker,
                                                   parser::Program *program, const std::string &pandaLibPath);
std::optional<checker::VerifiedType> GetContextualTypeFromParentOrAncestorTypeNode(ir::AstNode *node,
                                                                                   checker::ETSChecker *checker);
std::string GetTextOfNode(ir::AstNode *node, parser::Program *program);

RenameInfoSuccess GetRenameInfoSuccess(std::string displayName, std::string fullDisplayName, std::string kind,
                                       std::string kindModifiers, ir::AstNode *node);
RenameInfoFailure GetRenameInfoError(std::string diagnosticMessage);

ir::AstNode *TryGetImportFromModuleSpecifier(ir::AstNode *node);

bool IsValidAncestorType(ir::AstNodeType type);
bool NodeIsMissing(ir::AstNode *node);

TextSpan CreateTriggerSpanForNode(ir::AstNode *node);
std::string GetSourceTextOfNodeFromSourceFile(util::StringView sourceCode, ir::AstNode *node);

std::string StripQuotes(std::string name);
bool IsQuoteOrBacktick(int charCode);
std::optional<RenameInfoSuccess> GetRenameInfoForModule(ir::AstNode *node, parser::Program *program);

std::string GetNodeKindForRenameInfo(ir::AstNode *node);
bool IsImportOrExportSpecifierName(ir::AstNode *node);
bool IsStringOrNumericLiteralLike(ir::AstNode *node);

bool NodeIsEligibleForRename(ir::AstNode *node);
bool IsLiteralNameOfPropertyDeclarationOrIndexAccess(ir::AstNode *node);
ir::AstNode *GetNameOfDeclaration(ir::AstNode *node);
ir::AstNode *GetNonAssignedNameOfDeclaration(ir::AstNode *node);
ir::AstNode *GetAssignedName(ir::AstNode *node);
}  // namespace ark::es2panda::lsp
#endif
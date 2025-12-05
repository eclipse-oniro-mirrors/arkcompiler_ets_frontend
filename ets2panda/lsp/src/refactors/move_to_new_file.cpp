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

#include <iostream>
#include <ostream>
#include <string>
#include <string_view>
#include "refactors/move_to_new_file.h"
#include "es2panda.h"
#include "ir/astNode.h"
#include "refactor_provider.h"
#include "internal_api.h"
#include "public/public.h"
#include "refactors/refactor_types.h"
#include "types.h"
#include "compiler/lowering/util.h"
#include <fstream>
#include "lsp/include/services/text_change/change_tracker.h"
#include "lsp/include/organize_imports.h"

namespace ark::es2panda::lsp {

MoveToNewFileRefactor::MoveToNewFileRefactor()
{
    AddKind(std::string(TO_MOVE_TO_NEW_FILE_ACTION.kind));
}

template <class Sv>
inline std::string ToStdString(const Sv &v)
{
    return std::string(v.data(), v.size());
}

bool MoveToNewFileRefactor::NodeIsMissing(ir::AstNode *node) const
{
    if (node == nullptr) {
        return true;
    }
    size_t pos = node->Range().start.index;
    size_t end = node->Range().end.index;
    return pos == end;
}

std::string MoveToNewFileRefactor::GetSourceTextOfNodeFromSourceFile(es2panda_Context *context,
                                                                     util::StringView sourceCode,
                                                                     ir::AstNode *node) const
{
    if (NodeIsMissing(node)) {
        return "";
    }
    size_t pos = node->Range().start.index;
    size_t end = node->Range().end.index;
    std::string text;
    if (GetIsNodeHasExport(context, node)) {
        text += "export ";
    }
    text += std::string(sourceCode.Substr(pos, end));
    return text;
}

void MoveToNewFileRefactor::GetStatementsToMove(const RefactorContext &refContext) const
{
    statementsToMove_.clear();
    importStatementsOfOldFile_.clear();
    es2panda_Context *ctx = refContext.context;
    const auto context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    if (context == nullptr || context->parserProgram == nullptr || context->parserProgram->Ast() == nullptr) {
        return;
    }
    const auto ast = context->parserProgram->Ast();
    auto statements = ast->Statements();
    for (ark::es2panda::ir::AstNode *node : statements) {
        if (node->IsETSImportDeclaration()) {
            importStatementsOfOldFile_.push_back(node);
        } else {
            size_t statStart = node->Start().index;
            size_t statEnd = node->End().index;
            if ((refContext.span.pos >= statStart && refContext.span.pos <= statEnd) ||
                (refContext.span.end >= statStart && refContext.span.end <= statEnd) ||
                (refContext.span.pos <= statStart && refContext.span.end >= statEnd)) {
                statementsToMove_.push_back(node);
            }
        }
    }
}

inline const SourceFile *GetSourceFile(const RefactorContext &refContext)
{
    auto *pub = reinterpret_cast<public_lib::Context *>(refContext.context);
    return pub != nullptr ? pub->sourceFile : nullptr;
}

std::string MakeUniqueFileName(const SourceFile *oldFile, std::vector<ark::es2panda::ir::AstNode *> statementsToMove)
{
    if (statementsToMove.empty()) {
        return "";
    }
    auto path = oldFile->filePath;
    auto const pos = path.find_last_of('.');
    std::string extension(path.substr(pos + 1));
    auto name = compiler::GetNameOfDeclaration(statementsToMove[0]);
    std::string fileName = name.value_or("new_file");
    std::string directory = static_cast<std::string>(oldFile->fileFolder);
    std::string strNewFilePath = directory + "/" + fileName + "." + extension;
    fs::path filePath = fs::path(strNewFilePath);
    int number = 1;
    while (fs::exists(filePath)) {
        strNewFilePath = directory + "/";
        strNewFilePath += fileName + "_";
        strNewFilePath += std::to_string(number) + ".";
        strNewFilePath += extension;
        filePath = fs::path(strNewFilePath);
        number++;
    }
    return strNewFilePath;
}

bool GetIsNodeHasExport(es2panda_Context *context, ir::AstNode *node)
{
    const auto ast = reinterpret_cast<public_lib::Context *>(context)->parserProgram->Ast();
    if (!node->IsVariableDeclaration() && !node->IsFunctionDeclaration() && !node->IsFunctionExpression() &&
        !node->IsClassDeclaration()) {
        return false;
    }
    auto name = GetIdentifierName(node);
    auto found = ast->FindChild([name, &node](ir::AstNode *child) {
        if (child->IsCallExpression() && child->AsCallExpression()->Callee()->IsIdentifier() &&
            std::string(child->AsCallExpression()->Callee()->AsIdentifier()->Name()) == name) {
            return true;
        }
        if (child->IsIdentifier() && std::string(child->AsIdentifier()->Name()) == name &&
            child->Start().index != node->Start().index && child->End().index != node->End().index) {
            return true;
        }
        return false;
    });
    return found != nullptr;
}

void MoveToNewFileRefactor::FillTempFileAndDeleteNodes(es2panda_Context *context, ChangeTracker &tracker,
                                                       const std::string &tempNewFile, const SourceFile *oldFile) const
{
    std::ofstream ofsTempNewFile(tempNewFile);
    if (!ofsTempNewFile) {
        return;
    }
    auto src = reinterpret_cast<ark::es2panda::public_lib::Context *>(context)->sourceFile->source;
    for (auto node : importStatementsOfOldFile_) {
        ofsTempNewFile << GetSourceTextOfNodeFromSourceFile(context, src, node) << std::endl;
    }
    for (auto node : statementsToMove_) {
        ofsTempNewFile << GetSourceTextOfNodeFromSourceFile(context, src, node) << std::endl;
        tracker.DeleteNode(context, oldFile, node);
    }
    ofsTempNewFile.close();
}

void MoveToNewFileRefactor::DoChange(es2panda_Context *context, ChangeTracker &tracker, const SourceFile *oldFile) const
{
    const std::string newFile = MakeUniqueFileName(oldFile, statementsToMove_);
    const std::string tempNewFile = newFile + ".tmp";
    std::vector<const ark::es2panda::ir::Statement *> stt;
    for (auto node : statementsToMove_) {
        stt.push_back(node->AsStatement());
    }

    tracker.CreateNewFile(oldFile, newFile, stt);
    FillTempFileAndDeleteNodes(context, tracker, tempNewFile, oldFile);

    Initializer initializer;
    es2panda_Context *tempFileContext = initializer.CreateContext(tempNewFile.c_str(), ES2PANDA_STATE_CHECKED);
    std::vector<FileTextChanges> changes = OrganizeImports::Organize(tempFileContext, tempNewFile);
    if (changes.empty()) {
        return;
    }
    std::ofstream ofsNewFile(newFile);
    if (!ofsNewFile) {
        return;
    }
    const auto &change = changes[0];
    for (auto &tc : change.textChanges) {
        ofsNewFile << tc.newText << std::endl;
    }
    auto src = reinterpret_cast<ark::es2panda::public_lib::Context *>(context)->sourceFile->source;
    for (auto node : statementsToMove_) {
        ofsNewFile << GetSourceTextOfNodeFromSourceFile(context, src, node) << std::endl;
    }
    ofsNewFile.close();
    fs::remove(tempNewFile);

    std::string oldFilePath = static_cast<std::string>(oldFile->filePath);
    std::vector<FileTextChanges> oldFileImportChanges = OrganizeImports::Organize(context, oldFilePath);
    if (oldFileImportChanges.empty()) {
        return;
    }
    const auto ctx = reinterpret_cast<public_lib::Context *>(context);
    const auto basAst = ctx->parserProgram->Ast();
    basAst->FindChild([&tempFileContext, &ctx, &tracker](ir::AstNode *child) {
        if (GetIsNodeHasExport(tempFileContext, child)) {
            tracker.InsertText(ctx->sourceFile, child->Start().index, " export ");
        }
        return false;
    });
    OrganizeImports::Organize(context, newFile);
}

std::vector<ApplicableRefactorInfo> MoveToNewFileRefactor::GetAvailableActions(const RefactorContext &refContext) const
{
    ApplicableRefactorInfo applicableRef;
    std::vector<ApplicableRefactorInfo> res;

    if (!IsKind(refContext.kind)) {
        return res;
    }
    GetStatementsToMove(refContext);
    if (!statementsToMove_.empty()) {
        applicableRef.name = refactor_name::MOVE_TO_NEW_FILE_REFACTOR_NAME;
        applicableRef.description = refactor_description::MOVE_TO_NEW_FILE_REFACTOR_DESC;
        applicableRef.action.kind = std::string(TO_MOVE_TO_NEW_FILE_ACTION.kind);
        applicableRef.action.name = std::string(TO_MOVE_TO_NEW_FILE_ACTION.name);
        applicableRef.action.description = std::string(TO_MOVE_TO_NEW_FILE_ACTION.description);
        res.push_back(applicableRef);
    }
    return res;
}

std::unique_ptr<RefactorEditInfo> MoveToNewFileRefactor::GetEditsForAction(const RefactorContext &refContext,
                                                                           const std::string &actionName) const
{
    if (!actionName.empty() && actionName != ToStdString(refactor_name::MOVE_TO_NEW_FILE_REFACTOR_NAME)) {
        return nullptr;
    }

    const SourceFile *oldFile = GetSourceFile(refContext);

    if (statementsToMove_.empty() || oldFile == nullptr) {
        return nullptr;
    }

    TextChangesContext textChangesContext = *refContext.textChangesContext;

    std::vector<FileTextChanges> changes = ChangeTracker::With(
        textChangesContext, [&](ChangeTracker &tracker) { DoChange(refContext.context, tracker, oldFile); });

    return std::make_unique<RefactorEditInfo>(std::move(changes));
}
// NOLINTNEXTLINE(fuchsia-statically-constructed-objects, cert-err58-cpp)
AutoRefactorRegister<MoveToNewFileRefactor> g_moveToNewFileRefactorRegister("MoveToNewFileRefactor");

}  // namespace ark::es2panda::lsp

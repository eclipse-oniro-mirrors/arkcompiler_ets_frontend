/*
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

#include "lsp/include/code_fix_provider.h"
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "generated/code_fix_register.h"
#include "lsp/include/internal_api.h"

namespace ark::es2panda::lsp {

// Registers a new code fix and maps it to its diagnostic error codes and fix IDs
void CodeFixProvider::RegisterCodeFix([[maybe_unused]] const std::string &aliasName,
                                      std::unique_ptr<CodeFixRegistration> registration)
{
    ASSERT(!aliasName.empty());
    auto shared = std::shared_ptr<CodeFixRegistration>(std::move(registration));

    // Allow multiple fixes for the same error code
    for (auto error : shared->GetErrorCodes()) {
        auto &vec = errorCodeToFixes_[std::to_string(error)];
        vec.push_back(shared);
    }

    // Fix IDs remain one-to-one, so we keep the existing map
    for (const auto &fixId : shared->GetFixIds()) {
        fixIdToRegistration_.emplace(fixId, shared);
    }
}

CodeFixProvider &CodeFixProvider::Instance()
{
    static CodeFixProvider instance;
    return instance;
}

std::string CodeFixProvider::FormatWithArgs(const std::string &text)
{
    // This function is a placeholder for future implementation of string formatting with arguments.
    return text;
}

std::string CodeFixProvider::DiagnosticToString(const codefixes::DiagnosticCode &diag)
{
    return std::string(diag.GetMessage());
}

CodeFixAction CodeFixProvider::CreateCodeFixActionWorker(std::string &fixName, std::string &description,
                                                         std::vector<FileTextChanges> &changes, std::string &fixId,
                                                         std::string &fixAllDescription,
                                                         std::vector<CodeActionCommand> command)
{
    CodeAction codeAction;
    codeAction.description = description;
    codeAction.changes = changes;
    codeAction.commands = std::move(command);
    return {codeAction, fixName, fixId, fixAllDescription};
}

// Creates a code fix action that doesn't include fix-all functionality
CodeFixAction CodeFixProvider::CreateCodeFixActionWithoutFixAll(std::string &fixName,
                                                                std::vector<FileTextChanges> &changes,
                                                                codefixes::DiagnosticCode &diagCode)
{
    std::string fixId;
    std::string descriptionMessage = DiagnosticToString(diagCode);
    std::string fixAllDescription;
    return CreateCodeFixActionWorker(fixName, descriptionMessage, changes, fixId, fixAllDescription, {});
}

// Creates a full code fix action with fix-all and commands
CodeFixAction CodeFixProvider::CreateCodeFixAction(std::string fixName, std::vector<FileTextChanges> changes,
                                                   codefixes::DiagnosticCode diagCode, std::string fixId,
                                                   std::vector<CodeActionCommand> &command)
{
    auto descriptionMessage = std::string(diagCode.GetMessage());
    std::string fixAllDescriptionMessage = "Fix all: " + std::string(diagCode.GetMessage());

    return CreateCodeFixActionWorker(fixName, descriptionMessage, changes, fixId, fixAllDescriptionMessage,
                                     std::move(command));
}

std::string CodeFixProvider::GetFileName(const std::string &filePath)
{
    if (filePath.empty()) {
        return "";
    }

    std::size_t pos = filePath.find_last_of('/');
    if (pos != std::string::npos) {
        return filePath.substr(pos + 1);
    }

    pos = filePath.find_last_of('\\');
    if (pos != std::string::npos) {
        return filePath.substr(pos + 1);
    }

    return filePath;
}

std::vector<std::string> CodeFixProvider::GetSupportedErrorCodes()
{
    std::vector<std::string> result;
    for (const auto &kv : errorCodeToFixes_) {
        result.push_back(kv.first);
    }
    return result;
}

std::unique_ptr<DiagnosticReferences> CodeFixProvider::GetDiagnostics(const CodeFixContextBase &context)
{
    LSPAPI const *lspApi = GetImpl();
    ES2PANDA_ASSERT(lspApi != nullptr);
    Initializer initializer = Initializer();
    auto it = reinterpret_cast<public_lib::Context *>(context.context);
    ES2PANDA_ASSERT(it != nullptr && it->sourceFile != nullptr);
    const std::string_view fileName(it->sourceFile->filePath);
    const std::string_view source(it->sourceFile->source);
    const auto ctx = initializer.CreateContext(fileName.data(), ES2PANDA_STATE_CHECKED, source.data());
    auto [semantic, syntactic, suggestions] =
        std::make_tuple(lspApi->getSemanticDiagnostics(ctx), lspApi->getSyntacticDiagnostics(ctx),
                        lspApi->getSuggestionDiagnostics(ctx));

    auto result = std::make_unique<DiagnosticReferences>();
    result->diagnostic.reserve(semantic.diagnostic.size() + syntactic.diagnostic.size() +
                               suggestions.diagnostic.size());
    std::move(semantic.diagnostic.begin(), semantic.diagnostic.end(), std::back_inserter(result->diagnostic));
    std::move(syntactic.diagnostic.begin(), syntactic.diagnostic.end(), std::back_inserter(result->diagnostic));
    std::move(suggestions.diagnostic.begin(), suggestions.diagnostic.end(), std::back_inserter(result->diagnostic));
    initializer.DestroyContext(ctx);
    return result;
}

// Determines whether fix-all should be enabled for this registration based on diagnostic count
bool CodeFixProvider::ShouldIncludeFixAll(const CodeFixRegistration &registration,
                                          const std::vector<Diagnostic> &diagnostics)
{
    int maybeFixableDiagnostics = 0;
    const int minFixableDiagnostics = 1;
    for (const auto &diag : diagnostics) {
        if (std::holds_alternative<int>(diag.code_) &&
            std::find(registration.GetErrorCodes().begin(), registration.GetErrorCodes().end(),
                      std::get<int>(diag.code_)) != registration.GetErrorCodes().end()) {
            ++maybeFixableDiagnostics;
            if (maybeFixableDiagnostics > minFixableDiagnostics) {
                break;
            }
        }
    }
    return maybeFixableDiagnostics > minFixableDiagnostics;
}

// Returns all fixes associated with a fixId (used for fix-all)
CombinedCodeActions CodeFixProvider::GetAllFixes(const CodeFixAllContext &context)
{
    auto it = fixIdToRegistration_.find(context.fixId);
    if (it == fixIdToRegistration_.end() || !it->second) {
        return CombinedCodeActions();
    }
    const std::shared_ptr<CodeFixRegistration> &registration = it->second;
    return registration->GetAllCodeActions(context);
}

// Iterates through diagnostics matching given error codes and applies callback on each
void CodeFixProvider::EachDiagnostic(const CodeFixAllContext &context, const std::vector<int> &errorCodes,
                                     const std::function<void(const DiagnosticWithLocation &)> &cb)
{
    if (errorCodes.empty()) {
        return;
    }

    auto diagnostics = GetDiagnostics(context);
    if (diagnostics == nullptr) {
        return;
    }

    auto it = reinterpret_cast<public_lib::Context *>(context.context);
    ES2PANDA_ASSERT(it != nullptr && it->sourceFile != nullptr);
    auto *parserProgram = it->parserProgram;
    auto index = lexer::LineIndex(parserProgram->SourceCode());

    for (auto &diag : diagnostics->diagnostic) {
        auto isTargetError = [&diag](int code) {
            return std::holds_alternative<int>(diag.code_) && std::get<int>(diag.code_) == code;
        };
        if (!std::any_of(errorCodes.begin(), errorCodes.end(), isTargetError)) {
            continue;
        }

        size_t startOffset = index.GetOffset(
            lexer::SourceLocation(diag.range_.start.line_, diag.range_.start.character_, parserProgram));
        size_t endOffset =
            index.GetOffset(lexer::SourceLocation(diag.range_.end.line_, diag.range_.end.character_, parserProgram));

        ES2PANDA_ASSERT(endOffset >= startOffset);
        size_t length = endOffset - startOffset;
        auto diagWithLocate = DiagnosticWithLocation(std::move(diag), *it->sourceFile, startOffset, length);
        cb(diagWithLocate);
    }
}

bool IsAllContained(const std::vector<CodeFixAction> &actions, const std::vector<CodeFixAction> &existingActions)
{
    return std::all_of(actions.begin(), actions.end(), [&](const CodeFixAction &a) {
        return std::any_of(existingActions.begin(), existingActions.end(), [&](const CodeFixAction &r) {
            if (!a.fixId.empty() || !r.fixId.empty()) {
                return a.fixId == r.fixId;
            }
            return a.fixName == r.fixName;
        });
    });
}

// Returns applicable fix actions for a given error code
std::vector<CodeFixAction> CodeFixProvider::GetFixes(const CodeFixContext &context)
{
    std::vector<CodeFixAction> result;
    auto it = errorCodeToFixes_.find(std::to_string(context.errorCode));
    if (it != errorCodeToFixes_.end()) {
        for (auto &reg : it->second) {
            if (reg == nullptr) {
                continue;
            }
            auto actions = reg->GetCodeActions(context);
            if (actions.empty()) {
                continue;
            }
            bool allContained = IsAllContained(actions, result);
            if (allContained) {
                break;
            }
            result.insert(result.end(), actions.begin(), actions.end());
        }
    }
    return result;
}

// Applies fix-all logic using a callback for each matching diagnostic
CombinedCodeActions CodeFixProvider::CodeFixAll(
    const CodeFixAllContext &context, const std::vector<int> &errorCodes,
    std::function<void(ChangeTracker &, const DiagnosticWithLocation &)> use)
{
    std::vector<CodeActionCommand> commands;
    TextChangesContext textChangesContext {context.host, context.formatContext, context.preferences};
    auto changes = ChangeTracker::With(textChangesContext, [&](ChangeTracker &tracker) {
        EachDiagnostic(context, errorCodes, [&](const DiagnosticWithLocation &diag) { use(tracker, diag); });
    });
    return {changes, commands};
}

FileTextChanges CodeFixProvider::CreateFileTextChanges(const std::string &fileName,
                                                       const std::vector<TextChange> &textChanges)
{
    return {fileName, textChanges};
}

}  // namespace ark::es2panda::lsp
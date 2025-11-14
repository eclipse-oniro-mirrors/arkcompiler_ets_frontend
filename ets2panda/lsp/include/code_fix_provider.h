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

#ifndef CODE_FIX_PROVIDER_H
#define CODE_FIX_PROVIDER_H
#include <cstddef>
#include <iostream>
#include <memory>
#include <unordered_map>
#include "services/text_change/change_tracker.h"
#include "code_fixes/code_fix_types.h"
#include "generated/code_fix_register.h"
#include "es2panda.h"

namespace ark::es2panda::lsp {
using ark::es2panda::lsp::codefixes::DiagnosticCode;
class CodeFixProvider {
private:
    std::unordered_map<std::string, std::vector<std::shared_ptr<CodeFixRegistration>>> errorCodeToFixes_;
    std::unordered_map<std::string, std::shared_ptr<CodeFixRegistration>> fixIdToRegistration_;

public:
    std::unordered_map<std::string, std::vector<std::shared_ptr<CodeFixRegistration>>> GetErrorCodeToFixes() const
    {
        return errorCodeToFixes_;
    }
    std::unordered_map<std::string, std::shared_ptr<CodeFixRegistration>> GetFixIdToRegistration() const
    {
        return fixIdToRegistration_;
    }

    void RegisterCodeFix(const std::string &aliasName, std::unique_ptr<CodeFixRegistration> registration);
    static CodeFixProvider &Instance();

    std::string FormatWithArgs(const std::string &text);
    std::string DiagnosticToString(const codefixes::DiagnosticCode &diag);
    CodeFixAction CreateCodeFixActionWorker(std::string &fixName, std::string &description,
                                            std::vector<FileTextChanges> &changes, std::string &fixId,
                                            std::string &fixAllDescription, std::vector<CodeActionCommand> command);

    CodeFixAction CreateCodeFixActionWithoutFixAll(std::string &fixName, std::vector<FileTextChanges> &changes,
                                                   DiagnosticCode &diagCode);
    CodeFixAction CreateCodeFixAction(std::string fixName, std::vector<FileTextChanges> changes,
                                      DiagnosticCode diagCode, std::string fixId,
                                      std::vector<CodeActionCommand> &command);
    std::string GetFileName(const std::string &filePath);
    std::vector<std::string> GetSupportedErrorCodes();
    std::unique_ptr<DiagnosticReferences> GetDiagnostics(const CodeFixContextBase &context);

    bool ShouldIncludeFixAll(const CodeFixRegistration &registration, const std::vector<Diagnostic> &diagnostics);
    std::vector<CodeFixAction> GetFixes(const CodeFixContext &context);
    void EachDiagnostic(const CodeFixAllContext &context, const std::vector<int> &errorCodes,
                        const std::function<void(const DiagnosticWithLocation &)> &cb);
    CombinedCodeActions CodeFixAll(const CodeFixAllContext &context, const std::vector<int> &errorCodes,
                                   std::function<void(ChangeTracker &, const DiagnosticWithLocation &)> use);
    FileTextChanges CreateFileTextChanges(const std::string &fileName, const std::vector<TextChange> &textChanges);

    CombinedCodeActions GetAllFixes(const CodeFixAllContext &context);
};

template <typename T>
struct AutoCodeFixRegister {
    constexpr explicit AutoCodeFixRegister(const std::string &name, std::unique_ptr<T> registration)
    {
        CodeFixProvider::Instance().RegisterCodeFix(name, std::move(registration));
    }

    constexpr explicit AutoCodeFixRegister(const std::string &name) : AutoCodeFixRegister(name, std::make_unique<T>())
    {
    }
};
}  // namespace ark::es2panda::lsp
#endif

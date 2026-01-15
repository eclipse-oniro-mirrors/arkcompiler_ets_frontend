/**
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

#ifndef FORGOTTEN_THIS_PROPERTY_ACCESS_H
#define FORGOTTEN_THIS_PROPERTY_ACCESS_H

#include <string>
#include <vector>
#include "lsp/include/code_fixes/code_fix_types.h"
#include "public/es2panda_lib.h"
#include "lsp/include/services/text_change/change_tracker.h"

namespace ark::es2panda::lsp {

class ForgottenThisPropertyAccess : public CodeFixRegistration {
public:
    ForgottenThisPropertyAccess();

    std::vector<CodeFixAction> GetCodeActions(const CodeFixContext &context) override;

    CombinedCodeActions GetAllCodeActions(const CodeFixAllContext &codeFixAll) override;

private:
    void DoChanges(ChangeTracker &tracker, es2panda_Context *context, size_t pos);
    std::vector<FileTextChanges> GetCodeActionsToFix(const CodeFixContext &context);
};

struct Info {
private:
    ark::es2panda::ir::AstNode *node_;
    std::string className_;

public:
    Info(ark::es2panda::ir::AstNode *node, std::string className) : node_(node), className_(std::move(className)) {}

    ark::es2panda::ir::AstNode *GetNode() const
    {
        return node_;
    }
    const std::string &GetName() const
    {
        return className_;
    }
};

Info GetInfoThisProp(es2panda_Context *context, size_t offset);

}  // namespace ark::es2panda::lsp

#endif  // FORGOTTEN_THIS_PROPERTY_ACCESS_H
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

#ifndef FIX_SPELLING_H
#define FIX_SPELLING_H

#include <utility>
#include <vector>
#include <cstddef>
#include "lsp/include/code_fixes/code_fix_types.h"
#include "../services/text_change/change_tracker.h"

namespace ark::es2panda::lsp {

class FixSpelling : public CodeFixRegistration {
public:
    FixSpelling();

    std::vector<CodeFixAction> GetCodeActions(const CodeFixContext &context) override;

    CombinedCodeActions GetAllCodeActions(const CodeFixAllContext &codeFixAll) override;
};

struct Info {
private:
    std::string findClosestWord_;
    ark::es2panda::ir::AstNode *node_;

public:
    Info(std::string findClosestWord, ark::es2panda::ir::AstNode *node)
        : findClosestWord_(std::move(findClosestWord)), node_(node)
    {
    }
    const std::string &GetFindClosestWord() const
    {
        return findClosestWord_;
    }
    ark::es2panda::ir::AstNode *GetNode() const
    {
        return node_;
    }
};

Info GetInfoSpelling(es2panda_Context *context, size_t position);
void DoChanges(ChangeTracker &changes, es2panda_Context *context, ir::AstNode *node, const std::string &target);

}  // namespace ark::es2panda::lsp
#endif

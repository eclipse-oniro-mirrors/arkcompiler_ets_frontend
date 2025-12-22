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

#ifndef FIX_ADD_FUNCTION_RETURN_STATEMENT_H
#define FIX_ADD_FUNCTION_RETURN_STATEMENT_H

#include <vector>
#include "../services/text_change/change_tracker.h"
#include "lsp/include/types.h"
#include "lsp/include/code_fixes/code_fix_types.h"
#include "libarkbase/utils/arena_containers.h"

namespace ark::es2panda::lsp {

class FixAddFunctionReturnStatement : public CodeFixRegistration {
public:
    FixAddFunctionReturnStatement();

    std::vector<CodeFixAction> GetCodeActions(const CodeFixContext &context) override;

    CombinedCodeActions GetAllCodeActions(const CodeFixAllContext &ctx) override;
};

struct Info {
private:
    ark::es2panda::ir::AstNode *returnTypeNode_;
    ark::es2panda::ir::AstNode *body_;
    std::vector<ark::es2panda::ir::Statement *> statements_;

public:
    Info(ark::es2panda::ir::AstNode *returnTypeNode, ark::es2panda::ir::AstNode *body,
         std::vector<ark::es2panda::ir::Statement *> statements)
        : returnTypeNode_(returnTypeNode), body_(body), statements_(std::move(statements))
    {
    }
    ark::es2panda::ir::AstNode *GetReturnTypeNode() const
    {
        return returnTypeNode_;
    }
    ark::es2panda::ir::AstNode *GetBody() const
    {
        return body_;
    }
    const std::vector<ark::es2panda::ir::Statement *> &GetStatements() const
    {
        return statements_;
    }
};

ir::AstNode *FindAncessor(ir::AstNode *node);
Info GetInfo(es2panda_Context *context, size_t position);
void ReplaceReturnType(ChangeTracker &changes, es2panda_Context *context, Info &info);
void AddReturnStatement(ChangeTracker &changes, es2panda_Context *context, std::vector<ir::Statement *> statements,
                        ir::AstNode *body);

}  // namespace ark::es2panda::lsp
#endif

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

#include "get_node.h"
#include "public/es2panda_lib.h"
#include "public/public.h"

namespace ark::es2panda::lsp {
es2panda_AstNode *GetProgramAstImpl(es2panda_Context *context)
{
    if (context == nullptr) {
        return nullptr;
    }
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    return reinterpret_cast<es2panda_AstNode *>(ctx->parserProgram->Ast());
}

es2panda_AstNode *GetClassDefinitionImpl(es2panda_AstNode *astNode, const std::string &nodeName)
{
    if (astNode == nullptr) {
        return nullptr;
    }
    auto ast = reinterpret_cast<ir::AstNode *>(astNode);
    auto targetNode = ast->FindChild([&nodeName](ir::AstNode *childNode) {
        return childNode->IsClassDefinition() &&
               std::string(childNode->AsClassDefinition()->Ident()->Name()) == nodeName;
    });
    return reinterpret_cast<es2panda_AstNode *>(targetNode);
}

es2panda_AstNode *GetIdentifierImpl(es2panda_AstNode *astNode, const std::string &nodeName)
{
    if (astNode == nullptr) {
        return nullptr;
    }
    auto ast = reinterpret_cast<ir::AstNode *>(astNode);
    auto targetNode = ast->FindChild([&nodeName](ir::AstNode *childNode) {
        return childNode->IsIdentifier() && std::string(childNode->AsIdentifier()->Name()) == nodeName;
    });
    return reinterpret_cast<es2panda_AstNode *>(targetNode);
}

}  // namespace ark::es2panda::lsp
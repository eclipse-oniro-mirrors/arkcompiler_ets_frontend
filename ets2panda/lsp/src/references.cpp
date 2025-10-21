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

#include "references.h"
#include <cstddef>
#include "api.h"
#include "compiler/lowering/util.h"
#include "ir/astNode.h"
#include "public/es2panda_lib.h"
#include "public/public.h"

namespace ark::es2panda::lsp {

bool IsValidReference(ir::AstNode *astNode)
{
    switch (astNode->Type()) {
        case ark::es2panda::ir::AstNodeType::IDENTIFIER:
            return true;
        default:
            return false;
    }
}

DeclInfoType GetDeclInfoImpl(ir::AstNode *astNode)
{
    if (astNode == nullptr || !astNode->IsIdentifier()) {
        return {};
    }
    auto declNode = compiler::DeclarationFromIdentifier(astNode->AsIdentifier());
    if (declNode == nullptr) {
        return {};
    }
    auto node = declNode;
    while (node != nullptr) {
        if (node->Range().start.Program() != nullptr) {
            auto name = std::string(node->Range().start.Program()->SourceFilePath());
            return std::make_tuple(name, declNode->DumpEtsSrc());
        }
        if (node->IsETSModule()) {
            auto name = std::string(node->AsETSModule()->Program()->SourceFilePath());
            return std::make_tuple(name, declNode->DumpEtsSrc());
        }
        node = node->Parent();
    }
    return {};
}

References GetReferencesAtPositionImpl(es2panda_Context *context, const DeclInfoType &declInfo)
{
    References result;
    if (context == nullptr) {
        return result;
    }
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    if (ctx->parserProgram == nullptr || ctx->parserProgram->Ast() == nullptr) {
        return result;
    }
    auto astNode = reinterpret_cast<ir::AstNode *>(ctx->parserProgram->Ast());
    astNode->IterateRecursively([ctx, declInfo, &result](ir::AstNode *child) {
        auto info = GetDeclInfoImpl(child);
        if (info == declInfo) {
            size_t startPos = child->Start().index;
            size_t endPos = child->End().index;
            result.referenceInfos.emplace_back(ctx->sourceFileName, startPos, endPos - startPos);
        }
    });
    return result;
}

}  // namespace ark::es2panda::lsp
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
#include "internal_api.h"
#include <charconv>

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

ReferenceInfo ResolveInfo(const std::tuple<std::string, std::string> &info)
{
    const std::string &fileName = std::get<0>(info);
    const std::string &positionInfo = std::get<1>(info);
    if (fileName.empty() || positionInfo.empty()) {
        return ReferenceInfo();
    }

    size_t firstColon = positionInfo.find(':');
    if (firstColon == std::string::npos) {
        return ReferenceInfo();
    }
    size_t secondColon = positionInfo.find(':', firstColon + 1);
    if (secondColon == std::string::npos) {
        return ReferenceInfo();
    }

    std::string_view posStr1(positionInfo.c_str() + firstColon + 1, secondColon - firstColon - 1);
    std::string_view posStr2(positionInfo.c_str() + secondColon + 1, positionInfo.size() - secondColon - 1);

    size_t startPos = 0;
    size_t endPos = 0;

    auto result1 = std::from_chars(posStr1.data(), posStr1.data() + posStr1.size(), startPos);
    if (result1.ec != std::errc {}) {
        return ReferenceInfo();
    }
    auto result2 = std::from_chars(posStr2.data(), posStr2.data() + posStr2.size(), endPos);
    if (result2.ec != std::errc {}) {
        return ReferenceInfo();
    }
    if (endPos < startPos) {
        return ReferenceInfo();
    }

    return ReferenceInfo(fileName, startPos, endPos - startPos);
}

std::string GetPositionInfo(ir::AstNode *astNode)
{
    if (astNode == nullptr) {
        return "";
    }
    return astNode->DumpEtsSrc() + ":" + std::to_string(astNode->Start().index) + ":" +
           std::to_string(astNode->End().index);
}

DeclInfoType GetDeclInfoImpl(ir::AstNode *astNode)
{
    if (astNode == nullptr || !astNode->IsIdentifier()) {
        return {};
    }
    auto declNode = ark::es2panda::lsp::GetOwner(astNode);
    if (declNode == nullptr) {
        return {};
    }
    auto positionInfo = GetPositionInfo(declNode);

    auto node = declNode;
    while (node != nullptr) {
        if (node->Range().start.Program() != nullptr) {
            auto name = std::string(node->Range().start.Program()->SourceFilePath());
            return std::make_tuple(name, positionInfo);
        }
        if (node->IsETSModule()) {
            auto name = std::string(node->AsETSModule()->Program()->SourceFilePath());
            return std::make_tuple(name, positionInfo);
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
        auto position = GetPositionInfo(child);
        if (info == declInfo && std::get<1>(info) != position) {
            size_t startPos = child->Start().index;
            size_t endPos = child->End().index;
            result.referenceInfos.emplace_back(ctx->sourceFileName, startPos, endPos - startPos);
        }
    });
    return result;
}

}  // namespace ark::es2panda::lsp
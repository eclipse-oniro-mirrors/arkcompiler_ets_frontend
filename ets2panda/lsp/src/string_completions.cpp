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

#include "string_completions.h"
#include "completions.h"
#include <optional>
#include <set>
#include <string>
#include <vector>
#include "ir/astNode.h"

namespace ark::es2panda::lsp {
ir::AstNode *GetDeclarationFromParent(ir::AstNode *node)
{
    // NOTE: After GL22929
    (void)node;

    return nullptr;
}

void ControlTypeAndAddToEntry(std::string name, ir::AstNode *referencePart, std::vector<lsp::CompletionEntry> &entries,
                              std::set<std::string> &keys)
{
    if (referencePart == nullptr) {
        return;
    }
    auto *nameNode = referencePart->AsETSTypeReferencePart()->Name();
    if (!nameNode->IsIdentifier()) {
        return;
    }
    auto type = std::string(nameNode->AsIdentifier()->Name());

    std::string insertText = name += std::string(": ") += type;
    auto entry = lsp::CompletionEntry(name, CompletionEntryKind::TEXT, "", insertText);
    keys.insert(name);
    entries.push_back(entry);
}

void GetCompletionsFromBody(ArenaVector<ir::AstNode *> const &body, std::vector<lsp::CompletionEntry> &entries)
{
    std::set<std::string> keys = {};
    for (auto property : body) {
        if (property->IsMethodDefinition()) {
            auto methodDefinition = property->AsMethodDefinition();
            auto *key = methodDefinition->Key();
            if (!key->IsIdentifier()) {
                continue;
            }
            auto name = std::string(key->AsIdentifier()->Name());
            if (name == "constructor" || keys.count(name) == 1 || name.empty()) {
                continue;
            }

            ir::AstNode *referencePart = methodDefinition->Value()->FindChild(
                [](ir::AstNode *child) { return child->IsETSTypeReferencePart(); });

            ControlTypeAndAddToEntry(name, referencePart, entries, keys);
        } else if (property->IsClassProperty()) {
            auto classProperty = property->AsClassProperty();
            auto *key = classProperty->Key();
            if (!key->IsIdentifier()) {
                continue;
            }
            auto name = std::string(key->AsIdentifier()->Name());
            if (name.empty() || keys.count(name) == 1) {
                continue;
            }

            ir::AstNode *referencePart =
                classProperty->FindChild([](ir::AstNode *child) { return child->IsETSTypeReferencePart(); });

            ControlTypeAndAddToEntry(name, referencePart, entries, keys);
        }
    }
}

ark::es2panda::lsp::CompletionInfo GetCompletionInfo(ir::AstNode *node)
{
    std::vector<lsp::CompletionEntry> entries;
    switch (node->Type()) {
        case ir::AstNodeType::TS_INTERFACE_BODY: {
            GetCompletionsFromBody(node->AsTSInterfaceBody()->Body(), entries);
            break;
        }
        case ir::AstNodeType::TS_INTERFACE_DECLARATION: {
            GetCompletionsFromBody(node->AsTSInterfaceDeclaration()->Body()->Body(), entries);
            break;
        }
        case ir::AstNodeType::CLASS_DECLARATION: {
            GetCompletionsFromBody(node->AsClassDeclaration()->Definition()->Body(), entries);
            break;
        }
        default:
            break;
    }

    return ark::es2panda::lsp::CompletionInfo(entries);
}

// NOTE: The API that calls this function already know the location of the completion request, should just
// pass the Node of the completion request position instead of send the location and we get the node from context
std::optional<ark::es2panda::lsp::CompletionInfo> GetStringLiteralCompletions(ir::AstNode *cursor,
                                                                              ir::AstNode *declarationNode)
{
    if (cursor == nullptr || !cursor->IsStringLiteral() || declarationNode == nullptr) {
        return std::nullopt;
    }

    // Note: Should add more cases, like function declaration, Property Assignment, Element Access etc.
    switch (declarationNode->Type()) {
        case ir::AstNodeType::TS_INTERFACE_DECLARATION:
        case ir::AstNodeType::TS_INTERFACE_BODY: {
            return GetCompletionInfo(declarationNode);
        }
        case ir::AstNodeType::CLASS_DECLARATION: {
            return GetCompletionInfo(declarationNode);
        }
        case ir::AstNodeType::FUNCTION_DECLARATION:
            // Not Supported at the moment
            return std::nullopt;
        case ir::AstNodeType::TS_UNION_TYPE:
        case ir::AstNodeType::ETS_UNION_TYPE:
            // Not supported at the moment
            return std::nullopt;
        default:
            return std::nullopt;
    }
    return std::nullopt;
}

}  // namespace ark::es2panda::lsp

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

#ifndef NODE_MATCHERS_H
#define NODE_MATCHERS_H

#include <functional>
#include <unordered_map>
#include "ir/astNode.h"
#include "api.h"

#define DEFINE_SIMPLE_HANDLER(FunctionName, NodeType, NameAccessor, NodeTypeEnum) \
    void FunctionName(ir::AstNode *node, std::vector<NodeInfo> &result)           \
    {                                                                             \
        if (auto ident = node->As##NodeType()->NameAccessor()) {                  \
            result.emplace_back(std::string(ident->Name()), NodeTypeEnum);        \
        }                                                                         \
    }

namespace ark::es2panda::lsp {
using NodeMatcher = std::function<bool(ir::AstNode *, const NodeInfo *)>;
using NodeExtractor = ir::AstNode *(*)(ir::AstNode *, const NodeInfo *);
using NodeInfoHandler = std::function<void(ir::AstNode *, std::vector<NodeInfo> &)>;

bool MatchClassDefinition(ir::AstNode *childNode, const NodeInfo *info);
bool MatchIdentifier(ir::AstNode *childNode, const NodeInfo *info);
bool MatchClassProperty(ir::AstNode *childNode, const NodeInfo *info);
bool MatchProperty(ir::AstNode *childNode, const NodeInfo *info);
bool MatchMethodDefinition(ir::AstNode *childNode, const NodeInfo *info);
bool MatchTSEnumDeclaration(ir::AstNode *childNode, const NodeInfo *info);
bool MatchTSEnumMember(ir::AstNode *childNode, const NodeInfo *info);
bool MatchTSInterfaceDeclaration(ir::AstNode *childNode, const NodeInfo *info);
bool MatchTSTypeAliasDeclaration(ir::AstNode *childNode, const NodeInfo *info);
bool MatchExportSpecifier(ir::AstNode *childNode, const NodeInfo *info);
bool MatchMemberExpression(ir::AstNode *childNode, const NodeInfo *info);
bool MatchTSClassImplements(ir::AstNode *childNode, const NodeInfo *info);
bool MatchEtsStringLiteralType(ir::AstNode *childNode, const NodeInfo *info);
bool MatchEtsTypeReference(ir::AstNode *childNode, const NodeInfo *info);
bool MatchEtsKeyofType(ir::AstNode *childNode, const NodeInfo *info);
bool MatchEtsNewClassInstanceExpression(ir::AstNode *childNode, const NodeInfo *info);
bool MatchEtsStructDeclaration(ir::AstNode *childNode, const NodeInfo *info);
bool MatchSpreadElement(ir::AstNode *childNode, const NodeInfo *info);
bool MatchCallExpression(ir::AstNode *childNode, const NodeInfo *info);
bool MatchTsTypeReference(ir::AstNode *childNode, const NodeInfo *info);
bool MatchScriptFunction(ir::AstNode *childNode, const NodeInfo *info);
bool MatchVariableDeclarator(ir::AstNode *childNode, const NodeInfo *info);
bool MatchVariableDeclaration(ir::AstNode *childNode, const NodeInfo *info);
bool MatchClassDeclaration(ir::AstNode *childNode, const NodeInfo *info);
bool MatchAnnotationDeclaration(ir::AstNode *childNode, const NodeInfo *info);
bool MatchAnnotationUsage(ir::AstNode *childNode, const NodeInfo *info);
bool MatchAwaitExpression(ir::AstNode *childNode, const NodeInfo *info);
bool MatchBigIntLiteral(ir::AstNode *childNode, const NodeInfo *info);
bool MatchImportSpecifier(ir::AstNode *childNode, const NodeInfo *info);
bool MatchImportDefaultSpecifier(ir::AstNode *childNode, const NodeInfo *info);
bool MatchImportNamespaceSpecifier(ir::AstNode *childNode, const NodeInfo *info);
bool MatchTSTypeParameter(ir::AstNode *childNode, const NodeInfo *info);
bool MatchSwitchStatement(ir::AstNode *childNode, const NodeInfo *info);
bool MatchEtsParameterExpression(ir::AstNode *childNode, const NodeInfo *info);
bool MatchTsNonNullExpression(ir::AstNode *childNode, const NodeInfo *info);
bool MatchFunctionDeclaration(ir::AstNode *childNode, const NodeInfo *info);
void HandleIdentifier(ir::AstNode *node, std::vector<NodeInfo> &result);
void HandleMemberExpression(ir::AstNode *node, std::vector<NodeInfo> &result);
void HandleSpeadeElement(ir::AstNode *node, std::vector<NodeInfo> &result);
void HandleTSEnumMember(ir::AstNode *node, std::vector<NodeInfo> &result);
void HandleCallExpression(ir::AstNode *node, std::vector<NodeInfo> &result);

ir::AstNode *ExtractExportSpecifierIdentifier(ir::AstNode *node, const NodeInfo *info);
ir::AstNode *ExtractTSClassImplementsIdentifier(ir::AstNode *node, const NodeInfo *info);
ir::AstNode *ExtractETSStringLiteralTypeIdentifier(ir::AstNode *node, const NodeInfo *info);
ir::AstNode *ExtractETSKeyofTypeIdentifier(ir::AstNode *node, const NodeInfo *info);
ir::AstNode *ExtractCallExpressionIdentifier(ir::AstNode *node, const NodeInfo *info);
ir::AstNode *ExtractAwaitExpressionIdentifier(ir::AstNode *node, [[maybe_unused]] const NodeInfo *info);
ir::AstNode *ExtractIdentifierFromNode(ir::AstNode *node, const NodeInfo *info);

const std::unordered_map<ir::AstNodeType, NodeExtractor> &GetNodeExtractors();
const std::unordered_map<ir::AstNodeType, NodeMatcher> &GetNodeMatchers();
const std::unordered_map<ir::AstNodeType, NodeInfoHandler> &GetNodeInfoHandlers();
}  // namespace ark::es2panda::lsp
#endif  // NODE_MATCHERS_H
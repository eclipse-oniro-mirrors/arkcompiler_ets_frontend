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

namespace ark::es2panda::lsp {
using NodeMatcher = std::function<bool(ir::AstNode *, const NodeInfo *)>;
using NodeExtractor = ir::AstNode *(*)(ir::AstNode *, const NodeInfo *);

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

ir::AstNode *ExtractExportSpecifierIdentifier(ir::AstNode *node, const NodeInfo *info);
ir::AstNode *ExtractTSClassImplementsIdentifier(ir::AstNode *node, const NodeInfo *info);
ir::AstNode *ExtractIdentifierFromNode(ir::AstNode *node, const NodeInfo *info);
const std::unordered_map<ir::AstNodeType, NodeExtractor> &GetNodeExtractors();
const std::unordered_map<ir::AstNodeType, NodeMatcher> &GetNodeMatchers();
}  // namespace ark::es2panda::lsp
#endif  // NODE_MATCHERS_H
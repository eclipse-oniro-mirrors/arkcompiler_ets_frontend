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

#ifndef ES2PANDA_LSP_GET_ADJUSTED_LOCATION_H
#define ES2PANDA_LSP_GET_ADJUSTED_LOCATION_H

#include <optional>
#include <string>
#include <vector>
#include "ir/astNode.h"
#include "ir/astNodeFlags.h"
#include "es2panda.h"
#include "public/es2panda_lib.h"

namespace ark::es2panda::lsp {

// Main location adjustment functions
std::optional<ir::AstNode *> GetAdjustedLocation(ir::AstNode *node, ArenaAllocator *allocator);
std::optional<ir::AstNode *> GetAdjustedLocationForClass(ir::AstNode *node, ArenaAllocator *allocator);
std::optional<ir::AstNode *> GetAdjustedLocationForFunction(ir::AstNode *node, ArenaAllocator *allocator);
std::optional<ir::AstNode *> GetAdjustedLocationForDeclaration(ir::AstNode *node,
                                                               const std::vector<ir::AstNode *> &children,
                                                               ArenaAllocator *allocator);
std::optional<ir::AstNode *> GetAdjustedLocationForImportDeclaration(ir::AstNode *node,
                                                                     const std::vector<ir::AstNode *> &children);
std::optional<ir::AstNode *> GetAdjustedLocationForExportDeclaration(ir::AstNode *node,
                                                                     const std::vector<ir::AstNode *> &children);
std::optional<ir::AstNode *> GetAdjustedLocationForHeritageClause(ir::AstNode *node);
ir::AstNode *GetTouchingPropertyName(es2panda_Context *context, size_t pos);
ir::AstNode *GetTouchingIdentifierName(es2panda_Context *context, size_t pos);

// Node finding functions
ir::AstNode *FindFirstIdentifier(ir::AstNode *node, bool skipModifiers, const std::vector<ir::AstNode *> &children);
ir::AstNode *FindFirstExpression(ir::AstNode *node, const std::vector<ir::AstNode *> &children);
ir::AstNode *FindFirstExpressionAfter(ir::AstNode *node, ir::AstNode *after,
                                      const std::vector<ir::AstNode *> &children);
ir::AstNode *FindNodeOfType(ir::AstNode *node, ir::AstNodeType type, const std::vector<ir::AstNode *> &children);
ir::AstNode *FindTypeReference(ir::AstNode *node, const std::vector<ir::AstNode *> &children);
ir::AstNode *FindTypeParameter(ir::AstNode *node, const std::vector<ir::AstNode *> &children);
ir::AstNode *FindArrayType(ir::AstNode *node, const std::vector<ir::AstNode *> &children);

// Node property checkers
bool IsModifier(const ir::AstNode *node);
bool CanHaveModifiers(const ir::AstNode &node);
bool IsOuterExpression(const ir::AstNode *node);
bool IsDeclarationOrModifier(ir::AstNode *node, ir::AstNode *parent);

// Node manipulation
ir::AstNode *SkipOuterExpressions(ir::AstNode *node);

// Children collection
std::vector<ir::AstNode *> GetChildren(ir::AstNode *node, ArenaAllocator *allocator);

}  // namespace ark::es2panda::lsp
#endif  // ES2PANDA_LSP_GET_ADJUSTED_LOCATION_H

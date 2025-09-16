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

#ifndef ES2PANDA_LSP_INCLUDE_QUICK_INFO_H
#define ES2PANDA_LSP_INCLUDE_QUICK_INFO_H

#include "ir/astNode.h"
#include "public/es2panda_lib.h"
#include "api.h"
#include "types.h"

namespace ark::es2panda::lsp {

bool IsIncludedToken(const ir::AstNode *node);
ir::AstNode *GetTokenForQuickInfo(es2panda_Context *context, size_t position);
bool IsObjectLiteralElement(ir::AstNode *node);
ir::AstNode *GetContainingObjectLiteralNode(ir::AstNode *node);
ir::AstNode *GetContextualTypeNode(ir::AstNode *node);
ir::AstNode *GetPropertyNodeFromContextualType(ir::AstNode *node, ir::AstNode *contextualTypeNode);
ir::AstNode *GetNodeAtLocationForQuickInfo(ir::AstNode *node);
std::vector<SymbolDisplayPart> CreateDisplayForClass(ir::AstNode *node);
std::vector<SymbolDisplayPart> CreateDisplayForInterface(ir::AstNode *node);
std::vector<SymbolDisplayPart> CreateDisplayForTypeAlias(ir::AstNode *node);
std::vector<SymbolDisplayPart> CreateDisplayForEnum(ir::AstNode *node);
std::vector<SymbolDisplayPart> CreateDisplayForEnumMember(ir::AstNode *node);
std::vector<SymbolDisplayPart> CreateDisplayForTypeParameter(ir::AstNode *node);
std::vector<SymbolDisplayPart> CreateDisplayForMethodDefinition(ir::AstNode *node, const std::string &kindModifier,
                                                                checker::ETSChecker *checker);
std::vector<SymbolDisplayPart> CreateDisplayForClassProperty(ir::AstNode *node);
std::vector<SymbolDisplayPart> CreateDisplayForETSParameterExpression(ir::AstNode *node);
QuickInfo GetQuickInfoAtPositionImpl(es2panda_Context *context, size_t position, std::string fileName);
std::string GetNameForTypeReference(const ir::TypeNode *typeReference);
std::string GetNameForTypeNode(const ir::TypeNode *typeAnnotation);
std::vector<SymbolDisplayPart> CreateDisplayForImportDeclaration(ir::AstNode *node);
ir::AstNode *GetEnumMemberByName(ir::AstNode *node, const util::StringView &name);
std::string ModifiersToString(ir::ModifierFlags flags);
std::string GetKindModifiers(ir::AstNode *node);
bool IsClass(ir::AstNode *node);
ir::AstNode *GetContainerNode(ir::AstNode *node);

}  // namespace ark::es2panda::lsp

#endif
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

#ifndef ES2PANDA_LSP_INCLUDE_SUGGESTION_DIAGNOSTICS_H
#define ES2PANDA_LSP_INCLUDE_SUGGESTION_DIAGNOSTICS_H

#include "ir/astNode.h"
#include "public/es2panda_lib.h"
#include "public/public.h"
#include "api.h"
#include <iostream>
#include <vector>

namespace ark::es2panda::lsp {

std::vector<FileDiagnostic> GetSuggestionDiagnosticsImpl(ir::AstNode *astNode, es2panda_Context *context);
void Check(ir::AstNode *node, std::vector<FileDiagnostic> &diag, std::unordered_map<std::string, bool> &visitedFunc,
           es2panda_Context *context);
void AddConvertToAsyncFunctionDiagnostics(ir::AstNode *node, std::vector<FileDiagnostic> &diag,
                                          std::unordered_map<std::string, bool> &visitedFunc,
                                          es2panda_Context *context);
bool IsConvertibleFunction(ir::AstNode *node, std::unordered_map<std::string, bool> &visitedFunc);
bool HasReturnStatementWithPromiseHandler(ir::AstNode *node, std::unordered_map<std::string, bool> &visitedFunc);
bool IsReturnStatementWithFixablePromiseHandler(ir::AstNode *node, std::unordered_map<std::string, bool> &visitedFunc);
bool IsFixablePromiseHandler(ir::AstNode *node, std::unordered_map<std::string, bool> &visitedFunc);
bool IsPromiseHandler(ir::AstNode *node);
bool HasSupportedNumberOfArguments(ir::AstNode *node);
bool IsFixablePromiseArgument(ir::AstNode *node, std::unordered_map<std::string, bool> &visitedFunc);
std::string GetKeyFromNode(ir::AstNode *node);
bool CanBeConvertedToAsync(ir::AstNode *node);
bool HasPropertyAccessExpressionWithName(ir::AstNode *node, const std::string &func);

}  // namespace ark::es2panda::lsp

#endif

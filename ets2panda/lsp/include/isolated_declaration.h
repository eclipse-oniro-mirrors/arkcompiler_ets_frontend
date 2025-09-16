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

#ifndef ES2PANDA_LSP_INCLUDE_ISOLATED_DECLARATION_H
#define ES2PANDA_LSP_INCLUDE_ISOLATED_DECLARATION_H

#include "api.h"

namespace ark::es2panda::lsp {
std::optional<TextChange> ProcessIdentifier(ir::Identifier *identifier, checker::ETSChecker *checker,
                                            parser::Program *program);
std::string GetReturnTypeStr(const checker::Type *checkerType, checker::ETSChecker *checker);
const checker::Signature *GetFuncSignature(const checker::ETSFunctionType *etsFunctionType,
                                           const ir::MethodDefinition *methodDef);
}  // namespace ark::es2panda::lsp

#endif
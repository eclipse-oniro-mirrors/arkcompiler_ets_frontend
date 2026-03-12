/**
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef ES2PANDA_LSP_INCLUDE_SYMBOL_REFERENCE_INDEX_H
#define ES2PANDA_LSP_INCLUDE_SYMBOL_REFERENCE_INDEX_H

#include <cstddef>
#include <cstdint>
#include <string>
#include "api.h"
#include "public/es2panda_lib.h"

namespace ark::es2panda::lsp {

using SymbolId = uint64_t;

void InitSymbolReferenceIndex();
void ClearSymbolReferenceIndex();
bool BuildSymbolReferenceIndexForContext(es2panda_Context *context);
bool BuildSymbolReferenceIndexForContextWithExternal(es2panda_Context *context);
bool RemoveSymbolReferenceIndexForFile(const std::string &fileName);
References GetReferencesAtPositionFromIndex(es2panda_Context *context, size_t position);
std::string GetIndexedFileSource(const std::string &fileName);

}  // namespace ark::es2panda::lsp

#endif

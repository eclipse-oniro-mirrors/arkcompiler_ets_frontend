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

#ifndef ETS2PANDA_LSP_SAFE_DELETE_H
#define ETS2PANDA_LSP_SAFE_DELETE_H

#include "public/es2panda_lib.h"
#include "api.h"
#include <vector>
#include <string>

namespace ark::es2panda::lsp {

/**
 * Finds all reference locations for a given symbol.
 * @param ctx Pointer to the es2panda context.
 * @param declInfo Declaration information (typically includes file name and symbol name).
 * @return A list of reference locations.
 */
std::vector<SafeDeleteLocation> FindSafeDeleteLocationImpl(es2panda_Context *ctx, size_t position);

}  // namespace ark::es2panda::lsp

#endif  // ETS2PANDA_LSP_SAFE_DELETE_H

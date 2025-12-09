/**
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at*
 *
 * http://www.apache.org/licenses/LICENSE-2.0*
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef GETEDITSFORREFACTOR_H
#define GETEDITSFORREFACTOR_H

#include "public/es2panda_lib.h"
#include "refactors/refactor_types.h"
#include <string>
#include <string_view>
#include <vector>

namespace ark::es2panda::lsp {

std::unique_ptr<RefactorEditInfo> GetEditsForRefactorsImpl(const RefactorContext &context,
                                                           const std::string &refactorName,
                                                           const std::string &actionName);
}  // namespace ark::es2panda::lsp

#endif  // GETEDITSFORREFACTOR_H
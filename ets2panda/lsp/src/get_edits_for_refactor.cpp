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

#include <memory>
#include <unordered_map>
#include "refactors/refactor_types.h"
#include "applicable_refactors.h"
#include "lsp/include/refactor_provider.h"
#include "refactors/convert_function.h"

namespace ark::es2panda::lsp {

std::unique_ptr<RefactorEditInfo> GetEditsForRefactorsImpl(const RefactorContext &context,
                                                           const std::string &refactorName,
                                                           const std::string &actionName)
{
    return RefactorProvider::Instance().GetEditsForRefactor(context, refactorName, actionName);
}
}  // namespace ark::es2panda::lsp
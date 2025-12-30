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

#ifndef ES2PANDA_LSP_INCLUDE_REFERENCES_H
#define ES2PANDA_LSP_INCLUDE_REFERENCES_H

#include <tuple>
#include "api.h"
#include "ir/astNode.h"
#include "public/es2panda_lib.h"

namespace ark::es2panda::lsp {

using DeclInfoType = std::tuple<std::string, std::string>;

bool IsValidReference(ir::AstNode *astNode);
ReferenceInfo ResolveInfo(const std::tuple<std::string, std::string> &info);
DeclInfoType GetDeclInfoImpl(ir::AstNode *astNode);
template <typename T, typename Compare>
void RemoveDuplicates(std::vector<T> &vec, Compare comp)
{
    std::set<T, Compare> uniqueSet(vec.begin(), vec.end(), comp);
    vec.assign(uniqueSet.begin(), uniqueSet.end());
}
References GetReferencesAtPositionImpl(es2panda_Context *context, const DeclInfoType &declInfo);

}  // namespace ark::es2panda::lsp

#endif

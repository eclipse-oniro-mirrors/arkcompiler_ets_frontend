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

#ifndef LSP_UTILS_H
#define LSP_UTILS_H

#include <optional>
#include <string>
#include <vector>
#include "types.h"

namespace ark::es2panda::lsp {

size_t CodePointOffsetToByteOffset(const std::string &content, size_t charOffset);

size_t ByteOffsetToCodePointOffset(const std::string &content, size_t byteOffset);

std::string ApplyRefactorTextChangesToSource(const std::string &source, const std::vector<TextChange> &textChanges);

std::string GetRefactoredSourceForRenameLocation(const std::string &source,
                                                 const std::vector<FileTextChanges> &fileTextChanges,
                                                 const std::optional<std::string> &renameFileName);

}  // namespace ark::es2panda::lsp

#endif

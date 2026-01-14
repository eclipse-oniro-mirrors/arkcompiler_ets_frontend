/**
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

#ifndef EXTRACT_TYPES_H
#define EXTRACT_TYPES_H

#include <vector>
#include "refactor_types.h"

namespace ark::es2panda::lsp {

constexpr RefactorActionView EXTRACT_TYPE_ACTION {"extract_type", "Extract selected type to a type alias",
                                                  "refactor.extract.type"};
constexpr RefactorActionView EXTRACT_INTERFACE_ACTION {"extract_interface", "Extract selected type to an interface",
                                                       "refactor.extract.interface"};

class ExtractTypeRefactor : public Refactor {
public:
    ExtractTypeRefactor();
    std::vector<ApplicableRefactorInfo> GetAvailableActions(const RefactorContext &context) const override;
    std::unique_ptr<RefactorEditInfo> GetEditsForAction(const RefactorContext &context,
                                                        const std::string &actName) const override;
};

}  // namespace ark::es2panda::lsp

#endif  // EXTRACT_TYPES_H
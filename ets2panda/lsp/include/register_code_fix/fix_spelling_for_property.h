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

#ifndef FIX_SPELLING_FOR_PROPERTY_H
#define FIX_SPELLING_FOR_PROPERTY_H

#include <cstddef>
#include <string>
#include <vector>

#include "checker/types/ets/etsObjectType.h"
#include "lsp/include/code_fixes/code_fix_types.h"
#include "lsp/include/services/text_change/change_tracker.h"
#include "lsp/include/types.h"

namespace ark::es2panda::lsp {

class FixSpellingForProperty : public CodeFixRegistration {
public:
    FixSpellingForProperty();

    std::vector<CodeFixAction> GetCodeActions(const CodeFixContext &context) override;
    CombinedCodeActions GetAllCodeActions(const CodeFixAllContext &codeFixAll) override;

private:
    void MakeChangeForFixSpellingForProperty(ChangeTracker &changeTracker, es2panda_Context *context, size_t pos,
                                             const std::string &target);
    std::vector<FileTextChanges> GetCodeActionsToFixSpellingForProperty(const CodeFixContext &context,
                                                                        const std::string &target);
    static std::vector<std::string> GetPropertyCandidatesFromType(checker::ETSObjectType *objType,
                                                                  const std::string &misspelledName);
};

}  // namespace ark::es2panda::lsp

#endif

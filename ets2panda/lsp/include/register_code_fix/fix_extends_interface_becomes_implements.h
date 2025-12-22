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

#ifndef FIX_EXTENDS_INTERFACE_BECOMES_IMPLEMENTS_H
#define FIX_EXTENDS_INTERFACE_BECOMES_IMPLEMENTS_H

#include <cstddef>

#include "lsp/include/code_fixes/code_fix_types.h"
#include "lsp/include/services/text_change/change_tracker.h"
#include "lsp/include/types.h"

namespace ark::es2panda::lsp {

class FixExtendsInterfaceBecomesImplements : public CodeFixRegistration {
public:
    FixExtendsInterfaceBecomesImplements();

    std::vector<CodeFixAction> GetCodeActions(const CodeFixContext &context) override;
    CombinedCodeActions GetAllCodeActions(const CodeFixAllContext &codeFixAll) override;

private:
    void MakeChangeForExtendsInterfaceBecomesImplements(ChangeTracker &changeTracker, es2panda_Context *context,
                                                        size_t pos);
    std::vector<FileTextChanges> GetCodeActionsToExtendsInterfaceBecomesImplements(const CodeFixContext &context);
};

}  // namespace ark::es2panda::lsp
#endif

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

#ifndef ES2PANDA_IR_MODULE_AST_IMPORT_SPECIFIER_TYPE_ONLY_H
#define ES2PANDA_IR_MODULE_AST_IMPORT_SPECIFIER_TYPE_ONLY_H

#include "ir/module/importSpecifier.h"

namespace ark::es2panda::ir {

// Single entry point to the inline `type` modifier of an import/export specifier. It is not a public method of
// ImportSpecifier because ir/module/importSpecifier.h is scanned by the CAPI generator and this header is not; see the
// friend declaration in ImportSpecifier for the stale-object hazard that a new table entry causes.
class ImportSpecifierTypeOnly {
public:
    static bool Is(const ImportSpecifier *specifier)
    {
        return specifier != nullptr && specifier->isTypeOnly_;
    }

    static void Set(ImportSpecifier *specifier, bool isTypeOnly)
    {
        if (specifier != nullptr) {
            specifier->isTypeOnly_ = isTypeOnly;
        }
    }
};

}  // namespace ark::es2panda::ir

#endif

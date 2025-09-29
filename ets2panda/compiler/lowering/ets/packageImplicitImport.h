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

#ifndef ES2PANDA_COMPILER_LOWERING_ETS_PACKAGE_FILE_MERGE
#define ES2PANDA_COMPILER_LOWERING_ETS_PACKAGE_FILE_MERGE

#include "compiler/lowering/phase.h"

namespace ark::es2panda::compiler {

class PackageImplicitImport : public Phase {
public:
    std::string_view Name() const override
    {
        return "PackageImplicitImport";
    }

    bool Perform() override;
};

}  // namespace ark::es2panda::compiler

#endif  // ES2PANDA_COMPILER_LOWERING_ETS_PACKAGE_FILE_MERGE

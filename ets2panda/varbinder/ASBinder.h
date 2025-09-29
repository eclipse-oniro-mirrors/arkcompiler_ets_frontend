/**
 * Copyright (c) 2021-2026 Huawei Device Co., Ltd.
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

#ifndef ES2PANDA_VARBINDER_AS_VARBINDER_H
#define ES2PANDA_VARBINDER_AS_VARBINDER_H

#include "varbinder/varbinder.h"

namespace ark::es2panda::varbinder {
class ASBinder : public VarBinder {
public:
    using VarBinder::VarBinder;

    ASBinder() = delete;
    NO_COPY_SEMANTIC(ASBinder);
    NO_MOVE_SEMANTIC(ASBinder);
    ~ASBinder() override = default;

    [[nodiscard]] ScriptExtension Extension() const noexcept override
    {
        return ScriptExtension::AS;
    }

    [[nodiscard]] ResolveBindingOptions BindingOptions() const noexcept override
    {
        return ResolveBindingOptions::BINDINGS;
    }

    void IdentifierAnalysis() override {}
};
}  // namespace ark::es2panda::varbinder

#endif

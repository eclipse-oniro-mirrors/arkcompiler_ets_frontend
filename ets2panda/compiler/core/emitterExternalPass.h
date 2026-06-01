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

#ifndef ES2PANDA_COMPILER_CORE_EMITTER_EXTERNAL_PASS_H
#define ES2PANDA_COMPILER_CORE_EMITTER_EXTERNAL_PASS_H

#include <cstddef>

namespace ark::es2panda::compiler::detail {

static constexpr auto MAX_EXTERNAL_EMIT_PASSES = 2U;

template <typename Dependencies, typename TraverseExternalRecords>
size_t RunExternalEmitPasses(Dependencies *dependencies, TraverseExternalRecords traverseExternalRecords)
{
    dependencies->SetTrackExternalDeps(true);

    size_t passes = 0U;
    do {
        dependencies->UpdateLastToEmitSize();
        traverseExternalRecords();
        passes++;
    } while (dependencies->MaybeRetryExternalPass() && passes < MAX_EXTERNAL_EMIT_PASSES);

    return passes;
}

}  // namespace ark::es2panda::compiler::detail

#endif  // ES2PANDA_COMPILER_CORE_EMITTER_EXTERNAL_PASS_H

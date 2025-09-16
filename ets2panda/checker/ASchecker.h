/*
 * Copyright (c) 2021 - 2025 Huawei Device Co., Ltd.
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

#ifndef ES2PANDA_CHECKER_AS_CHECKER_H
#define ES2PANDA_CHECKER_AS_CHECKER_H

#include "checker/checker.h"

namespace ark::es2panda::checker {

class ASChecker : public Checker {
public:
    // NOLINTNEXTLINE(readability-redundant-member-init)
    explicit ASChecker([[maybe_unused]] ThreadSafeArenaAllocator *allocator, util::DiagnosticEngine &diagnosticEngine,
                       [[maybe_unused]] ArenaAllocator *programAllocator)
        : Checker(allocator, diagnosticEngine)
    {
    }

    bool StartChecker([[maybe_unused]] varbinder::VarBinder *varbinder,
                      [[maybe_unused]] const util::Options &options) override;
    Type *CheckTypeCached([[maybe_unused]] ir::Expression *expr) override
    {
        return nullptr;
    }

    void ResolveStructuredTypeMembers([[maybe_unused]] Type *type) override {}

    Type *GetTypeOfVariable([[maybe_unused]] varbinder::Variable *var) override
    {
        return nullptr;
    }
};

}  // namespace ark::es2panda::checker

#endif /* CHECKER_H */

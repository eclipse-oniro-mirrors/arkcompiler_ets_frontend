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

#include "refactor_provider.h"
#include "refactors/refactor_types.h"

namespace ark::es2panda::lsp {

void RefactorProvider::RegisterRefactor(const std::string &name, std::unique_ptr<Refactor> refactor)
{
    refactors_.emplace(name, std::move(refactor));
}

RefactorProvider &RefactorProvider::Instance()
{
    static RefactorProvider instance;
    return instance;
}

std::unique_ptr<RefactorEditInfo> RefactorProvider::GetEditsForRefactor(const RefactorContext &context,
                                                                        const std::string &refactorName,
                                                                        const std::string &actionName) const
{
    auto it = refactors_.find(refactorName);
    if (it == refactors_.end()) {
        return nullptr;
    }

    return it->second->GetEditsForAction(context, actionName);
}

std::vector<ApplicableRefactorInfo> RefactorProvider::GetApplicableRefactors(const RefactorContext &context) const
{
    std::vector<ApplicableRefactorInfo> applicable;

    for (const auto &[name, refactor] : refactors_) {
        auto result = refactor->GetAvailableActions(context);
        // NOLINTNEXTLINE(modernize-loop-convert)
        for (size_t i = 0; i < result.size(); i++) {
            if (!result[i].name.empty()) {
                applicable.push_back(result[i]);
            }
        }
    }
    return applicable;
}

const std::unordered_map<std::string, std::shared_ptr<Refactor>> &RefactorProvider::GetRefactors() const
{
    return refactors_;
}

}  // namespace ark::es2panda::lsp

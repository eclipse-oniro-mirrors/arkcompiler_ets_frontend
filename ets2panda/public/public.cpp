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

#include "public/public.h"
#include "compiler/lowering/phase.h"

namespace ark::es2panda::public_lib {

checker::Checker *Context::GetChecker() const
{
    return checkers_[compiler::GetPhaseManager()->GetCurrentMajor()];
}

checker::SemanticAnalyzer *Context::GetAnalyzer() const
{
    return analyzers_[compiler::GetPhaseManager()->GetCurrentMajor()];
}

void Context::MarkGenAbcForExternal(std::unordered_set<std::string> &genAbcList, public_lib::ExternalSource &extSources)
{
    size_t genCount = 0;
    std::unordered_set<std::string> genAbcListAbsolute;

    for (auto &path : genAbcList) {
        genAbcListAbsolute.insert(os::GetAbsolutePath(path));
    }
    for (auto &[_, extPrograms] : extSources) {
        (void)_;
        bool setFlag = false;
        for (auto *prog : extPrograms) {
            if (auto it = genAbcListAbsolute.find(prog->AbsoluteName().Mutf8()); it != genAbcListAbsolute.end()) {
                ++genCount;
                setFlag = true;
            }
        }
        if (!setFlag) {
            continue;
        }
        for (auto *prog : extPrograms) {
            prog->SetGenAbcForExternalSources();
        }
    }

    if (genCount != genAbcListAbsolute.size()) {
        diagnosticEngine->LogFatalError(diagnostic::SIMULTANEOUSLY_MARK_FAILED.Message());
    }
}

}  // namespace ark::es2panda::public_lib

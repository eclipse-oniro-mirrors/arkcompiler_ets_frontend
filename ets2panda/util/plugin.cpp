/**
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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
#include "plugin.h"
#include "libarkbase/os/library_loader.h"

namespace ark::es2panda::util {

std::string Plugin::FullNameForProcedure(std::string const &shortName)
{
    return std::string(name_.Utf8()) + "_" + shortName;
}

Plugin::Plugin(util::StringView const &name) : name_ {name}, err_ {0}, h_ {nullptr}
{
    std::string soName =
        os::library_loader::DYNAMIC_LIBRARY_PREFIX + std::string(name) + os::library_loader::DYNAMIC_LIBRARY_SUFFIX;
    if (auto loadRes = os::library_loader::Load(soName); loadRes.HasValue()) {
        h_ = std::move(loadRes.Value());
    } else {
        err_ = loadRes.Error();
        ok_ = false;
    }

    if (auto initRes = os::library_loader::ResolveSymbol(h_, FullNameForProcedure("Initialize")); initRes.HasValue()) {
        initialize_ = reinterpret_cast<void (*)()>(initRes.Value());
    }

    if (auto apRes = os::library_loader::ResolveSymbol(h_, FullNameForProcedure("AfterParse")); apRes.HasValue()) {
        afterParse_ = reinterpret_cast<void (*)(es2panda_Context *)>(apRes.Value());
    }

    if (auto abRes = os::library_loader::ResolveSymbol(h_, FullNameForProcedure("AfterBind")); abRes.HasValue()) {
        afterBind_ = reinterpret_cast<void (*)(es2panda_Context *)>(abRes.Value());
    }

    if (auto acRes = os::library_loader::ResolveSymbol(h_, FullNameForProcedure("AfterCheck")); acRes.HasValue()) {
        afterCheck_ = reinterpret_cast<void (*)(es2panda_Context *)>(acRes.Value());
    }

    if (auto alRes = os::library_loader::ResolveSymbol(h_, FullNameForProcedure("AfterLowerings")); alRes.HasValue()) {
        afterLowerings_ = reinterpret_cast<void (*)(es2panda_Context *)>(alRes.Value());
    }
}

}  // namespace ark::es2panda::util

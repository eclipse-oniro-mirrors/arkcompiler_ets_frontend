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
#ifndef ES2PANDA_UTIL_PLUGINS_H
#define ES2PANDA_UTIL_PLUGINS_H

#include "util/es2pandaMacros.h"
#include "libarkbase/os/library_loader.h"
#include "public/es2panda_lib.h"
#include "util/ustring.h"

namespace ark::es2panda::util {

class Plugin {
public:
    explicit Plugin(util::StringView const &name);
    ~Plugin() = default;

    NO_COPY_SEMANTIC(Plugin);
    DEFAULT_MOVE_SEMANTIC(Plugin);

    bool IsOk()
    {
        return ok_;
    }

    os::Error Error()
    {
        return err_;
    }

    void Initialize() const
    {
        if (initialize_ != nullptr) {
            initialize_();
        }
    }

    void AfterParse(es2panda_Context *context) const
    {
        if (afterParse_ != nullptr) {
            afterParse_(context);
        }
    }

    void AfterBind(es2panda_Context *context) const
    {
        if (afterBind_ != nullptr) {
            afterBind_(context);
        }
    }

    void AfterCheck(es2panda_Context *context) const
    {
        if (afterCheck_ != nullptr) {
            afterCheck_(context);
        }
    }

    void AfterLowerings(es2panda_Context *context) const
    {
        if (afterLowerings_ != nullptr) {
            afterLowerings_(context);
        }
    }

private:
    std::string FullNameForProcedure(std::string const &shortName);

    util::StringView name_;
    bool ok_ {true};
    os::Error err_;
    os::library_loader::LibraryHandle h_;

    void (*initialize_)() = nullptr;
    void (*afterParse_)(es2panda_Context *) = nullptr;
    void (*afterBind_)(es2panda_Context *) = nullptr;
    void (*afterCheck_)(es2panda_Context *) = nullptr;
    void (*afterLowerings_)(es2panda_Context *) = nullptr;
};

}  // namespace ark::es2panda::util

#endif

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

#include "util.h"
#include <algorithm>
#include <cstddef>
#include <functional>
#include <ostream>
#include <fstream>
#include <string>
#include "libarkbase/macros.h"

static es2panda_Impl *g_implPtr = nullptr;

es2panda_Impl *GetImpl()
{
    if (g_implPtr != nullptr) {
        return g_implPtr;
    }

    std::string soName = ark::os::library_loader::DYNAMIC_LIBRARY_PREFIX + std::string("es2panda-public") +
                         ark::os::library_loader::DYNAMIC_LIBRARY_SUFFIX;
    auto libraryRes = ark::os::library_loader::Load(soName);
    if (!libraryRes.HasValue()) {
        std::cout << "Error in load lib" << std::endl;
        return nullptr;
    }

    auto library = std::move(libraryRes.Value());
    auto getImpl = ark::os::library_loader::ResolveSymbol(library, "es2panda_GetImpl");
    if (!getImpl.HasValue()) {
        std::cout << "Error in load func get g_implPtr" << std::endl;
        return nullptr;
    }

    auto getImplFunc = reinterpret_cast<const es2panda_Impl *(*)(int)>(getImpl.Value());
    if (getImplFunc != nullptr) {
        g_implPtr = const_cast<es2panda_Impl *>(getImplFunc(ES2PANDA_LIB_VERSION));
        return g_implPtr;
    }
    return nullptr;
}

void CheckForErrors(const std::string &stateName, es2panda_Context *context)
{
    if (g_implPtr->ContextState(context) == ES2PANDA_STATE_ERROR) {
        std::cout << "PROCEED TO " << stateName << " ERROR" << std::endl;
        std::cout << g_implPtr->ContextErrorMessage(context) << std::endl;
    } else {
        std::cout << "PROCEED TO " << stateName << " SUCCESS" << std::endl;
    }
}

std::string GetDeclPrefix(const char *etsSrcName)
{
    if (etsSrcName == nullptr) {
        return "";
    }
    std::string_view etsSrc(etsSrcName);
    auto pos = etsSrc.find_last_of('/');
    if (pos != std::string_view::npos) {
        etsSrc = etsSrc.substr(pos + 1);
    }
    std::cout << "ETS SRC: " << etsSrc << std::endl;
    pos = etsSrc.find_last_of('.');
    if (pos != std::string_view::npos) {
        etsSrc = etsSrc.substr(0, pos);
    }
    std::cout << "ETS SRC PREFIX: " << etsSrc << std::endl;
    return std::string(etsSrc);
}
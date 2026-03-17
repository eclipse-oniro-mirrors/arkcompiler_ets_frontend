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

#include "common.h"
#include "dynamic-loader.h"

#include <iostream>

static es2panda_Impl const *g_impl = nullptr;

void *FindLibrary()
{
    void *library = LoadLibrary(G_LIB_ES2_PANDA_PUBLIC);
    if (library != nullptr) {
        return library;
    }

    return nullptr;
}

const es2panda_Impl *GetPublicImpl()
{
    if (g_impl != nullptr) {
        return g_impl;
    }
    auto library = FindLibrary();
    if (library == nullptr) {
        std::cout << "Cannot find " << G_LIB_ES2_PANDA_PUBLIC << std::endl;
    }
    auto symbol = FindSymbol(library, "es2panda_GetImpl");
    if (symbol == nullptr) {
        std::cout << "Cannot find es2panda_Impl Entry point" << std::endl;
    }
    g_impl = reinterpret_cast<es2panda_Impl *(*)(int)>(symbol)(ES2PANDA_LIB_VERSION);
    return g_impl;
}

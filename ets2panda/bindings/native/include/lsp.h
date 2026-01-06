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

#ifndef LSP_H
#define LSP_H

#include "common.h"
#include "lsp/include/api.h"

const LSPAPI *GetLspApiImpl();

static LSPAPI const *g_lspImpl = nullptr;

const LSPAPI *GetLspApiImpl()
{
    if (g_lspImpl != nullptr) {
        return g_lspImpl;
    }
    auto library = FindLibrary();
    if (library == nullptr) {
        std::cout << "Cannot find " << G_LIB_ES2_PANDA_PUBLIC << std::endl;
    }
    auto symbol = FindSymbol(library, "GetImpl");
    if (symbol == nullptr) {
        std::cout << "Cannot find Impl Entry point" << std::endl;
    }
    g_lspImpl = reinterpret_cast<LSPAPI *(*)(int)>(symbol)(ES2PANDA_LIB_VERSION);
    return g_lspImpl;
}

#endif  // LSP_H
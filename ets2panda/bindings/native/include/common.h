/**
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

#ifndef COMMON_H
#define COMMON_H

#include "common-interop.h"
#include "stdexcept"
#include <string>
#include <iostream>
#include <vector>
#include "public/es2panda_lib.h"
#include "dynamic-loader.h"

// NOLINTBEGIN
#ifdef _WIN32
#include <windows.h>
#define PLUGIN_DIR "windows_host_tools"
#define LIB_PREFIX "lib"
#define LIB_SUFFIX ".dll"
#else
#include <dlfcn.h>

#ifdef __x86_64__
#define PLUGIN_DIR "linux_host_tools"
#else
#define PLUGIN_DIR "linux_arm64_host_tools"
#endif

#define LIB_PREFIX "lib"
#define LIB_SUFFIX ".so"
#endif
// NOLINTEND

const es2panda_Impl *GetPublicImpl();
void *FindLibrary();

constexpr const char *G_LIB_ES2_PANDA_PUBLIC_OHOS = LIB_PREFIX "es2panda_public" LIB_SUFFIX;
constexpr const char *G_LIB_ES2_PANDA_PUBLIC = LIB_PREFIX "es2panda-public" LIB_SUFFIX;

// CC-OFFNXT(G.NAM.01) false positive
std::string GetString(KStringPtr ptr);

inline char *GetStringCopy(KStringPtr &ptr)
{
    return strdup(ptr.CStr());
}

inline KUInt UnpackUInt(const KByte *bytes);

// NOLINTBEGIN(fuchsia-statically-constructed-objects)
static std::string g_pandaLibPath;
// NOLINTEND(fuchsia-statically-constructed-objects)
#endif  // COMMON_H_

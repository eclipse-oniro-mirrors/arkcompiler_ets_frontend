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

#ifndef DYNAMIC_LOADER_H
#define DYNAMIC_LOADER_H

#include <string>

#ifdef _WIN32
#include <windows.h>

inline void *LoadLibrary(const std::string &libPath)
{
    return LoadLibraryA(libPath.c_str());
}

inline const char *LibraryError()
{
    static char error[256];
    return error;
}

inline void *FindSymbol(void *library, const char *name)
{
    return (void *)GetProcAddress(reinterpret_cast<HMODULE>(library), name);
}

inline std::string LibName(const char *lib)
{
    return std::string(lib) + ".dll";
}

#elif defined(__linux__) || defined(__APPLE__)
#include <dlfcn.h>

inline void *LoadLibrary(const std::string &libPath)
{
    void *handle = dlopen(libPath.c_str(), RTLD_LOCAL | RTLD_NOW);
    if (handle == nullptr) {
        return nullptr;
    }
    return handle;
}

// NOLINTNEXTLINE(modernize-redundant-void-arg)
inline const char *LibraryError(void)
{
    return dlerror();
}

// CC-OFFNXT(G.NAM.01) false positive
inline std::string SymbolName(const char *name)
{
    return name;
}

inline void *FindSymbol(void *library, const char *name)
{
    return dlsym(library, SymbolName(name).c_str());
}

// CC-OFFNXT(G.NAM.01) false positive
inline std::string LibName(const char *lib)
{
    std::string result;
    std::string suffix = ".so";
    result = "lib" + std::string(lib) + suffix;
    return result;
}

#else

#include <cstdio>

inline void *LoadLibrary(const std::string &libPath)
{
    return nullptr;
}

inline const char *LibraryError()
{
    return nullptr;
}

inline std::string SymbolName(const char *name)
{
    return "";
}

inline void *FindSymbol(void *library, const char *name)
{
    return nullptr;
}

inline std::string LibName(const char *lib)
{
    return "";
}

#endif

#endif  // _DYNAMIC_LOADER_H

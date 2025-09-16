/*
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

#include "common.h"
#include <string>
#include <iostream>
#include "public/es2panda_lib.h"
#include "dynamic-loader.h"

// NOLINTBEGIN

using std::string, std::cout, std::endl, std::vector;

static es2panda_Impl const *impl = nullptr;

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

const char *LIB_ES2PANDA_PUBLIC = LIB_PREFIX "es2panda_public" LIB_SUFFIX;

void *FindLibrary()
{
    std::string libraryName;
    char *envValue = getenv("PANDA_SDK_PATH");
    if (envValue) {
        libraryName = string(envValue) + ("/" PLUGIN_DIR "/lib/") + LIB_ES2PANDA_PUBLIC;
    } else {
        if (g_pandaLibPath == "") {
            libraryName = LIB_ES2PANDA_PUBLIC;
        } else {
            libraryName = g_pandaLibPath + "/" + LIB_ES2PANDA_PUBLIC;
        }
    }
    return LoadLibrary(libraryName);
}

const es2panda_Impl *GetPublicImpl()
{
    if (impl) {
        return impl;
    }
    auto library = FindLibrary();
    if (!library) {
        std::cout << "Cannot find " << LIB_ES2PANDA_PUBLIC << endl;
    }
    auto symbol = FindSymbol(library, "es2panda_GetImpl");
    if (!symbol) {
        std::cout << "Cannot find Impl Entry point" << endl;
    }
    impl = reinterpret_cast<es2panda_Impl *(*)(int)>(symbol)(ES2PANDA_LIB_VERSION);
    return impl;
}

std::string GetString(KStringPtr ptr)
{
    return ptr.data();
}

char *GetStringCopy(KStringPtr &ptr)
{
    return strdup(ptr.c_str());
}

inline KUInt UnpackUInt(const KByte *bytes)
{
    return (bytes[0] | (bytes[1] << 8U) | (bytes[2U] << 16U) | (bytes[3U] << 24U));
}

inline std::string_view GetStringView(KStringPtr &ptr)
{
    return std::string_view(ptr.c_str(), static_cast<size_t>(ptr.length()));
}

KNativePointer impl_CreateConfig(KInt argc, KStringArray argvPtr, KStringPtr &pandaLibPath)
{
    const std::size_t HEADER_LEN = 4;
    g_pandaLibPath = GetStringView(pandaLibPath);
    const char **argv = new const char *[static_cast<unsigned int>(argc)];
    std::size_t position = HEADER_LEN;
    std::size_t strLen;
    for (std::size_t i = 0; i < static_cast<std::size_t>(argc); ++i) {
        strLen = UnpackUInt(argvPtr + position);
        position += HEADER_LEN;
        argv[i] = strdup(std::string(reinterpret_cast<const char *>(argvPtr + position), strLen).c_str());
        position += strLen;
    }
    return GetPublicImpl()->CreateConfig(argc, argv);
}
TS_INTEROP_3(CreateConfig, KNativePointer, KInt, KStringArray, KStringPtr)

KNativePointer impl_DestroyConfig(KNativePointer configPtr)
{
    auto config = reinterpret_cast<es2panda_Config *>(configPtr);
    GetPublicImpl()->DestroyConfig(config);
    return nullptr;
}
TS_INTEROP_1(DestroyConfig, KNativePointer, KNativePointer)

KNativePointer impl_DestroyContext(KNativePointer contextPtr)
{
    auto context = reinterpret_cast<es2panda_Context *>(contextPtr);
    GetPublicImpl()->DestroyContext(context);
    return nullptr;
}
TS_INTEROP_1(DestroyContext, KNativePointer, KNativePointer)

void impl_MemInitialize(KStringPtr &pandaLibPath)
{
    g_pandaLibPath = GetStringView(pandaLibPath);
    GetPublicImpl()->MemInitialize();
}
TS_INTEROP_V1(MemInitialize, KStringPtr)

void impl_MemFinalize()
{
    GetPublicImpl()->MemFinalize();
}
TS_INTEROP_V0(MemFinalize)

KNativePointer impl_CreateGlobalContext(KNativePointer configPtr, KStringArray externalFileListPtr, KInt fileNum)
{
    auto config = reinterpret_cast<es2panda_Config *>(configPtr);

    const std::size_t headerLen = 4;
    if (fileNum <= 0) {
        return nullptr;
    }
    const char **externalFileList = new const char *[fileNum];
    std::size_t position = headerLen;
    std::size_t strLen;
    for (std::size_t i = 0; i < static_cast<std::size_t>(fileNum); ++i) {
        strLen = UnpackUInt(externalFileListPtr + position);
        position += headerLen;
        externalFileList[i] =
            strdup(std::string(reinterpret_cast<const char *>(externalFileListPtr + position), strLen).c_str());
        position += strLen;
    }

    return GetPublicImpl()->CreateGlobalContext(config, externalFileList, fileNum, true);
}
TS_INTEROP_3(CreateGlobalContext, KNativePointer, KNativePointer, KStringArray, KInt)

void impl_DestroyGlobalContext(KNativePointer globalContextPtr)
{
    auto context = reinterpret_cast<es2panda_GlobalContext *>(globalContextPtr);
    GetPublicImpl()->DestroyGlobalContext(context);
}
TS_INTEROP_V1(DestroyGlobalContext, KNativePointer)

KNativePointer impl_CreateCacheContextFromString(KNativePointer configPtr, KStringPtr &sourcePtr,
                                                 KStringPtr &filenamePtr, KNativePointer globalContext,
                                                 KBoolean isExternal)
{
    auto config = reinterpret_cast<es2panda_Config *>(configPtr);
    auto context = reinterpret_cast<es2panda_GlobalContext *>(globalContext);
    return GetPublicImpl()->CreateCacheContextFromString(config, sourcePtr.data(), filenamePtr.data(), context,
                                                         isExternal);
}
TS_INTEROP_5(CreateCacheContextFromString, KNativePointer, KNativePointer, KStringPtr, KStringPtr, KNativePointer,
             KBoolean)

void impl_RemoveFileCache(KNativePointer globalContextPtr, KStringPtr &filenamePtr)
{
    auto context = reinterpret_cast<es2panda_GlobalContext *>(globalContextPtr);
    return GetPublicImpl()->RemoveFileCache(context, filenamePtr.data());
}
TS_INTEROP_V2(RemoveFileCache, KNativePointer, KStringPtr)

void impl_AddFileCache(KNativePointer globalContextPtr, KStringPtr &filenamePtr)
{
    auto context = reinterpret_cast<es2panda_GlobalContext *>(globalContextPtr);
    return GetPublicImpl()->AddFileCache(context, filenamePtr.data());
}
TS_INTEROP_V2(AddFileCache, KNativePointer, KStringPtr)

void impl_InvalidateFileCache(KNativePointer globalContextPtr, KStringPtr &filenamePtr)
{
    auto context = reinterpret_cast<es2panda_GlobalContext *>(globalContextPtr);
    return GetPublicImpl()->InvalidateFileCache(context, filenamePtr.data());
}
TS_INTEROP_V2(InvalidateFileCache, KNativePointer, KStringPtr)
// NOLINTEND

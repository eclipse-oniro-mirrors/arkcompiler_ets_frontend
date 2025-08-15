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

using std::string, std::cout, std::endl, std::vector;

static es2panda_Impl const *g_impl = nullptr;

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

constexpr const char *G_LIB_ES2_PANDA_PUBLIC_OHOS = LIB_PREFIX "es2panda_public" LIB_SUFFIX;
constexpr const char *G_LIB_ES2_PANDA_PUBLIC = LIB_PREFIX "es2panda-public" LIB_SUFFIX;

void *FindLibrary()
{
    std::string basePath;
    char *envValue = getenv("PANDA_SDK_PATH");
    if (envValue) {
        basePath = std::string(envValue) + "/" + PLUGIN_DIR + "/lib/";
    } else {
        char *envBuildPath = getenv("BUILD_DIR");
        if (!g_pandaLibPath.empty()) {
            basePath = g_pandaLibPath + "/";
        } else if (envBuildPath) {
            basePath = std::string(envBuildPath) + "/lib/";
        } else {
            basePath = "";
        }
    }

    std::string libraryName = basePath + G_LIB_ES2_PANDA_PUBLIC_OHOS;
    void *library = LoadLibrary(libraryName);
    if (library != nullptr) {
        return library;
    }

    std::string altLibraryName = basePath + G_LIB_ES2_PANDA_PUBLIC;
    library = LoadLibrary(altLibraryName);
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
        std::cout << "Cannot find " << G_LIB_ES2_PANDA_PUBLIC << endl;
    }
    auto symbol = FindSymbol(library, "es2panda_GetImpl");
    if (symbol == nullptr) {
        std::cout << "Cannot find Impl Entry point" << endl;
    }
    g_impl = reinterpret_cast<es2panda_Impl *(*)(int)>(symbol)(ES2PANDA_LIB_VERSION);
    return g_impl;
}

std::string GetString(KStringPtr ptr)
{
    return ptr.Data();
}

// NOLINTBEGIN
inline KUInt UnpackUInt(const KByte *bytes)
{
    const KUInt oneByte = 8U;
    const KUInt twoByte = 16U;
    const KUInt threeByte = 24U;
    return (static_cast<KUInt>(bytes[0]) | (static_cast<KUInt>(bytes[1]) << oneByte) |
            (static_cast<KUInt>(bytes[twoByte / oneByte]) << twoByte) |
            (static_cast<KUInt>(bytes[threeByte / oneByte]) << threeByte));
}
// NOLINTEND

inline std::string_view GetStringView(KStringPtr &ptr)
{
    return std::string_view(ptr.CStr(), static_cast<size_t>(ptr.Length()));
}

KNativePointer impl_CreateConfig(KInt argc, KStringArray argvPtr, KStringPtr &pandaLibPath)
{
    const std::size_t headerLen = 4;
    g_pandaLibPath = GetStringView(pandaLibPath);

    const char **argv = new const char *[static_cast<unsigned int>(argc)];
    std::size_t position = headerLen;
    std::size_t strLen;
    for (std::size_t i = 0; i < static_cast<std::size_t>(argc); ++i) {
        strLen = UnpackUInt(argvPtr + position);
        position += headerLen;
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

void impl_MemInitialize()
{
    GetPublicImpl()->MemInitialize();
}
TS_INTEROP_V0(MemInitialize)

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
    return GetPublicImpl()->CreateCacheContextFromString(config, sourcePtr.Data(), filenamePtr.Data(), context,
                                                         isExternal);
}
TS_INTEROP_5(CreateCacheContextFromString, KNativePointer, KNativePointer, KStringPtr, KStringPtr, KNativePointer,
             KBoolean)

void impl_RemoveFileCache(KNativePointer globalContextPtr, KStringPtr &filenamePtr)
{
    auto context = reinterpret_cast<es2panda_GlobalContext *>(globalContextPtr);
    return GetPublicImpl()->RemoveFileCache(context, filenamePtr.Data());
}
TS_INTEROP_V2(RemoveFileCache, KNativePointer, KStringPtr)

void impl_AddFileCache(KNativePointer globalContextPtr, KStringPtr &filenamePtr)
{
    auto context = reinterpret_cast<es2panda_GlobalContext *>(globalContextPtr);
    return GetPublicImpl()->AddFileCache(context, filenamePtr.Data());
}
TS_INTEROP_V2(AddFileCache, KNativePointer, KStringPtr)

void impl_InvalidateFileCache(KNativePointer globalContextPtr, KStringPtr &filenamePtr)
{
    auto context = reinterpret_cast<es2panda_GlobalContext *>(globalContextPtr);
    return GetPublicImpl()->InvalidateFileCache(context, filenamePtr.Data());
}
TS_INTEROP_V2(InvalidateFileCache, KNativePointer, KStringPtr)

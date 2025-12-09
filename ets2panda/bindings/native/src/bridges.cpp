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

#include "public/es2panda_lib.h"

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

KNativePointer impl_ProceedToState(KNativePointer contextPtr, KInt state)
{
    auto context = reinterpret_cast<es2panda_Context *>(contextPtr);
    return GetPublicImpl()->ProceedToState(context, es2panda_ContextState(state));
}
TS_INTEROP_2(ProceedToState, KNativePointer, KNativePointer, KInt)

KNativePointer impl_ContextProgram(KNativePointer contextPtr)
{
    auto context = reinterpret_cast<es2panda_Context *>(contextPtr);
    return GetPublicImpl()->ContextProgram(context);
}
TS_INTEROP_1(ContextProgram, KNativePointer, KNativePointer)

KNativePointer impl_CreateCacheContextFromFile(KNativePointer configPtr, KStringPtr &sourceFileName,
                                               KNativePointer globalContext, KBoolean isExternal)
{
    auto config = reinterpret_cast<es2panda_Config *>(configPtr);
    auto context = reinterpret_cast<es2panda_GlobalContext *>(globalContext);
    return GetPublicImpl()->CreateCacheContextFromFile(config, sourceFileName.Data(), context, isExternal != 0);
}
TS_INTEROP_4(CreateCacheContextFromFile, KNativePointer, KNativePointer, KStringPtr, KNativePointer, KBoolean)

KNativePointer impl_CreateContextFromString(KNativePointer configPtr, KStringPtr &sourcePtr, KStringPtr &filenamePtr)
{
    auto config = reinterpret_cast<es2panda_Config *>(configPtr);
    return GetPublicImpl()->CreateContextFromString(config, sourcePtr.Data(), filenamePtr.Data());
}
TS_INTEROP_3(CreateContextFromString, KNativePointer, KNativePointer, KStringPtr, KStringPtr)

KNativePointer impl_CreateContextFromStringWithHistory(KNativePointer configPtr, KStringPtr &sourcePtr,
                                                       KStringPtr &filenamePtr)
{
    auto config = reinterpret_cast<es2panda_Config *>(configPtr);
    return GetPublicImpl()->CreateContextFromStringWithHistory(config, sourcePtr.Data(), filenamePtr.Data());
}
TS_INTEROP_3(CreateContextFromStringWithHistory, KNativePointer, KNativePointer, KStringPtr, KStringPtr)

KInt impl_GenerateTsDeclarationsFromContext(KNativePointer contextPtr, KStringPtr &outputDeclEts, KStringPtr &outputEts,
                                            KBoolean exportAll, KBoolean isolated, KStringPtr &recordFile,
                                            KBoolean genAnnotations)
{
    auto context = reinterpret_cast<es2panda_Context *>(contextPtr);
    return static_cast<KInt>(GetPublicImpl()->GenerateTsDeclarationsFromContext(
        context, outputDeclEts.Data(), outputEts.Data(), exportAll != 0, isolated != 0, recordFile.Data(),
        genAnnotations != 0));
}
TS_INTEROP_7(GenerateTsDeclarationsFromContext, KInt, KNativePointer, KStringPtr, KStringPtr, KBoolean, KBoolean,
             KStringPtr, KBoolean)

KNativePointer impl_CreateContextGenerateAbcForExternalSourceFiles(KNativePointer configPtr, KInt fileNamesCount,
                                                                   KStringArray filenames)
{
    auto config = reinterpret_cast<es2panda_Config *>(configPtr);
    const std::size_t headerLen = 4;
    if (fileNamesCount <= 0) {
        return nullptr;
    }
    const char **externalFileList = new const char *[fileNamesCount];
    std::size_t position = headerLen;
    std::size_t strLen;
    for (std::size_t i = 0; i < static_cast<std::size_t>(fileNamesCount); ++i) {
        strLen = UnpackUInt(filenames + position);
        position += headerLen;
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        externalFileList[i] = strdup(std::string(reinterpret_cast<const char *>(filenames + position), strLen).c_str());
        position += strLen;
    }
    return GetPublicImpl()->CreateContextGenerateAbcForExternalSourceFiles(config, fileNamesCount, externalFileList);
}
TS_INTEROP_3(CreateContextGenerateAbcForExternalSourceFiles, KNativePointer, KNativePointer, KInt, KStringArray)

KInt impl_GenerateStaticDeclarationsFromContext(KNativePointer contextPtr, KStringPtr &outputPath)
{
    auto context = reinterpret_cast<es2panda_Context *>(contextPtr);
    return static_cast<KInt>(
        GetPublicImpl()->GenerateStaticDeclarationsFromContext(context, GetStringCopy(outputPath)));
}
TS_INTEROP_2(GenerateStaticDeclarationsFromContext, KInt, KNativePointer, KStringPtr)

KNativePointer impl_CreateContextFromFile(KNativePointer configPtr, KStringPtr &filenamePtr)
{
    auto config = reinterpret_cast<es2panda_Config *>(configPtr);
    return GetPublicImpl()->CreateContextFromFile(config, GetStringCopy(filenamePtr));
}
TS_INTEROP_2(CreateContextFromFile, KNativePointer, KNativePointer, KStringPtr)

KInt impl_ContextState(KNativePointer contextPtr)
{
    auto context = reinterpret_cast<es2panda_Context *>(contextPtr);
    return static_cast<KInt>(GetPublicImpl()->ContextState(context));
}
TS_INTEROP_1(ContextState, KInt, KNativePointer)

KNativePointer impl_ContextErrorMessage(KNativePointer contextPtr)
{
    auto context = reinterpret_cast<es2panda_Context *>(contextPtr);
    return new std::string(GetPublicImpl()->ContextErrorMessage(context));
}
TS_INTEROP_1(ContextErrorMessage, KNativePointer, KNativePointer)

KNativePointer impl_GetAllErrorMessages(KNativePointer contextPtr)
{
    auto context = reinterpret_cast<es2panda_Context *>(contextPtr);
    return new std::string(GetPublicImpl()->GetAllErrorMessages(context));
}
TS_INTEROP_1(GetAllErrorMessages, KNativePointer, KNativePointer)

KNativePointer impl_ProgramAst(KNativePointer contextPtr, KNativePointer programPtr)
{
    auto context = reinterpret_cast<es2panda_Context *>(contextPtr);
    auto program = reinterpret_cast<es2panda_Program *>(programPtr);
    return GetPublicImpl()->ProgramAst(context, program);
}
TS_INTEROP_2(ProgramAst, KNativePointer, KNativePointer, KNativePointer)

KNativePointer impl_FreeCompilerPartMemory(KNativePointer contextPtr)
{
    auto context = reinterpret_cast<es2panda_Context *>(contextPtr);
    GetPublicImpl()->FreeCompilerPartMemory(context);
    return nullptr;
}
TS_INTEROP_1(FreeCompilerPartMemory, KNativePointer, KNativePointer)

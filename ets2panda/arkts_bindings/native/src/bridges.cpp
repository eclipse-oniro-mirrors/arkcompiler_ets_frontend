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
#include "converters-ani.h"
#include "ets-types.h"

#include "public/es2panda_lib.h"

// CC-OFFNXT(G.NAM.03-CPP) project code style
EtsNativePointer impl_CreateConfig(EtsInt argc, EtsStringArray argv)
{
    return GetPublicImpl()->CreateConfig(argc, argv);
}
ETS_INTEROP_2(CreateConfig, EtsNativePointer, EtsInt, EtsStringArray)

// CC-OFFNXT(G.NAM.03-CPP) project code style
EtsNativePointer impl_DestroyConfig(EtsNativePointer configPtr)
{
    auto config = reinterpret_cast<es2panda_Config *>(configPtr);
    GetPublicImpl()->DestroyConfig(config);
    return nullptr;
}
ETS_INTEROP_1(DestroyConfig, EtsNativePointer, EtsNativePointer)

// CC-OFFNXT(G.NAM.03-CPP) project code style
EtsNativePointer impl_DestroyConfigWithoutLog(EtsNativePointer configPtr)
{
    auto config = reinterpret_cast<es2panda_Config *>(configPtr);
    GetPublicImpl()->DestroyConfigWithoutLog(config);
    return nullptr;
}
ETS_INTEROP_1(DestroyConfigWithoutLog, EtsNativePointer, EtsNativePointer)

// CC-OFFNXT(G.NAM.03-CPP) project code style
void impl_MemInitialize()
{
    GetPublicImpl()->MemInitialize();
}
ETS_INTEROP_V0(MemInitialize)

// CC-OFFNXT(G.NAM.03-CPP) project code style
void impl_MemFinalize()
{
    GetPublicImpl()->MemFinalize();
}
ETS_INTEROP_V0(MemFinalize)

// CC-OFFNXT(G.NAM.03-CPP) project code style
EtsNativePointer impl_CreateGlobalContext(EtsNativePointer configPtr, EtsStringArray externalFileList, EtsInt fileNum)
{
    auto config = reinterpret_cast<es2panda_Config *>(configPtr);
    return GetPublicImpl()->CreateGlobalContext(config, const_cast<const char **>(externalFileList), fileNum, true);
}
ETS_INTEROP_3(CreateGlobalContext, EtsNativePointer, EtsNativePointer, EtsStringArray, EtsInt)

// CC-OFFNXT(G.NAM.03-CPP) project code style
void impl_DestroyGlobalContext(EtsNativePointer globalContextPtr)
{
    auto context = reinterpret_cast<es2panda_GlobalContext *>(globalContextPtr);
    GetPublicImpl()->DestroyGlobalContext(context);
}
ETS_INTEROP_V1(DestroyGlobalContext, EtsNativePointer)

// CC-OFFNXT(G.NAM.03-CPP) project code style
EtsNativePointer impl_CreateContextFromString(EtsNativePointer configPtr, EtsStringPtr &sourcePtr,
                                              EtsStringPtr &filenamePtr)
{
    auto config = reinterpret_cast<es2panda_Config *>(configPtr);
    return GetPublicImpl()->CreateContextFromString(config, sourcePtr.Data(), filenamePtr.Data());
}
ETS_INTEROP_3(CreateContextFromString, EtsNativePointer, EtsNativePointer, EtsStringPtr, EtsStringPtr)

// CC-OFFNXT(G.NAM.03-CPP) project code style
EtsNativePointer impl_CreateContextFromStringWithHistory(EtsNativePointer configPtr, EtsStringPtr &sourcePtr,
                                                         EtsStringPtr &filenamePtr)
{
    auto config = reinterpret_cast<es2panda_Config *>(configPtr);
    return GetPublicImpl()->CreateContextFromStringWithHistory(config, sourcePtr.Data(), filenamePtr.Data());
}
ETS_INTEROP_3(CreateContextFromStringWithHistory, EtsNativePointer, EtsNativePointer, EtsStringPtr, EtsStringPtr)

// CC-OFFNXT(G.NAM.03-CPP) project code style
EtsNativePointer impl_CreateContextFromFile(EtsNativePointer configPtr, EtsStringPtr &filenamePtr)
{
    auto config = reinterpret_cast<es2panda_Config *>(configPtr);
    return GetPublicImpl()->CreateContextFromFile(config, filenamePtr.Data());
}
ETS_INTEROP_2(CreateContextFromFile, EtsNativePointer, EtsNativePointer, EtsStringPtr)

// CC-OFFNXT(G.FUN.01-CPP) solid logic
// CC-OFFNXT(G.NAM.03-CPP) project code style
EtsNativePointer impl_CreateCacheContextFromString(EtsNativePointer configPtr, EtsStringPtr &sourcePtr,
                                                   EtsStringPtr &filenamePtr, EtsNativePointer globalContext,
                                                   EtsBoolean isExternal, EtsBoolean isLspUsage)
{
    auto config = reinterpret_cast<es2panda_Config *>(configPtr);
    auto context = reinterpret_cast<es2panda_GlobalContext *>(globalContext);
    return GetPublicImpl()->CreateCacheContextFromString(config, sourcePtr.Data(), filenamePtr.Data(), context,
                                                         isExternal, isLspUsage);
}
ETS_INTEROP_6(CreateCacheContextFromString, EtsNativePointer, EtsNativePointer, EtsStringPtr, EtsStringPtr,
              EtsNativePointer, EtsBoolean, EtsBoolean)

// CC-OFFNXT(G.NAM.03-CPP) project code style
EtsNativePointer impl_CreateCacheContextFromFile(EtsNativePointer configPtr, EtsStringPtr &sourceFileName,
                                                 EtsNativePointer globalContext, EtsBoolean isExternal)
{
    auto config = reinterpret_cast<es2panda_Config *>(configPtr);
    auto context = reinterpret_cast<es2panda_GlobalContext *>(globalContext);
    return GetPublicImpl()->CreateCacheContextFromFile(config, sourceFileName.Data(), context, isExternal != 0);
}
ETS_INTEROP_4(CreateCacheContextFromFile, EtsNativePointer, EtsNativePointer, EtsStringPtr, EtsNativePointer,
              EtsBoolean)

// CC-OFFNXT(G.NAM.03-CPP) project code style
EtsNativePointer impl_CreateContextGenerateAbcForExternalSourceFiles(EtsNativePointer configPtr, EtsInt fileNamesCount,
                                                                     EtsStringArray externalFileList)
{
    auto config = reinterpret_cast<es2panda_Config *>(configPtr);
    if (fileNamesCount <= 0) {
        return nullptr;
    }
    return GetPublicImpl()->CreateContextGenerateAbcForExternalSourceFiles(config, fileNamesCount, externalFileList);
}
ETS_INTEROP_3(CreateContextGenerateAbcForExternalSourceFiles, EtsNativePointer, EtsNativePointer, EtsInt,
              EtsStringArray)

// CC-OFFNXT(G.NAM.03-CPP) project code style
EtsNativePointer impl_DestroyContext(EtsNativePointer contextPtr)
{
    auto context = reinterpret_cast<es2panda_Context *>(contextPtr);
    GetPublicImpl()->DestroyContext(context);
    return nullptr;
}
ETS_INTEROP_1(DestroyContext, EtsNativePointer, EtsNativePointer)

// CC-OFFNXT(G.NAM.03-CPP) project code style
EtsInt impl_ContextState(EtsNativePointer contextPtr)
{
    auto context = reinterpret_cast<es2panda_Context *>(contextPtr);
    return static_cast<EtsInt>(GetPublicImpl()->ContextState(context));
}
ETS_INTEROP_1(ContextState, EtsInt, EtsNativePointer)

// CC-OFFNXT(G.NAM.03-CPP) project code style
EtsCString impl_ContextErrorMessage(EtsNativePointer contextPtr)
{
    auto context = reinterpret_cast<es2panda_Context *>(contextPtr);
    return GetPublicImpl()->ContextErrorMessage(context);
}
ETS_INTEROP_1(ContextErrorMessage, EtsCString, EtsNativePointer)

// CC-OFFNXT(G.NAM.03-CPP) project code style
EtsCString impl_GetAllErrorMessages(EtsNativePointer contextPtr)
{
    auto context = reinterpret_cast<es2panda_Context *>(contextPtr);
    return GetPublicImpl()->GetAllErrorMessages(context);
}
ETS_INTEROP_1(GetAllErrorMessages, EtsCString, EtsNativePointer)

// CC-OFFNXT(G.NAM.03-CPP) project code style
EtsNativePointer impl_ProceedToState(EtsNativePointer contextPtr, EtsInt state)
{
    auto context = reinterpret_cast<es2panda_Context *>(contextPtr);
    return GetPublicImpl()->ProceedToState(context, es2panda_ContextState(state));
}
ETS_INTEROP_2(ProceedToState, EtsNativePointer, EtsNativePointer, EtsInt)

// CC-OFFNXT(G.FUN.01-CPP) solid logic
// CC-OFFNXT(G.NAM.03-CPP) project code style
EtsInt impl_GenerateTsDeclarationsFromContext(EtsNativePointer contextPtr, EtsStringPtr &outputDeclEts,
                                              EtsStringPtr &outputEts, EtsBoolean exportAll, EtsBoolean isolated,
                                              EtsStringPtr &recordFile, EtsBoolean genAnnotations)
{
    auto context = reinterpret_cast<es2panda_Context *>(contextPtr);
    return static_cast<EtsInt>(GetPublicImpl()->GenerateTsDeclarationsFromContext(
        context, outputDeclEts.Data(), outputEts.Data(), exportAll, isolated, recordFile.Data(), genAnnotations));
}
ETS_INTEROP_7(GenerateTsDeclarationsFromContext, EtsInt, EtsNativePointer, EtsStringPtr, EtsStringPtr, EtsBoolean,
              EtsBoolean, EtsStringPtr, EtsBoolean)

// CC-OFFNXT(G.NAM.03-CPP) project code style
EtsInt impl_GenerateStaticDeclarationsFromContext(EtsNativePointer contextPtr, EtsStringPtr &outputPath)
{
    auto context = reinterpret_cast<es2panda_Context *>(contextPtr);
    return static_cast<EtsInt>(GetPublicImpl()->GenerateStaticDeclarationsFromContext(context, outputPath.Data()));
}
ETS_INTEROP_2(GenerateStaticDeclarationsFromContext, EtsInt, EtsNativePointer, EtsStringPtr)

// CC-OFFNXT(G.NAM.03-CPP) project code style
EtsNativePointer impl_ContextProgram(EtsNativePointer contextPtr)
{
    auto context = reinterpret_cast<es2panda_Context *>(contextPtr);
    return GetPublicImpl()->ContextProgram(context);
}
ETS_INTEROP_1(ContextProgram, EtsNativePointer, EtsNativePointer)

// CC-OFFNXT(G.NAM.03-CPP) project code style
EtsNativePointer impl_ProgramAst(EtsNativePointer contextPtr, EtsNativePointer programPtr)
{
    auto context = reinterpret_cast<es2panda_Context *>(contextPtr);
    auto program = reinterpret_cast<es2panda_Program *>(programPtr);
    return GetPublicImpl()->ProgramAst(context, program);
}
ETS_INTEROP_2(ProgramAst, EtsNativePointer, EtsNativePointer, EtsNativePointer)

// CC-OFFNXT(G.NAM.03-CPP) project code style
EtsNativePointer impl_FreeCompilerPartMemory(EtsNativePointer contextPtr)
{
    auto context = reinterpret_cast<es2panda_Context *>(contextPtr);
    GetPublicImpl()->FreeCompilerPartMemory(context);
    return nullptr;
}
ETS_INTEROP_1(FreeCompilerPartMemory, EtsNativePointer, EtsNativePointer)

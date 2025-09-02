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

KNativePointer impl_CreateContextFromString(KNativePointer configPtr, KStringPtr &sourcePtr, KStringPtr &filenamePtr)
{
    auto config = reinterpret_cast<es2panda_Config *>(configPtr);
    return GetPublicImpl()->CreateContextFromString(config, sourcePtr.data(), filenamePtr.data());
}
TS_INTEROP_3(CreateContextFromString, KNativePointer, KNativePointer, KStringPtr, KStringPtr)

KNativePointer impl_CreateContextFromStringWithHistory(KNativePointer configPtr, KStringPtr &sourcePtr,
                                                       KStringPtr &filenamePtr)
{
    auto config = reinterpret_cast<es2panda_Config *>(configPtr);
    return GetPublicImpl()->CreateContextFromStringWithHistory(config, sourcePtr.data(), filenamePtr.data());
}
TS_INTEROP_3(CreateContextFromStringWithHistory, KNativePointer, KNativePointer, KStringPtr, KStringPtr)

KInt impl_GenerateTsDeclarationsFromContext(KNativePointer contextPtr, KStringPtr &outputDeclEts, KStringPtr &outputEts,
                                            KBoolean exportAll, KBoolean isolated, KStringPtr &recordFile)
{
    auto context = reinterpret_cast<es2panda_Context *>(contextPtr);
    return static_cast<KInt>(GetPublicImpl()->GenerateTsDeclarationsFromContext(
        context, outputDeclEts.data(), outputEts.data(), exportAll != 0, isolated != 0, recordFile.data()));
}
TS_INTEROP_6(GenerateTsDeclarationsFromContext, KInt, KNativePointer, KStringPtr, KStringPtr, KBoolean, KBoolean,
             KStringPtr)

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

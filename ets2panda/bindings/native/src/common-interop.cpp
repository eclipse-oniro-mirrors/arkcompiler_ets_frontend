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

#include <cstddef>
#include <string>
#include <variant>
#include <vector>
#include <map>
#include <securec.h>

#ifdef TS_INTEROP_MODULE
#undef TS_INTEROP_MODULE
#endif

#define TS_INTEROP_MODULE InteropNativeModule
#include "interop-logging.h"
#include "convertors-napi.h"
#include "common-interop.h"

// NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
#if TS_INTEROP_PROFILER
#include "profiler.h"

InteropProfiler *InteropProfiler::_instance = nullptr;

#endif

using std::string;

// Callback dispatcher MOVED to convertors-napi.cc.
// Let's keep platform-specific parts of the code together

using HoldT = void (*)(KInt);

KInt impl_getTypeOfVariant(KNativePointer varPtr)
{
    auto *var = reinterpret_cast<std::variant<int, std::string> *>(varPtr);
    if (std::get_if<int>(var) != nullptr) {
        return 0;
    }
    return 1;
}
TS_INTEROP_1(getTypeOfVariant, KInt, KNativePointer)

KNativePointer impl_GetStringFromVariant(KNativePointer varPtr)
{
    auto *var = reinterpret_cast<std::variant<int, std::string> *>(varPtr);
    auto *res = new std::string(*std::get_if<std::string>(var));
    return res;
}
TS_INTEROP_1(GetStringFromVariant, KNativePointer, KNativePointer)

KInt impl_GetIntFromVariant(KNativePointer varPtr)
{
    auto *var = reinterpret_cast<std::variant<int, std::string> *>(varPtr);
    auto res = *std::get_if<int>(var);
    return res;
}
TS_INTEROP_1(GetIntFromVariant, KInt, KNativePointer)

KInteropBuffer impl_MaterializeBuffer(KNativePointer data, KLong length, KInt resourceId, KNativePointer holdPtr,
                                      KNativePointer releasePtr)
{
    auto hold = reinterpret_cast<void (*)(KInt)>(holdPtr);
    auto release = reinterpret_cast<void (*)(KInt)>(releasePtr);
    hold(resourceId);
    return KInteropBuffer {length, data, resourceId, release};
}
TS_INTEROP_5(MaterializeBuffer, KInteropBuffer, KNativePointer, KLong, KInt, KNativePointer, KNativePointer)

KNativePointer impl_GetNativeBufferPointer(KInteropBuffer buffer)
{
    return buffer.data;
}
TS_INTEROP_1(GetNativeBufferPointer, KNativePointer, KInteropBuffer)

KInt impl_StringLength(KNativePointer ptr)
{
    auto *s = reinterpret_cast<string *>(ptr);
    return s->length();
}
TS_INTEROP_1(StringLength, KInt, KNativePointer)

void impl_StringData(KNativePointer ptr, KByte *bytes, KUInt size)
{
    auto *s = reinterpret_cast<string *>(ptr);
    if (s != nullptr) {
        memcpy_s(bytes, size, s->c_str(), size);
    }
}
TS_INTEROP_V3(StringData, KNativePointer, KByte *, KUInt)

KNativePointer impl_StringMake(const KStringPtr &str)
{
    return new string(str.CStr());
}
TS_INTEROP_1(StringMake, KNativePointer, KStringPtr)

// For slow runtimes w/o fast encoders.
KInt impl_ManagedStringWrite(const KStringPtr &str, KByte *buffer, KInt offset)
{
    memcpy_s(buffer + offset, str.Length() + 1, str.CStr(), str.Length() + 1);
    return str.Length() + 1;
}
TS_INTEROP_3(ManagedStringWrite, KInt, KStringPtr, KByte *, KInt)

void StringFinalizer(string *ptr)
{
    delete ptr;
}
KNativePointer impl_GetStringFinalizer()
{
    return FnPtr<string>(StringFinalizer);
}
TS_INTEROP_0(GetStringFinalizer, KNativePointer)

void impl_InvokeFinalizer(KNativePointer obj, KNativePointer finalizer)
{
    auto finalizerF = reinterpret_cast<void (*)(KNativePointer)>(finalizer);
    finalizerF(obj);
}
TS_INTEROP_V2(InvokeFinalizer, KNativePointer, KNativePointer)

KInt impl_GetPtrVectorSize(KNativePointer ptr)
{
    auto *vec = reinterpret_cast<std::vector<void *> *>(ptr);
    return vec->size();
}
TS_INTEROP_1(GetPtrVectorSize, KInt, KNativePointer)

KNativePointer impl_GetPtrVectorElement(KNativePointer ptr, KInt index)
{
    auto vector = reinterpret_cast<std::vector<void *> *>(ptr);
    auto element = vector->at(index);
    return element;
}
TS_INTEROP_2(GetPtrVectorElement, KNativePointer, KNativePointer, KInt)

inline KUInt UnpackUInt(const KByte *bytes)
{
    const KUInt oneByte = 8U;
    const KUInt twoByte = 16U;
    const KUInt threeByte = 24U;
    return (static_cast<KUInt>(bytes[0]) | (static_cast<KUInt>(bytes[1]) << oneByte) |
            (static_cast<KUInt>(bytes[2]) << twoByte) | (static_cast<KUInt>(bytes[3]) << threeByte));
}

std::vector<KStringPtr> MakeStringVector(KStringArray strArray)
{
    if (strArray == nullptr) {
        return std::vector<KStringPtr>(0);
    }
    KUInt arraySize = UnpackUInt(strArray);
    std::vector<KStringPtr> res(arraySize);
    size_t offset = sizeof(KUInt);
    for (KUInt i = 0; i < arraySize; ++i) {
        int len = UnpackUInt(strArray + offset);
        res[i].Assign(reinterpret_cast<const char *>(strArray + offset + sizeof(KUInt)), len);
        offset += len + sizeof(KUInt);
    }
    return res;
}

std::vector<KStringPtr> MakeStringVector(KNativePointerArray arr, KInt length)
{
    if (arr == nullptr) {
        return std::vector<KStringPtr>(0);
    }
    std::vector<KStringPtr> res(length);
    char **strings = reinterpret_cast<char **>(arr);
    for (KInt i = 0; i < length; ++i) {
        const char *str = reinterpret_cast<const char *>(strings[i]);
        res[i].Assign(str);
    }
    return res;
}

using LoadVirtualMachineT = KInt (*)(KInt vmKind, const char *classPath, const char *libraryPath,
                                     void *currentVMContext);
using StartApplicationT = KNativePointer (*)(const char *appUrl, const char *appParams);
using RunApplicationT = KBoolean (*)(const KInt arg0, const KInt arg1);
using EmitEventT = void (*)(const KInt type, const KInt target, const KInt arg0, const KInt arg1);

static CallbackCallert g_callbackCaller = nullptr;
void SetCallbackCaller(CallbackCallert callbackCaller)
{
    g_callbackCaller = callbackCaller;
}

void impl_CallCallback(KInt callbackKind, KByte *args, KInt argsSize)
{
    if (g_callbackCaller != nullptr) {
        g_callbackCaller(callbackKind, args, argsSize);
    }
}
TS_INTEROP_V3(CallCallback, KInt, KByte *, KInt)

static CallbackCallerSynct g_callbackCallerSync = nullptr;
void SetCallbackCallerSync(CallbackCallerSynct callbackCallerSync)
{
    g_callbackCallerSync = callbackCallerSync;
}

void impl_CallCallbackSync(KVMContext vmContext, KInt callbackKind, KByte *args, KInt argsSize)
{
    if (g_callbackCallerSync != nullptr) {
        g_callbackCallerSync(vmContext, callbackKind, args, argsSize);
    }
}
TS_INTEROP_CTX_V3(CallCallbackSync, KInt, KByte *, KInt)

void impl_CallCallbackResourceHolder(KNativePointer holder, KInt resourceId)
{
    reinterpret_cast<void (*)(KInt)>(holder)(resourceId);
}
TS_INTEROP_V2(CallCallbackResourceHolder, KNativePointer, KInt)

void impl_CallCallbackResourceReleaser(KNativePointer releaser, KInt resourceId)
{
    reinterpret_cast<void (*)(KInt)>(releaser)(resourceId);
}
TS_INTEROP_V2(CallCallbackResourceReleaser, KNativePointer, KInt)

// NOLINTBEGIN(cppcoreguidelines-macro-usage)

// CC-OFFNXT(G.EXP.01) false positive
#define __QUOTE(x) #x
#define QUOTE(x) __QUOTE(x)

// NOLINTEND(cppcoreguidelines-macro-usage)

#ifndef INTEROP_LIBRARY_NAME
#error "INTEROP_LIBRARY_NAME must be defined"
#endif

void impl_NativeLog(const KStringPtr &str)
{
    std::cout << QUOTE(INTEROP_LIBRARY_NAME) << ": " << str.CStr() << std::endl;
}
TS_INTEROP_V1(NativeLog, KStringPtr)

int32_t CallCallback(KVMContext context, int32_t methodId, uint8_t *argsData, int32_t argsLength)
{
    TS_INTEROP_CALL_INT(context, methodId, argsLength, argsData);
    return 0;
}

void ResolveDeferred(KVMDeferred *deferred, [[maybe_unused]] uint8_t *argsData, [[maybe_unused]] int32_t argsLength)
{
    napi_acquire_threadsafe_function(static_cast<napi_threadsafe_function>(deferred->handler));
    auto status = napi_call_threadsafe_function(static_cast<napi_threadsafe_function>(deferred->handler), deferred,
                                                napi_tsfn_nonblocking);
    if (status != napi_ok) {
        LogE("cannot call thread-safe function; status=", status);
    }
    napi_release_threadsafe_function(static_cast<napi_threadsafe_function>(deferred->handler), napi_tsfn_release);
}

void RejectDeferred(KVMDeferred *deferred, [[maybe_unused]] const char *message)
{
    napi_release_threadsafe_function(static_cast<napi_threadsafe_function>(deferred->handler), napi_tsfn_release);
    delete deferred;
}

void ResolveDeferredImpl(napi_env env, [[maybe_unused]] napi_value jsCallback, KVMDeferred *deferred,
                         [[maybe_unused]] void *data)
{
    napi_value undefined = nullptr;
    napi_get_undefined(env, &undefined);
    auto status = napi_resolve_deferred(env, reinterpret_cast<napi_deferred>(deferred->context), undefined);
    if (status != napi_ok) {
        LogE("cannot resolve deferred; status=", status);
    }
    delete deferred;
}

[[maybe_unused]] static void ReleaseDeferred(KVMDeferred *deferred)
{
    delete deferred;
}

KVMDeferred *CreateDeferred(KVMContext vmContext, KVMObjectHandle *promiseHandle)
{
    auto *deferred = new KVMDeferred();
    deferred->resolve = ResolveDeferred;
    deferred->reject = RejectDeferred;
    // NOTE(khil): mb move\remove to interop!
    auto env = reinterpret_cast<napi_env>(vmContext);
    napi_value promise;
    napi_value resourceName;
    size_t napiStrLen = 5;
    napi_create_string_utf8(env, "Async", napiStrLen, &resourceName);
    auto status = napi_create_promise(env, reinterpret_cast<napi_deferred *>(&deferred->context), &promise);
    if (status != napi_ok) {
        LogE("cannot make a promise; status=", status);
    }
    status = napi_create_threadsafe_function(env, nullptr, nullptr, resourceName, 0, 1, nullptr, nullptr, deferred,
                                             reinterpret_cast<napi_threadsafe_function_call_js>(ResolveDeferredImpl),
                                             reinterpret_cast<napi_threadsafe_function *>(&deferred->handler));
    if (status != napi_ok) {
        LogE("cannot make threadsafe function; status=", status);
    }
    *promiseHandle = reinterpret_cast<KVMObjectHandle>(promise);
    return deferred;
}

// Allocate, so CTX versions.
KStringPtr impl_Utf8ToString([[maybe_unused]] KVMContext vmContext, KByte *data, KInt offset, KInt length)
{
    KStringPtr result(reinterpret_cast<const char *>(data + offset), length, false);
    return result;
}
TS_INTEROP_CTX_3(Utf8ToString, KStringPtr, KByte *, KInt, KInt)

KStringPtr impl_StdStringToString([[maybe_unused]] KVMContext vmContext, KNativePointer stringPtr)
{
    auto *str = reinterpret_cast<std::string *>(stringPtr);
    KStringPtr result(str->c_str(), str->size(), false);
    return result;
}
TS_INTEROP_CTX_1(StdStringToString, KStringPtr, KNativePointer)

KInteropReturnBuffer impl_RawReturnData([[maybe_unused]] KVMContext vmContext, KInt v1, KInt v2)
{
    void *data = new int8_t[v1];
    memset_s(data, v2, v1, v2);
    KInteropReturnBuffer buffer = {v1, data,
                                   [](KNativePointer ptr, KInt) { delete[] reinterpret_cast<int8_t *>(ptr); }};
    return buffer;
}
TS_INTEROP_CTX_2(RawReturnData, KInteropReturnBuffer, KInt, KInt)
// NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)

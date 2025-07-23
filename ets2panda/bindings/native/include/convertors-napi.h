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

#ifndef CONVERTORS_NAPI_H_
#define CONVERTORS_NAPI_H_

#include <array>
#include <unordered_map>
#include <vector>
#include <string>
#include <iostream>
#include <stdexcept>

#ifndef TS_NAPI_OHOS
#include <node_api.h>
#else
#include <native_api.h>
#include <native_node_api.h>
#endif
#include "panda_types.h"

template <class T>
struct InteropTypeConverter {
    using InteropType = T;
    static T ConvertFrom([[maybe_unused]] napi_env env, InteropType value)
    {
        return value;
    }
    static InteropType ConvertTo([[maybe_unused]] napi_env env, T value)
    {
        return value;
    }
    static void Release([[maybe_unused]] napi_env env, [[maybe_unused]] InteropType value, [[maybe_unused]] T converted)
    {
    }
};

template <typename Type>
inline typename InteropTypeConverter<Type>::InteropType MakeResult(napi_env env, Type value)
{
    return InteropTypeConverter<Type>::ConvertTo(env, value);
}

template <typename Type>
inline Type GetArgument(napi_env env, typename InteropTypeConverter<Type>::InteropType arg)
{
    return InteropTypeConverter<Type>::ConvertFrom(env, arg);
}

template <typename Type>
inline void ReleaseArgument(napi_env env, typename InteropTypeConverter<Type>::InteropType arg, Type data)
{
    InteropTypeConverter<Type>::Release(env, arg, data);
}

template <>
struct InteropTypeConverter<KInteropBuffer> {
    using InteropType = napi_value;
    static KInteropBuffer ConvertFrom(napi_env env, InteropType value)
    {
        auto placeholder = 0;
        KInteropBuffer result = {placeholder, nullptr, 0, nullptr};
        bool isArrayBuffer = false;
        napi_is_arraybuffer(env, value, &isArrayBuffer);
        if (isArrayBuffer) {
            napi_get_arraybuffer_info(env, value, &result.data, reinterpret_cast<size_t *>(&result.length));
        } else {
            bool isDataView = false;
            napi_is_dataview(env, value, &isDataView);
            if (isDataView) {
                napi_get_dataview_info(env, value, reinterpret_cast<size_t *>(&result.length), &result.data, nullptr,
                                       nullptr);
            }
        }
        return result;
    }
    static InteropType ConvertTo(napi_env env, KInteropBuffer value)
    {
        auto *copy = new KInteropBuffer(value);
        napi_value result;
        napi_status status = napi_create_external_arraybuffer(
            env, value.data, value.length,
            []([[maybe_unused]] napi_env envArg, [[maybe_unused]] void *finalizeData, void *finalizeHint) {
                auto *buffer = reinterpret_cast<KInteropBuffer *>(finalizeHint);
                buffer->dispose(buffer->resourceId);
                delete buffer;
            },
            (void *)copy, &result);
        if (status != napi_ok) {
            // do smth here
        }
        return result;
    };
    static void Release([[maybe_unused]] napi_env env, [[maybe_unused]] InteropType value,
                        [[maybe_unused]] KInteropBuffer converted)
    {
    }
};

template <>
struct InteropTypeConverter<KStringPtr> {
    using InteropType = napi_value;
    static KStringPtr ConvertFrom(napi_env env, InteropType value)
    {
        if (value == nullptr) {
            return KStringPtr();
        }
        KStringPtr result;
        size_t length = 0;
        napi_status status = napi_get_value_string_utf8(env, value, nullptr, 0, &length);
        if (status != 0) {
            return result;
        }
        result.Resize(length);
        napi_get_value_string_utf8(env, value, result.Data(), length + 1, nullptr);
        return result;
    }
    static InteropType ConvertTo(napi_env env, const KStringPtr &value)
    {
        napi_value result;
        napi_create_string_utf8(env, value.CStr(), value.Length(), &result);
        return result;
    }
    static void Release([[maybe_unused]] napi_env env, [[maybe_unused]] InteropType value,
                        [[maybe_unused]] const KStringPtr &converted)
    {
    }
};

template <>
struct InteropTypeConverter<KInteropNumber> {
    using InteropType = napi_value;
    static KInteropNumber ConvertFrom(napi_env env, InteropType interopValue)
    {
        double value = 0.0;
        napi_get_value_double(env, interopValue, &value);
        return KInteropNumber::FromDouble(value);
    }
    static InteropType ConvertTo(napi_env env, KInteropNumber value)
    {
        napi_value result;
        napi_create_double(env, value.AsDouble(), &result);
        return result;
    }
    static void Release([[maybe_unused]] napi_env env, [[maybe_unused]] InteropType value,
                        [[maybe_unused]] KInteropNumber converted)
    {
    }
};

template <>
struct InteropTypeConverter<KVMObjectHandle> {
    using InteropType = napi_value;
    static inline KVMObjectHandle ConvertFrom([[maybe_unused]] napi_env env, InteropType value)
    {
        return reinterpret_cast<KVMObjectHandle>(value);
    }
    static InteropType ConvertTo([[maybe_unused]] napi_env env, KVMObjectHandle value)
    {
        return reinterpret_cast<napi_value>(value);
    }
    static inline void Release([[maybe_unused]] napi_env env, [[maybe_unused]] InteropType value,
                               [[maybe_unused]] KVMObjectHandle converted)
    {
    }
};

template <>
struct InteropTypeConverter<KInteropReturnBuffer> {
    using InteropType = napi_value;
    static inline KInteropReturnBuffer ConvertFrom(napi_env env, InteropType value) = delete;
    static void Disposer([[maybe_unused]] napi_env env, [[maybe_unused]] void *data, void *hint)
    {
        auto *bufferCopy = static_cast<KInteropReturnBuffer *>(hint);
        bufferCopy->dispose(bufferCopy->data, bufferCopy->length);
        delete bufferCopy;
    }
    static InteropType ConvertTo(napi_env env, KInteropReturnBuffer value)
    {
        napi_value result = nullptr;
        napi_value arrayBuffer = nullptr;
        auto clone = new KInteropReturnBuffer();
        *clone = value;
        napi_create_external_arraybuffer(env, value.data, value.length, Disposer, clone, &arrayBuffer);
        napi_create_typedarray(env, napi_uint8_array, value.length, arrayBuffer, 0, &result);
        return result;
    }
    static inline void Release(napi_env env, InteropType value, const KInteropReturnBuffer &converted) = delete;
};

// NOLINTBEGIN(cppcoreguidelines-macro-usage)

#define TS_INTEROP_THROW(vmcontext, object, ...)                                   \
    do {                                                                           \
        napi_env env = (napi_env)vmcontext;                                        \
        napi_handle_scope scope = nullptr;                                         \
        [[maybe_unused]] napi_status status = napi_open_handle_scope(env, &scope); \
        napi_throw((napi_env)vmcontext, object);                                   \
        napi_close_handle_scope(env, scope);                                       \
        /* CC-OFFNXT(G.PRE.05) function gen */                                     \
        return __VA_ARGS__;                                                        \
    } while (0)

#define TS_INTEROP_THROW_STRING(vmContext, message, ...)                                \
    do {                                                                                \
        napi_value value;                                                               \
        napi_create_string_utf8((napi_env)vmContext, message, strlen(message), &value); \
        TS_INTEROP_THROW(vmContext, value, __VA_ARGS__);                                \
    } while (0)

// CC-OFFNXT(G.PRE.02-CPP) code generation
#define NAPI_ASSERT_INDEX(info, index, result)                          \
    do {                                                                \
        /* CC-OFFNXT(G.PRE.02) name part*/                              \
        if ((static_cast<size_t>(index)) >= (info).Length()) {          \
            /* CC-OFFNXT(G.PRE.02) name part*/                          \
            napi_throw_error((info).Env(), nullptr, "No such element"); \
            /* CC-OFFNXT(G.PRE.05) function gen */                      \
            return result;                                              \
        }                                                               \
    } while (0)

// Helpers from node-addon-api
// CC-OFFNXT(G.PRE.02-CPP) code generation
#define TS_NAPI_THROW_IF_FAILED(env, status, ...)                 \
    if ((status) != napi_ok) {                                    \
        const napi_extended_error_info *errorInfo;                \
        napi_get_last_error_info(env, &errorInfo);                \
        napi_throw_error(env, nullptr, errorInfo->error_message); \
        /* CC-OFFNXT(G.PRE.05) function gen */                    \
        return __VA_ARGS__;                                       \
    }
// CC-OFFNXT(G.PRE.02-CPP) code generation
#define TS_NAPI_THROW_IF_FAILED_VOID(env, status)                 \
    if ((status) != napi_ok) {                                    \
        const napi_extended_error_info *errorInfo;                \
        napi_get_last_error_info(env, &errorInfo);                \
        napi_throw_error(env, nullptr, errorInfo->error_message); \
        /* CC-OFFNXT(G.PRE.05) function gen */                    \
        return;                                                   \
    }

// NOLINTEND(cppcoreguidelines-macro-usage)

class CallbackInfo {
public:
    CallbackInfo(napi_env env, napi_callback_info info) : env_(env)
    {
        size_t size = 0;
        napi_status status = napi_get_cb_info(env, info, &size, nullptr, nullptr, nullptr);
        TS_NAPI_THROW_IF_FAILED_VOID(env, status);
        if (size > 0) {
            args_.resize(
                size);  // NOTE(khil): statically allocate small array for common case with few arguments passed
            status = napi_get_cb_info(env, info, &size, args_.data(), nullptr, nullptr);
            TS_NAPI_THROW_IF_FAILED_VOID(env, status);
        }
    }

    napi_value operator[](size_t idx) const
    {
        if (idx >= Length()) {
            napi_value result;
            napi_get_undefined(env_, &result);
            return result;
        }
        return args_[idx];
    }

    napi_env Env() const
    {
        return env_;
    }

    size_t Length() const
    {
        return args_.size();
    }

private:
    napi_env env_;
    // napi_callback_info _info;
    std::vector<napi_value> args_;
};

template <typename ElemType>
inline napi_typedarray_type GetNapiType() = delete;

template <>
inline napi_typedarray_type GetNapiType<float>()
{
    return napi_float32_array;
}

template <>
inline napi_typedarray_type GetNapiType<int8_t>()
{
    return napi_int8_array;
}

template <>
inline napi_typedarray_type GetNapiType<uint8_t>()
{
    return napi_uint8_array;
}

template <>
inline napi_typedarray_type GetNapiType<int16_t>()
{
    return napi_int16_array;
}

template <>
inline napi_typedarray_type GetNapiType<uint16_t>()
{
    return napi_uint16_array;
}

template <>
inline napi_typedarray_type GetNapiType<int32_t>()
{
    return napi_int32_array;
}

template <>
inline napi_typedarray_type GetNapiType<uint32_t>()
{
    return napi_uint32_array;
}

template <>
inline napi_typedarray_type GetNapiType<KNativePointer>()
{
    return napi_biguint64_array;
}

napi_valuetype GetValueTypeChecked(napi_env env, napi_value value);
bool IsTypedArray(napi_env env, napi_value value);

template <typename ElemType>
// CC-OFFNXT(G.FUD.06) solid logic, ODR
inline ElemType *GetTypedElements(napi_env env, napi_value value)
{
    napi_valuetype valueType = GetValueTypeChecked(env, value);
    if (valueType == napi_null) {
        return nullptr;
    }
    if (!IsTypedArray(env, value)) {
        napi_throw_error(env, nullptr, "Expected TypedArray");
        return nullptr;
    }
    napi_value arrayBuffer;
    void *data = nullptr;
    size_t byteLength;
    size_t byteOffset;
    napi_typedarray_type type;
    napi_status status = napi_get_typedarray_info(env, value, &type, &byteLength, &data, &arrayBuffer, &byteOffset);
    TS_NAPI_THROW_IF_FAILED(env, status, nullptr);
    if (type != GetNapiType<ElemType>()) {
        std::cout << "Array type mismatch. Expected " << GetNapiType<ElemType>() << " got " << type << std::endl;
        napi_throw_error(env, nullptr, "Array type mismatch");
        return nullptr;
    }
    return reinterpret_cast<ElemType *>(data);
}

template <typename ElemType>
inline ElemType *GetTypedElements(const CallbackInfo &info, int index)
{
    NAPI_ASSERT_INDEX(info, index, nullptr);
    return GetTypedElements<ElemType>(info.Env(), info[index]);
}

inline uint8_t *GetUInt8Elements(const CallbackInfo &info, int index)
{
    return GetTypedElements<uint8_t>(info, index);
}

inline int8_t *GetInt8Elements(const CallbackInfo &info, int index)
{
    return GetTypedElements<int8_t>(info, index);
}

inline uint16_t *GetUInt16Elements(const CallbackInfo &info, int index)
{
    return GetTypedElements<uint16_t>(info, index);
}

inline int16_t *GetInt16Elements(const CallbackInfo &info, int index)
{
    return GetTypedElements<int16_t>(info, index);
}

inline uint32_t *GetUInt32Elements(const CallbackInfo &info, int index)
{
    return GetTypedElements<uint32_t>(info, index);
}

inline uint32_t *GetUInt32Elements(napi_env env, napi_value value)
{
    return GetTypedElements<uint32_t>(env, value);
}

inline int32_t *GetInt32Elements(const CallbackInfo &info, int index)
{
    return GetTypedElements<int32_t>(info, index);
}

inline float *GetFloat32Elements(const CallbackInfo &info, int index)
{
    return GetTypedElements<float>(info, index);
}

inline KNativePointer *GetPointerElements(const CallbackInfo &info, int index)
{
    return GetTypedElements<KNativePointer>(info, index);
}

KInt GetInt32(napi_env env, napi_value value);
inline int32_t GetInt32(const CallbackInfo &info, int index)
{
    NAPI_ASSERT_INDEX(info, index, 0);
    return GetInt32(info.Env(), info[index]);
}
KUInt GetUInt32(napi_env env, napi_value value);
inline uint32_t GetUInt32(const CallbackInfo &info, int index)
{
    NAPI_ASSERT_INDEX(info, index, 0);
    return GetUInt32(info.Env(), info[index]);
}
KFloat GetFloat32(napi_env env, napi_value value);
inline float GetFloat32(const CallbackInfo &info, int index)
{
    NAPI_ASSERT_INDEX(info, index, 0.0F);
    return GetFloat32(info.Env(), info[index]);
}
KDouble GetFloat64(napi_env env, napi_value value);
inline KDouble GetFloat64(const CallbackInfo &info, int index)
{
    NAPI_ASSERT_INDEX(info, index, 0.0);
    return GetFloat64(info.Env(), info[index]);
}
KStringPtr GetString(napi_env env, napi_value value);
inline KStringPtr GetString(const CallbackInfo &info, int index)
{
    NAPI_ASSERT_INDEX(info, index, KStringPtr());
    return GetString(info.Env(), info[index]);
}
void *GetPointer(napi_env env, napi_value value);
inline void *GetPointer(const CallbackInfo &info, int index)
{
    NAPI_ASSERT_INDEX(info, index, nullptr);
    return GetPointer(info.Env(), info[index]);
}
KLong GetInt64(napi_env env, napi_value value);
inline KLong GetInt64(const CallbackInfo &info, int index)
{
    NAPI_ASSERT_INDEX(info, index, 0);
    return GetInt64(info.Env(), info[index]);
}
KBoolean GetBoolean(napi_env env, napi_value value);
inline KBoolean GetBoolean(const CallbackInfo &info, int index)
{
    NAPI_ASSERT_INDEX(info, index, false);
    return GetBoolean(info.Env(), info[index]);
}

template <typename Type>
inline Type GetArgument(const CallbackInfo &info, int index) = delete;

template <>
inline KBoolean GetArgument<KBoolean>(const CallbackInfo &info, int index)
{
    return GetBoolean(info, index);
}

template <>
inline KUInt GetArgument<uint32_t>(const CallbackInfo &info, int index)
{
    return GetUInt32(info, index);
}

template <>
inline KInt GetArgument<int32_t>(const CallbackInfo &info, int index)
{
    return GetInt32(info, index);
}

template <>
inline KInteropNumber GetArgument<KInteropNumber>(const CallbackInfo &info, int index)
{
    KInteropNumber res {};
    NAPI_ASSERT_INDEX(info, index, res);
    return GetArgument<KInteropNumber>(info.Env(), info[index]);
}

template <>
// CC-OFFNXT(G.FUD.06) solid logic, ODR
inline KLength GetArgument<KLength>(const CallbackInfo &info, int index)
{
    KLength result = {0, 0.0F, 0, 0};
    NAPI_ASSERT_INDEX(info, index, result);
    auto value = info[index];
    napi_valuetype type;
    auto typeStatus = napi_typeof(info.Env(), value, &type);
    if (typeStatus != 0) {
        return result;
    }
    switch (type) {
        case napi_number: {
            result.value = GetFloat32(info.Env(), value);
            result.unit = 1;
            result.type = 0;
            break;
        }
        case napi_string: {
            KStringPtr string = GetString(info.Env(), value);
            ParseKLength(string, &result);
            result.type = 1;
            result.resource = 0;
            break;
        }
        case napi_object: {
            result.value = 0;
            result.unit = 1;
            result.type = 2U;
            napi_value field;
            napi_status status = napi_get_named_property(info.Env(), value, "id", &field);
            if (status == 0) {
                status = napi_get_value_int32(info.Env(), field, &result.resource);
                if (status != 0) {
                    result.resource = 0;
                }
            } else {
                result.resource = 0;
            }
            break;
        }
        default:
            throw std::runtime_error("Error, unexpected KLength type");
    }
    return result;
}

template <>
inline KInteropBuffer GetArgument<KInteropBuffer>(const CallbackInfo &info, int index)
{
    KInteropBuffer res = {0, nullptr, 0, nullptr};
    NAPI_ASSERT_INDEX(info, index, res);
    return GetArgument<KInteropBuffer>((napi_env)info.Env(), (napi_value)info[index]);
}

template <>
inline KFloat GetArgument<KFloat>(const CallbackInfo &info, int index)
{
    return GetFloat32(info, index);
}

template <>
inline KDouble GetArgument<KDouble>(const CallbackInfo &info, int index)
{
    return GetFloat64(info, index);
}

template <>
inline KNativePointer GetArgument<KNativePointer>(const CallbackInfo &info, int index)
{
    return GetPointer(info, index);
}

template <>
inline KLong GetArgument<KLong>(const CallbackInfo &info, int index)
{
    return GetInt64(info, index);
}

template <>
inline KNativePointerArray GetArgument<KNativePointerArray>(const CallbackInfo &info, int index)
{
    return GetPointerElements(info, index);
}

template <>
inline uint8_t *GetArgument<uint8_t *>(const CallbackInfo &info, int index)
{
    return GetUInt8Elements(info, index);
}

template <>
inline const uint8_t *GetArgument<const uint8_t *>(const CallbackInfo &info, int index)
{
    return GetUInt8Elements(info, index);
}

template <>
inline int8_t *GetArgument<int8_t *>(const CallbackInfo &info, int index)
{
    return GetInt8Elements(info, index);
}

template <>
inline int16_t *GetArgument<int16_t *>(const CallbackInfo &info, int index)
{
    return GetInt16Elements(info, index);
}

template <>
inline uint16_t *GetArgument<uint16_t *>(const CallbackInfo &info, int index)
{
    return GetUInt16Elements(info, index);
}

template <>
inline int32_t *GetArgument<int32_t *>(const CallbackInfo &info, int index)
{
    return GetInt32Elements(info, index);
}

template <>
inline uint32_t *GetArgument<uint32_t *>(const CallbackInfo &info, int index)
{
    return GetUInt32Elements(info, index);
}

template <>
inline float *GetArgument<float *>(const CallbackInfo &info, int index)
{
    return GetFloat32Elements(info, index);
}

template <>
inline KStringPtr GetArgument<KStringPtr>(const CallbackInfo &info, int index)
{
    return GetString(info, index);
}

napi_value MakeString(napi_env env, KStringPtr value);
napi_value MakeString(napi_env env, const std::string &value);
napi_value MakeBoolean(napi_env env, KBoolean value);
napi_value MakeInt32(napi_env env, int32_t value);
napi_value MakeUInt32(napi_env env, uint32_t value);
napi_value MakeFloat32(napi_env env, float value);
napi_value MakePointer(napi_env env, void *value);
napi_value MakeVoid(napi_env env);

inline napi_value MakeVoid(const CallbackInfo &info)
{
    return MakeVoid(info.Env());
}

template <typename Type>
inline napi_value MakeResult(const CallbackInfo &info, Type value) = delete;

template <>
inline napi_value MakeResult<KBoolean>(const CallbackInfo &info, KBoolean value)
{
    return MakeBoolean(info.Env(), value);
}

template <>
inline napi_value MakeResult<int32_t>(const CallbackInfo &info, int32_t value)
{
    return MakeInt32(info.Env(), value);
}

template <>
inline napi_value MakeResult<uint32_t>(const CallbackInfo &info, uint32_t value)
{
    return MakeUInt32(info.Env(), value);
}

template <>
inline napi_value MakeResult<float>(const CallbackInfo &info, float value)
{
    return MakeFloat32(info.Env(), value);
}

template <>
inline napi_value MakeResult<KNativePointer>(const CallbackInfo &info, KNativePointer value)
{
    return MakePointer(info.Env(), value);
}

template <>
inline napi_value MakeResult<KVMObjectHandle>(const CallbackInfo &info, KVMObjectHandle value)
{
    return InteropTypeConverter<KVMObjectHandle>::ConvertTo(info.Env(), value);
}

template <>
inline napi_value MakeResult<KStringPtr>(const CallbackInfo &info, KStringPtr value)
{
    return InteropTypeConverter<KStringPtr>::ConvertTo(info.Env(), value);
}

template <>
inline napi_value MakeResult<KInteropBuffer>(const CallbackInfo &info, KInteropBuffer value)
{
    return InteropTypeConverter<KInteropBuffer>::ConvertTo(info.Env(), value);
}

template <>
inline napi_value MakeResult<KInteropReturnBuffer>(const CallbackInfo &info, KInteropReturnBuffer value)
{
    return InteropTypeConverter<KInteropReturnBuffer>::ConvertTo(info.Env(), value);
}

template <>
inline napi_value MakeResult<KInteropNumber>(const CallbackInfo &info, KInteropNumber value)
{
    return InteropTypeConverter<KInteropNumber>::ConvertTo(info.Env(), value);
}

using NapiTypeT = napi_value (*)(napi_env, napi_callback_info);

class Exports {
    std::unordered_map<std::string, std::vector<std::pair<std::string, NapiTypeT>>> implementations_;

public:
    static Exports *GetInstance();

    std::vector<std::string> GetModules();
    void addMethod(const char *module, const char *name, NapiTypeT impl);
    const std::vector<std::pair<std::string, NapiTypeT>> &GetMethods(const std::string &module);
};

// NOLINTBEGIN(cppcoreguidelines-macro-usage)

// CC-OFFNXT(G.DCL.01) false positive
// CC-OFFNXT(G.NAM.01) false positive
#define __QUOTE(x) #x
#define QUOTE(x) __QUOTE(x)

#define MAKE_NODE_EXPORT(module, name)                                            \
    __attribute__((constructor)) static void __init_##name()                      \
    {                                                                             \
        Exports::GetInstance()->addMethod(QUOTE(module), "_" #name, Node_##name); \
    }

// NOLINTEND(cppcoreguidelines-macro-usage)

#ifndef TS_INTEROP_MODULE
#error TS_INTEROP_MODULE is undefined
#endif

// NOLINTBEGIN(cppcoreguidelines-macro-usage)

#define MAKE_INTEROP_NODE_EXPORT(name) MAKE_NODE_EXPORT(TS_INTEROP_MODULE, name)

#define TS_INTEROP_0(name, Ret)                                     \
    napi_value Node_##name(napi_env env, napi_callback_info cbinfo) \
    {                                                               \
        TS_MAYBE_LOG(name)                                          \
        CallbackInfo info(env, cbinfo);                             \
        /* CC-OFFNXT(G.PRE.05) function gen */                      \
        return MakeResult<Ret>(info, impl_##name());                \
    }                                                               \
    MAKE_NODE_EXPORT(TS_INTEROP_MODULE, name)

#define TS_INTEROP_1(name, Ret, P0)                                 \
    napi_value Node_##name(napi_env env, napi_callback_info cbinfo) \
    {                                                               \
        TS_MAYBE_LOG(name)                                          \
        CallbackInfo info(env, cbinfo);                             \
        P0 p0 = GetArgument<P0>(info, 0);                           \
        /* CC-OFFNXT(G.PRE.05) function gen */                      \
        return MakeResult<Ret>(info, impl_##name(p0));              \
    }                                                               \
    MAKE_NODE_EXPORT(TS_INTEROP_MODULE, name)

#define TS_INTEROP_2(name, Ret, P0, P1)                             \
    napi_value Node_##name(napi_env env, napi_callback_info cbinfo) \
    {                                                               \
        TS_MAYBE_LOG(name)                                          \
        CallbackInfo info(env, cbinfo);                             \
        P0 p0 = GetArgument<P0>(info, 0);                           \
        P1 p1 = GetArgument<P1>(info, 1);                           \
        /* CC-OFFNXT(G.PRE.05) function gen */                      \
        return MakeResult<Ret>(info, impl_##name(p0, p1));          \
    }                                                               \
    MAKE_NODE_EXPORT(TS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) solid logic
#define TS_INTEROP_3(name, Ret, P0, P1, P2)                         \
    napi_value Node_##name(napi_env env, napi_callback_info cbinfo) \
    {                                                               \
        TS_MAYBE_LOG(name)                                          \
        CallbackInfo info(env, cbinfo);                             \
        P0 p0 = GetArgument<P0>(info, 0);                           \
        P1 p1 = GetArgument<P1>(info, 1);                           \
        P2 p2 = GetArgument<P2>(info, 2);                           \
        /* CC-OFFNXT(G.PRE.05) function gen */                      \
        return MakeResult<Ret>(info, impl_##name(p0, p1, p2));      \
    }                                                               \
    MAKE_NODE_EXPORT(TS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) solid logic
#define TS_INTEROP_4(name, Ret, P0, P1, P2, P3)                     \
    napi_value Node_##name(napi_env env, napi_callback_info cbinfo) \
    {                                                               \
        TS_MAYBE_LOG(name)                                          \
        CallbackInfo info(env, cbinfo);                             \
        P0 p0 = GetArgument<P0>(info, 0);                           \
        P1 p1 = GetArgument<P1>(info, 1);                           \
        P2 p2 = GetArgument<P2>(info, 2);                           \
        P3 p3 = GetArgument<P3>(info, 3);                           \
        /* CC-OFFNXT(G.PRE.05) function gen */                      \
        return MakeResult<Ret>(info, impl_##name(p0, p1, p2, p3));  \
    }                                                               \
    MAKE_NODE_EXPORT(TS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) solid logic
#define TS_INTEROP_5(name, Ret, P0, P1, P2, P3, P4)                    \
    napi_value Node_##name(napi_env env, napi_callback_info cbinfo)    \
    {                                                                  \
        TS_MAYBE_LOG(name)                                             \
        CallbackInfo info(env, cbinfo);                                \
        P0 p0 = GetArgument<P0>(info, 0);                              \
        P1 p1 = GetArgument<P1>(info, 1);                              \
        P2 p2 = GetArgument<P2>(info, 2);                              \
        P3 p3 = GetArgument<P3>(info, 3);                              \
        P4 p4 = GetArgument<P4>(info, 4);                              \
        /* CC-OFFNXT(G.PRE.05) function gen */                         \
        return MakeResult<Ret>(info, impl_##name(p0, p1, p2, p3, p4)); \
    }                                                                  \
    MAKE_NODE_EXPORT(TS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) solid logic
#define TS_INTEROP_6(name, Ret, P0, P1, P2, P3, P4, P5)                    \
    napi_value Node_##name(napi_env env, napi_callback_info cbinfo)        \
    {                                                                      \
        TS_MAYBE_LOG(name)                                                 \
        CallbackInfo info(env, cbinfo);                                    \
        P0 p0 = GetArgument<P0>(info, 0);                                  \
        P1 p1 = GetArgument<P1>(info, 1);                                  \
        P2 p2 = GetArgument<P2>(info, 2);                                  \
        P3 p3 = GetArgument<P3>(info, 3);                                  \
        P4 p4 = GetArgument<P4>(info, 4);                                  \
        P5 p5 = GetArgument<P5>(info, 5);                                  \
        /* CC-OFFNXT(G.PRE.05) function gen */                             \
        return MakeResult<Ret>(info, impl_##name(p0, p1, p2, p3, p4, p5)); \
    }                                                                      \
    MAKE_NODE_EXPORT(TS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) solid logic
#define TS_INTEROP_7(name, Ret, P0, P1, P2, P3, P4, P5, P6)                    \
    napi_value Node_##name(napi_env env, napi_callback_info cbinfo)            \
    {                                                                          \
        TS_MAYBE_LOG(name)                                                     \
        CallbackInfo info(env, cbinfo);                                        \
        P0 p0 = GetArgument<P0>(info, 0);                                      \
        P1 p1 = GetArgument<P1>(info, 1);                                      \
        P2 p2 = GetArgument<P2>(info, 2);                                      \
        P3 p3 = GetArgument<P3>(info, 3);                                      \
        P4 p4 = GetArgument<P4>(info, 4);                                      \
        P5 p5 = GetArgument<P5>(info, 5);                                      \
        P6 p6 = GetArgument<P6>(info, 6);                                      \
        /* CC-OFFNXT(G.PRE.05) function gen */                                 \
        return MakeResult<Ret>(info, impl_##name(p0, p1, p2, p3, p4, p5, p6)); \
    }                                                                          \
    MAKE_NODE_EXPORT(TS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) solid logic
#define TS_INTEROP_8(name, Ret, P0, P1, P2, P3, P4, P5, P6, P7)                    \
    napi_value Node_##name(napi_env env, napi_callback_info cbinfo)                \
    {                                                                              \
        TS_MAYBE_LOG(name)                                                         \
        CallbackInfo info(env, cbinfo);                                            \
        P0 p0 = GetArgument<P0>(info, 0);                                          \
        P1 p1 = GetArgument<P1>(info, 1);                                          \
        P2 p2 = GetArgument<P2>(info, 2);                                          \
        P3 p3 = GetArgument<P3>(info, 3);                                          \
        P4 p4 = GetArgument<P4>(info, 4);                                          \
        P5 p5 = GetArgument<P5>(info, 5);                                          \
        P6 p6 = GetArgument<P6>(info, 6);                                          \
        P7 p7 = GetArgument<P7>(info, 7);                                          \
        /* CC-OFFNXT(G.PRE.05) function gen */                                     \
        return MakeResult<Ret>(info, impl_##name(p0, p1, p2, p3, p4, p5, p6, p7)); \
    }                                                                              \
    MAKE_NODE_EXPORT(TS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) solid logic
#define TS_INTEROP_9(name, Ret, P0, P1, P2, P3, P4, P5, P6, P7, P8)                    \
    napi_value Node_##name(napi_env env, napi_callback_info cbinfo)                    \
    {                                                                                  \
        TS_MAYBE_LOG(name)                                                             \
        CallbackInfo info(env, cbinfo);                                                \
        P0 p0 = GetArgument<P0>(info, 0);                                              \
        P1 p1 = GetArgument<P1>(info, 1);                                              \
        P2 p2 = GetArgument<P2>(info, 2);                                              \
        P3 p3 = GetArgument<P3>(info, 3);                                              \
        P4 p4 = GetArgument<P4>(info, 4);                                              \
        P5 p5 = GetArgument<P5>(info, 5);                                              \
        P6 p6 = GetArgument<P6>(info, 6);                                              \
        P7 p7 = GetArgument<P7>(info, 7);                                              \
        P8 p8 = GetArgument<P8>(info, 8);                                              \
        /* CC-OFFNXT(G.PRE.05) function gen */                                         \
        return MakeResult<Ret>(info, impl_##name(p0, p1, p2, p3, p4, p5, p6, p7, p8)); \
    }                                                                                  \
    MAKE_NODE_EXPORT(TS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) solid logic
#define TS_INTEROP_10(name, Ret, P0, P1, P2, P3, P4, P5, P6, P7, P8, P9)                   \
    napi_value Node_##name(napi_env env, napi_callback_info cbinfo)                        \
    {                                                                                      \
        TS_MAYBE_LOG(name)                                                                 \
        CallbackInfo info(env, cbinfo);                                                    \
        P0 p0 = GetArgument<P0>(info, 0);                                                  \
        P1 p1 = GetArgument<P1>(info, 1);                                                  \
        P2 p2 = GetArgument<P2>(info, 2);                                                  \
        P3 p3 = GetArgument<P3>(info, 3);                                                  \
        P4 p4 = GetArgument<P4>(info, 4);                                                  \
        P5 p5 = GetArgument<P5>(info, 5);                                                  \
        P6 p6 = GetArgument<P6>(info, 6);                                                  \
        P7 p7 = GetArgument<P7>(info, 7);                                                  \
        P8 p8 = GetArgument<P8>(info, 8);                                                  \
        P9 p9 = GetArgument<P9>(info, 9);                                                  \
        /* CC-OFFNXT(G.PRE.05) function gen */                                             \
        return MakeResult<Ret>(info, impl_##name(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9)); \
    }                                                                                      \
    MAKE_NODE_EXPORT(TS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) solid logic
#define TS_INTEROP_11(name, Ret, P0, P1, P2, P3, P4, P5, P6, P7, P8, P9, P10)                   \
    napi_value Node_##name(napi_env env, napi_callback_info cbinfo)                             \
    {                                                                                           \
        TS_MAYBE_LOG(name)                                                                      \
        CallbackInfo info(env, cbinfo);                                                         \
        P0 p0 = GetArgument<P0>(info, 0);                                                       \
        P1 p1 = GetArgument<P1>(info, 1);                                                       \
        P2 p2 = GetArgument<P2>(info, 2);                                                       \
        P3 p3 = GetArgument<P3>(info, 3);                                                       \
        P4 p4 = GetArgument<P4>(info, 4);                                                       \
        P5 p5 = GetArgument<P5>(info, 5);                                                       \
        P6 p6 = GetArgument<P6>(info, 6);                                                       \
        P7 p7 = GetArgument<P7>(info, 7);                                                       \
        P8 p8 = GetArgument<P8>(info, 8);                                                       \
        P9 p9 = GetArgument<P9>(info, 9);                                                       \
        P10 p10 = GetArgument<P10>(info, 10);                                                   \
        /* CC-OFFNXT(G.PRE.05) function gen */                                                  \
        return MakeResult<Ret>(info, impl_##name(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10)); \
    }                                                                                           \
    MAKE_NODE_EXPORT(TS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) solid logic
#define TS_INTEROP_12(name, Ret, P0, P1, P2, P3, P4, P5, P6, P7, P8, P9, P10, P11)                   \
    napi_value Node_##name(napi_env env, napi_callback_info cbinfo)                                  \
    {                                                                                                \
        TS_MAYBE_LOG(name)                                                                           \
        CallbackInfo info(env, cbinfo);                                                              \
        P0 p0 = GetArgument<P0>(info, 0);                                                            \
        P1 p1 = GetArgument<P1>(info, 1);                                                            \
        P2 p2 = GetArgument<P2>(info, 2);                                                            \
        P3 p3 = GetArgument<P3>(info, 3);                                                            \
        P4 p4 = GetArgument<P4>(info, 4);                                                            \
        P5 p5 = GetArgument<P5>(info, 5);                                                            \
        P6 p6 = GetArgument<P6>(info, 6);                                                            \
        P7 p7 = GetArgument<P7>(info, 7);                                                            \
        P8 p8 = GetArgument<P8>(info, 8);                                                            \
        P9 p9 = GetArgument<P9>(info, 9);                                                            \
        P10 p10 = GetArgument<P10>(info, 10);                                                        \
        P11 p11 = GetArgument<P11>(info, 11);                                                        \
        /* CC-OFFNXT(G.PRE.05) function gen */                                                       \
        return MakeResult<Ret>(info, impl_##name(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11)); \
    }                                                                                                \
    MAKE_NODE_EXPORT(TS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) solid logic
#define TS_INTEROP_13(name, Ret, P0, P1, P2, P3, P4, P5, P6, P7, P8, P9, P10, P11, P12)                   \
    napi_value Node_##name(napi_env env, napi_callback_info cbinfo)                                       \
    {                                                                                                     \
        TS_MAYBE_LOG(name)                                                                                \
        CallbackInfo info(env, cbinfo);                                                                   \
        P0 p0 = GetArgument<P0>(info, 0);                                                                 \
        P1 p1 = GetArgument<P1>(info, 1);                                                                 \
        P2 p2 = GetArgument<P2>(info, 2);                                                                 \
        P3 p3 = GetArgument<P3>(info, 3);                                                                 \
        P4 p4 = GetArgument<P4>(info, 4);                                                                 \
        P5 p5 = GetArgument<P5>(info, 5);                                                                 \
        P6 p6 = GetArgument<P6>(info, 6);                                                                 \
        P7 p7 = GetArgument<P7>(info, 7);                                                                 \
        P8 p8 = GetArgument<P8>(info, 8);                                                                 \
        P9 p9 = GetArgument<P9>(info, 9);                                                                 \
        P10 p10 = GetArgument<P10>(info, 10);                                                             \
        P11 p11 = GetArgument<P11>(info, 11);                                                             \
        P12 p12 = GetArgument<P12>(info, 12);                                                             \
        /* CC-OFFNXT(G.PRE.05) function gen */                                                            \
        return MakeResult<Ret>(info, impl_##name(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12)); \
    }                                                                                                     \
    MAKE_NODE_EXPORT(TS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) solid logic
#define TS_INTEROP_14(name, Ret, P0, P1, P2, P3, P4, P5, P6, P7, P8, P9, P10, P11, P12, P13)                   \
    napi_value Node_##name(napi_env env, napi_callback_info cbinfo)                                            \
    {                                                                                                          \
        TS_MAYBE_LOG(name)                                                                                     \
        CallbackInfo info(env, cbinfo);                                                                        \
        P0 p0 = GetArgument<P0>(info, 0);                                                                      \
        P1 p1 = GetArgument<P1>(info, 1);                                                                      \
        P2 p2 = GetArgument<P2>(info, 2);                                                                      \
        P3 p3 = GetArgument<P3>(info, 3);                                                                      \
        P4 p4 = GetArgument<P4>(info, 4);                                                                      \
        P5 p5 = GetArgument<P5>(info, 5);                                                                      \
        P6 p6 = GetArgument<P6>(info, 6);                                                                      \
        P7 p7 = GetArgument<P7>(info, 7);                                                                      \
        P8 p8 = GetArgument<P8>(info, 8);                                                                      \
        P9 p9 = GetArgument<P9>(info, 9);                                                                      \
        P10 p10 = GetArgument<P10>(info, 10);                                                                  \
        P11 p11 = GetArgument<P11>(info, 11);                                                                  \
        P12 p12 = GetArgument<P12>(info, 12);                                                                  \
        P13 p13 = GetArgument<P13>(info, 13);                                                                  \
        /* CC-OFFNXT(G.PRE.05) function gen */                                                                 \
        return MakeResult<Ret>(info, impl_##name(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13)); \
    }                                                                                                          \
    MAKE_NODE_EXPORT(TS_INTEROP_MODULE, name)

#define TS_INTEROP_V0(name)                                         \
    napi_value Node_##name(napi_env env, napi_callback_info cbinfo) \
    {                                                               \
        TS_MAYBE_LOG(name)                                          \
        CallbackInfo info(env, cbinfo);                             \
        impl_##name();                                              \
        /* CC-OFFNXT(G.PRE.05) function gen */                      \
        return MakeVoid(info);                                      \
    }                                                               \
    MAKE_NODE_EXPORT(TS_INTEROP_MODULE, name)

#define TS_INTEROP_V1(name, P0)                                     \
    napi_value Node_##name(napi_env env, napi_callback_info cbinfo) \
    {                                                               \
        TS_MAYBE_LOG(name)                                          \
        CallbackInfo info(env, cbinfo);                             \
        P0 p0 = GetArgument<P0>(info, 0);                           \
        impl_##name(p0);                                            \
        /* CC-OFFNXT(G.PRE.05) function gen */                      \
        return MakeVoid(info);                                      \
    }                                                               \
    MAKE_NODE_EXPORT(TS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) solid logic
#define TS_INTEROP_V2(name, P0, P1)                                 \
    napi_value Node_##name(napi_env env, napi_callback_info cbinfo) \
    {                                                               \
        TS_MAYBE_LOG(name)                                          \
        CallbackInfo info(env, cbinfo);                             \
        P0 p0 = GetArgument<P0>(info, 0);                           \
        P1 p1 = GetArgument<P1>(info, 1);                           \
        impl_##name(p0, p1);                                        \
        /* CC-OFFNXT(G.PRE.05) function gen */                      \
        return MakeVoid(info);                                      \
    }                                                               \
    MAKE_NODE_EXPORT(TS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) solid logic
#define TS_INTEROP_V3(name, P0, P1, P2)                             \
    napi_value Node_##name(napi_env env, napi_callback_info cbinfo) \
    {                                                               \
        TS_MAYBE_LOG(name)                                          \
        CallbackInfo info(env, cbinfo);                             \
        P0 p0 = GetArgument<P0>(info, 0);                           \
        P1 p1 = GetArgument<P1>(info, 1);                           \
        P2 p2 = GetArgument<P2>(info, 2);                           \
        impl_##name(p0, p1, p2);                                    \
        /* CC-OFFNXT(G.PRE.05) function gen */                      \
        return MakeVoid(info);                                      \
    }                                                               \
    MAKE_NODE_EXPORT(TS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) solid logic
#define TS_INTEROP_V4(name, P0, P1, P2, P3)                         \
    napi_value Node_##name(napi_env env, napi_callback_info cbinfo) \
    {                                                               \
        TS_MAYBE_LOG(name)                                          \
        CallbackInfo info(env, cbinfo);                             \
        P0 p0 = GetArgument<P0>(info, 0);                           \
        P1 p1 = GetArgument<P1>(info, 1);                           \
        P2 p2 = GetArgument<P2>(info, 2);                           \
        P3 p3 = GetArgument<P3>(info, 3);                           \
        impl_##name(p0, p1, p2, p3);                                \
        /* CC-OFFNXT(G.PRE.05) function gen */                      \
        return MakeVoid(info);                                      \
    }                                                               \
    MAKE_NODE_EXPORT(TS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) solid logic
#define TS_INTEROP_V5(name, P0, P1, P2, P3, P4)                     \
    napi_value Node_##name(napi_env env, napi_callback_info cbinfo) \
    {                                                               \
        TS_MAYBE_LOG(name)                                          \
        CallbackInfo info(env, cbinfo);                             \
        P0 p0 = GetArgument<P0>(info, 0);                           \
        P1 p1 = GetArgument<P1>(info, 1);                           \
        P2 p2 = GetArgument<P2>(info, 2);                           \
        P3 p3 = GetArgument<P3>(info, 3);                           \
        P4 p4 = GetArgument<P4>(info, 4);                           \
        impl_##name(p0, p1, p2, p3, p4);                            \
        /* CC-OFFNXT(G.PRE.05) function gen */                      \
        return MakeVoid(info);                                      \
    }                                                               \
    MAKE_NODE_EXPORT(TS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) solid logic
#define TS_INTEROP_V6(name, P0, P1, P2, P3, P4, P5)                 \
    napi_value Node_##name(napi_env env, napi_callback_info cbinfo) \
    {                                                               \
        TS_MAYBE_LOG(name)                                          \
        CallbackInfo info(env, cbinfo);                             \
        P0 p0 = GetArgument<P0>(info, 0);                           \
        P1 p1 = GetArgument<P1>(info, 1);                           \
        P2 p2 = GetArgument<P2>(info, 2);                           \
        P3 p3 = GetArgument<P3>(info, 3);                           \
        P4 p4 = GetArgument<P4>(info, 4);                           \
        P5 p5 = GetArgument<P5>(info, 5);                           \
        impl_##name(p0, p1, p2, p3, p4, p5);                        \
        /* CC-OFFNXT(G.PRE.05) function gen */                      \
        return MakeVoid(info);                                      \
    }                                                               \
    MAKE_NODE_EXPORT(TS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) solid logic
#define TS_INTEROP_V7(name, P0, P1, P2, P3, P4, P5, P6)             \
    napi_value Node_##name(napi_env env, napi_callback_info cbinfo) \
    {                                                               \
        TS_MAYBE_LOG(name)                                          \
        CallbackInfo info(env, cbinfo);                             \
        P0 p0 = GetArgument<P0>(info, 0);                           \
        P1 p1 = GetArgument<P1>(info, 1);                           \
        P2 p2 = GetArgument<P2>(info, 2);                           \
        P3 p3 = GetArgument<P3>(info, 3);                           \
        P4 p4 = GetArgument<P4>(info, 4);                           \
        P5 p5 = GetArgument<P5>(info, 5);                           \
        P6 p6 = GetArgument<P6>(info, 6);                           \
        impl_##name(p0, p1, p2, p3, p4, p5, p6);                    \
        /* CC-OFFNXT(G.PRE.05) function gen */                      \
        return MakeVoid(info);                                      \
    }                                                               \
    MAKE_NODE_EXPORT(TS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) solid logic
#define TS_INTEROP_V8(name, P0, P1, P2, P3, P4, P5, P6, P7)         \
    napi_value Node_##name(napi_env env, napi_callback_info cbinfo) \
    {                                                               \
        TS_MAYBE_LOG(name)                                          \
        CallbackInfo info(env, cbinfo);                             \
        P0 p0 = GetArgument<P0>(info, 0);                           \
        P1 p1 = GetArgument<P1>(info, 1);                           \
        P2 p2 = GetArgument<P2>(info, 2);                           \
        P3 p3 = GetArgument<P3>(info, 3);                           \
        P4 p4 = GetArgument<P4>(info, 4);                           \
        P5 p5 = GetArgument<P5>(info, 5);                           \
        P6 p6 = GetArgument<P6>(info, 6);                           \
        P7 p7 = GetArgument<P7>(info, 7);                           \
        impl_##name(p0, p1, p2, p3, p4, p5, p6, p7);                \
        /* CC-OFFNXT(G.PRE.05) function gen */                      \
        return MakeVoid(info);                                      \
    }                                                               \
    MAKE_NODE_EXPORT(TS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) solid logic
#define TS_INTEROP_V9(name, P0, P1, P2, P3, P4, P5, P6, P7, P8)     \
    napi_value Node_##name(napi_env env, napi_callback_info cbinfo) \
    {                                                               \
        TS_MAYBE_LOG(impl_##name)                                   \
        CallbackInfo info(env, cbinfo);                             \
        P0 p0 = GetArgument<P0>(info, 0);                           \
        P1 p1 = GetArgument<P1>(info, 1);                           \
        P2 p2 = GetArgument<P2>(info, 2);                           \
        P3 p3 = GetArgument<P3>(info, 3);                           \
        P4 p4 = GetArgument<P4>(info, 4);                           \
        P5 p5 = GetArgument<P5>(info, 5);                           \
        P6 p6 = GetArgument<P6>(info, 6);                           \
        P7 p7 = GetArgument<P7>(info, 7);                           \
        P8 p8 = GetArgument<P8>(info, 8);                           \
        impl_##name(p0, p1, p2, p3, p4, p5, p6, p7, p8);            \
        /* CC-OFFNXT(G.PRE.05) function gen */                      \
        return MakeVoid(info);                                      \
    }                                                               \
    MAKE_NODE_EXPORT(TS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) solid logic
#define TS_INTEROP_V10(name, P0, P1, P2, P3, P4, P5, P6, P7, P8, P9) \
    napi_value Node_##name(napi_env env, napi_callback_info cbinfo)  \
    {                                                                \
        TS_MAYBE_LOG(name)                                           \
        CallbackInfo info(env, cbinfo);                              \
        P0 p0 = GetArgument<P0>(info, 0);                            \
        P1 p1 = GetArgument<P1>(info, 1);                            \
        P2 p2 = GetArgument<P2>(info, 2);                            \
        P3 p3 = GetArgument<P3>(info, 3);                            \
        P4 p4 = GetArgument<P4>(info, 4);                            \
        P5 p5 = GetArgument<P5>(info, 5);                            \
        P6 p6 = GetArgument<P6>(info, 6);                            \
        P7 p7 = GetArgument<P7>(info, 7);                            \
        P8 p8 = GetArgument<P8>(info, 8);                            \
        P9 p9 = GetArgument<P9>(info, 9);                            \
        impl_##name(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9);         \
        /* CC-OFFNXT(G.PRE.05) function gen */                       \
        return MakeVoid(info);                                       \
    }                                                                \
    MAKE_NODE_EXPORT(TS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) solid logic
#define TS_INTEROP_V11(name, P0, P1, P2, P3, P4, P5, P6, P7, P8, P9, P10) \
    napi_value Node_##name(napi_env env, napi_callback_info cbinfo)       \
    {                                                                     \
        TS_MAYBE_LOG(impl_##name)                                         \
        CallbackInfo info(env, cbinfo);                                   \
        P0 p0 = GetArgument<P0>(info, 0);                                 \
        P1 p1 = GetArgument<P1>(info, 1);                                 \
        P2 p2 = GetArgument<P2>(info, 2);                                 \
        P3 p3 = GetArgument<P3>(info, 3);                                 \
        P4 p4 = GetArgument<P4>(info, 4);                                 \
        P5 p5 = GetArgument<P5>(info, 5);                                 \
        P6 p6 = GetArgument<P6>(info, 6);                                 \
        P7 p7 = GetArgument<P7>(info, 7);                                 \
        P8 p8 = GetArgument<P8>(info, 8);                                 \
        P9 p9 = GetArgument<P9>(info, 9);                                 \
        P10 p10 = GetArgument<P10>(info, 10);                             \
        impl_##name(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10);         \
        /* CC-OFFNXT(G.PRE.05) function gen */                            \
        return MakeVoid(info);                                            \
    }                                                                     \
    MAKE_NODE_EXPORT(TS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) solid logic
#define TS_INTEROP_V12(name, P0, P1, P2, P3, P4, P5, P6, P7, P8, P9, P10, P11) \
    napi_value Node_##name(napi_env env, napi_callback_info cbinfo)            \
    {                                                                          \
        TS_MAYBE_LOG(impl_##name)                                              \
        CallbackInfo info(env, cbinfo);                                        \
        P0 p0 = GetArgument<P0>(info, 0);                                      \
        P1 p1 = GetArgument<P1>(info, 1);                                      \
        P2 p2 = GetArgument<P2>(info, 2);                                      \
        P3 p3 = GetArgument<P3>(info, 3);                                      \
        P4 p4 = GetArgument<P4>(info, 4);                                      \
        P5 p5 = GetArgument<P5>(info, 5);                                      \
        P6 p6 = GetArgument<P6>(info, 6);                                      \
        P7 p7 = GetArgument<P7>(info, 7);                                      \
        P8 p8 = GetArgument<P8>(info, 8);                                      \
        P9 p9 = GetArgument<P9>(info, 9);                                      \
        P10 p10 = GetArgument<P10>(info, 10);                                  \
        P11 p11 = GetArgument<P11>(info, 11);                                  \
        impl_##name(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11);         \
        /* CC-OFFNXT(G.PRE.05) function gen */                                 \
        return MakeVoid(info);                                                 \
    }                                                                          \
    MAKE_NODE_EXPORT(TS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) solid logic
#define TS_INTEROP_V13(name, P0, P1, P2, P3, P4, P5, P6, P7, P8, P9, P10, P11, P12) \
    napi_value Node_##name(napi_env env, napi_callback_info cbinfo)                 \
    {                                                                               \
        TS_MAYBE_LOG(impl_##name)                                                   \
        CallbackInfo info(env, cbinfo);                                             \
        P0 p0 = GetArgument<P0>(info, 0);                                           \
        P1 p1 = GetArgument<P1>(info, 1);                                           \
        P2 p2 = GetArgument<P2>(info, 2);                                           \
        P3 p3 = GetArgument<P3>(info, 3);                                           \
        P4 p4 = GetArgument<P4>(info, 4);                                           \
        P5 p5 = GetArgument<P5>(info, 5);                                           \
        P6 p6 = GetArgument<P6>(info, 6);                                           \
        P7 p7 = GetArgument<P7>(info, 7);                                           \
        P8 p8 = GetArgument<P8>(info, 8);                                           \
        P9 p9 = GetArgument<P9>(info, 9);                                           \
        P10 p10 = GetArgument<P10>(info, 10);                                       \
        P11 p11 = GetArgument<P11>(info, 11);                                       \
        P12 p12 = GetArgument<P12>(info, 12);                                       \
        impl_##name(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12);         \
        /* CC-OFFNXT(G.PRE.05) function gen */                                      \
        return MakeVoid(info);                                                      \
    }                                                                               \
    MAKE_NODE_EXPORT(TS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) solid logic
#define TS_INTEROP_V14(name, P0, P1, P2, P3, P4, P5, P6, P7, P8, P9, P10, P11, P12, P13) \
    napi_value Node_##name(napi_env env, napi_callback_info cbinfo)                      \
    {                                                                                    \
        TS_MAYBE_LOG(name)                                                               \
        CallbackInfo info(env, cbinfo);                                                  \
        P0 p0 = GetArgument<P0>(info, 0);                                                \
        P1 p1 = GetArgument<P1>(info, 1);                                                \
        P2 p2 = GetArgument<P2>(info, 2);                                                \
        P3 p3 = GetArgument<P3>(info, 3);                                                \
        P4 p4 = GetArgument<P4>(info, 4);                                                \
        P5 p5 = GetArgument<P5>(info, 5);                                                \
        P6 p6 = GetArgument<P6>(info, 6);                                                \
        P7 p7 = GetArgument<P7>(info, 7);                                                \
        P8 p8 = GetArgument<P8>(info, 8);                                                \
        P9 p9 = GetArgument<P9>(info, 9);                                                \
        P10 p10 = GetArgument<P10>(info, 10);                                            \
        P11 p11 = GetArgument<P11>(info, 11);                                            \
        P12 p12 = GetArgument<P12>(info, 12);                                            \
        P13 p13 = GetArgument<P13>(info, 13);                                            \
        impl_##name(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13);         \
        /* CC-OFFNXT(G.PRE.05) function gen */                                           \
        return MakeVoid(info);                                                           \
    }                                                                                    \
    MAKE_NODE_EXPORT(TS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) solid logic
#define TS_INTEROP_V15(name, P0, P1, P2, P3, P4, P5, P6, P7, P8, P9, P10, P11, P12, P13, P14) \
    napi_value Node_##name(napi_env env, napi_callback_info cbinfo)                           \
    {                                                                                         \
        TS_MAYBE_LOG(name)                                                                    \
        CallbackInfo info(env, cbinfo);                                                       \
        P0 p0 = GetArgument<P0>(info, 0);                                                     \
        P1 p1 = GetArgument<P1>(info, 1);                                                     \
        P2 p2 = GetArgument<P2>(info, 2);                                                     \
        P3 p3 = GetArgument<P3>(info, 3);                                                     \
        P4 p4 = GetArgument<P4>(info, 4);                                                     \
        P5 p5 = GetArgument<P5>(info, 5);                                                     \
        P6 p6 = GetArgument<P6>(info, 6);                                                     \
        P7 p7 = GetArgument<P7>(info, 7);                                                     \
        P8 p8 = GetArgument<P8>(info, 8);                                                     \
        P9 p9 = GetArgument<P9>(info, 9);                                                     \
        P10 p10 = GetArgument<P10>(info, 10);                                                 \
        P11 p11 = GetArgument<P11>(info, 11);                                                 \
        P12 p12 = GetArgument<P12>(info, 12);                                                 \
        P13 p13 = GetArgument<P13>(info, 13);                                                 \
        P14 p14 = GetArgument<P14>(info, 14);                                                 \
        impl_##name(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14);         \
        /* CC-OFFNXT(G.PRE.05) function gen */                                                \
        return MakeVoid(info);                                                                \
    }                                                                                         \
    MAKE_NODE_EXPORT(TS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) solid logic
#define TS_INTEROP_CTX_0(name, Ret)                                          \
    napi_value Node_##name(napi_env env, napi_callback_info cbinfo)          \
    {                                                                        \
        TS_MAYBE_LOG(impl_##name)                                            \
        CallbackInfo info(env, cbinfo);                                      \
        KVMContext ctx = reinterpret_cast<KVMContext>((napi_env)info.Env()); \
        /* CC-OFFNXT(G.PRE.05) function gen */                               \
        return MakeResult<Ret>(info, impl_##name(ctx));                      \
    }                                                                        \
    MAKE_NODE_EXPORT(TS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) solid logic
#define TS_INTEROP_CTX_1(name, Ret, P0)                                      \
    napi_value Node_##name(napi_env env, napi_callback_info cbinfo)          \
    {                                                                        \
        TS_MAYBE_LOG(impl_##name)                                            \
        CallbackInfo info(env, cbinfo);                                      \
        KVMContext ctx = reinterpret_cast<KVMContext>((napi_env)info.Env()); \
        P0 p0 = GetArgument<P0>(info, 0);                                    \
        /* CC-OFFNXT(G.PRE.05) function gen */                               \
        return MakeResult<Ret>(info, impl_##name(ctx, p0));                  \
    }                                                                        \
    MAKE_NODE_EXPORT(TS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) solid logic
#define TS_INTEROP_CTX_2(name, Ret, P0, P1)                                  \
    napi_value Node_##name(napi_env env, napi_callback_info cbinfo)          \
    {                                                                        \
        TS_MAYBE_LOG(name)                                                   \
        CallbackInfo info(env, cbinfo);                                      \
        KVMContext ctx = reinterpret_cast<KVMContext>((napi_env)info.Env()); \
        P0 p0 = GetArgument<P0>(info, 0);                                    \
        P1 p1 = GetArgument<P1>(info, 1);                                    \
        /* CC-OFFNXT(G.PRE.05) function gen */                               \
        return MakeResult<Ret>(info, impl_##name(ctx, p0, p1));              \
    }                                                                        \
    MAKE_NODE_EXPORT(TS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) solid logic
#define TS_INTEROP_CTX_3(name, Ret, P0, P1, P2)                              \
    napi_value Node_##name(napi_env env, napi_callback_info cbinfo)          \
    {                                                                        \
        TS_MAYBE_LOG(name)                                                   \
        CallbackInfo info(env, cbinfo);                                      \
        KVMContext ctx = reinterpret_cast<KVMContext>((napi_env)info.Env()); \
        P0 p0 = GetArgument<P0>(info, 0);                                    \
        P1 p1 = GetArgument<P1>(info, 1);                                    \
        P2 p2 = GetArgument<P2>(info, 2);                                    \
        /* CC-OFFNXT(G.PRE.05) function gen */                               \
        return MakeResult<Ret>(info, impl_##name(ctx, p0, p1, p2));          \
    }                                                                        \
    MAKE_NODE_EXPORT(TS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) solid logic
#define TS_INTEROP_CTX_4(name, Ret, P0, P1, P2, P3)                          \
    napi_value Node_##name(napi_env env, napi_callback_info cbinfo)          \
    {                                                                        \
        TS_MAYBE_LOG(name)                                                   \
        CallbackInfo info(env, cbinfo);                                      \
        KVMContext ctx = reinterpret_cast<KVMContext>((napi_env)info.Env()); \
        P0 p0 = GetArgument<P0>(info, 0);                                    \
        P1 p1 = GetArgument<P1>(info, 1);                                    \
        P2 p2 = GetArgument<P2>(info, 2);                                    \
        P3 p3 = GetArgument<P3>(info, 3);                                    \
        /* CC-OFFNXT(G.PRE.05) function gen */                               \
        return MakeResult<Ret>(info, impl_##name(ctx, p0, p1, p2, p3));      \
    }                                                                        \
    MAKE_NODE_EXPORT(TS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) solid logic
#define TS_INTEROP_CTX_V0(name)                                              \
    napi_value Node_##name(napi_env env, napi_callback_info cbinfo)          \
    {                                                                        \
        TS_MAYBE_LOG(name)                                                   \
        CallbackInfo info(env, cbinfo);                                      \
        KVMContext ctx = reinterpret_cast<KVMContext>((napi_env)info.Env()); \
        impl_##name(ctx);                                                    \
        /* CC-OFFNXT(G.PRE.05) function gen */                               \
        return MakeVoid(info);                                               \
    }                                                                        \
    MAKE_NODE_EXPORT(TS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) solid logic
#define TS_INTEROP_CTX_V1(name, P0)                                          \
    napi_value Node_##name(napi_env env, napi_callback_info cbinfo)          \
    {                                                                        \
        TS_MAYBE_LOG(name)                                                   \
        CallbackInfo info(env, cbinfo);                                      \
        KVMContext ctx = reinterpret_cast<KVMContext>((napi_env)info.Env()); \
        P0 p0 = GetArgument<P0>(info, 0);                                    \
        impl_##name(ctx, p0);                                                \
        /* CC-OFFNXT(G.PRE.05) function gen */                               \
        return MakeVoid(info);                                               \
    }                                                                        \
    MAKE_NODE_EXPORT(TS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) solid logic
#define TS_INTEROP_CTX_V2(name, P0, P1)                                      \
    napi_value Node_##name(napi_env env, napi_callback_info cbinfo)          \
    {                                                                        \
        TS_MAYBE_LOG(name)                                                   \
        CallbackInfo info(env, cbinfo);                                      \
        KVMContext ctx = reinterpret_cast<KVMContext>((napi_env)info.Env()); \
        P0 p0 = GetArgument<P0>(info, 0);                                    \
        P1 p1 = GetArgument<P1>(info, 1);                                    \
        impl_##name(ctx, p0, p1);                                            \
        /* CC-OFFNXT(G.PRE.05) function gen */                               \
        return MakeVoid(info);                                               \
    }                                                                        \
    MAKE_NODE_EXPORT(TS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) solid logic
#define TS_INTEROP_CTX_V3(name, P0, P1, P2)                                  \
    napi_value Node_##name(napi_env env, napi_callback_info cbinfo)          \
    {                                                                        \
        TS_MAYBE_LOG(name)                                                   \
        CallbackInfo info(env, cbinfo);                                      \
        KVMContext ctx = reinterpret_cast<KVMContext>((napi_env)info.Env()); \
        P0 p0 = GetArgument<P0>(info, 0);                                    \
        P1 p1 = GetArgument<P1>(info, 1);                                    \
        P2 p2 = GetArgument<P2>(info, 2);                                    \
        impl_##name(ctx, p0, p1, p2);                                        \
        /* CC-OFFNXT(G.PRE.05) function gen */                               \
        return MakeVoid(info);                                               \
    }                                                                        \
    MAKE_NODE_EXPORT(TS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) solid logic
#define NODEJS_GET_AND_THROW_LAST_ERROR(env)                                                           \
    do {                                                                                               \
        const napi_extended_error_info *error_info;                                                    \
        napi_get_last_error_info((env), &error_info);                                                  \
        bool is_pending;                                                                               \
        napi_is_exception_pending((env), &is_pending);                                                 \
        /* If an exception is already pending, don't rethrow it */                                     \
        if (!is_pending) {                                                                             \
            const char *error_message =                                                                \
                error_info->error_message != NULL ? error_info->error_message : "empty error message"; \
            napi_throw_error((env), NULL, error_message);                                              \
        }                                                                                              \
    } while (0)

napi_value GetKoalaNapiCallbackDispatcher(napi_env env);

// CC-OFFNXT(G.PRE.06) solid logic
#define TS_INTEROP_CALL_VOID(venv, id, length, args)                                                     \
    {                                                                                                    \
        napi_env env = reinterpret_cast<napi_env>(venv);                                                 \
        napi_value bridge = GetKoalaNapiCallbackDispatcher(env), global = nullptr, return_val = nullptr; \
        napi_handle_scope scope = nullptr;                                                               \
        napi_open_handle_scope(env, &scope);                                                             \
        napi_status status = napi_get_global(env, &global);                                              \
        std::array<napi_value, 3> node_args;                                                             \
        napi_create_int32(env, id, &node_args[0]);                                                       \
        napi_value buffer = nullptr;                                                                     \
        napi_create_external_arraybuffer(                                                                \
            env, args, length, [](napi_env, void *data, void *hint) {}, nullptr, &buffer);               \
        napi_create_typedarray(env, napi_uint8_array, length, buffer, 0, &node_args[1]);                 \
        napi_create_int32(env, length, &node_args[2]);                                                   \
        status = napi_call_function(env, global, bridge, 3, node_args.data(), &return_val);              \
        if (status != napi_ok) {                                                                         \
            NODEJS_GET_AND_THROW_LAST_ERROR((env));                                                      \
        }                                                                                                \
        napi_close_handle_scope(env, scope);                                                             \
    }

// CC-OFFNXT(G.PRE.06) solid logic
#define TS_INTEROP_CALL_INT(venv, id, length, args)                                                                \
    {                                                                                                              \
        napi_env env = reinterpret_cast<napi_env>(venv);                                                           \
        napi_value bridge = GetKoalaNapiCallbackDispatcher(env);                                                   \
        napi_value global = nullptr;                                                                               \
        napi_value return_val = nullptr;                                                                           \
        napi_handle_scope scope = nullptr;                                                                         \
        napi_open_handle_scope(env, &scope);                                                                       \
        napi_status status = napi_get_global(env, &global);                                                        \
        std::array<napi_value, 3> node_args {};                                                                    \
        napi_create_int32(env, id, &node_args[0]);                                                                 \
        napi_value buffer = nullptr;                                                                               \
        napi_create_external_arraybuffer(                                                                          \
            env, args, length, [](napi_env, [[maybe_unused]] void *data, [[maybe_unused]] void *hint) {}, nullptr, \
            &buffer);                                                                                              \
        napi_create_typedarray(env, napi_uint8_array, length, buffer, 0, &node_args[1]);                           \
        napi_create_int32(env, length, &node_args[2]);                                                             \
        status = napi_call_function(env, global, bridge, 3, node_args.data(), &return_val);                        \
        if (status != napi_ok) {                                                                                   \
            NODEJS_GET_AND_THROW_LAST_ERROR((env));                                                                \
        }                                                                                                          \
        int result;                                                                                                \
        status = napi_get_value_int32(env, return_val, &result);                                                   \
        napi_close_handle_scope(env, scope);                                                                       \
        /* CC-OFFNXT(G.PRE.05) function gen */                                                                     \
        return result;                                                                                             \
    }

#define TS_INTEROP_CALL_VOID_INTS32(venv, id, argc, args) TS_INTEROP_CALL_VOID(venv, id, (argc) * sizeof(int32_t), args)
#define TS_INTEROP_CALL_INT_INTS32(venv, id, argc, args) TS_INTEROP_CALL_INT(venv, id, (argc) * sizeof(int32_t), args)

// NOLINTEND(cppcoreguidelines-macro-usage)

#endif  // CONVERTORS_NAPI_H_

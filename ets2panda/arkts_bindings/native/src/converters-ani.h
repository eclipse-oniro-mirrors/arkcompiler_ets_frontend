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

#ifndef CONVERTERS_ANI_H
#define CONVERTERS_ANI_H

#include "ets-types.h"

#include "ani.h"

#include <unordered_map>
#include <vector>
#include <string>
#include <cstring>
#include <stdexcept>

ani_env *GetAniEnv();

// NOLINTBEGIN(cppcoreguidelines-macro-usage)

// CC-OFFNXT(G.PRE.02-CPP) error handling
#define ANI_FATAL_ERROR(status, message)                                                             \
    if ((status) != ANI_OK) {                                                                        \
        throw std::runtime_error(std::string("Fatal error during ANI operation: ").append(message)); \
    }

// NOLINTEND(cppcoreguidelines-macro-usage)

// CC-OFFNXT(G.FUD.06) solid logic, ODR
inline void ThrowEtsError(const std::string &message, const std::string &errorType = "std.core.Error")
{
    ani_env *env = GetAniEnv();

    ani_class cls {};
    ani_status status = env->FindClass(errorType.c_str(), &cls);
    ANI_FATAL_ERROR(status, std::string("Failed to find ").append(errorType).append(" class"));
    ani_method ctor {};
    status = env->Class_FindMethod(cls, "<ctor>", "C{std.core.String}C{escompat.ErrorOptions}:", &ctor);
    ANI_FATAL_ERROR(status, std::string("Failed to find ").append(errorType).append(" constructor"));

    ani_string errMsg {};
    status = env->String_NewUTF8(message.c_str(), message.length() + 1, &errMsg);
    ANI_FATAL_ERROR(status, "Failed to create String object");

    ani_ref undef {};
    status = env->GetUndefined(&undef);
    ANI_FATAL_ERROR(status, "Failed to create Undefined object");

    ani_object err {};
    status = env->Object_New(cls, ctor, &err, errMsg, undef);
    ANI_FATAL_ERROR(status, "Failed to create Error object");
    status = env->ThrowError(static_cast<ani_error>(err));
    ANI_FATAL_ERROR(status, "Failed to throw an error");

    status = env->Reference_Delete(err);
    ANI_FATAL_ERROR(status, "Failed to delete Error object");
    status = env->Reference_Delete(undef);
    ANI_FATAL_ERROR(status, "Failed to delete Undefined object");
    status = env->Reference_Delete(errMsg);
    ANI_FATAL_ERROR(status, "Failed to delete String object");
}

// NOLINTBEGIN(cppcoreguidelines-macro-usage)

// CC-OFFNXT(G.PRE.02-CPP) error handling
#define ANI_THROW_IF_FAILED(status, message)                                        \
    if ((status) != ANI_OK) {                                                       \
        ThrowEtsError(std::string("Error during ANI operation: ").append(message)); \
    }

// NOLINTEND(cppcoreguidelines-macro-usage)

template <class T>
struct InteropTypeConverter;

template <>
struct InteropTypeConverter<EtsBoolean> {
    using InteropType = ani_boolean;
    static EtsBoolean ConvertFrom([[maybe_unused]] ani_env *env, InteropType value)
    {
        return value;
    }
    static InteropType ConvertTo([[maybe_unused]] ani_env *env, EtsBoolean value)
    {
        return value;
    }
    static void Release([[maybe_unused]] ani_env *env, [[maybe_unused]] InteropType value,
                        [[maybe_unused]] const EtsBoolean &converted)
    {
    }
};

template <>
struct InteropTypeConverter<EtsByte> {
    using InteropType = ani_byte;
    static EtsByte ConvertFrom([[maybe_unused]] ani_env *env, InteropType value)
    {
        return value;
    }
    static InteropType ConvertTo([[maybe_unused]] ani_env *env, EtsByte value)
    {
        return value;
    }
    static void Release([[maybe_unused]] ani_env *env, [[maybe_unused]] InteropType value,
                        [[maybe_unused]] const EtsByte &converted)
    {
    }
};

template <>
struct InteropTypeConverter<EtsShort> {
    using InteropType = ani_short;
    static EtsShort ConvertFrom([[maybe_unused]] ani_env *env, InteropType value)
    {
        return value;
    }
    static InteropType ConvertTo([[maybe_unused]] ani_env *env, EtsShort value)
    {
        return value;
    }
    static void Release([[maybe_unused]] ani_env *env, [[maybe_unused]] InteropType value,
                        [[maybe_unused]] const EtsShort &converted)
    {
    }
};

template <>
struct InteropTypeConverter<EtsInt> {
    using InteropType = ani_int;
    static EtsInt ConvertFrom([[maybe_unused]] ani_env *env, InteropType value)
    {
        return value;
    }
    static InteropType ConvertTo([[maybe_unused]] ani_env *env, EtsInt value)
    {
        return value;
    }
    static void Release([[maybe_unused]] ani_env *env, [[maybe_unused]] InteropType value,
                        [[maybe_unused]] const EtsInt &converted)
    {
    }
};

template <>
struct InteropTypeConverter<EtsLong> {
    using InteropType = ani_long;
    static EtsLong ConvertFrom([[maybe_unused]] ani_env *env, InteropType value)
    {
        return value;
    }
    static InteropType ConvertTo([[maybe_unused]] ani_env *env, EtsLong value)
    {
        return value;
    }
    static void Release([[maybe_unused]] ani_env *env, [[maybe_unused]] InteropType value,
                        [[maybe_unused]] const EtsLong &converted)
    {
    }
};

template <>
struct InteropTypeConverter<EtsFloat> {
    using InteropType = ani_float;
    static EtsFloat ConvertFrom([[maybe_unused]] ani_env *env, InteropType value)
    {
        return value;
    }
    static InteropType ConvertTo([[maybe_unused]] ani_env *env, EtsFloat value)
    {
        return value;
    }
    static void Release([[maybe_unused]] ani_env *env, [[maybe_unused]] InteropType value,
                        [[maybe_unused]] const EtsFloat &converted)
    {
    }
};

template <>
struct InteropTypeConverter<EtsDouble> {
    using InteropType = ani_double;
    static EtsDouble ConvertFrom([[maybe_unused]] ani_env *env, InteropType value)
    {
        return value;
    }
    static InteropType ConvertTo([[maybe_unused]] ani_env *env, EtsDouble value)
    {
        return value;
    }
    static void Release([[maybe_unused]] ani_env *env, [[maybe_unused]] InteropType value,
                        [[maybe_unused]] const EtsDouble &converted)
    {
    }
};

template <>
struct InteropTypeConverter<EtsChar> {
    using InteropType = ani_char;
    static EtsChar ConvertFrom([[maybe_unused]] ani_env *env, InteropType value)
    {
        return value;
    }
    static InteropType ConvertTo([[maybe_unused]] ani_env *env, EtsChar value)
    {
        return value;
    }
    static void Release([[maybe_unused]] ani_env *env, [[maybe_unused]] InteropType value,
                        [[maybe_unused]] const EtsChar &converted)
    {
    }
};

template <>
struct InteropTypeConverter<EtsStringPtr> {
    using InteropType = ani_string;
    static EtsStringPtr ConvertFrom(ani_env *env, InteropType value)
    {
        if (value == nullptr) {
            return EtsStringPtr();
        }
        EtsStringPtr result;
        ani_size length = 0;
        ani_status status = env->String_GetUTF8Size(value, &length);
        ANI_THROW_IF_FAILED(status, "Failed to get size of UTF-8 string");
        result.Resize(length + 1);
        ani_size sz;
        status = env->String_GetUTF8(value, result.Data(), result.Length(), &sz);
        ANI_THROW_IF_FAILED(status, "Failed to get value of UTF-8 string");

        return result;
    }
    static InteropType ConvertTo(ani_env *env, const EtsStringPtr &value)
    {
        ani_string result;
        ani_status status = env->String_NewUTF8(value.CStr(), value.Length(), &result);
        ANI_THROW_IF_FAILED(status, "Failed to create a UTF-8 string");
        return result;
    }
    static void Release([[maybe_unused]] ani_env *env, [[maybe_unused]] InteropType value,
                        [[maybe_unused]] const EtsStringPtr &converted)
    {
    }
};

template <>
struct InteropTypeConverter<EtsCString> {
    using InteropType = ani_string;
    static EtsCString ConvertFrom(ani_env *env, InteropType value) = delete;
    static InteropType ConvertTo(ani_env *env, const EtsCString value)
    {
        ani_string result;
        ani_status status = env->String_NewUTF8(value, std::strlen(value), &result);
        ANI_THROW_IF_FAILED(status, "Failed to create UTF-8 string");
        return result;
    }
    static void Release([[maybe_unused]] ani_env *env, [[maybe_unused]] InteropType value,
                        [[maybe_unused]] const EtsCString converted)
    {
    }
};

template <>
struct InteropTypeConverter<EtsNativePointer> {
    using InteropType = ani_long;
    static EtsNativePointer ConvertFrom([[maybe_unused]] ani_env *env, InteropType value)
    {
        return reinterpret_cast<EtsNativePointer>(value);
    }
    static InteropType ConvertTo([[maybe_unused]] ani_env *env, EtsNativePointer value)
    {
        return static_cast<int64_t>(reinterpret_cast<intptr_t>(value));
    }
    static void Release([[maybe_unused]] ani_env *env, [[maybe_unused]] InteropType value,
                        [[maybe_unused]] const EtsNativePointer &converted)
    {
    }
};

template <>
struct InteropTypeConverter<EtsStringArray> {
    using InteropType = ani_array;
    static EtsStringArray ConvertFrom(ani_env *env, InteropType value)
    {
        if (value == nullptr) {
            return nullptr;
        }

        // get array size
        ani_size length = 0;
        ani_status status = env->Array_GetLength(value, &length);
        ANI_THROW_IF_FAILED(status, "Failed to get an array length");

        if (length == 0) {
            return nullptr;
        }

        // create result array
        // CC-OFFNXT(G.MEM.01) interop data conversion
        char **result = new char *[length];

        // init array and insert all values
        for (size_t i = 0; i < length; ++i) {
            ani_ref item {};
            status = env->Array_Get(value, i, &item);
            ANI_THROW_IF_FAILED(status, "Failed to get an element of array");

            auto itemStr = reinterpret_cast<ani_string>(item);

            ani_size strLength = 0;
            status = env->String_GetUTF8Size(itemStr, &strLength);
            ANI_THROW_IF_FAILED(status, "Failed to get size of UTF-8 string");

            size_t bufSize = strLength + 1;
            char *buf = new char[bufSize + 1] {};

            ani_size strSize;
            status = env->String_GetUTF8(itemStr, buf, bufSize, &strSize);
            ANI_THROW_IF_FAILED(status, "Failed to get value of UTF-8 string");

            result[i] = buf;
        }

        return result;
    }
    static InteropType ConvertTo(ani_env *env, const EtsStringArray value) = delete;
    static void Release([[maybe_unused]] ani_env *env, InteropType value, const EtsStringArray converted)
    {
        if (value == nullptr) {
            return;
        }

        ani_size length = 0;
        ani_status status = env->Array_GetLength(value, &length);
        ANI_THROW_IF_FAILED(status, "Failed to get an array length");

        if (length > 0) {
            for (size_t i = 0; i < length; ++i) {
                if (converted[i]) {
                    delete[] converted[i];
                }
            }
        }

        if (converted != nullptr) {
            delete[] converted;
        }
    }
};

template <typename Type>
inline typename InteropTypeConverter<Type>::InteropType MakeResult(ani_env *env, Type value)
{
    return InteropTypeConverter<Type>::ConvertTo(env, value);
}

template <typename Type>
inline Type GetArgument(ani_env *env, typename InteropTypeConverter<Type>::InteropType arg)
{
    return InteropTypeConverter<Type>::ConvertFrom(env, arg);
}

template <typename Type>
inline void ReleaseArgument(ani_env *env, typename InteropTypeConverter<Type>::InteropType arg, Type &data)
{
    InteropTypeConverter<Type>::Release(env, arg, data);
}

class Exports {
    std::unordered_map<std::string, std::vector<std::pair<std::string, void *>>> implementations_;

public:
    static Exports *GetInstance();

    std::vector<std::string> GetModules();
    void AddMethod(const char *module, const char *name, void *impl);
    const std::vector<std::pair<std::string, void *>> &GetMethods(const std::string &module);
};

// NOLINTBEGIN(cppcoreguidelines-macro-usage)

// CC-OFFNXT(G.PRE.02-CPP) code generation
#define QUOTE_IMPL(x) #x
// CC-OFFNXT(G.PRE.02-CPP) code generation
#define QUOTE(x) QUOTE_IMPL(x)

#define MAKE_ANI_EXPORT(module, name)                                                                      \
    __attribute__((constructor)) static void __init_##name()                                               \
    {                                                                                                      \
        Exports::GetInstance()->AddMethod(QUOTE(module), "_" #name, reinterpret_cast<void *>(Ani_##name)); \
    }

// NOLINTEND(cppcoreguidelines-macro-usage)

#ifndef ETS_INTEROP_MODULE
#error ETS_INTEROP_MODULE is undefined
#endif

// NOLINTBEGIN(cppcoreguidelines-macro-usage)

#define ETS_INTEROP_0(name, Ret)                                                                      \
    InteropTypeConverter<Ret>::InteropType Ani_##name(ani_env *env, [[maybe_unused]] ani_object self) \
    {                                                                                                 \
        /* CC-OFFNXT(G.PRE.05) code generation */                                                     \
        return MakeResult<Ret>(env, impl_##name());                                                   \
    }                                                                                                 \
    MAKE_ANI_EXPORT(ETS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) code generation
#define ETS_INTEROP_1(name, Ret, P0)                                                                  \
    InteropTypeConverter<Ret>::InteropType Ani_##name(ani_env *env, [[maybe_unused]] ani_object self, \
                                                      InteropTypeConverter<P0>::InteropType _p0)      \
    {                                                                                                 \
        P0 p0 = GetArgument<P0>(env, _p0);                                                            \
        auto res = MakeResult<Ret>(env, impl_##name(p0));                                             \
        ReleaseArgument(env, _p0, p0);                                                                \
        /* CC-OFFNXT(G.PRE.05) code generation */                                                     \
        return res;                                                                                   \
    }                                                                                                 \
    MAKE_ANI_EXPORT(ETS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) code generation
#define ETS_INTEROP_2(name, Ret, P0, P1)                                                              \
    InteropTypeConverter<Ret>::InteropType Ani_##name(ani_env *env, [[maybe_unused]] ani_object self, \
                                                      InteropTypeConverter<P0>::InteropType _p0,      \
                                                      InteropTypeConverter<P1>::InteropType _p1)      \
    {                                                                                                 \
        P0 p0 = GetArgument<P0>(env, _p0);                                                            \
        P1 p1 = GetArgument<P1>(env, _p1);                                                            \
        auto res = MakeResult<Ret>(env, impl_##name(p0, p1));                                         \
        ReleaseArgument(env, _p0, p0);                                                                \
        ReleaseArgument(env, _p1, p1);                                                                \
        /* CC-OFFNXT(G.PRE.05) code generation */                                                     \
        return res;                                                                                   \
    }                                                                                                 \
    MAKE_ANI_EXPORT(ETS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) code generation
#define ETS_INTEROP_3(name, Ret, P0, P1, P2)                                                       \
    InteropTypeConverter<Ret>::InteropType Ani_##name(                                             \
        ani_env *env, [[maybe_unused]] ani_object self, InteropTypeConverter<P0>::InteropType _p0, \
        InteropTypeConverter<P1>::InteropType _p1, InteropTypeConverter<P2>::InteropType _p2)      \
    {                                                                                              \
        P0 p0 = GetArgument<P0>(env, _p0);                                                         \
        P1 p1 = GetArgument<P1>(env, _p1);                                                         \
        P2 p2 = GetArgument<P2>(env, _p2);                                                         \
        auto res = MakeResult<Ret>(env, impl_##name(p0, p1, p2));                                  \
        ReleaseArgument(env, _p0, p0);                                                             \
        ReleaseArgument(env, _p1, p1);                                                             \
        ReleaseArgument(env, _p2, p2);                                                             \
        /* CC-OFFNXT(G.PRE.05) code generation */                                                  \
        return res;                                                                                \
    }                                                                                              \
    MAKE_ANI_EXPORT(ETS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) code generation
#define ETS_INTEROP_4(name, Ret, P0, P1, P2, P3)                                                   \
    InteropTypeConverter<Ret>::InteropType Ani_##name(                                             \
        ani_env *env, [[maybe_unused]] ani_object self, InteropTypeConverter<P0>::InteropType _p0, \
        InteropTypeConverter<P1>::InteropType _p1, InteropTypeConverter<P2>::InteropType _p2,      \
        InteropTypeConverter<P3>::InteropType _p3)                                                 \
    {                                                                                              \
        P0 p0 = GetArgument<P0>(env, _p0);                                                         \
        P1 p1 = GetArgument<P1>(env, _p1);                                                         \
        P2 p2 = GetArgument<P2>(env, _p2);                                                         \
        P3 p3 = GetArgument<P3>(env, _p3);                                                         \
        auto res = MakeResult<Ret>(env, impl_##name(p0, p1, p2, p3));                              \
        ReleaseArgument(env, _p0, p0);                                                             \
        ReleaseArgument(env, _p1, p1);                                                             \
        ReleaseArgument(env, _p2, p2);                                                             \
        ReleaseArgument(env, _p3, p3);                                                             \
        /* CC-OFFNXT(G.PRE.05) code generation */                                                  \
        return res;                                                                                \
    }                                                                                              \
    MAKE_ANI_EXPORT(ETS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) code generation
#define ETS_INTEROP_5(name, Ret, P0, P1, P2, P3, P4)                                               \
    InteropTypeConverter<Ret>::InteropType Ani_##name(                                             \
        ani_env *env, [[maybe_unused]] ani_object self, InteropTypeConverter<P0>::InteropType _p0, \
        InteropTypeConverter<P1>::InteropType _p1, InteropTypeConverter<P2>::InteropType _p2,      \
        InteropTypeConverter<P3>::InteropType _p3, InteropTypeConverter<P4>::InteropType _p4)      \
    {                                                                                              \
        P0 p0 = GetArgument<P0>(env, _p0);                                                         \
        P1 p1 = GetArgument<P1>(env, _p1);                                                         \
        P2 p2 = GetArgument<P2>(env, _p2);                                                         \
        P3 p3 = GetArgument<P3>(env, _p3);                                                         \
        P4 p4 = GetArgument<P4>(env, _p4);                                                         \
        auto res = MakeResult<Ret>(env, impl_##name(p0, p1, p2, p3, p4));                          \
        ReleaseArgument(env, _p0, p0);                                                             \
        ReleaseArgument(env, _p1, p1);                                                             \
        ReleaseArgument(env, _p2, p2);                                                             \
        ReleaseArgument(env, _p3, p3);                                                             \
        ReleaseArgument(env, _p4, p4);                                                             \
        /* CC-OFFNXT(G.PRE.05) code generation */                                                  \
        return res;                                                                                \
    }                                                                                              \
    MAKE_ANI_EXPORT(ETS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) code generation
#define ETS_INTEROP_6(name, Ret, P0, P1, P2, P3, P4, P5)                                           \
    InteropTypeConverter<Ret>::InteropType Ani_##name(                                             \
        ani_env *env, [[maybe_unused]] ani_object self, InteropTypeConverter<P0>::InteropType _p0, \
        InteropTypeConverter<P1>::InteropType _p1, InteropTypeConverter<P2>::InteropType _p2,      \
        InteropTypeConverter<P3>::InteropType _p3, InteropTypeConverter<P4>::InteropType _p4,      \
        InteropTypeConverter<P5>::InteropType _p5)                                                 \
    {                                                                                              \
        P0 p0 = GetArgument<P0>(env, _p0);                                                         \
        P1 p1 = GetArgument<P1>(env, _p1);                                                         \
        P2 p2 = GetArgument<P2>(env, _p2);                                                         \
        P3 p3 = GetArgument<P3>(env, _p3);                                                         \
        P4 p4 = GetArgument<P4>(env, _p4);                                                         \
        P5 p5 = GetArgument<P5>(env, _p5);                                                         \
        auto res = MakeResult<Ret>(env, impl_##name(p0, p1, p2, p3, p4, p5));                      \
        ReleaseArgument(env, _p0, p0);                                                             \
        ReleaseArgument(env, _p1, p1);                                                             \
        ReleaseArgument(env, _p2, p2);                                                             \
        ReleaseArgument(env, _p3, p3);                                                             \
        ReleaseArgument(env, _p4, p4);                                                             \
        ReleaseArgument(env, _p5, p5);                                                             \
        /* CC-OFFNXT(G.PRE.05) code generation */                                                  \
        return res;                                                                                \
    }                                                                                              \
    MAKE_ANI_EXPORT(ETS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) code generation
#define ETS_INTEROP_7(name, Ret, P0, P1, P2, P3, P4, P5, P6)                                       \
    InteropTypeConverter<Ret>::InteropType Ani_##name(                                             \
        ani_env *env, [[maybe_unused]] ani_object self, InteropTypeConverter<P0>::InteropType _p0, \
        InteropTypeConverter<P1>::InteropType _p1, InteropTypeConverter<P2>::InteropType _p2,      \
        InteropTypeConverter<P3>::InteropType _p3, InteropTypeConverter<P4>::InteropType _p4,      \
        InteropTypeConverter<P5>::InteropType _p5, InteropTypeConverter<P6>::InteropType _p6)      \
    {                                                                                              \
        P0 p0 = GetArgument<P0>(env, _p0);                                                         \
        P1 p1 = GetArgument<P1>(env, _p1);                                                         \
        P2 p2 = GetArgument<P2>(env, _p2);                                                         \
        P3 p3 = GetArgument<P3>(env, _p3);                                                         \
        P4 p4 = GetArgument<P4>(env, _p4);                                                         \
        P5 p5 = GetArgument<P5>(env, _p5);                                                         \
        P6 p6 = GetArgument<P6>(env, _p6);                                                         \
        auto res = MakeResult<Ret>(env, impl_##name(p0, p1, p2, p3, p4, p5, p6));                  \
        ReleaseArgument(env, _p0, p0);                                                             \
        ReleaseArgument(env, _p1, p1);                                                             \
        ReleaseArgument(env, _p2, p2);                                                             \
        ReleaseArgument(env, _p3, p3);                                                             \
        ReleaseArgument(env, _p4, p4);                                                             \
        ReleaseArgument(env, _p5, p5);                                                             \
        ReleaseArgument(env, _p6, p6);                                                             \
        /* CC-OFFNXT(G.PRE.05) code generation */                                                  \
        return res;                                                                                \
    }                                                                                              \
    MAKE_ANI_EXPORT(ETS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) code generation
#define ETS_INTEROP_8(name, Ret, P0, P1, P2, P3, P4, P5, P6, P7)                                   \
    InteropTypeConverter<Ret>::InteropType Ani_##name(                                             \
        ani_env *env, [[maybe_unused]] ani_object self, InteropTypeConverter<P0>::InteropType _p0, \
        InteropTypeConverter<P1>::InteropType _p1, InteropTypeConverter<P2>::InteropType _p2,      \
        InteropTypeConverter<P3>::InteropType _p3, InteropTypeConverter<P4>::InteropType _p4,      \
        InteropTypeConverter<P5>::InteropType _p5, InteropTypeConverter<P6>::InteropType _p6,      \
        InteropTypeConverter<P7>::InteropType _p7)                                                 \
    {                                                                                              \
        P0 p0 = GetArgument<P0>(env, _p0);                                                         \
        P1 p1 = GetArgument<P1>(env, _p1);                                                         \
        P2 p2 = GetArgument<P2>(env, _p2);                                                         \
        P3 p3 = GetArgument<P3>(env, _p3);                                                         \
        P4 p4 = GetArgument<P4>(env, _p4);                                                         \
        P5 p5 = GetArgument<P5>(env, _p5);                                                         \
        P6 p6 = GetArgument<P6>(env, _p6);                                                         \
        P7 p7 = GetArgument<P7>(env, _p7);                                                         \
        auto res = MakeResult<Ret>(env, impl_##name(p0, p1, p2, p3, p4, p5, p6, p7));              \
        ReleaseArgument(env, _p0, p0);                                                             \
        ReleaseArgument(env, _p1, p1);                                                             \
        ReleaseArgument(env, _p2, p2);                                                             \
        ReleaseArgument(env, _p3, p3);                                                             \
        ReleaseArgument(env, _p4, p4);                                                             \
        ReleaseArgument(env, _p5, p5);                                                             \
        ReleaseArgument(env, _p6, p6);                                                             \
        ReleaseArgument(env, _p7, p7);                                                             \
        /* CC-OFFNXT(G.PRE.05) code generation */                                                  \
        return res;                                                                                \
    }                                                                                              \
    MAKE_ANI_EXPORT(ETS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) code generation
#define ETS_INTEROP_9(name, Ret, P0, P1, P2, P3, P4, P5, P6, P7, P8)                               \
    InteropTypeConverter<Ret>::InteropType Ani_##name(                                             \
        ani_env *env, [[maybe_unused]] ani_object self, InteropTypeConverter<P0>::InteropType _p0, \
        InteropTypeConverter<P1>::InteropType _p1, InteropTypeConverter<P2>::InteropType _p2,      \
        InteropTypeConverter<P3>::InteropType _p3, InteropTypeConverter<P4>::InteropType _p4,      \
        InteropTypeConverter<P5>::InteropType _p5, InteropTypeConverter<P6>::InteropType _p6,      \
        InteropTypeConverter<P7>::InteropType _p7, InteropTypeConverter<P8>::InteropType _p8)      \
    {                                                                                              \
        P0 p0 = GetArgument<P0>(env, _p0);                                                         \
        P1 p1 = GetArgument<P1>(env, _p1);                                                         \
        P2 p2 = GetArgument<P2>(env, _p2);                                                         \
        P3 p3 = GetArgument<P3>(env, _p3);                                                         \
        P4 p4 = GetArgument<P4>(env, _p4);                                                         \
        P5 p5 = GetArgument<P5>(env, _p5);                                                         \
        P6 p6 = GetArgument<P6>(env, _p6);                                                         \
        P7 p7 = GetArgument<P7>(env, _p7);                                                         \
        P8 p8 = GetArgument<P8>(env, _p8);                                                         \
        auto res = MakeResult<Ret>(env, impl_##name(p0, p1, p2, p3, p4, p5, p6, p7, p8));          \
        ReleaseArgument(env, _p0, p0);                                                             \
        ReleaseArgument(env, _p1, p1);                                                             \
        ReleaseArgument(env, _p2, p2);                                                             \
        ReleaseArgument(env, _p3, p3);                                                             \
        ReleaseArgument(env, _p4, p4);                                                             \
        ReleaseArgument(env, _p5, p5);                                                             \
        ReleaseArgument(env, _p6, p6);                                                             \
        ReleaseArgument(env, _p7, p7);                                                             \
        ReleaseArgument(env, _p8, p8);                                                             \
        /* CC-OFFNXT(G.PRE.05) code generation */                                                  \
        return res;                                                                                \
    }                                                                                              \
    MAKE_ANI_EXPORT(ETS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) code generation
#define ETS_INTEROP_10(name, Ret, P0, P1, P2, P3, P4, P5, P6, P7, P8, P9)                          \
    InteropTypeConverter<Ret>::InteropType Ani_##name(                                             \
        ani_env *env, [[maybe_unused]] ani_object self, InteropTypeConverter<P0>::InteropType _p0, \
        InteropTypeConverter<P1>::InteropType _p1, InteropTypeConverter<P2>::InteropType _p2,      \
        InteropTypeConverter<P3>::InteropType _p3, InteropTypeConverter<P4>::InteropType _p4,      \
        InteropTypeConverter<P5>::InteropType _p5, InteropTypeConverter<P6>::InteropType _p6,      \
        InteropTypeConverter<P7>::InteropType _p7, InteropTypeConverter<P8>::InteropType _p8,      \
        InteropTypeConverter<P9>::InteropType _p9)                                                 \
    {                                                                                              \
        P0 p0 = GetArgument<P0>(env, _p0);                                                         \
        P1 p1 = GetArgument<P1>(env, _p1);                                                         \
        P2 p2 = GetArgument<P2>(env, _p2);                                                         \
        P3 p3 = GetArgument<P3>(env, _p3);                                                         \
        P4 p4 = GetArgument<P4>(env, _p4);                                                         \
        P5 p5 = GetArgument<P5>(env, _p5);                                                         \
        P6 p6 = GetArgument<P6>(env, _p6);                                                         \
        P7 p7 = GetArgument<P7>(env, _p7);                                                         \
        P8 p8 = GetArgument<P8>(env, _p8);                                                         \
        P9 p9 = GetArgument<P9>(env, _p9);                                                         \
        auto res = MakeResult<Ret>(env, impl_##name(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9));      \
        ReleaseArgument(env, _p0, p0);                                                             \
        ReleaseArgument(env, _p1, p1);                                                             \
        ReleaseArgument(env, _p2, p2);                                                             \
        ReleaseArgument(env, _p3, p3);                                                             \
        ReleaseArgument(env, _p4, p4);                                                             \
        ReleaseArgument(env, _p5, p5);                                                             \
        ReleaseArgument(env, _p6, p6);                                                             \
        ReleaseArgument(env, _p7, p7);                                                             \
        ReleaseArgument(env, _p8, p8);                                                             \
        ReleaseArgument(env, _p9, p9);                                                             \
        /* CC-OFFNXT(G.PRE.05) code generation */                                                  \
        return res;                                                                                \
    }                                                                                              \
    MAKE_ANI_EXPORT(ETS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) code generation
#define ETS_INTEROP_11(name, Ret, P0, P1, P2, P3, P4, P5, P6, P7, P8, P9, P10)                     \
    InteropTypeConverter<Ret>::InteropType Ani_##name(                                             \
        ani_env *env, [[maybe_unused]] ani_object self, InteropTypeConverter<P0>::InteropType _p0, \
        InteropTypeConverter<P1>::InteropType _p1, InteropTypeConverter<P2>::InteropType _p2,      \
        InteropTypeConverter<P3>::InteropType _p3, InteropTypeConverter<P4>::InteropType _p4,      \
        InteropTypeConverter<P5>::InteropType _p5, InteropTypeConverter<P6>::InteropType _p6,      \
        InteropTypeConverter<P7>::InteropType _p7, InteropTypeConverter<P8>::InteropType _p8,      \
        InteropTypeConverter<P9>::InteropType _p9, InteropTypeConverter<P10>::InteropType _p10)    \
    {                                                                                              \
        P0 p0 = GetArgument<P0>(env, _p0);                                                         \
        P1 p1 = GetArgument<P1>(env, _p1);                                                         \
        P2 p2 = GetArgument<P2>(env, _p2);                                                         \
        P3 p3 = GetArgument<P3>(env, _p3);                                                         \
        P4 p4 = GetArgument<P4>(env, _p4);                                                         \
        P5 p5 = GetArgument<P5>(env, _p5);                                                         \
        P6 p6 = GetArgument<P6>(env, _p6);                                                         \
        P7 p7 = GetArgument<P7>(env, _p7);                                                         \
        P8 p8 = GetArgument<P8>(env, _p8);                                                         \
        P9 p9 = GetArgument<P9>(env, _p9);                                                         \
        P10 p10 = GetArgument<P10>(env, _p10);                                                     \
        auto res = MakeResult<Ret>(env, impl_##name(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10)); \
        ReleaseArgument(env, _p0, p0);                                                             \
        ReleaseArgument(env, _p1, p1);                                                             \
        ReleaseArgument(env, _p2, p2);                                                             \
        ReleaseArgument(env, _p3, p3);                                                             \
        ReleaseArgument(env, _p4, p4);                                                             \
        ReleaseArgument(env, _p5, p5);                                                             \
        ReleaseArgument(env, _p6, p6);                                                             \
        ReleaseArgument(env, _p7, p7);                                                             \
        ReleaseArgument(env, _p8, p8);                                                             \
        ReleaseArgument(env, _p9, p9);                                                             \
        ReleaseArgument(env, _p10, p10);                                                           \
        /* CC-OFFNXT(G.PRE.05) code generation */                                                  \
        return res;                                                                                \
    }                                                                                              \
    MAKE_ANI_EXPORT(ETS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) code generation
#define ETS_INTEROP_12(name, Ret, P0, P1, P2, P3, P4, P5, P6, P7, P8, P9, P10, P11)                     \
    InteropTypeConverter<Ret>::InteropType Ani_##name(                                                  \
        ani_env *env, [[maybe_unused]] ani_object self, InteropTypeConverter<P0>::InteropType _p0,      \
        InteropTypeConverter<P1>::InteropType _p1, InteropTypeConverter<P2>::InteropType _p2,           \
        InteropTypeConverter<P3>::InteropType _p3, InteropTypeConverter<P4>::InteropType _p4,           \
        InteropTypeConverter<P5>::InteropType _p5, InteropTypeConverter<P6>::InteropType _p6,           \
        InteropTypeConverter<P7>::InteropType _p7, InteropTypeConverter<P8>::InteropType _p8,           \
        InteropTypeConverter<P9>::InteropType _p9, InteropTypeConverter<P10>::InteropType _p10,         \
        InteropTypeConverter<P11>::InteropType _p11)                                                    \
    {                                                                                                   \
        P0 p0 = GetArgument<P0>(env, _p0);                                                              \
        P1 p1 = GetArgument<P1>(env, _p1);                                                              \
        P2 p2 = GetArgument<P2>(env, _p2);                                                              \
        P3 p3 = GetArgument<P3>(env, _p3);                                                              \
        P4 p4 = GetArgument<P4>(env, _p4);                                                              \
        P5 p5 = GetArgument<P5>(env, _p5);                                                              \
        P6 p6 = GetArgument<P6>(env, _p6);                                                              \
        P7 p7 = GetArgument<P7>(env, _p7);                                                              \
        P8 p8 = GetArgument<P8>(env, _p8);                                                              \
        P9 p9 = GetArgument<P9>(env, _p9);                                                              \
        P10 p10 = GetArgument<P10>(env, _p10);                                                          \
        P11 p11 = GetArgument<P11>(env, _p11);                                                          \
        auto res = MakeResult<Ret>(env, impl_##name(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11)); \
        ReleaseArgument(env, _p0, p0);                                                                  \
        ReleaseArgument(env, _p1, p1);                                                                  \
        ReleaseArgument(env, _p2, p2);                                                                  \
        ReleaseArgument(env, _p3, p3);                                                                  \
        ReleaseArgument(env, _p4, p4);                                                                  \
        ReleaseArgument(env, _p5, p5);                                                                  \
        ReleaseArgument(env, _p6, p6);                                                                  \
        ReleaseArgument(env, _p7, p7);                                                                  \
        ReleaseArgument(env, _p8, p8);                                                                  \
        ReleaseArgument(env, _p9, p9);                                                                  \
        ReleaseArgument(env, _p10, p10);                                                                \
        ReleaseArgument(env, _p11, p11);                                                                \
        /* CC-OFFNXT(G.PRE.05) code generation */                                                       \
        return res;                                                                                     \
    }                                                                                                   \
    MAKE_ANI_EXPORT(ETS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) code generation
#define ETS_INTEROP_13(name, Ret, P0, P1, P2, P3, P4, P5, P6, P7, P8, P9, P10, P11, P12)                     \
    InteropTypeConverter<Ret>::InteropType Ani_##name(                                                       \
        ani_env *env, [[maybe_unused]] ani_object self, InteropTypeConverter<P0>::InteropType _p0,           \
        InteropTypeConverter<P1>::InteropType _p1, InteropTypeConverter<P2>::InteropType _p2,                \
        InteropTypeConverter<P3>::InteropType _p3, InteropTypeConverter<P4>::InteropType _p4,                \
        InteropTypeConverter<P5>::InteropType _p5, InteropTypeConverter<P6>::InteropType _p6,                \
        InteropTypeConverter<P7>::InteropType _p7, InteropTypeConverter<P8>::InteropType _p8,                \
        InteropTypeConverter<P9>::InteropType _p9, InteropTypeConverter<P10>::InteropType _p10,              \
        InteropTypeConverter<P11>::InteropType _p11, InteropTypeConverter<P12>::InteropType _p12)            \
    {                                                                                                        \
        P0 p0 = GetArgument<P0>(env, _p0);                                                                   \
        P1 p1 = GetArgument<P1>(env, _p1);                                                                   \
        P2 p2 = GetArgument<P2>(env, _p2);                                                                   \
        P3 p3 = GetArgument<P3>(env, _p3);                                                                   \
        P4 p4 = GetArgument<P4>(env, _p4);                                                                   \
        P5 p5 = GetArgument<P5>(env, _p5);                                                                   \
        P6 p6 = GetArgument<P6>(env, _p6);                                                                   \
        P7 p7 = GetArgument<P7>(env, _p7);                                                                   \
        P8 p8 = GetArgument<P8>(env, _p8);                                                                   \
        P9 p9 = GetArgument<P9>(env, _p9);                                                                   \
        P10 p10 = GetArgument<P10>(env, _p10);                                                               \
        P11 p11 = GetArgument<P11>(env, _p11);                                                               \
        P12 p12 = GetArgument<P12>(env, _p12);                                                               \
        auto res = MakeResult<Ret>(env, impl_##name(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12)); \
        ReleaseArgument(env, _p0, p0);                                                                       \
        ReleaseArgument(env, _p1, p1);                                                                       \
        ReleaseArgument(env, _p2, p2);                                                                       \
        ReleaseArgument(env, _p3, p3);                                                                       \
        ReleaseArgument(env, _p4, p4);                                                                       \
        ReleaseArgument(env, _p5, p5);                                                                       \
        ReleaseArgument(env, _p6, p6);                                                                       \
        ReleaseArgument(env, _p7, p7);                                                                       \
        ReleaseArgument(env, _p8, p8);                                                                       \
        ReleaseArgument(env, _p9, p9);                                                                       \
        ReleaseArgument(env, _p10, p10);                                                                     \
        ReleaseArgument(env, _p11, p11);                                                                     \
        ReleaseArgument(env, _p12, p12);                                                                     \
        /* CC-OFFNXT(G.PRE.05) code generation */                                                            \
        return res;                                                                                          \
    }                                                                                                        \
    MAKE_ANI_EXPORT(ETS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) code generation
#define ETS_INTEROP_14(name, Ret, P0, P1, P2, P3, P4, P5, P6, P7, P8, P9, P10, P11, P12, P13)                     \
    InteropTypeConverter<Ret>::InteropType Ani_##name(                                                            \
        ani_env *env, [[maybe_unused]] ani_object self, InteropTypeConverter<P0>::InteropType _p0,                \
        InteropTypeConverter<P1>::InteropType _p1, InteropTypeConverter<P2>::InteropType _p2,                     \
        InteropTypeConverter<P3>::InteropType _p3, InteropTypeConverter<P4>::InteropType _p4,                     \
        InteropTypeConverter<P5>::InteropType _p5, InteropTypeConverter<P6>::InteropType _p6,                     \
        InteropTypeConverter<P7>::InteropType _p7, InteropTypeConverter<P8>::InteropType _p8,                     \
        InteropTypeConverter<P9>::InteropType _p9, InteropTypeConverter<P10>::InteropType _p10,                   \
        InteropTypeConverter<P11>::InteropType _p11, InteropTypeConverter<P12>::InteropType _p12,                 \
        InteropTypeConverter<P13>::InteropType _p13)                                                              \
    {                                                                                                             \
        P0 p0 = GetArgument<P0>(env, _p0);                                                                        \
        P1 p1 = GetArgument<P1>(env, _p1);                                                                        \
        P2 p2 = GetArgument<P2>(env, _p2);                                                                        \
        P3 p3 = GetArgument<P3>(env, _p3);                                                                        \
        P4 p4 = GetArgument<P4>(env, _p4);                                                                        \
        P5 p5 = GetArgument<P5>(env, _p5);                                                                        \
        P6 p6 = GetArgument<P6>(env, _p6);                                                                        \
        P7 p7 = GetArgument<P7>(env, _p7);                                                                        \
        P8 p8 = GetArgument<P8>(env, _p8);                                                                        \
        P9 p9 = GetArgument<P9>(env, _p9);                                                                        \
        P10 p10 = GetArgument<P10>(env, _p10);                                                                    \
        P11 p11 = GetArgument<P11>(env, _p11);                                                                    \
        P12 p12 = GetArgument<P12>(env, _p12);                                                                    \
        P13 p13 = GetArgument<P13>(env, _p13);                                                                    \
        auto res = MakeResult<Ret>(env, impl_##name(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13)); \
        ReleaseArgument(env, _p0, p0);                                                                            \
        ReleaseArgument(env, _p1, p1);                                                                            \
        ReleaseArgument(env, _p2, p2);                                                                            \
        ReleaseArgument(env, _p3, p3);                                                                            \
        ReleaseArgument(env, _p4, p4);                                                                            \
        ReleaseArgument(env, _p5, p5);                                                                            \
        ReleaseArgument(env, _p6, p6);                                                                            \
        ReleaseArgument(env, _p7, p7);                                                                            \
        ReleaseArgument(env, _p8, p8);                                                                            \
        ReleaseArgument(env, _p9, p9);                                                                            \
        ReleaseArgument(env, _p10, p10);                                                                          \
        ReleaseArgument(env, _p11, p11);                                                                          \
        ReleaseArgument(env, _p12, p12);                                                                          \
        ReleaseArgument(env, _p13, p13);                                                                          \
        /* CC-OFFNXT(G.PRE.05) code generation */                                                                 \
        return res;                                                                                               \
    }                                                                                                             \
    MAKE_ANI_EXPORT(ETS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) code generation
#define ETS_INTEROP_15(name, Ret, P0, P1, P2, P3, P4, P5, P6, P7, P8, P9, P10, P11, P12, P13, P14)                     \
    InteropTypeConverter<Ret>::InteropType Ani_##name(                                                                 \
        ani_env *env, [[maybe_unused]] ani_object self, InteropTypeConverter<P0>::InteropType _p0,                     \
        InteropTypeConverter<P1>::InteropType _p1, InteropTypeConverter<P2>::InteropType _p2,                          \
        InteropTypeConverter<P3>::InteropType _p3, InteropTypeConverter<P4>::InteropType _p4,                          \
        InteropTypeConverter<P5>::InteropType _p5, InteropTypeConverter<P6>::InteropType _p6,                          \
        InteropTypeConverter<P7>::InteropType _p7, InteropTypeConverter<P8>::InteropType _p8,                          \
        InteropTypeConverter<P9>::InteropType _p9, InteropTypeConverter<P10>::InteropType _p10,                        \
        InteropTypeConverter<P11>::InteropType _p11, InteropTypeConverter<P12>::InteropType _p12,                      \
        InteropTypeConverter<P13>::InteropType _p13, InteropTypeConverter<P14>::InteropType _p14)                      \
    {                                                                                                                  \
        P0 p0 = GetArgument<P0>(env, _p0);                                                                             \
        P1 p1 = GetArgument<P1>(env, _p1);                                                                             \
        P2 p2 = GetArgument<P2>(env, _p2);                                                                             \
        P3 p3 = GetArgument<P3>(env, _p3);                                                                             \
        P4 p4 = GetArgument<P4>(env, _p4);                                                                             \
        P5 p5 = GetArgument<P5>(env, _p5);                                                                             \
        P6 p6 = GetArgument<P6>(env, _p6);                                                                             \
        P7 p7 = GetArgument<P7>(env, _p7);                                                                             \
        P8 p8 = GetArgument<P8>(env, _p8);                                                                             \
        P9 p9 = GetArgument<P9>(env, _p9);                                                                             \
        P10 p10 = GetArgument<P10>(env, _p10);                                                                         \
        P11 p11 = GetArgument<P11>(env, _p11);                                                                         \
        P12 p12 = GetArgument<P12>(env, _p12);                                                                         \
        P13 p13 = GetArgument<P13>(env, _p13);                                                                         \
        P14 p14 = GetArgument<P14>(env, _p14);                                                                         \
        auto res = MakeResult<Ret>(env, impl_##name(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14)); \
        ReleaseArgument(env, _p0, p0);                                                                                 \
        ReleaseArgument(env, _p1, p1);                                                                                 \
        ReleaseArgument(env, _p2, p2);                                                                                 \
        ReleaseArgument(env, _p3, p3);                                                                                 \
        ReleaseArgument(env, _p4, p4);                                                                                 \
        ReleaseArgument(env, _p5, p5);                                                                                 \
        ReleaseArgument(env, _p6, p6);                                                                                 \
        ReleaseArgument(env, _p7, p7);                                                                                 \
        ReleaseArgument(env, _p8, p8);                                                                                 \
        ReleaseArgument(env, _p9, p9);                                                                                 \
        ReleaseArgument(env, _p10, p10);                                                                               \
        ReleaseArgument(env, _p11, p11);                                                                               \
        ReleaseArgument(env, _p12, p12);                                                                               \
        ReleaseArgument(env, _p13, p13);                                                                               \
        ReleaseArgument(env, _p14, p14);                                                                               \
        /* CC-OFFNXT(G.PRE.05) code generation */                                                                      \
        return res;                                                                                                    \
    }                                                                                                                  \
    MAKE_ANI_EXPORT(ETS_INTEROP_MODULE, name)

#define ETS_INTEROP_V0(name)                                                         \
    void Ani_##name([[maybe_unused]] ani_env *env, [[maybe_unused]] ani_object self) \
    {                                                                                \
        impl_##name();                                                               \
    }                                                                                \
    MAKE_ANI_EXPORT(ETS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) code generation
#define ETS_INTEROP_V1(name, P0)                                                                               \
    void Ani_##name(ani_env *env, [[maybe_unused]] ani_object self, InteropTypeConverter<P0>::InteropType _p0) \
    {                                                                                                          \
        P0 p0 = GetArgument<P0>(env, _p0);                                                                     \
        impl_##name(p0);                                                                                       \
        ReleaseArgument(env, _p0, p0);                                                                         \
    }                                                                                                          \
    MAKE_ANI_EXPORT(ETS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) code generation
#define ETS_INTEROP_V2(name, P0, P1)                                                                           \
    void Ani_##name(ani_env *env, [[maybe_unused]] ani_object self, InteropTypeConverter<P0>::InteropType _p0, \
                    InteropTypeConverter<P1>::InteropType _p1)                                                 \
    {                                                                                                          \
        P0 p0 = GetArgument<P0>(env, _p0);                                                                     \
        P1 p1 = GetArgument<P1>(env, _p1);                                                                     \
        impl_##name(p0, p1);                                                                                   \
        ReleaseArgument(env, _p0, p0);                                                                         \
        ReleaseArgument(env, _p1, p1);                                                                         \
    }                                                                                                          \
    MAKE_ANI_EXPORT(ETS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) code generation
#define ETS_INTEROP_V3(name, P0, P1, P2)                                                                       \
    void Ani_##name(ani_env *env, [[maybe_unused]] ani_object self, InteropTypeConverter<P0>::InteropType _p0, \
                    InteropTypeConverter<P1>::InteropType _p1, InteropTypeConverter<P2>::InteropType _p2)      \
    {                                                                                                          \
        P0 p0 = GetArgument<P0>(env, _p0);                                                                     \
        P1 p1 = GetArgument<P1>(env, _p1);                                                                     \
        P2 p2 = GetArgument<P2>(env, _p2);                                                                     \
        impl_##name(p0, p1, p2);                                                                               \
        ReleaseArgument(env, _p0, p0);                                                                         \
        ReleaseArgument(env, _p1, p1);                                                                         \
        ReleaseArgument(env, _p2, p2);                                                                         \
    }                                                                                                          \
    MAKE_ANI_EXPORT(ETS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) code generation
#define ETS_INTEROP_V4(name, P0, P1, P2, P3)                                                                   \
    void Ani_##name(ani_env *env, [[maybe_unused]] ani_object self, InteropTypeConverter<P0>::InteropType _p0, \
                    InteropTypeConverter<P1>::InteropType _p1, InteropTypeConverter<P2>::InteropType _p2,      \
                    InteropTypeConverter<P3>::InteropType _p3)                                                 \
    {                                                                                                          \
        P0 p0 = GetArgument<P0>(env, _p0);                                                                     \
        P1 p1 = GetArgument<P1>(env, _p1);                                                                     \
        P2 p2 = GetArgument<P2>(env, _p2);                                                                     \
        P3 p3 = GetArgument<P3>(env, _p3);                                                                     \
        impl_##name(p0, p1, p2, p3);                                                                           \
        ReleaseArgument(env, _p0, p0);                                                                         \
        ReleaseArgument(env, _p1, p1);                                                                         \
        ReleaseArgument(env, _p2, p2);                                                                         \
        ReleaseArgument(env, _p3, p3);                                                                         \
    }                                                                                                          \
    MAKE_ANI_EXPORT(ETS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) code generation
#define ETS_INTEROP_V5(name, P0, P1, P2, P3, P4)                                                               \
    void Ani_##name(ani_env *env, [[maybe_unused]] ani_object self, InteropTypeConverter<P0>::InteropType _p0, \
                    InteropTypeConverter<P1>::InteropType _p1, InteropTypeConverter<P2>::InteropType _p2,      \
                    InteropTypeConverter<P3>::InteropType _p3, InteropTypeConverter<P4>::InteropType _p4)      \
    {                                                                                                          \
        P0 p0 = GetArgument<P0>(env, _p0);                                                                     \
        P1 p1 = GetArgument<P1>(env, _p1);                                                                     \
        P2 p2 = GetArgument<P2>(env, _p2);                                                                     \
        P3 p3 = GetArgument<P3>(env, _p3);                                                                     \
        P4 p4 = GetArgument<P4>(env, _p4);                                                                     \
        impl_##name(p0, p1, p2, p3, p4);                                                                       \
        ReleaseArgument(env, _p0, p0);                                                                         \
        ReleaseArgument(env, _p1, p1);                                                                         \
        ReleaseArgument(env, _p2, p2);                                                                         \
        ReleaseArgument(env, _p3, p3);                                                                         \
        ReleaseArgument(env, _p4, p4);                                                                         \
    }                                                                                                          \
    MAKE_ANI_EXPORT(ETS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) code generation
#define ETS_INTEROP_V6(name, P0, P1, P2, P3, P4, P5)                                                           \
    void Ani_##name(ani_env *env, [[maybe_unused]] ani_object self, InteropTypeConverter<P0>::InteropType _p0, \
                    InteropTypeConverter<P1>::InteropType _p1, InteropTypeConverter<P2>::InteropType _p2,      \
                    InteropTypeConverter<P3>::InteropType _p3, InteropTypeConverter<P4>::InteropType _p4,      \
                    InteropTypeConverter<P5>::InteropType _p5)                                                 \
    {                                                                                                          \
        P0 p0 = GetArgument<P0>(env, _p0);                                                                     \
        P1 p1 = GetArgument<P1>(env, _p1);                                                                     \
        P2 p2 = GetArgument<P2>(env, _p2);                                                                     \
        P3 p3 = GetArgument<P3>(env, _p3);                                                                     \
        P4 p4 = GetArgument<P4>(env, _p4);                                                                     \
        P5 p5 = GetArgument<P5>(env, _p5);                                                                     \
        impl_##name(p0, p1, p2, p3, p4, p5);                                                                   \
        ReleaseArgument(env, _p0, p0);                                                                         \
        ReleaseArgument(env, _p1, p1);                                                                         \
        ReleaseArgument(env, _p2, p2);                                                                         \
        ReleaseArgument(env, _p3, p3);                                                                         \
        ReleaseArgument(env, _p4, p4);                                                                         \
        ReleaseArgument(env, _p5, p5);                                                                         \
    }                                                                                                          \
    MAKE_ANI_EXPORT(ETS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) code generation
#define ETS_INTEROP_V7(name, P0, P1, P2, P3, P4, P5, P6)                                                       \
    void Ani_##name(ani_env *env, [[maybe_unused]] ani_object self, InteropTypeConverter<P0>::InteropType _p0, \
                    InteropTypeConverter<P1>::InteropType _p1, InteropTypeConverter<P2>::InteropType _p2,      \
                    InteropTypeConverter<P3>::InteropType _p3, InteropTypeConverter<P4>::InteropType _p4,      \
                    InteropTypeConverter<P5>::InteropType _p5, InteropTypeConverter<P6>::InteropType _p6)      \
    {                                                                                                          \
        P0 p0 = GetArgument<P0>(env, _p0);                                                                     \
        P1 p1 = GetArgument<P1>(env, _p1);                                                                     \
        P2 p2 = GetArgument<P2>(env, _p2);                                                                     \
        P3 p3 = GetArgument<P3>(env, _p3);                                                                     \
        P4 p4 = GetArgument<P4>(env, _p4);                                                                     \
        P5 p5 = GetArgument<P5>(env, _p5);                                                                     \
        P6 p6 = GetArgument<P6>(env, _p6);                                                                     \
        impl_##name(p0, p1, p2, p3, p4, p5, p6);                                                               \
        ReleaseArgument(env, _p0, p0);                                                                         \
        ReleaseArgument(env, _p1, p1);                                                                         \
        ReleaseArgument(env, _p2, p2);                                                                         \
        ReleaseArgument(env, _p3, p3);                                                                         \
        ReleaseArgument(env, _p4, p4);                                                                         \
        ReleaseArgument(env, _p5, p5);                                                                         \
        ReleaseArgument(env, _p6, p6);                                                                         \
    }                                                                                                          \
    MAKE_ANI_EXPORT(ETS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) code generation
// CC-OFFNXT(WordsTool.190) sensitive word conflict
#define ETS_INTEROP_V8(name, P0, P1, P2, P3, P4, P5, P6, P7)                                                   \
    void Ani_##name(ani_env *env, [[maybe_unused]] ani_object self, InteropTypeConverter<P0>::InteropType _p0, \
                    InteropTypeConverter<P1>::InteropType _p1, InteropTypeConverter<P2>::InteropType _p2,      \
                    InteropTypeConverter<P3>::InteropType _p3, InteropTypeConverter<P4>::InteropType _p4,      \
                    InteropTypeConverter<P5>::InteropType _p5, InteropTypeConverter<P6>::InteropType _p6,      \
                    InteropTypeConverter<P7>::InteropType _p7)                                                 \
    {                                                                                                          \
        P0 p0 = GetArgument<P0>(env, _p0);                                                                     \
        P1 p1 = GetArgument<P1>(env, _p1);                                                                     \
        P2 p2 = GetArgument<P2>(env, _p2);                                                                     \
        P3 p3 = GetArgument<P3>(env, _p3);                                                                     \
        P4 p4 = GetArgument<P4>(env, _p4);                                                                     \
        P5 p5 = GetArgument<P5>(env, _p5);                                                                     \
        P6 p6 = GetArgument<P6>(env, _p6);                                                                     \
        P7 p7 = GetArgument<P7>(env, _p7);                                                                     \
        impl_##name(p0, p1, p2, p3, p4, p5, p6, p7);                                                           \
        ReleaseArgument(env, _p0, p0);                                                                         \
        ReleaseArgument(env, _p1, p1);                                                                         \
        ReleaseArgument(env, _p2, p2);                                                                         \
        ReleaseArgument(env, _p3, p3);                                                                         \
        ReleaseArgument(env, _p4, p4);                                                                         \
        ReleaseArgument(env, _p5, p5);                                                                         \
        ReleaseArgument(env, _p6, p6);                                                                         \
        ReleaseArgument(env, _p7, p7);                                                                         \
    }                                                                                                          \
    MAKE_ANI_EXPORT(ETS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) code generation
#define ETS_INTEROP_V9(name, P0, P1, P2, P3, P4, P5, P6, P7, P8)                                               \
    void Ani_##name(ani_env *env, [[maybe_unused]] ani_object self, InteropTypeConverter<P0>::InteropType _p0, \
                    InteropTypeConverter<P1>::InteropType _p1, InteropTypeConverter<P2>::InteropType _p2,      \
                    InteropTypeConverter<P3>::InteropType _p3, InteropTypeConverter<P4>::InteropType _p4,      \
                    InteropTypeConverter<P5>::InteropType _p5, InteropTypeConverter<P6>::InteropType _p6,      \
                    InteropTypeConverter<P7>::InteropType _p7, InteropTypeConverter<P8>::InteropType _p8)      \
    {                                                                                                          \
        P0 p0 = GetArgument<P0>(env, _p0);                                                                     \
        P1 p1 = GetArgument<P1>(env, _p1);                                                                     \
        P2 p2 = GetArgument<P2>(env, _p2);                                                                     \
        P3 p3 = GetArgument<P3>(env, _p3);                                                                     \
        P4 p4 = GetArgument<P4>(env, _p4);                                                                     \
        P5 p5 = GetArgument<P5>(env, _p5);                                                                     \
        P6 p6 = GetArgument<P6>(env, _p6);                                                                     \
        P7 p7 = GetArgument<P7>(env, _p7);                                                                     \
        P8 p8 = GetArgument<P8>(env, _p8);                                                                     \
        impl_##name(p0, p1, p2, p3, p4, p5, p6, p7, p8);                                                       \
        ReleaseArgument(env, _p0, p0);                                                                         \
        ReleaseArgument(env, _p1, p1);                                                                         \
        ReleaseArgument(env, _p2, p2);                                                                         \
        ReleaseArgument(env, _p3, p3);                                                                         \
        ReleaseArgument(env, _p4, p4);                                                                         \
        ReleaseArgument(env, _p5, p5);                                                                         \
        ReleaseArgument(env, _p6, p6);                                                                         \
        ReleaseArgument(env, _p7, p7);                                                                         \
        ReleaseArgument(env, _p8, p8);                                                                         \
    }                                                                                                          \
    MAKE_ANI_EXPORT(ETS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) code generation
#define ETS_INTEROP_V10(name, P0, P1, P2, P3, P4, P5, P6, P7, P8, P9)                                          \
    void Ani_##name(ani_env *env, [[maybe_unused]] ani_object self, InteropTypeConverter<P0>::InteropType _p0, \
                    InteropTypeConverter<P1>::InteropType _p1, InteropTypeConverter<P2>::InteropType _p2,      \
                    InteropTypeConverter<P3>::InteropType _p3, InteropTypeConverter<P4>::InteropType _p4,      \
                    InteropTypeConverter<P5>::InteropType _p5, InteropTypeConverter<P6>::InteropType _p6,      \
                    InteropTypeConverter<P7>::InteropType _p7, InteropTypeConverter<P8>::InteropType _p8,      \
                    InteropTypeConverter<P9>::InteropType _p9)                                                 \
    {                                                                                                          \
        P0 p0 = GetArgument<P0>(env, _p0);                                                                     \
        P1 p1 = GetArgument<P1>(env, _p1);                                                                     \
        P2 p2 = GetArgument<P2>(env, _p2);                                                                     \
        P3 p3 = GetArgument<P3>(env, _p3);                                                                     \
        P4 p4 = GetArgument<P4>(env, _p4);                                                                     \
        P5 p5 = GetArgument<P5>(env, _p5);                                                                     \
        P6 p6 = GetArgument<P6>(env, _p6);                                                                     \
        P7 p7 = GetArgument<P7>(env, _p7);                                                                     \
        P8 p8 = GetArgument<P8>(env, _p8);                                                                     \
        P9 p9 = GetArgument<P9>(env, _p9);                                                                     \
        impl_##name(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9);                                                   \
        ReleaseArgument(env, _p0, p0);                                                                         \
        ReleaseArgument(env, _p1, p1);                                                                         \
        ReleaseArgument(env, _p2, p2);                                                                         \
        ReleaseArgument(env, _p3, p3);                                                                         \
        ReleaseArgument(env, _p4, p4);                                                                         \
        ReleaseArgument(env, _p5, p5);                                                                         \
        ReleaseArgument(env, _p6, p6);                                                                         \
        ReleaseArgument(env, _p7, p7);                                                                         \
        ReleaseArgument(env, _p8, p8);                                                                         \
        ReleaseArgument(env, _p9, p9);                                                                         \
    }                                                                                                          \
    MAKE_ANI_EXPORT(ETS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) code generation
#define ETS_INTEROP_V11(name, P0, P1, P2, P3, P4, P5, P6, P7, P8, P9, P10)                                     \
    void Ani_##name(ani_env *env, [[maybe_unused]] ani_object self, InteropTypeConverter<P0>::InteropType _p0, \
                    InteropTypeConverter<P1>::InteropType _p1, InteropTypeConverter<P2>::InteropType _p2,      \
                    InteropTypeConverter<P3>::InteropType _p3, InteropTypeConverter<P4>::InteropType _p4,      \
                    InteropTypeConverter<P5>::InteropType _p5, InteropTypeConverter<P6>::InteropType _p6,      \
                    InteropTypeConverter<P7>::InteropType _p7, InteropTypeConverter<P8>::InteropType _p8,      \
                    InteropTypeConverter<P9>::InteropType _p9, InteropTypeConverter<P10>::InteropType _p10)    \
    {                                                                                                          \
        P0 p0 = GetArgument<P0>(env, _p0);                                                                     \
        P1 p1 = GetArgument<P1>(env, _p1);                                                                     \
        P2 p2 = GetArgument<P2>(env, _p2);                                                                     \
        P3 p3 = GetArgument<P3>(env, _p3);                                                                     \
        P4 p4 = GetArgument<P4>(env, _p4);                                                                     \
        P5 p5 = GetArgument<P5>(env, _p5);                                                                     \
        P6 p6 = GetArgument<P6>(env, _p6);                                                                     \
        P7 p7 = GetArgument<P7>(env, _p7);                                                                     \
        P8 p8 = GetArgument<P8>(env, _p8);                                                                     \
        P9 p9 = GetArgument<P9>(env, _p9);                                                                     \
        P10 p10 = GetArgument<P10>(env, _p10);                                                                 \
        impl_##name(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10);                                              \
        ReleaseArgument(env, _p0, p0);                                                                         \
        ReleaseArgument(env, _p1, p1);                                                                         \
        ReleaseArgument(env, _p2, p2);                                                                         \
        ReleaseArgument(env, _p3, p3);                                                                         \
        ReleaseArgument(env, _p4, p4);                                                                         \
        ReleaseArgument(env, _p5, p5);                                                                         \
        ReleaseArgument(env, _p6, p6);                                                                         \
        ReleaseArgument(env, _p7, p7);                                                                         \
        ReleaseArgument(env, _p8, p8);                                                                         \
        ReleaseArgument(env, _p9, p9);                                                                         \
        ReleaseArgument(env, _p10, p10);                                                                       \
    }                                                                                                          \
    MAKE_ANI_EXPORT(ETS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) code generation
#define ETS_INTEROP_V12(name, P0, P1, P2, P3, P4, P5, P6, P7, P8, P9, P10, P11)                                \
    void Ani_##name(ani_env *env, [[maybe_unused]] ani_object self, InteropTypeConverter<P0>::InteropType _p0, \
                    InteropTypeConverter<P1>::InteropType _p1, InteropTypeConverter<P2>::InteropType _p2,      \
                    InteropTypeConverter<P3>::InteropType _p3, InteropTypeConverter<P4>::InteropType _p4,      \
                    InteropTypeConverter<P5>::InteropType _p5, InteropTypeConverter<P6>::InteropType _p6,      \
                    InteropTypeConverter<P7>::InteropType _p7, InteropTypeConverter<P8>::InteropType _p8,      \
                    InteropTypeConverter<P9>::InteropType _p9, InteropTypeConverter<P10>::InteropType _p10,    \
                    InteropTypeConverter<P11>::InteropType _p11)                                               \
    {                                                                                                          \
        P0 p0 = GetArgument<P0>(env, _p0);                                                                     \
        P1 p1 = GetArgument<P1>(env, _p1);                                                                     \
        P2 p2 = GetArgument<P2>(env, _p2);                                                                     \
        P3 p3 = GetArgument<P3>(env, _p3);                                                                     \
        P4 p4 = GetArgument<P4>(env, _p4);                                                                     \
        P5 p5 = GetArgument<P5>(env, _p5);                                                                     \
        P6 p6 = GetArgument<P6>(env, _p6);                                                                     \
        P7 p7 = GetArgument<P7>(env, _p7);                                                                     \
        P8 p8 = GetArgument<P8>(env, _p8);                                                                     \
        P9 p9 = GetArgument<P9>(env, _p9);                                                                     \
        P10 p10 = GetArgument<P10>(env, _p10);                                                                 \
        P11 p11 = GetArgument<P11>(env, _p11);                                                                 \
        impl_##name(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11);                                         \
        ReleaseArgument(env, _p0, p0);                                                                         \
        ReleaseArgument(env, _p1, p1);                                                                         \
        ReleaseArgument(env, _p2, p2);                                                                         \
        ReleaseArgument(env, _p3, p3);                                                                         \
        ReleaseArgument(env, _p4, p4);                                                                         \
        ReleaseArgument(env, _p5, p5);                                                                         \
        ReleaseArgument(env, _p6, p6);                                                                         \
        ReleaseArgument(env, _p7, p7);                                                                         \
        ReleaseArgument(env, _p8, p8);                                                                         \
        ReleaseArgument(env, _p9, p9);                                                                         \
        ReleaseArgument(env, _p10, p10);                                                                       \
        ReleaseArgument(env, _p11, p11);                                                                       \
    }                                                                                                          \
    MAKE_ANI_EXPORT(ETS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) code generation
#define ETS_INTEROP_V13(name, P0, P1, P2, P3, P4, P5, P6, P7, P8, P9, P10, P11, P12)                           \
    void Ani_##name(ani_env *env, [[maybe_unused]] ani_object self, InteropTypeConverter<P0>::InteropType _p0, \
                    InteropTypeConverter<P1>::InteropType _p1, InteropTypeConverter<P2>::InteropType _p2,      \
                    InteropTypeConverter<P3>::InteropType _p3, InteropTypeConverter<P4>::InteropType _p4,      \
                    InteropTypeConverter<P5>::InteropType _p5, InteropTypeConverter<P6>::InteropType _p6,      \
                    InteropTypeConverter<P7>::InteropType _p7, InteropTypeConverter<P8>::InteropType _p8,      \
                    InteropTypeConverter<P9>::InteropType _p9, InteropTypeConverter<P10>::InteropType _p10,    \
                    InteropTypeConverter<P11>::InteropType _p11, InteropTypeConverter<P12>::InteropType _p12)  \
    {                                                                                                          \
        P0 p0 = GetArgument<P0>(env, _p0);                                                                     \
        P1 p1 = GetArgument<P1>(env, _p1);                                                                     \
        P2 p2 = GetArgument<P2>(env, _p2);                                                                     \
        P3 p3 = GetArgument<P3>(env, _p3);                                                                     \
        P4 p4 = GetArgument<P4>(env, _p4);                                                                     \
        P5 p5 = GetArgument<P5>(env, _p5);                                                                     \
        P6 p6 = GetArgument<P6>(env, _p6);                                                                     \
        P7 p7 = GetArgument<P7>(env, _p7);                                                                     \
        P8 p8 = GetArgument<P8>(env, _p8);                                                                     \
        P9 p9 = GetArgument<P9>(env, _p9);                                                                     \
        P10 p10 = GetArgument<P10>(env, _p10);                                                                 \
        P11 p11 = GetArgument<P11>(env, _p11);                                                                 \
        P12 p12 = GetArgument<P12>(env, _p12);                                                                 \
        impl_##name(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12);                                    \
        ReleaseArgument(env, _p0, p0);                                                                         \
        ReleaseArgument(env, _p1, p1);                                                                         \
        ReleaseArgument(env, _p2, p2);                                                                         \
        ReleaseArgument(env, _p3, p3);                                                                         \
        ReleaseArgument(env, _p4, p4);                                                                         \
        ReleaseArgument(env, _p5, p5);                                                                         \
        ReleaseArgument(env, _p6, p6);                                                                         \
        ReleaseArgument(env, _p7, p7);                                                                         \
        ReleaseArgument(env, _p8, p8);                                                                         \
        ReleaseArgument(env, _p9, p9);                                                                         \
        ReleaseArgument(env, _p10, p10);                                                                       \
        ReleaseArgument(env, _p11, p11);                                                                       \
        ReleaseArgument(env, _p12, p12);                                                                       \
    }                                                                                                          \
    MAKE_ANI_EXPORT(ETS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) code generation
#define ETS_INTEROP_V14(name, P0, P1, P2, P3, P4, P5, P6, P7, P8, P9, P10, P11, P12, P13)                      \
    void Ani_##name(ani_env *env, [[maybe_unused]] ani_object self, InteropTypeConverter<P0>::InteropType _p0, \
                    InteropTypeConverter<P1>::InteropType _p1, InteropTypeConverter<P2>::InteropType _p2,      \
                    InteropTypeConverter<P3>::InteropType _p3, InteropTypeConverter<P4>::InteropType _p4,      \
                    InteropTypeConverter<P5>::InteropType _p5, InteropTypeConverter<P6>::InteropType _p6,      \
                    InteropTypeConverter<P7>::InteropType _p7, InteropTypeConverter<P8>::InteropType _p8,      \
                    InteropTypeConverter<P9>::InteropType _p9, InteropTypeConverter<P10>::InteropType _p10,    \
                    InteropTypeConverter<P11>::InteropType _p11, InteropTypeConverter<P12>::InteropType _p12,  \
                    InteropTypeConverter<P13>::InteropType _p13)                                               \
    {                                                                                                          \
        P0 p0 = GetArgument<P0>(env, _p0);                                                                     \
        P1 p1 = GetArgument<P1>(env, _p1);                                                                     \
        P2 p2 = GetArgument<P2>(env, _p2);                                                                     \
        P3 p3 = GetArgument<P3>(env, _p3);                                                                     \
        P4 p4 = GetArgument<P4>(env, _p4);                                                                     \
        P5 p5 = GetArgument<P5>(env, _p5);                                                                     \
        P6 p6 = GetArgument<P6>(env, _p6);                                                                     \
        P7 p7 = GetArgument<P7>(env, _p7);                                                                     \
        P8 p8 = GetArgument<P8>(env, _p8);                                                                     \
        P9 p9 = GetArgument<P9>(env, _p9);                                                                     \
        P10 p10 = GetArgument<P10>(env, _p10);                                                                 \
        P11 p11 = GetArgument<P11>(env, _p11);                                                                 \
        P12 p12 = GetArgument<P12>(env, _p12);                                                                 \
        P13 p13 = GetArgument<P13>(env, _p13);                                                                 \
        impl_##name(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13);                               \
        ReleaseArgument(env, _p0, p0);                                                                         \
        ReleaseArgument(env, _p1, p1);                                                                         \
        ReleaseArgument(env, _p2, p2);                                                                         \
        ReleaseArgument(env, _p3, p3);                                                                         \
        ReleaseArgument(env, _p4, p4);                                                                         \
        ReleaseArgument(env, _p5, p5);                                                                         \
        ReleaseArgument(env, _p6, p6);                                                                         \
        ReleaseArgument(env, _p7, p7);                                                                         \
        ReleaseArgument(env, _p8, p8);                                                                         \
        ReleaseArgument(env, _p9, p9);                                                                         \
        ReleaseArgument(env, _p10, p10);                                                                       \
        ReleaseArgument(env, _p11, p11);                                                                       \
        ReleaseArgument(env, _p12, p12);                                                                       \
        ReleaseArgument(env, _p13, p13);                                                                       \
    }                                                                                                          \
    MAKE_ANI_EXPORT(ETS_INTEROP_MODULE, name)

// CC-OFFNXT(G.PRE.06) code generation
#define ETS_INTEROP_V15(name, P0, P1, P2, P3, P4, P5, P6, P7, P8, P9, P10, P11, P12, P13, P14)                 \
    void Ani_##name(ani_env *env, [[maybe_unused]] ani_object self, InteropTypeConverter<P0>::InteropType _p0, \
                    InteropTypeConverter<P1>::InteropType _p1, InteropTypeConverter<P2>::InteropType _p2,      \
                    InteropTypeConverter<P3>::InteropType _p3, InteropTypeConverter<P4>::InteropType _p4,      \
                    InteropTypeConverter<P5>::InteropType _p5, InteropTypeConverter<P6>::InteropType _p6,      \
                    InteropTypeConverter<P7>::InteropType _p7, InteropTypeConverter<P8>::InteropType _p8,      \
                    InteropTypeConverter<P9>::InteropType _p9, InteropTypeConverter<P10>::InteropType _p10,    \
                    InteropTypeConverter<P11>::InteropType _p11, InteropTypeConverter<P12>::InteropType _p12,  \
                    InteropTypeConverter<P13>::InteropType _p13, InteropTypeConverter<P14>::InteropType _p14)  \
    {                                                                                                          \
        P0 p0 = GetArgument<P0>(env, _p0);                                                                     \
        P1 p1 = GetArgument<P1>(env, _p1);                                                                     \
        P2 p2 = GetArgument<P2>(env, _p2);                                                                     \
        P3 p3 = GetArgument<P3>(env, _p3);                                                                     \
        P4 p4 = GetArgument<P4>(env, _p4);                                                                     \
        P5 p5 = GetArgument<P5>(env, _p5);                                                                     \
        P6 p6 = GetArgument<P6>(env, _p6);                                                                     \
        P7 p7 = GetArgument<P7>(env, _p7);                                                                     \
        P8 p8 = GetArgument<P8>(env, _p8);                                                                     \
        P9 p9 = GetArgument<P9>(env, _p9);                                                                     \
        P10 p10 = GetArgument<P10>(env, _p10);                                                                 \
        P11 p11 = GetArgument<P11>(env, _p11);                                                                 \
        P12 p12 = GetArgument<P12>(env, _p12);                                                                 \
        P13 p13 = GetArgument<P13>(env, _p13);                                                                 \
        P14 p14 = GetArgument<P14>(env, _p14);                                                                 \
        impl_##name(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14);                          \
        ReleaseArgument(env, _p0, p0);                                                                         \
        ReleaseArgument(env, _p1, p1);                                                                         \
        ReleaseArgument(env, _p2, p2);                                                                         \
        ReleaseArgument(env, _p3, p3);                                                                         \
        ReleaseArgument(env, _p4, p4);                                                                         \
        ReleaseArgument(env, _p5, p5);                                                                         \
        ReleaseArgument(env, _p6, p6);                                                                         \
        ReleaseArgument(env, _p7, p7);                                                                         \
        ReleaseArgument(env, _p8, p8);                                                                         \
        ReleaseArgument(env, _p9, p9);                                                                         \
        ReleaseArgument(env, _p10, p10);                                                                       \
        ReleaseArgument(env, _p11, p11);                                                                       \
        ReleaseArgument(env, _p12, p12);                                                                       \
        ReleaseArgument(env, _p13, p13);                                                                       \
        ReleaseArgument(env, _p14, p14);                                                                       \
    }                                                                                                          \
    MAKE_ANI_EXPORT(ETS_INTEROP_MODULE, name)

// NOLINTEND(cppcoreguidelines-macro-usage)

#endif  // CONVERTERS_ANI_H

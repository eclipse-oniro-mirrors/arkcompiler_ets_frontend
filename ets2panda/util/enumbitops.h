/**
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#ifndef ES2PANDA_UTIL_INCLUDE_ENUM_BITOPS_H
#define ES2PANDA_UTIL_INCLUDE_ENUM_BITOPS_H

#include <type_traits>

// clang-format off
#define ENUMBITOPS_OPERATORS \
    enumbitops::operator~,   \
    enumbitops::operator!,   \
    enumbitops::operator|,   \
    enumbitops::operator&,   \
    enumbitops::operator^,   \
    enumbitops::operator|=,  \
    enumbitops::operator&=,  \
    enumbitops::operator^=,  \
    enumbitops::All,         \
    enumbitops::Any
// clang-format on

namespace enumbitops {

template <class T>
struct IsAllowedType : std::false_type {
};

template <class T, std::enable_if_t<IsAllowedType<T>::value, bool> = true>
inline constexpr T operator~(T a)
{
    using Utype = std::underlying_type_t<T>;
    // NOLINTNEXTLINE(hicpp-signed-bitwise)
    return static_cast<T>(~static_cast<Utype>(a));
}

template <class T, std::enable_if_t<IsAllowedType<T>::value, bool> = true>
inline constexpr bool operator!(T a)
{
    using Utype = std::underlying_type_t<T>;
    // NOLINTNEXTLINE(hicpp-signed-bitwise)
    return (!static_cast<Utype>(a));
}

template <class T, std::enable_if_t<IsAllowedType<T>::value, bool> = true>
inline constexpr T operator|(T a, T b)
{
    using Utype = std::underlying_type_t<T>;
    // NOLINTNEXTLINE(hicpp-signed-bitwise)
    return static_cast<T>(static_cast<Utype>(a) | static_cast<Utype>(b));
}

template <class T, std::enable_if_t<IsAllowedType<T>::value, bool> = true>
inline constexpr std::underlying_type_t<T> operator&(T a, T b)
{
    using Utype = std::underlying_type_t<T>;
    // NOLINTNEXTLINE(hicpp-signed-bitwise)
    return static_cast<Utype>(static_cast<Utype>(a) & static_cast<Utype>(b));
}

template <class T, std::enable_if_t<IsAllowedType<T>::value, bool> = true>
inline constexpr T operator^(T a, T b)
{
    using Utype = std::underlying_type_t<T>;
    // NOLINTNEXTLINE(hicpp-signed-bitwise)
    return static_cast<T>(static_cast<Utype>(a) ^ static_cast<Utype>(b));
}

template <class T, std::enable_if_t<IsAllowedType<T>::value, bool> = true>
inline constexpr T &operator|=(T &a, T b)
{
    // NOLINTNEXTLINE(hicpp-signed-bitwise)
    return a = a | b;
}

template <class T, std::enable_if_t<IsAllowedType<T>::value, bool> = true>
inline constexpr T &operator&=(T &a, T b)
{
    using Utype = std::underlying_type_t<T>;
    // NOLINTNEXTLINE(hicpp-signed-bitwise)
    return a = static_cast<T>(static_cast<Utype>(a) & static_cast<Utype>(b));
}

template <class T, std::enable_if_t<IsAllowedType<T>::value, bool> = true>
inline constexpr T &operator^=(T &a, T b)
{
    // NOLINTNEXTLINE(hicpp-signed-bitwise)
    return a = a ^ b;
}

template <class T, std::enable_if_t<IsAllowedType<T>::value, bool> = true>
inline constexpr bool All(T flags, T test)
{
    using Utype = std::underlying_type_t<T>;
    return (flags & test) == static_cast<Utype>(test);
}

template <class T, std::enable_if_t<IsAllowedType<T>::value, bool> = true>
inline constexpr bool Any(T flags, T test)
{
    using Utype = std::underlying_type_t<T>;
    return (flags & test) != static_cast<Utype>(0);
}

}  // namespace enumbitops

#endif

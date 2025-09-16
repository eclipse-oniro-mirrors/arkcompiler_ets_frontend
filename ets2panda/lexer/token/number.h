/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

#ifndef ES2PANDA_LEXER_TOKEN_NUMBER_H
#define ES2PANDA_LEXER_TOKEN_NUMBER_H

#include "util/diagnosticEngine.h"
#include "util/ustring.h"
#include "util/enumbitops.h"

namespace ark::es2panda::lexer {

using ENUMBITOPS_OPERATORS;

enum class NumberFlags : uint32_t {
    NONE,
    BIGINT = 1U << 0U,
    DECIMAL_POINT = 1U << 1U,
    EXPONENT = 1U << 2U,
    ERROR = 1U << 3U,
};

}  // namespace ark::es2panda::lexer

template <>
struct enumbitops::IsAllowedType<ark::es2panda::lexer::NumberFlags> : std::true_type {
};

namespace ark::es2panda::lexer {

// NOLINTBEGIN(readability-identifier-naming)
// NOLINTBEGIN(fuchsia-multiple-inheritance)
template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};

template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;
// NOLINTEND(fuchsia-multiple-inheritance)

template <typename>
inline constexpr bool dependent_false_v = false;
// NOLINTEND(readability-identifier-naming)

class Number {
public:
    explicit Number() noexcept : num_(static_cast<int32_t>(0)) {};
    // NOLINTNEXTLINE(bugprone-exception-escape)
    explicit Number(util::StringView str, NumberFlags flags) noexcept;
    explicit Number(util::StringView str, double num) noexcept : str_(str), num_(num) {}
    explicit Number(uint8_t num) noexcept : Number(static_cast<int8_t>(num)) {}
    explicit Number(int8_t num) noexcept : num_(num) {}
    explicit Number(uint16_t num) noexcept : Number(static_cast<int16_t>(num)) {}
    explicit Number(int16_t num) noexcept : num_(num) {}
    explicit Number(uint32_t num) noexcept : Number(static_cast<int32_t>(num)) {}
    explicit Number(int32_t num) noexcept : num_(num) {}
    explicit Number(uint64_t num) noexcept : Number(static_cast<int64_t>(num)) {}
    explicit Number(int64_t num) noexcept : num_(num) {}
    explicit Number(float num) noexcept : num_(num) {}
    explicit Number(double num) noexcept : num_(num) {}
    DEFAULT_COPY_SEMANTIC(Number);
    DEFAULT_MOVE_SEMANTIC(Number);
    ~Number() = default;

    bool IsByte() const noexcept
    {
        return std::holds_alternative<int8_t>(num_);
    }

    bool IsShort() const noexcept
    {
        return std::holds_alternative<int16_t>(num_);
    }

    bool IsInt() const noexcept
    {
        return std::holds_alternative<int32_t>(num_);
    }

    bool IsLong() const noexcept
    {
        return std::holds_alternative<int64_t>(num_);
    }

    bool IsInteger() const noexcept
    {
        return IsByte() || IsShort() || IsInt() || IsLong();
    }

    bool IsFloat() const noexcept
    {
        return std::holds_alternative<float>(num_);
    }

    bool IsDouble() const noexcept
    {
        return std::holds_alternative<double>(num_);
    }

    bool IsReal() const noexcept
    {
        return IsFloat() || IsDouble();
    }

    bool ConversionError() const
    {
        return (flags_ & NumberFlags::ERROR) != 0;
    }

    int8_t GetByte() const
    {
        ES2PANDA_ASSERT(IsByte());
        return std::get<int8_t>(num_);
    }

    int16_t GetShort() const
    {
        return std::visit(overloaded {[](int16_t value) { return value; },
                                      [](int8_t value) { return static_cast<int16_t>(value); },
                                      []([[maybe_unused]] auto value) -> int16_t { ES2PANDA_UNREACHABLE(); }},
                          num_);
    }

    int32_t GetInt() const
    {
        return std::visit(overloaded {[](int32_t value) { return value; },
                                      [](int16_t value) { return static_cast<int32_t>(value); },
                                      [](int8_t value) { return static_cast<int32_t>(value); },
                                      []([[maybe_unused]] auto value) -> int32_t { ES2PANDA_UNREACHABLE(); }},
                          num_);
    }

    int64_t GetLong() const
    {
        return std::visit(overloaded {[](int64_t value) { return value; },
                                      [](int32_t value) { return static_cast<int64_t>(value); },
                                      [](int16_t value) { return static_cast<int64_t>(value); },
                                      [](int8_t value) { return static_cast<int64_t>(value); },
                                      []([[maybe_unused]] auto value) -> int64_t { ES2PANDA_UNREACHABLE(); }},
                          num_);
    }

    float GetFloat() const
    {
        return std::visit(overloaded {[](float value) { return value; },
                                      [](int64_t value) { return static_cast<float>(value); },
                                      [](int32_t value) { return static_cast<float>(value); },
                                      [](int16_t value) { return static_cast<float>(value); },
                                      [](int8_t value) { return static_cast<float>(value); },
                                      []([[maybe_unused]] auto value) -> float { ES2PANDA_UNREACHABLE(); }},
                          num_);
    }

    double GetDouble() const
    {
        return std::visit(overloaded {[]([[maybe_unused]] std::monostate value) -> double { ES2PANDA_UNREACHABLE(); },
                                      [](double value) { return value; },
                                      [](auto value) { return static_cast<double>(value); }},
                          num_);
    }

    const util::StringView &Str() const
    {
        return str_;
    }

    void SetStr(util::StringView str)
    {
        str_ = str;
    }

    void Negate()
    {
        std::visit(overloaded {[]([[maybe_unused]] std::monostate value) { ES2PANDA_UNREACHABLE(); },
                               [](auto &value) { value = -value; }},
                   num_);
        if (std::holds_alternative<int64_t>(num_)) {
            int64_t num = std::get<int64_t>(num_);
            if (num == INT32_MIN) {
                SetValue<int32_t>(num);
            }
        }
    }

    bool IsZero() const
    {
        return std::visit(overloaded {[]([[maybe_unused]] std::monostate value) -> bool { ES2PANDA_UNREACHABLE(); },
                                      [](auto &value) { return value == 0; }},
                          num_);
    }

    // NOLINTBEGIN(readability-else-after-return)
    template <typename RT>
    bool CanGetValue() const noexcept
    {
        using T = typename std::remove_cv_t<typename std::remove_reference_t<RT>>;

        if constexpr (std::is_same_v<T, int64_t>) {
            return IsInteger();
        } else if constexpr (std::is_same_v<T, int32_t>) {
            return IsInt() || IsShort() || IsByte();
        } else if constexpr (std::is_same_v<T, double>) {
            return true;
        } else if constexpr (std::is_same_v<T, float>) {
            return IsInteger() || IsFloat();
        } else if constexpr (std::is_same_v<T, int16_t>) {
            return IsShort() || IsByte();
        } else if constexpr (std::is_same_v<T, int8_t>) {
            return IsByte();
        } else {
            return false;
        }
    }

    template <typename RT>
    auto GetValue() const
    {
        using T = typename std::remove_cv_t<typename std::remove_reference_t<RT>>;

        if constexpr (std::is_same_v<T, int64_t>) {
            return GetLong();
        } else if constexpr (std::is_same_v<T, int32_t>) {
            return GetInt();
        } else if constexpr (std::is_same_v<T, double>) {
            return GetDouble();
        } else if constexpr (std::is_same_v<T, float>) {
            return GetFloat();
        } else if constexpr (std::is_same_v<T, int16_t>) {
            return GetShort();
        } else if constexpr (std::is_same_v<T, int8_t>) {
            return GetByte();
        } else {
            static_assert(dependent_false_v<T>, "Invalid value type was requested for Number.");
        }
    }

    template <typename TargetType>
    TargetType GetValueAndCastTo() const
    {
        if (IsByte()) {
            return static_cast<TargetType>(GetByte());
        } else if (IsShort()) {
            return static_cast<TargetType>(GetShort());
        } else if (IsInt()) {
            return static_cast<TargetType>(GetInt());
        } else if (IsLong()) {
            return static_cast<TargetType>(GetLong());
        } else if (IsFloat()) {
            return static_cast<TargetType>(GetFloat());
        } else if (IsDouble()) {
            return static_cast<TargetType>(GetDouble());
        }
        ES2PANDA_UNREACHABLE();
    }
    // NOLINTEND(readability-else-after-return)

    template <typename RT>
    void SetValue(RT &&value)
    {
        using T = typename std::remove_cv_t<typename std::remove_reference_t<RT>>;

        if constexpr (std::is_same_v<T, int64_t> || std::is_same_v<T, int32_t> || std::is_same_v<T, int16_t> ||
                      std::is_same_v<T, int8_t> || std::is_same_v<T, double> || std::is_same_v<T, float>) {
            num_ = std::forward<RT>(value);
        } else {
            static_assert(dependent_false_v<T>, "Invalid value type was requested for Number.");
        }
    }

private:
    util::StringView str_ {};
    std::variant<std::monostate, int8_t, int16_t, int32_t, int64_t, float, double> num_;
    NumberFlags flags_ {NumberFlags::NONE};
};
}  // namespace ark::es2panda::lexer

#endif

/**
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

#ifndef ES2PANDA_DYNAMIC_LANGUAGE_H
#define ES2PANDA_DYNAMIC_LANGUAGE_H

#include <algorithm>
#include <array>
#include <optional>
#include <string_view>
#include <tuple>

#include "util/es2pandaMacros.h"

namespace ark::es2panda {

class Language {
public:
    enum class Id {
        AS,
        JS,
        TS,
        ETS,
        COUNT,
    };

    // NOLINTNEXTLINE(*-explicit-constructor)
    constexpr Language(Id id) : id_(id) {}

    constexpr std::string_view ToString() const
    {
        for (auto [id, name, _] : ID_TABLE) {
            if (id_ == id) {
                return name;
            }
        }

        ES2PANDA_UNREACHABLE();
    }

    static std::optional<Language> FromString(std::string_view str)
    {
        for (auto [id, name, _] : ID_TABLE) {
            if (str == name) {
                return Language(id);
            }
        }

        return std::nullopt;
    }

    Id GetId() const
    {
        return id_;
    }

    bool IsDynamic() const
    {
        for (auto [id, _, isDynamic] : ID_TABLE) {
            if (id_ == id) {
                return isDynamic;
            }
        }

        ES2PANDA_UNREACHABLE();
    }

    bool IsValid() const
    {
        for (auto [id, _, isDynamic] : ID_TABLE) {
            if (id_ == id) {
                return true;
            }
        }

        return false;
    }

    bool operator==(const Language &l) const
    {
        return id_ == l.id_;
    }

    bool operator!=(const Language &l) const
    {
        return id_ != l.id_;
    }

private:
    static constexpr auto COUNT = static_cast<size_t>(Id::COUNT);
    static constexpr auto ID_TABLE = {
        std::tuple {Id::AS, "as", false},
        {Id::JS, "js", true},
        {Id::TS, "ts", true},
        {Id::ETS, "ets", false},
    };

public:
    static std::array<Language, COUNT> All()
    {
        static_assert(std::size(ID_TABLE) == COUNT);

        std::array<Id, COUNT> arr = {};
        std::transform(ID_TABLE.begin(), ID_TABLE.end(), arr.begin(), [](const auto &tpl) { return std::get<0>(tpl); });

        return std::apply([](auto... id) { return std::array<Language, COUNT> {Language(id)...}; }, arr);
    }

private:
    Id id_;
};

}  // namespace ark::es2panda

// NOLINTNEXTLINE(cert-dcl58-cpp)
namespace std {

template <>
// NOLINTNEXTLINE(altera-struct-pack-align)
struct hash<ark::es2panda::Language> {
    std::size_t operator()(ark::es2panda::Language lang) const
    {
        return std::hash<ark::es2panda::Language::Id> {}(lang.GetId());
    }
};

}  // namespace std

#endif  // ES2PANDA_DYNAMIC_LANGUAGE_H

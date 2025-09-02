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

#ifndef ES2PANDA_COMPILER_LOWERING_PHASE_ID_H
#define ES2PANDA_COMPILER_LOWERING_PHASE_ID_H

namespace ark::es2panda::compiler {

constexpr int32_t INVALID_PHASE_ID = -2;
constexpr int32_t PARSER_PHASE_ID = -1;

struct PhaseId {
    int32_t major;  // NOLINT(misc-non-private-member-variables-in-classes)
    int32_t minor;  // NOLINT(misc-non-private-member-variables-in-classes)

    bool operator<(const PhaseId &other) const
    {
        return major == other.major ? minor < other.minor : major < other.major;
    }

    bool operator<=(const PhaseId &other) const
    {
        return major == other.major ? minor <= other.minor : major <= other.major;
    }

    bool operator==(const PhaseId &other) const
    {
        return major == other.major && minor == other.minor;
    }

    bool operator>=(const PhaseId &other) const
    {
        return major == other.major ? minor >= other.minor : major >= other.major;
    }

    bool operator>(const PhaseId &other) const
    {
        return major == other.major ? minor > other.minor : major > other.major;
    }
};

}  // namespace ark::es2panda::compiler

#endif

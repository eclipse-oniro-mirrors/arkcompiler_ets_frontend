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

#ifndef ES2PANDA_UTIL_APIVERSION_H
#define ES2PANDA_UTIL_APIVERSION_H

#include <cstdint>

namespace ark::es2panda::api_version {

// Minimum supported target API version for bytecode generation
constexpr uint8_t MIN_SUPPORTED_API_VERSION = 24;

// API version constants
constexpr uint8_t API_24 = 24;
constexpr uint8_t API_26 = 26;

// ─── ApiFeature: a language/bytecode feature gated by API version ───
struct ApiFeature {
    uint8_t minVersion;
    const char *description;

    // 0 means "latest" — all features are enabled (matches api_version_map convention: [0, <latest>])
    constexpr bool AllowedInVersion(uint8_t targetApi) const
    {
        return targetApi == 0 || targetApi >= minVersion;
    }
};

// Metadata emission (API 26+); read side is gated by abc file header version
constexpr ApiFeature METADATA {API_26, "metadata"};

}  // namespace ark::es2panda::api_version

#endif  // ES2PANDA_UTIL_APIVERSION_H

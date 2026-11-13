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

#ifndef ES2PANDA_UTIL_STABLE_CODE_MAP_H
#define ES2PANDA_UTIL_STABLE_CODE_MAP_H

#include "generated/stableMap.h"

#include <cstddef>
#include <cstdint>

namespace ark::es2panda::diagnostic {

/**
 * @brief Lookup of stable ordered diagnostic codes.
 *
 * The registry (util/diagnostic/stable_map.json) assigns every YAML-defined
 * diagnostic a stable sequential code, generated into the table
 * `STABLE_CODE_MAP` by `scripts/sync_stable_map --emit-header` at build time.
 * This class is the read side: it maps (category, yamlId) to the stable code.
 *
 * Engine-side wiring (diagnosticEngine.cpp) is expected to:
 *
 * 1. Translate `util::DiagnosticType` to `StableCodeMap::Category` (plain
 *    switch; PLUGIN_ERROR/PLUGIN_WARNING/SUGGESTION have no registry entries
 *    and always report 0);
 * 2. Look the code up **by the effective diagnostic type**: a strict warning
 *    escalated to a semantic error (soft/hard lists, ES2PANDA_STRICT) is
 *    registered under BOTH categories with the same yamlId, so looking up
 *    Category::SEMANTIC for an escalated warning resolves to the mirror
 *    entry, and Category::WARNING to the warning-side entry - the caller
 *    picks the code matching the severity it is about to report;
 * 3. Treat 0 as "not in the registry" (codes start from 1, so 0 is safe).
 *
 * The table is sorted by (category, yamlId); (category, yamlId) is unique
 * among live entries.
 */
class StableCodeMap {
public:
    StableCodeMap() = delete;

    /**
     * @brief Category ids, ordered exactly as CATEGORY_FILES in
     * scripts/sync_stable_map (the generator emits the same order).
     */
    enum class Category : uint8_t {
        FATAL = 0,
        SYNTAX = 1,
        SEMANTIC = 2,
        WARNING = 3,
        ARKTS_CONFIG_ERROR = 4,
        DECLGEN_ETS2TS_ERROR = 5,
        DECLGEN_ETS2TS_WARNING = 6,
        ISOLATED_DECLGEN = 7,
    };

    /**
     * @brief Stable code of a diagnostic, or 0 when the (category, yamlId)
     * pair is not registered.
     */
    static uint32_t GetCode(Category category, uint32_t yamlId)
    {
        const StableCodeEntry *entry = Find(category, yamlId);
        return entry == nullptr ? 0U : entry->code;
    }

    /**
     * @brief Whether the entry is a `strict: true` warning (or its semantic
     * mirror), i.e. a warning that can be escalated to an error at runtime.
     */
    static bool IsStrict(Category category, uint32_t yamlId)
    {
        const StableCodeEntry *entry = Find(category, yamlId);
        return entry != nullptr && entry->strict;
    }

    /**
     * @brief Whether the (category, yamlId) pair has a registry entry.
     */
    static bool Contains(Category category, uint32_t yamlId)
    {
        return Find(category, yamlId) != nullptr;
    }

    /**
     * @brief Number of live registry entries.
     */
    static std::size_t Size()
    {
        return STABLE_CODE_MAP.size();
    }

private:
    static const StableCodeEntry *Find(Category category, uint32_t yamlId)
    {
        const uint8_t cat = static_cast<uint8_t>(category);
        std::size_t lo = 0;
        std::size_t hi = STABLE_CODE_MAP.size();
        while (lo < hi) {
            const std::size_t mid = lo + (hi - lo) / 2U;
            const StableCodeEntry &entry = STABLE_CODE_MAP[mid];
            if (entry.category < cat || (entry.category == cat && entry.yamlId < yamlId)) {
                lo = mid + 1U;
            } else if (entry.category == cat && entry.yamlId == yamlId) {
                return &entry;
            } else {
                hi = mid;
            }
        }
        return nullptr;
    }
};

}  // namespace ark::es2panda::diagnostic

#endif  // ES2PANDA_UTIL_STABLE_CODE_MAP_H

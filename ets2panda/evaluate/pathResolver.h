/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

#ifndef ES2PANDA_EVALUATE_PATH_RESOLVER_H
#define ES2PANDA_EVALUATE_PATH_RESOLVER_H

#include "utils/arena_containers.h"

namespace ark::es2panda::evaluate {

class DebugInfoStorage;
struct EntityInfo;

class PathResolver final {
public:
    NO_COPY_SEMANTIC(PathResolver);
    NO_MOVE_SEMANTIC(PathResolver);

    explicit PathResolver(DebugInfoStorage &debugInfoStorage) : debugInfoStorage_(debugInfoStorage) {}

    ~PathResolver() = default;

    /**
     * @brief Returns import path on success, empty string otherwise
     */
    std::string_view FindNamedImportAll(std::string_view filePath, std::string_view bindingName);

    std::optional<EntityInfo> FindImportedEntity(std::string_view filePath, std::string_view entityName);

    void FindImportedFunctions(ArenaVector<EntityInfo> &overloadSet, std::string_view filePath,
                               std::string_view entityName);

private:
    void FindExportedFunctions(ArenaVector<EntityInfo> &overloadSet, std::string_view filePath,
                               std::string_view entityName);
    std::optional<EntityInfo> FindExportedEntity(std::string_view filePath, std::string_view entityName);

private:
    static constexpr std::string_view STAR_IMPORT = "*";

    DebugInfoStorage &debugInfoStorage_;
};

}  // namespace ark::es2panda::evaluate

#endif  // ES2PANDA_EVALUATE_PATH_RESOLVER_H

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

#ifndef ES2PANDA_EVALUATE_IMPORT_EXPORT_TABLE_H
#define ES2PANDA_EVALUATE_IMPORT_EXPORT_TABLE_H

#include "utils/arena_containers.h"

namespace ark::es2panda::evaluate {

struct EntityInfo final {
    EntityInfo(std::string_view path, std::string_view entity) : filePath(path), entityName(entity) {}

    // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
    std::string_view filePath;
    std::string_view entityName;
    // NOLINTEND(misc-non-private-member-variables-in-classes)
};

class ImportExportTable final {
public:
    using AliasMap = ArenaUnorderedMap<std::string_view, ArenaVector<EntityInfo>>;

public:
    explicit ImportExportTable(ArenaAllocator *allocator);

    const AliasMap &GetImports() const
    {
        return imports_;
    }

    const AliasMap &GetExports() const
    {
        return exports_;
    }

private:
    AliasMap imports_;
    AliasMap exports_;
};

}  // namespace ark::es2panda::evaluate

#endif  // ES2PANDA_EVALUATE_IMPORT_EXPORT_TABLE_H

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

#ifndef ES2PANDA_EVALUATE_PROXY_PROGRAMS_MAP_H
#define ES2PANDA_EVALUATE_PROXY_PROGRAMS_MAP_H

#include "util/ustring.h"

#include "mem/arena_allocator.h"
#include "utils/arena_containers.h"

namespace ark::es2panda::parser {
class Program;
}  // namespace ark::es2panda::parser

namespace ark::es2panda::evaluate {

class ProxyProgramsCache final {
public:
    explicit ProxyProgramsCache(ArenaAllocator *allocator);
    NO_COPY_SEMANTIC(ProxyProgramsCache);
    NO_MOVE_SEMANTIC(ProxyProgramsCache);

    ~ProxyProgramsCache() = default;

    parser::Program *GetProgram(util::StringView fileName);

    void AddProgram(parser::Program *program);

private:
    ArenaUnorderedMap<util::StringView, parser::Program *> cache_;
};

}  // namespace ark::es2panda::evaluate

#endif  // ES2PANDA_EVALUATE_PROXY_PROGRAMS_MAP_H

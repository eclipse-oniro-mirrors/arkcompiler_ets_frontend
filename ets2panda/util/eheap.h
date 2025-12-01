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

#ifndef ES2PANDA_UTIL_EHEAP_H
#define ES2PANDA_UTIL_EHEAP_H

#include "libarkbase/mem/arena_allocator.h"
#include "libarkbase/utils/arena_containers.h"

namespace ark::es2panda {

class EHeap {
public:
    static void Initialize();
    static void Finalize();

    // legacy API
    static bool IsInitialized();

    static ArenaAllocator CreateAllocator()
    {
        return ArenaAllocator {SpaceType::SPACE_TYPE_COMPILER, nullptr, true};
    }

    static std::unique_ptr<ArenaAllocator> NewAllocator()
    {
        return std::make_unique<ArenaAllocator>(SpaceType::SPACE_TYPE_COMPILER, nullptr, true);
    }

private:
    EHeap() = delete;
    ~EHeap() = delete;
};

}  // namespace ark::es2panda

#endif  // ES2PANDA_UTIL_EHEAP_H

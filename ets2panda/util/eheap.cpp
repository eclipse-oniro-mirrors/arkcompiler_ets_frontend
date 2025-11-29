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

#include "util/eheap.h"
#include "libarkbase/mem/pool_manager.h"

namespace ark::es2panda {

void EHeap::Initialize()
{
    constexpr auto COMPILER_SIZE = sizeof(void *) <= 4U ? 2_GB : 32_GB;

    mem::MemConfig::Initialize(0, 0, COMPILER_SIZE, 0, 0, 0);
    PoolManager::Initialize(PoolType::MMAP);
}

void EHeap::Finalize()
{
    PoolManager::Finalize();
    mem::MemConfig::Finalize();
}

bool EHeap::IsInitialized()
{
    return mem::MemConfig::IsInitialized();
}

}  // namespace ark::es2panda

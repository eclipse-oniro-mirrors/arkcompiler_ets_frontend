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

#ifndef ES2PANDA_COMPILER_CHECKER_ETS_ARITHMETIC_H
#define ES2PANDA_COMPILER_CHECKER_ETS_ARITHMETIC_H

#include "checker/ETSchecker.h"
#include "checker/ETSAnalyzer.h"
#include "checker/types/globalTypesHolder.h"

namespace ark::es2panda::checker {
template <typename IntegerUType, typename FloatOrIntegerUType>
inline IntegerUType CastIfFloat(FloatOrIntegerUType num)
{
    if constexpr (std::is_floating_point_v<FloatOrIntegerUType>) {
        return CastFloatToInt<FloatOrIntegerUType, IntegerUType>(num);
    } else {
        return num;
    }
}
}  // namespace ark::es2panda::checker

#endif

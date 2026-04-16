/**
 * Copyright (c) 2021 - 2026 Huawei Device Co., Ltd.
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

#include "boxingConverter.h"
#include "checker/ETSchecker.h"
#include "util/helpers.h"
#include "checker/types/globalTypesHolder.h"

namespace ark::es2panda::checker {

Type *BoxingConverter::Convert(ETSChecker const *checker, Type const *source)
{
    auto typeHolder = checker->GetGlobalTypesHolder();

    switch (checker::ETSChecker::TypeKind(source)) {
        case checker::TypeFlag::ETS_BOOLEAN:
            return typeHolder->GlobalETSBooleanBuiltinType();
        case checker::TypeFlag::BYTE:
            return typeHolder->GlobalByteBuiltinType();
        case checker::TypeFlag::SHORT:
            return typeHolder->GlobalShortBuiltinType();
        case checker::TypeFlag::CHAR:
            return typeHolder->GlobalCharBuiltinType();
        case checker::TypeFlag::INT:
            return typeHolder->GlobalIntegerBuiltinType();
        case checker::TypeFlag::LONG:
            return typeHolder->GlobalLongBuiltinType();
        case checker::TypeFlag::FLOAT:
            return typeHolder->GlobalFloatBuiltinType();
        case checker::TypeFlag::DOUBLE:
            return typeHolder->GlobalDoubleBuiltinType();
        case checker::TypeFlag::ETS_VOID:
            return typeHolder->GlobalETSUndefinedType();
        default:
            ES2PANDA_UNREACHABLE();
    }
}

}  // namespace ark::es2panda::checker

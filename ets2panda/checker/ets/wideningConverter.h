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

#ifndef ES2PANDA_COMPILER_CHECKER_ETS_WIDENING_CONVERTER_H
#define ES2PANDA_COMPILER_CHECKER_ETS_WIDENING_CONVERTER_H

#include "checker/ETSchecker.h"
#include "checker/ets/typeConverter.h"

namespace ark::es2panda::checker {

class WideningConverter : public TypeConverter {
public:
    explicit WideningConverter(ETSChecker *checker, TypeRelation *relation, Type *target, Type *source)
        : TypeConverter(checker, relation, target, source)
    {
        if (!Relation()->ApplyWidening()) {
            return;
        }

        ApplyGlobalWidening();
    }

private:
    // NOTE(dkofanov): Deprecated operations on 'char' #28006
    static constexpr TypeFlag WIDENABLE_TO_SHORT = TypeFlag::BYTE;
    static constexpr TypeFlag WIDENABLE_TO_CHAR = TypeFlag::BYTE;
    static constexpr TypeFlag WIDENABLE_TO_INT = TypeFlag::CHAR | TypeFlag::SHORT | WIDENABLE_TO_SHORT;
    static constexpr TypeFlag WIDENABLE_TO_LONG = TypeFlag::INT | WIDENABLE_TO_INT;
    static constexpr TypeFlag WIDENABLE_TO_FLOAT = TypeFlag::LONG | WIDENABLE_TO_LONG;
    static constexpr TypeFlag WIDENABLE_TO_DOUBLE = TypeFlag::FLOAT | WIDENABLE_TO_FLOAT;

    void ApplyGlobalWidening()
    {
        switch (ETSChecker::ETSChecker::ETSType(Target())) {
            // NOTE(dkofanov): Deprecated operations on 'char' #28006
            case TypeFlag::CHAR: {
                ApplyGlobalWidening(WIDENABLE_TO_CHAR);
                break;
            }
            case TypeFlag::SHORT: {
                ApplyGlobalWidening(WIDENABLE_TO_SHORT);
                break;
            }
            case TypeFlag::INT: {
                ApplyGlobalWidening(WIDENABLE_TO_INT);
                break;
            }
            case TypeFlag::LONG: {
                ApplyGlobalWidening(WIDENABLE_TO_LONG);
                break;
            }
            case TypeFlag::FLOAT: {
                ApplyGlobalWidening(WIDENABLE_TO_FLOAT);
                break;
            }
            case TypeFlag::DOUBLE: {
                ApplyGlobalWidening(WIDENABLE_TO_DOUBLE);
                break;
            }
            default: {
                break;
            }
        }
    }

    void ApplyGlobalWidening(TypeFlag flag)
    {
        if (!Source()->HasTypeFlag(flag)) {
            return;
        }

        if (!Relation()->OnlyCheckWidening()) {
            ES2PANDA_ASSERT(Relation()->GetNode());
            switch (ETSChecker::ETSChecker::ETSType(Source())) {
                case TypeFlag::BYTE: {
                    Relation()->GetNode()->SetTsType(Checker()->GlobalByteBuiltinType());
                    break;
                }
                case TypeFlag::SHORT: {
                    Relation()->GetNode()->SetTsType(Checker()->GlobalShortBuiltinType());
                    break;
                }
                case TypeFlag::INT: {
                    Relation()->GetNode()->SetTsType(Checker()->GlobalIntBuiltinType());
                    break;
                }
                // NOTE(dkofanov): Deprecated operations on 'char' #28006
                case TypeFlag::CHAR: {
                    Relation()->GetNode()->SetTsType(Checker()->GlobalCharBuiltinType());
                    break;
                }
                case TypeFlag::LONG: {
                    Relation()->GetNode()->SetTsType(Checker()->GlobalLongBuiltinType());
                    break;
                }
                case TypeFlag::FLOAT: {
                    Relation()->GetNode()->SetTsType(Checker()->GlobalFloatBuiltinType());
                    break;
                }
                case TypeFlag::DOUBLE: {
                    Relation()->GetNode()->SetTsType(Checker()->GlobalDoubleBuiltinType());
                    break;
                }
                default: {
                    return;
                }
            }
        }

        Relation()->Result(true);
    }
};
}  // namespace ark::es2panda::checker

#endif

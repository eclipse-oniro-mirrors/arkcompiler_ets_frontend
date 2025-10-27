/**
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

#ifndef PANDA_ANNOTATIONS_EMIT_TEST_H
#define PANDA_ANNOTATIONS_EMIT_TEST_H

#include <gtest/gtest.h>

#include "test/utils/asm_test.h"

namespace ark::es2panda::compiler::test {

class AnnotationEmitTest : public ::test::utils::AsmTest {
public:
    AnnotationEmitTest() = default;

    ~AnnotationEmitTest() override = default;

protected:
    static constexpr uint32_t COLOR_OPTION_0 = 0;
    static constexpr uint32_t COLOR_OPTION_1 = 1;
    static constexpr double AGE_18 = 18.0;
    static constexpr double AGE_21 = 21.0;
    static constexpr double AGE_32 = 32.0;
    static constexpr double VALUE_1 = 1.0;
    static constexpr double VALUE_2 = 2.0;
    static constexpr double VALUE_3 = 3.0;
    static constexpr double VALUE_4 = 4.0;
    static constexpr double VALUE_5 = 5.0;
    static constexpr double VALUE_6 = 6.0;
    static constexpr double VALUE_7 = 7.0;
    static constexpr double VALUE_8 = 8.0;
    static constexpr double VALUE_9 = 9.0;
    static constexpr uint32_t COLOR_0 = 0;
    static constexpr uint32_t COLOR_1 = 1;
    static constexpr double REVIEWER_AGE_19 = 19.0;
    static constexpr double REVIEWER_AGE_20 = 20.0;
    static constexpr double REVIEWER_AGE_24 = 24.0;

private:
    NO_COPY_SEMANTIC(AnnotationEmitTest);
    NO_MOVE_SEMANTIC(AnnotationEmitTest);
};

}  // namespace ark::es2panda::compiler::test

#endif  // PANDA_ANNOTATIONS_EMIT_TEST_H

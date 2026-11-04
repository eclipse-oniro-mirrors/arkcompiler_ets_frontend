/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

enum Access {
    READ = 1,
    WRITE = 2,
    READWRITE = 3
}

enum TestEnum {
    A = 144,
    B,
    C = 795
}

function main(): void {
    test_access_enum_bitwise(Access.WRITE);
    arktest.assertEQ((Access.WRITE & Access.READWRITE), 2)

    test_test_enum_bitwise(TestEnum.C);
    arktest.assertEQ((TestEnum.B & TestEnum.A), 144)

}

function test_access_enum_bitwise(enum_val: Access): void {
    arktest.assertEQ((enum_val & Access.READ), 0)
    arktest.assertEQ((enum_val & Access.WRITE), 2)
    arktest.assertEQ((enum_val | Access.READ), 3)
    arktest.assertEQ((enum_val | Access.READWRITE), 3)
    arktest.assertEQ((enum_val & Access.READWRITE), 2)
}

function test_test_enum_bitwise(enum_val: TestEnum): void {
    arktest.assertEQ((enum_val & TestEnum.A), 16)
    arktest.assertEQ((enum_val | TestEnum.A), 923)
    arktest.assertEQ((enum_val & TestEnum.B), 17)
    arktest.assertEQ((enum_val | TestEnum.B), 923)
}

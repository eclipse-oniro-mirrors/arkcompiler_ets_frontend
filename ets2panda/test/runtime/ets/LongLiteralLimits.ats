/*
 * Copyright (c) 2022-2025 Huawei Device Co., Ltd.
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

function main(): void {
    const decLongMax = 9223372036854775807;
    const binLongMax = 0b0111_1111_1111_1111_1111_1111_1111_1111_1111_1111_1111_1111_1111_1111_1111_1111;
    const hexLongMax = 0x7FFFFFFFFFFFFFFF;
    const octLongMax = 0o777777777777777777777;
    arktest.assertEQ(decLongMax - binLongMax + hexLongMax - octLongMax, 0,  "Incorrect long max result")

    const decLongMin = ~decLongMax;
    const binLongMin = ~binLongMax;
    const hexLongMin = ~hexLongMax;
    const octLongMin = ~octLongMax;
    arktest.assertEQ(decLongMin - binLongMin + hexLongMin - octLongMin, 0,  "Incorrect long min result")
}
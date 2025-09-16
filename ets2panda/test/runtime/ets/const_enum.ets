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

const enum Comparison { LessThan = -1, EqualTo = 0, GreaterThan = 1 };

function main() : void {
    let c1 = Comparison.LessThan;
    arktest.assertEQ(c1, Comparison.LessThan)
    arktest.assertEQ(c1.valueOf(), -1)

    let c2 = Comparison.EqualTo;
    arktest.assertEQ(c2, Comparison.EqualTo)
    arktest.assertEQ(c2.valueOf(), 0)

    let c3 = Comparison.GreaterThan;
    arktest.assertEQ(c3, Comparison.GreaterThan)
    arktest.assertEQ(c3.valueOf(), 1)
}

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

function foo<T>(a0: T): T { return a0; }
function bar<T>(a0: T): T { return a0; }

function main() {
    let fooInt1: (a0: Int) => Int = foo<Int>
    arktest.assertEQ(0, fooInt1(0));
    let fooInt2 = bar<Int>
    arktest.assertEQ(1, fooInt2(1));
    let fooInt3: (a0: Int) => Int = fooInt1
    arktest.assertEQ(2, fooInt3(2));

    let fooInt4 = foo<Int>
    arktest.assertEQ(3, fooInt4(3));
    let fooInt5: (a0: Int) => Int = fooInt4
    arktest.assertEQ(4, fooInt5(4));

    let fooInt6 = foo<Int>
    arktest.assertEQ(5, fooInt6(5));
    let fooInt7: (a0: Int) => Int = fooInt6
    arktest.assertEQ(6, fooInt7(6));
}

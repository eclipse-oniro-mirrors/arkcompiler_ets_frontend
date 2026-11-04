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

function foo() : Char {
    return c'c'
}

function foo1(a: Char) : Char {
    return a;
}

function foo2() : Char {
    return Int.toChar(Char.toInt(c'd') - 1)
}

function main() {
    let c : Char = c'c';
    let val : Char = foo()
    arktest.assertEQ(val, c)

    let val1 : Char = foo1(c'c')
    arktest.assertEQ(val1, c)

    let val2 : Char = foo2()
    arktest.assertEQ(val2, c)
}

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

@interface MyAnno {
    a : int
    b : float
    c : boolean
}

@interface Anno {
    a: FixedArray<boolean> = [true & false, false ^ true, true | false]
    b: FixedArray<int> = [1 + 1 + 1 ^ 11, 48 ? 10 : 20]
    c: string = "a" + "b"
}

@MyAnno({a = 1 + -1, b = 1554.4 ^ 10, c = 12 >= 10})
function foo() {}

const annoA = 10
const annoB = 1451.1
const annoC = 123
@MyAnno({a = annoA, b = annoB ^ 10, c = annoC > 10})
function foo1() {}

function main() {
    arktest.assertTrue(true)
}

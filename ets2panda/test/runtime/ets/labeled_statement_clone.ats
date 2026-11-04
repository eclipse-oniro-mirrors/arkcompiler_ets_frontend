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

function foo(f: () => int): int {
    return f()
}

// AstNodes inside the trailing lambda are cloned - check that Clone works correctly
let test1 = foo() {
    let res = 0
    a: for (let i = 0; i < 5; i++) {
        b: for (let j = 0; j < 5; j++) {
            if (j > i)
                break a
            res++
        }
    }
    return res
}

let test2 = foo() {
    let res = 0
    a: for (let i = 0; i < 5; i++) {
        b: for (let j = 0; j < 5; j++) {
            if (j > i)
                break b
            res++
        }
    }
    return res
}

let test3 = foo() {
    let res = 0
    a: for (let i = 0; i < 5; i++) {
        b: for (let j = 0; j < 5; j++) {
            if (j > i)
                continue a
            res++
        }
        res += 1000
    }
    return res
}

let test4 = foo() {
    let res = 0
    a: for (let i = 0; i < 5; i++) {
        b: for (let j = 0; j < 5; j++) {
            if (j > i)
                continue b
            res++
        }
        res += 1000
    }
    return res
}


function main(): void {
    arktest.assertEQ(test1, 1)
    arktest.assertEQ(test2, 15)
    arktest.assertEQ(test3, 1015)
    arktest.assertEQ(test4, 5015)
}

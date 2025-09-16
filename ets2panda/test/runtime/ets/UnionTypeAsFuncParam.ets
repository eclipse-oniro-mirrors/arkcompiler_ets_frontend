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

type Func1 = () => void
type Func2 = (num: number, str: string) => number

let func1: Func1 = ():void => {
    console.log("func1")
}

function foo(num: number, str: string): number {
    console.log(str)
    return num
}

let func2: Func2 = foo

function test(fn: Func1 | Func2 | number | string) {
    if (fn instanceof Func1) {
        fn()
    } else if (fn instanceof Func2) {
        arktest.assertEQ(fn(2,"func2"), 2)
    } else if (fn instanceof Number) {
        arktest.assertEQ(fn, 3)
    } else if (fn instanceof String) {
        arktest.assertEQ(fn, "4")
    }
}

function main(): void {
    test(func1)
    test(func2)
    test(3)
    test("4")
}
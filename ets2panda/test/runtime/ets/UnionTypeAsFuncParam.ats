/*
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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

type Func1 = () => string
type Func2 = (num: Any, str: Any) => never


let func1: Func1 = (): string => {
    return "func1";
}

function foo(num: Any, str: Any): never {
    throw new Error();
}

let func2: Func2 = foo

function test(fn: Func1 | Func2 | number | string) {
    if (fn instanceof Func2) {
        arktest.expectThrow(() => { fn(2,"func2"); }, (e) => e instanceof Error);
    } else if (fn instanceof Number) {
        arktest.assertEQ(fn, 3)
    } else if (fn instanceof String) {
        arktest.assertEQ(fn, "4")
    } else {
        arktest.assertEQ(fn, "func1")
    }
}

function main(): void {
    test(func1)
    test(func2)
    test(3)
    test("4")
}

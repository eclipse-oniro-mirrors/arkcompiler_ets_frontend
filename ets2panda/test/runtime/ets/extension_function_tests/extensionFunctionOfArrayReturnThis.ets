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

class Fruit {
    data: int = 1;
    constructor(d: int) {
        this.data = d;
    }
}

function fooFruit(this: Fruit[]): this {
    return this;
}

function foo(this: Double[]): this {
    return this
}

function main() {
    let arr: Double[] = [1.1, 2.2, 3.3, 4.4]
    let arr_return = arr.foo()
    let arr_return2 = foo(arr_return);
    arktest.assertEQ(arr.length, arr_return.length)
    arktest.assertEQ(arr.length, arr_return2.length)
    for (let idx = 0; idx < arr.length; ++idx) {
        arktest.assertEQ(arr[idx], arr_return[idx])
        arktest.assertEQ(arr[idx], arr_return2[idx])
    }

    let fruits: Fruit[] = [new Fruit(1), new Fruit(2), new Fruit(3)]
    let fruits_return = fruits.fooFruit();
    let fruits_return2 = fooFruit(fruits_return);
    arktest.assertEQ(fruits.length, fruits_return.length)
    arktest.assertEQ(fruits.length, fruits_return2.length)
    for (let idx = 0; idx < fruits.length; ++idx) {
        arktest.assertEQ(fruits[idx], fruits_return[idx])
        arktest.assertEQ(fruits[idx], fruits_return2[idx])
    }
}
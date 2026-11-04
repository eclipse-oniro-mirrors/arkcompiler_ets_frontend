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

class Fruit {
    private _price: int = 1;
    get price(): int { return this._price; };
    set price(a: int) {
        this._price = a;
    }
}

function extension(this: Fruit): int {
    this.price = 25;
    return this.price;
}

function asFuncCall() {
    let fruit = new Fruit();
    let res1: int = fruit.price;
    arktest.assertEQ(res1, 1 )
    let res2: int = extension(fruit);
    arktest.assertEQ(res2, 25 )
    let res3: int = fruit.price;
    arktest.assertEQ(res3, 25 )
}

function asMethodCall() {
    let fruit = new Fruit();
    let res1: int = fruit.price;
    arktest.assertEQ(res1, 1 )
    let res2: int = fruit.extension();
    arktest.assertEQ(res2, 25 )
    let res3: int = fruit.price;
    arktest.assertEQ(res3, 25 )
}

function main() {
    asFuncCall();
    asMethodCall();
}

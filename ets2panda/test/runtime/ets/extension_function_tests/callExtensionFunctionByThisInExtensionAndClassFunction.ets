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
    name(): int {
        return 2 + this.amount();
    }

    public price_: int = 5;
}

function name(this:Fruit, id: int): int {
    return id + this.name() + this.price_;
}

function price(this:Fruit): int {
    return this.name() + this.price_ + this.name(2);
}

function amount(this:Fruit): int {
    return 6;
}

function asFuncCall() {
    let fruit = new Fruit();
    let res = price(fruit);
    arktest.assertEQ(res, 28 )
}

function asMethodCall() {
    let fruit = new Fruit();
    let res = fruit.price();
    arktest.assertEQ(res, 28 )
}

function main() {
    asFuncCall();
    asMethodCall();
}

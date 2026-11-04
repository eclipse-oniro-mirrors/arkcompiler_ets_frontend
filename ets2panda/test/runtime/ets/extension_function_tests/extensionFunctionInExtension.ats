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
}

function price(this:Fruit): int {
    return 5;
}

function name(this:Fruit): int {
    return 1 + this.price();
}

function name2(this:Fruit): int {
    return 1 + price(this);
}

function asFuncCall() {
    let fruit = new Fruit();
    let res: int = name(fruit);
    let res2: int = name2(fruit);
    arktest.assertEQ(res, 6 )
    arktest.assertEQ(res2, 6 )
}

function asMethodCall() {
    let fruit = new Fruit();
    let res: int = fruit.name();
    let res2: int = fruit.name2();
    arktest.assertEQ(res, 6 )
    arktest.assertEQ(res2, 6 )
}

function main() {
    asFuncCall();
    asMethodCall();
}

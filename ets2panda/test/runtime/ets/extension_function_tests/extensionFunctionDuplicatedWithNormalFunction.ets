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

function name(this:Fruit): int {
    return 1;
}

function name(): int {
    return 2;
}

function asFuncCall() {
    let fruit = new Fruit();
    let res: int = name(fruit);
    assertEQ(res, 1 )
    let res1: int = name();
    assertEQ(res1, 2 )
}

function asMethodCall() {
    let fruit = new Fruit();
    let res: int = fruit.name();
    assertEQ(res, 1 )
    let res1: int = name();
    assertEQ(res1, 2 )
}

function main() {
    asFuncCall();
    asMethodCall();
}

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

interface Fruit {}

function foo(this:Fruit): int {
    return 1;
}

class Banana implements Fruit {}

function foo(this:Banana):int {
    return 2
}

function asFuncCall() {
    let banana = new Banana();
    let res: int = foo(banana);
    arktest.assertEQ(res, 2 )
    let res1: int = foo((banana as Fruit));
    arktest.assertEQ(res1, 1 )
}

function asMethodCall() {
    let banana = new Banana();
    let res: int = banana.foo();
    arktest.assertEQ(res, 2 )
    let res1: int = (banana as Fruit).foo();
    arktest.assertEQ(res1, 1 )
}

function main() {
    asFuncCall();
    asMethodCall();
}

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

interface Foo {
    a: int
}

async function foo1() : Promise<Foo> {
    return { a: 0 }
}

async function foo2() : Promise<Record<string, int>> {
    return { "b": 10 }
}

async function foo3() : Promise<Map<string, int>> {
    return { "c": 20 }
}

async function startTest() {
    let r1 = await foo1()
    arktest.assertEQ(r1.a, 0)

    let r2 = await foo2()
    arktest.assertEQ(r2["b"], 10)

    let r3 = await foo3()
    arktest.assertEQ(r3.get("c"), 20)
}

function main() {
    startTest()
}
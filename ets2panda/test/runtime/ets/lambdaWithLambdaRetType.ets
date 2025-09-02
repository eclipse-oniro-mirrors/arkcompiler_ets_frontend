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

function foo(x: boolean): () => int {
    let f1 = () => 1;
    return x ? (f1) : () => 2;
}

function foo1(): () => () => void {
    return (): () => void => { return () => {}};
}

function foo2(x: boolean): (a: int) => int {
    let f1 = () => 1;
    let f2: (a: int) => int = (x) => x * x;
    return x ? (f1: int): int => 1 : (f1: int) => 2;
}

function main(): void {
    arktest.assertEQ(foo(false)(), 2)
    arktest.assertEQ(foo(true)(), 1)
    foo1();
    arktest.assertEQ(foo2(false)(0), 2)
    arktest.assertEQ(foo2(true)(0), 1)
}

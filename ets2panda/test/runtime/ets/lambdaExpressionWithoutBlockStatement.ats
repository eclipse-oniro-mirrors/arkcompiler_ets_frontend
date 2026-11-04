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

function test(a: int): int {
    return a+27;
}

function main(): void {

    let test1: () => int = () => 111;
    let test2: (b: int) => int = (b: int) => test(b);
    let test3: (a: int, b: int) => int = (a: int, b: int) => a*b;
    let test4: (c: int, d: int) => int = (c: int, d: int) => test1() + test2(test1()) * test3(c,d);

    let result = test4(5, 4);

    arktest.assertEQ(result, 2871)
}

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

let res: number = 0;
function foo(i: int, j: int): void {
    res = 19;
}
function foo(i: number, j: Number): void {
    res = 21;
}
function boo(x: Number, y: number) {
    res = 34;
}
function boo(x: number, y: Number) {
    res = 47;
}
function main(): void {
    let n: Number = new Number();
    foo(1, n);
    arktest.assertEQ(res, 21);
    boo(6 as Number, 8);
    arktest.assertEQ(res, 34);
}

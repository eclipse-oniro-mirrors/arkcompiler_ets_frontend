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

function f(b?: boolean | string | null | int | object): string {
    return !b ? "not good" : "good";
}

function f1(b?: boolean | string | null | int | object): string {
    return b ? "not good" : "good";
}

function f2(): void {
    let result = (2 + 2 === 4) ? "Yes" : "No";
    arktest.assertEQ(result, "Yes");
    let result1 = (2 + 2 !== 4) ? "Yes" : "No";
    arktest.assertEQ(result1, "No");
    let result2 = (2 > 4) ? "Yes" : "No";
    arktest.assertEQ(result2, "No");
    let result3 = (2 < 4) ? "Yes" : "No";
    arktest.assertEQ(result3, "Yes");
}

function f3(): void {
    let result = true ? 1 : (false ? 2 : 3);
    arktest.assertEQ(result, 1);
    let result1 = false ? 1 : (false ? 2 : 3);
    arktest.assertEQ(result1, 3);
}

function testF() {
    arktest.assertEQ(f(true), "good");
    arktest.assertEQ(f(false), "not good");
    arktest.assertEQ(f("false"), "good");
    arktest.assertEQ(f("true"), "good");
    arktest.assertEQ(f(undefined), "not good");
    arktest.assertEQ(f(null), "not good");
    arktest.assertEQ(f(""), "not good");
    arktest.assertEQ(f("Hello World"), "good");
    arktest.assertEQ(f(0), "not good");
    arktest.assertEQ(f(1), "good");
    arktest.assertEQ(f({}), "good");
    arktest.assertEQ(f(), "not good");
}

function testF1() {
    arktest.assertEQ(f1(true), "not good");
    arktest.assertEQ(f1(false), "good");
    arktest.assertEQ(f1("false"), "not good");
    arktest.assertEQ(f1("true"), "not good");
    arktest.assertEQ(f1(undefined), "good");
    arktest.assertEQ(f1(null), "good");
    arktest.assertEQ(f1(""), "good");
    arktest.assertEQ(f1("Hello World"), "not good");
    arktest.assertEQ(f1(0), "good");
    arktest.assertEQ(f1(1), "not good");
    arktest.assertEQ(f1({}), "not good");
    arktest.assertEQ(f1(), "good");
}

function main(): void {
    testF();
    testF1();
    f2();
    f3();
}
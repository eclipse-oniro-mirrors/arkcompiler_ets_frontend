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
    assertEQ(result, "Yes");
    let result1 = (2 + 2 !== 4) ? "Yes" : "No";
    assertEQ(result1, "No");
    let result2 = (2 > 4) ? "Yes" : "No";
    assertEQ(result2, "No");
    let result3 = (2 < 4) ? "Yes" : "No";
    assertEQ(result3, "Yes");
}

function f3(): void {
    let result = true ? 1 : (false ? 2 : 3);
    assertEQ(result, 1);
    let result1 = false ? 1 : (false ? 2 : 3);
    assertEQ(result1, 3);
}

function testF() {
    assertEQ(f(true), "good");
    assertEQ(f(false), "not good");
    assertEQ(f("false"), "good");
    assertEQ(f("true"), "good");
    assertEQ(f(undefined), "not good");
    assertEQ(f(null), "not good");
    assertEQ(f(""), "not good");
    assertEQ(f("Hello World"), "good");
    assertEQ(f(0), "not good");
    assertEQ(f(1), "good");
    assertEQ(f({}), "good");
    assertEQ(f(), "not good");
}

function testF1() {
    assertEQ(f1(true), "not good");
    assertEQ(f1(false), "good");
    assertEQ(f1("false"), "not good");
    assertEQ(f1("true"), "not good");
    assertEQ(f1(undefined), "good");
    assertEQ(f1(null), "good");
    assertEQ(f1(""), "good");
    assertEQ(f1("Hello World"), "not good");
    assertEQ(f1(0), "good");
    assertEQ(f1(1), "not good");
    assertEQ(f1({}), "not good");
    assertEQ(f1(), "good");
}

function main(): void {
    testF();
    testF1();
    f2();
    f3();
}
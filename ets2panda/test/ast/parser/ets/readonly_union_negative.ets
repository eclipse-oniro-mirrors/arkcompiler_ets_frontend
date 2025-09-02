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

// Negative test cases for readonly union types - these should fail compilation

// Error: readonly on a non-applicable primitive type in a union
function test_negative_1(param: readonly number | string): void {
}

// Error: readonly on a non-applicable object type in a union
class MyObject {}
function test_negative_2(param: readonly MyObject | string): void {
}

// Error: readonly on a non-applicable type, even if another member is applicable
function test_negative_5(param: readonly boolean | string[]): void {
}

// Error: readonly on void type
function test_negative_6(param: readonly void | number): void {
}

// Error: readonly on function type
function test_negative_7(param: readonly (() => void) | string): void {
}

function main(): void {
    // Function calls to trigger parsing of parameter types
    test_negative_1(42, "test");
    test_negative_2(new MyObject(), "test");
    test_negative_5(true, ["test"]);
    test_negative_6(undefined, 42);
    test_negative_7(() => {}, "test");
} 


/* @@? 19:49 Error SyntaxError: 'readonly' type modifier is only permitted on resizable array and tuple types. */
/* @@? 24:51 Error SyntaxError: 'readonly' type modifier is only permitted on resizable array and tuple types. */
/* @@? 28:50 Error SyntaxError: 'readonly' type modifier is only permitted on resizable array and tuple types. */
/* @@? 32:33 Error TypeError: 'void' used as type annotation. */
/* @@? 32:47 Error SyntaxError: 'readonly' type modifier is only permitted on resizable array and tuple types. */
/* @@? 36:55 Error SyntaxError: 'readonly' type modifier is only permitted on resizable array and tuple types. */
/* @@? 41:5 Error TypeError: Expected 1 arguments, got 2. */
/* @@? 41:5 Error TypeError: No matching call signature for test_negative_1(Int, "test") */
/* @@? 42:5 Error TypeError: Expected 1 arguments, got 2. */
/* @@? 42:5 Error TypeError: No matching call signature for test_negative_2(MyObject, "test") */
/* @@? 43:5 Error TypeError: Expected 1 arguments, got 2. */
/* @@? 43:5 Error TypeError: No matching call signature for test_negative_5(Boolean, Array<String>) */
/* @@? 44:5 Error TypeError: Expected 1 arguments, got 2. */
/* @@? 44:5 Error TypeError: No matching call signature for test_negative_6(undefined, Int) */
/* @@? 45:5 Error TypeError: Expected 1 arguments, got 2. */
/* @@? 45:5 Error TypeError: No matching call signature for test_negative_7(() => void, "test") */
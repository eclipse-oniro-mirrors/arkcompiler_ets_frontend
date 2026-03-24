/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

// Test 1: Export function without type annotation
export function foo() {
    return 42;
}

// Test 2: Export variable without type annotation
export let a = 100;

// Test 3: Export class with method without type annotation
export class MyClass {
    value = 0;
    
    // Method without type annotation
    getValue() {
        return this.value;
    }
}

// Test 4: Export default function without type annotation
export default function bar() {
    console.log("Hello");
}

/* @@? 17:1 Error Semantic error ESE71336: Exported function 'foo' requires type annotation. */
/* @@? 22:12 Error Semantic error ESE71336: Exported variable 'a' requires type annotation. */
/* @@? 26:5 Error Semantic error ESE71336: Exported class property 'value' requires type annotation. */
/* @@? 29:5 Error Semantic error ESE71336: Exported function 'getValue' requires type annotation. */
/* @@? 35:1 Error Semantic error ESE71336: Exported function 'bar' requires type annotation. */

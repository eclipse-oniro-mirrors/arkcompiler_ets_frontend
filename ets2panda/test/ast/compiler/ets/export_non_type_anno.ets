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

function foo() {
    return 42;
}
export { foo }

export let x = 1

export function exportedFunc() {
    return 1;
}

export class A {
    a = 1
    private c = 1
    foo() {
        return 1;
    }
    private bar() {
        return 1;
    }
}

class B {
    a = 1
    private c = 1
    foo() {
        return 1;
    }
    private bar() {
        return 1;
    }
}

export { B }

/* @@? 16:1 Error Warning W71336: Exported function 'foo' requires type annotation. */
/* @@? 21:12 Error Warning W71336: Exported variable 'x' requires type annotation. */
/* @@? 23:8 Error Warning W71336: Exported function 'exportedFunc' requires type annotation. */
/* @@? 28:5 Error Warning W71336: Exported class property 'a' requires type annotation. */
/* @@? 30:5 Error Warning W71336: Exported function 'foo' requires type annotation. */
/* @@? 39:5 Error Warning W71336: Exported class property 'a' requires type annotation. */
/* @@? 41:5 Error Warning W71336: Exported function 'foo' requires type annotation. */

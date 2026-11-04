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

class C<T> {
    fld: T;

    constructor(p: T) {
        this.fld = p;
    }
}

function bar(x: C<Any>) { x.fld = "a" }

function foo(obj: object) {
    if (obj instanceof C) {
        let x = obj;                                // OK
        /* @@ label1 */bar(/* @@ label2 */x);       // CTE
        let y: Object = /* @@ label3 */x.fld;       // CTE
        let z = x.fld;                              // OK
    }
}

function main() {
    let a = new C<Int>(77)
    foo(a)
}


/* @@@ label1 Error Semantic error ESE0127: No matching call signature for bar(C<*>) */
/* @@@ label2 Error Semantic error ESE0046: Type 'C<*>' is not compatible with type 'C<Any>' at index 1 */
/* @@@ label3 Error Semantic error ESE0318: Type '*' cannot be assigned to type 'Object' */

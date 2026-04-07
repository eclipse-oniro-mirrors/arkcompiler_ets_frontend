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

class Base {
    overload constructor { fromNum, fromStr }

    constructor fromNum(n: number) { console.log(1) }
    constructor fromStr(s: string) { console.log(2) }
}

class D1 extends Base {
    constructor() {
        super(1);
    }
    overload constructor { super.fromNum, super.fromStr }  // CTE, identifier required
}

class D2 extends Base {
    constructor() {
        super.fromNum(1.0);
    }
    overload constructor { fromNum, fromStr }  // CTE
}

/* @@? 27:28 Error Syntax error ESY0324: The overloaded method name in class/interface method overload declaration must be identifier. */
/* @@? 27:28 Error Syntax error ESY0112: Unexpected token, expected an identifier. */
/* @@? 27:43 Error Syntax error ESY0324: The overloaded method name in class/interface method overload declaration must be identifier. */
/* @@? 27:43 Error Syntax error ESY0112: Unexpected token, expected an identifier. */
/* @@? 31:16 Error Semantic error ESE0192: Must call super constructor */
/* @@? 32:15 Error Semantic error ESE0087: Property 'fromNum' does not exist on type 'Base' */
/* @@? 34:28 Error Semantic error ESE0143: Unresolved reference fromNum */
/* @@? 34:37 Error Semantic error ESE0143: Unresolved reference fromStr */

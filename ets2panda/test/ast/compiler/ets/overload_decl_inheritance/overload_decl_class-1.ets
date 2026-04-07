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
    overload process { processNumber, processString }

    processNumber(n: number) { console.log(1) }
    processString(s: string) { console.log(2) }
}

class D1 extends Base {
    // method is overridden
    override processNumber(n: number) { console.log(3) }
    // overload declaration is inherited
}

class D11 extends D1 {
    overload process { processString }  // CTE: no processNumber
}

class D12 extends D1 {
    overload process { process, processString }  // CTE
}

class D13 extends D1 {
    overload process { process }  // should be CTE?
}

class D14 extends D1 {
    overload process { processNumber, processString }  // OK
}

// With new overload

class D3 extends Base {
    // method is added:
    processInt(n: int) {console.log(4)}
    // new order for overloaded methods is specified:
    overload process { processInt, process }  // CTE
}

class D31 extends Base {
    // method is added:
    processInt(n: int) {console.log(4)}
    // new order for overloaded methods is specified:
    overload process { processInt, processNumber, processString }  // OK
}

class D4 extends Base {
    // method is added:
    processNumber(n: float): string {return ""}  // not override
    // new order for overloaded methods is specified:
    overload process { processNumber, processString }  // OK
}

class D41 extends Base {
    // method is added:
    processNumber(n: float): string {return ""}  // not override
    // new order for overloaded methods is specified:
    overload process { processNumber, process }  // CTE?
}

class D5 extends Base {
    overload process { process, processNumber, processString }  // CTE?
}

class Base2 {
    overload process { process, processNumber, processString }

    process(i: int) { console.log(0) }
    processNumber(n: number) { console.log(1) }
    processString(s: string) { console.log(2) }
}

class D6 extends Base2 {
    overload process { process }  // CTE?
}

new D6().process("abc") // CTE of OK?

class D61 extends Base2 {
    overload process { process, processNumber, processString }  // OK
}

/* @@? 30:5 Error Semantic error ESE0390: Method 'processNumber' is missed in the overridden explicit overload 'process', but listed in the parent explicit overload. */
/* @@? 34:5 Error Semantic error ESE0390: Method 'processNumber' is missed in the overridden explicit overload 'process', but listed in the parent explicit overload. */
/* @@? 34:24 Error Semantic error ESE0384: overloaded name must refer to an accessible method. */
/* @@? 38:5 Error Semantic error ESE0390: Method 'processNumber' is missed in the overridden explicit overload 'process', but listed in the parent explicit overload. */
/* @@? 38:5 Error Semantic error ESE0390: Method 'processString' is missed in the overridden explicit overload 'process', but listed in the parent explicit overload. */
/* @@? 38:24 Error Semantic error ESE0384: overloaded name must refer to an accessible method. */
/* @@? 51:5 Error Semantic error ESE0390: Method 'processNumber' is missed in the overridden explicit overload 'process', but listed in the parent explicit overload. */
/* @@? 51:5 Error Semantic error ESE0390: Method 'processString' is missed in the overridden explicit overload 'process', but listed in the parent explicit overload. */
/* @@? 51:36 Error Semantic error ESE0384: overloaded name must refer to an accessible method. */
/* @@? 72:5 Error Semantic error ESE0390: Method 'processString' is missed in the overridden explicit overload 'process', but listed in the parent explicit overload. */
/* @@? 72:39 Error Semantic error ESE0384: overloaded name must refer to an accessible method. */
/* @@? 76:24 Error Semantic error ESE0384: overloaded name must refer to an accessible method. */
/* @@? 87:18 Error Semantic error ESE0216: Class D6 cannot extend class Base2 because the overriding declaration type for overload process is not compatible with the declaration type required by the base class. */
/* @@? 88:5 Error Semantic error ESE0390: Method 'processNumber' is missed in the overridden explicit overload 'process', but listed in the parent explicit overload. */
/* @@? 88:5 Error Semantic error ESE0390: Method 'processString' is missed in the overridden explicit overload 'process', but listed in the parent explicit overload. */
/* @@? 91:1 Error Semantic error ESE0127: No matching call signature for process("abc") */
/* @@? 91:18 Error Semantic error ESE0046: Type '"abc"' is not compatible with type 'Int' at index 1 */

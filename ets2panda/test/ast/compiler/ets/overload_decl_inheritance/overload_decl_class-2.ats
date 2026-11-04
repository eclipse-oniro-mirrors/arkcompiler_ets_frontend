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
    protected overload process { processNumber, processString }

    protected processNumber(n: number) { console.log(1) }
    public processString(s: string) { console.log(2) }
    private processU(u: string|int) { console.log(3) }
    protected processU2(u: string|int|double) { console.log(4) }
}

class D1 extends Base {
    private overload process { processU, processNumber, processString }  // should be CTE: processU is private
}

class D2 extends Base {
    processU3(u: string|double) { console.log(5) }
    protected overload process { processString, processNumber, processU2, processU3 }  // OK
}

class D3 extends Base {
    protected overload process { processString }  // CTE
}

/* @@? 26:32 Error Semantic error ESE0384: overloaded name must refer to an accessible method. */
/* @@? 35:5 Error Semantic error ESE0390: Method 'processNumber' is missed in the overridden explicit overload 'process', but listed in the parent explicit overload. */
/* @@? 31:24 Warning Warning W2323: The order of entities in an overload set implies that the overloaded entity with the signature '(u: String|Double) => undefined' will never be selected for a call, as the preceding entity with the signature '(u: String|Int|Double) => undefined' prevails. */

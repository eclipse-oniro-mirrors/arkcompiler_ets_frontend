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

    overload bar { bar1, bar2, bar3, bar4 }

    bar1() {}
    bar2(l1: "aa") {}
    bar3(l2: "bb") {}
    bar4(l3: "cc", x: ()=>void) {}

    // ---------------------------------

    overload process { processNumber, processString }

    processNumber(n: number) {}
    processString(s: string) {}


    // ---------------------------------

    overload foo { foo1, foo2 }

    foo1(n: number) {}
    foo2(s: string) {}
}

class D1 extends Base {
    // methods are overridden
    override bar2(l: "aa") {}
    override foo1(n: number) {}
    // overload declaration is inherited
}

class D2 extends Base {
    // methods are overridden
    override bar2(l: "aa") {}
    override foo1(n: number) {}

    overload foo { foo2, foo1 }  // OK
    overload bar { bar1, bar2, bar4 }  // CTE
}

class D3 extends Base {
    overload fooOverriden { foo2 }  // OK, another name
    overload bar { bar1, bar2, bar3, bar4 }   // OK
    overload process { processNumber, processString }   // OK
}

class D4 extends Base {
    overload processOverriden { processNumber }  // OK
    overload bar { bar4 }   // CTE
    overload foo { foo }  // CTE
    overload process { processNumber }   // CTE
}

/* @@? 54:5 Error Semantic error ESE0390: Method 'bar3' is missed in the overridden explicit overload 'bar', but listed in the parent explicit overload. */
/* @@? 65:5 Error Semantic error ESE0390: Method 'bar1' is missed in the overridden explicit overload 'bar', but listed in the parent explicit overload. */
/* @@? 65:5 Error Semantic error ESE0390: Method 'bar2' is missed in the overridden explicit overload 'bar', but listed in the parent explicit overload. */
/* @@? 65:5 Error Semantic error ESE0390: Method 'bar3' is missed in the overridden explicit overload 'bar', but listed in the parent explicit overload. */
/* @@? 66:5 Error Semantic error ESE0390: Method 'foo1' is missed in the overridden explicit overload 'foo', but listed in the parent explicit overload. */
/* @@? 66:5 Error Semantic error ESE0390: Method 'foo2' is missed in the overridden explicit overload 'foo', but listed in the parent explicit overload. */
/* @@? 66:20 Error Semantic error ESE0384: overloaded name must refer to an accessible method. */
/* @@? 67:5 Error Semantic error ESE0390: Method 'processString' is missed in the overridden explicit overload 'process', but listed in the parent explicit overload. */

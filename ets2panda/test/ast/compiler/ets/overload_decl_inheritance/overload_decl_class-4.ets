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
}

class D extends Base {
    override bar2(l: "aa") {}
    overload bar { bar1, bar2, bar4, bar3 }  // OK
}

class A1 extends D {}

class A2 extends A1 {
    overload bar { bar1, bar3 }  // CTE
}

class A3 extends A1 {
    private overload bar { bar1, bar2, bar4, bar3 }  // OK
}

class A4 extends A3 {
    private overload bar { bar1, bar2, bar4, bar3 }  // no CTE for now, but seems it should be
}

class B1 extends A1 {
    override bar3(l: "bb") {}
    overload bar { bar1, bar2, bar4, bar3 }  // OK
}

class B2 extends B1 {
    overload bar { bar1, bar2, bar4, bar3 }  // OK
}

class B3 extends B2 {
    overload bar { bar1, bar2, bar3 }  // CTE
}

/* @@? 34:5 Error Semantic error ESE0390: Method 'bar2' is missed in the overridden explicit overload 'bar', but listed in the parent explicit overload. */
/* @@? 34:5 Error Semantic error ESE0390: Method 'bar4' is missed in the overridden explicit overload 'bar', but listed in the parent explicit overload. */
/* @@? 55:5 Error Semantic error ESE0390: Method 'bar4' is missed in the overridden explicit overload 'bar', but listed in the parent explicit overload. */

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

class Base {
    baseFoo() {}
    static staticBaseFoo() {}
}

interface I {
    interfaceFoo() {}
}

class C extends Base implements I {
    foo1() {}
    static foo2() {}

    constructor()
    {
        this.interfaceFoo = bar;
    }

    constructor(arg: () => void)
    {
        this.foo1 = arg;
    }
}

function bar() {}

let c = new C();
c = new C(bar);
c.baseFoo = bar;
C.staticBaseFoo = bar;
c.foo1 = bar;
C.foo2 = bar;
c.interfaceFoo = bar;

/* @@? 31:9 Error TypeError: Class methods cannot be overwritten. */
/* @@? 36:9 Error TypeError: Class methods cannot be overwritten. */
/* @@? 44:1 Error TypeError: Class methods cannot be overwritten. */
/* @@? 45:1 Error TypeError: Class methods cannot be overwritten. */
/* @@? 46:1 Error TypeError: Class methods cannot be overwritten. */
/* @@? 47:1 Error TypeError: Class methods cannot be overwritten. */
/* @@? 48:1 Error TypeError: Class methods cannot be overwritten. */

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

/*---
flags: [dynamic-ast]
---*/

import { A, I, Animal } from "dynamic_import_tests/modules/module"

let a = /* @@ label */new Animal() // CTE

class Bird extends Animal /* @@ label1 */{ // OK
    move(): void { }
}

class C extends A /* @@ label2 */{
    override foo(p: double): string {
    } //ok
} // ok

let c = new C()
c.f1 // ok
C.f2 // ok
c./* @@ label3 */f3 // cte
c.foo(123) // ok
C.bar() // ok
c./* @@ label4 */foa() //cte

class C1 implements I /* @@ label5 */{ // ok
    s: string
    f1(p: string): double {
        return 12
    }
    f2(p: double): string {
        return ""
    }
}

function foo(i: I) {
    i.s // ok
    i./* @@ label6 */n // cte
    i.f1("") // ok
    i./* @@ label7 */f3() // cte
}

/* @@@ label Error TypeError: Animal is abstract therefore cannot be instantiated. */
/* @@@ label1 Error TypeError: class Bird can not extends class Animal which is from dynamic declaration file. */
/* @@@ label2 Error TypeError: class C can not extends class A which is from dynamic declaration file. */
/* @@@ label3 Error TypeError: Property 'f3' does not exist on type 'C' */
/* @@@ label4 Error TypeError: Property 'foa' does not exist on type 'C' */
/* @@@ label5 Error TypeError: class C1 can not implements interface I which is from dynamic declaration file. */
/* @@@ label6 Error TypeError: Property 'n' does not exist on type 'I' */
/* @@@ label7 Error TypeError: Property 'f3' does not exist on type 'I' */
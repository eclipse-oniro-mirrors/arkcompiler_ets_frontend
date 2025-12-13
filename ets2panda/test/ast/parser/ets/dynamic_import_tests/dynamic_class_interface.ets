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

let a = new Animal() // CTE

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
c.f3 // cte
c.foo(123) // ok
C.bar() // ok
c.foa() //cte

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
    i.n // cte
    i.f1("") // ok
    i.f3() // cte
}

/* @@? 22:9 Error Semantic error ESE0052: Animal is abstract therefore cannot be instantiated. */
/* @@? 24:42 Error Semantic error ESE24143: class Bird can not extends class Animal which is from dynamic declaration file. */
/* @@? 28:34 Error Semantic error ESE24143: class C can not extends class A which is from dynamic declaration file. */
/* @@? 36:3 Error Semantic error ESE0087: Property 'f3' does not exist on type 'C' */
/* @@? 39:3 Error Semantic error ESE0087: Property 'foa' does not exist on type 'C' */
/* @@? 41:38 Error Semantic error ESE24143: class C1 can not implements interface I which is from dynamic declaration file. */
/* @@? 53:7 Error Semantic error ESE0087: Property 'n' does not exist on type 'I' */
/* @@? 55:7 Error Semantic error ESE0087: Property 'f3' does not exist on type 'I' */

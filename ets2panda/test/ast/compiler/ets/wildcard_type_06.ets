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

class A { fooA() {} }
class B { fooB() {} }

class C<T> {
    fld: T;

    constructor(p: T) {
        this.fld = p;
    }
}

function foo(p: C<A>|C<B>|undefined) {
    let v: C<A>|C<B>;
    if (p instanceof C) {
        v = p;				                    // OK
        let y: C<A> = /* @@ label */p;	  // CTE
        y.fld.fooA();
    }
}

foo(new C<A>(new A ()))
foo(new C<B>(new B()))


/* @@@ label Error Semantic error ESE0318: Type 'C<A>|C<B>' cannot be assigned to type 'C<A>' */

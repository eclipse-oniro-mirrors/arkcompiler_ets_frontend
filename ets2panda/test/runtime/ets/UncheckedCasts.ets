/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

function assert_ccexc(f: () => void) {
    try {
        f();
    } catch (e) {
        arktest.assertTrue(e instanceof ClassCastError)
        return;
    }
    arktest.assertTrue(false, "exception expected")
}

class A { }
class B { }
class C { }
class X<T> { }

function erase<T>(x: Object | null | undefined): T { return x as T; }

function test_substitution() {
    assert_ccexc(() => { erase<Object>(null); })
    assert_ccexc(() => { erase<Object>(undefined); })
    assert_ccexc(() => { erase<A>(null); })
    assert_ccexc(() => { erase<A>(undefined); })

    assert_ccexc(() => { erase<Object | undefined>(null); })
    assert_ccexc(() => { erase<Object | null>(undefined); })
    assert_ccexc(() => { erase<A | undefined>(null); })
    assert_ccexc(() => { erase<A | null>(undefined); })

    assert_ccexc(() => { erase<A>(undefined); })
    assert_ccexc(() => { erase<A>(new Object()); })
    assert_ccexc(() => { erase<A>(new B()); })

    assert_ccexc(() => { erase<A | B>(undefined); })
    assert_ccexc(() => { erase<A | B>(new Object()); })
    assert_ccexc(() => { erase<A | B>(new C()); })

    assert_ccexc(() => { erase<FixedArray<A>>(new B[0]); })
}

class Erased<T> {
    constructor(x: Object | null | undefined) { this.t = x as T; }
    t: T;
}

function test_substitution_memberexpr() {
    assert_ccexc(() => { new Erased<Object>(null).t; })
    assert_ccexc(() => { new Erased<Object>(undefined).t; })
    assert_ccexc(() => { new Erased<A>(null).t; })
    assert_ccexc(() => { new Erased<A>(undefined).t; })

    assert_ccexc(() => { new Erased<Object | undefined>(null).t; })
    assert_ccexc(() => { new Erased<Object | null>(undefined).t; })
    assert_ccexc(() => { new Erased<A | undefined>(null).t; })
    assert_ccexc(() => { new Erased<A | null>(undefined).t; })

    assert_ccexc(() => { new Erased<A>(undefined).t; })
    assert_ccexc(() => { new Erased<A>(new Object()).t; })
    assert_ccexc(() => { new Erased<A>(new B()).t; })

    assert_ccexc(() => { new Erased<A | B>(undefined).t; })
    assert_ccexc(() => { new Erased<A | B>(new Object()).t; })
    assert_ccexc(() => { new Erased<A | B>(new C()).t; })

    assert_ccexc(() => { new Erased<FixedArray<A>>(new B[0]).t; })
}

function cast_to_tparam<T extends A | B | null>(x: Object | null | undefined) { x as T; }

function test_constraint() {
    assert_ccexc(() => { cast_to_tparam<A>(undefined); })
    assert_ccexc(() => { cast_to_tparam<A>(new Object()); })
    assert_ccexc(() => { cast_to_tparam<A>(new C()); })
}

function to_basetype<T>(x: Object | null | undefined) { return x as X<T>; }

function test_basetype() {
    assert_ccexc(() => { to_basetype<A>(null); })
    assert_ccexc(() => { to_basetype<A>(undefined); })
    assert_ccexc(() => { to_basetype<A>(new Object()); })
    assert_ccexc(() => { to_basetype<A>(new C()); })
}

function main() {
    test_substitution();
    test_substitution_memberexpr();
    test_constraint();
    test_basetype();
}

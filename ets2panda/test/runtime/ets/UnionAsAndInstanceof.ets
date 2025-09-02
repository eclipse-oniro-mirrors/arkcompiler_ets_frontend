/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

function assert_nothrow(f: () => void) {
    try {
        f();
    } catch (e) {
        arktest.assertTrue(false, "unexpected exception")
    }
}

class A { }
class B { }
class C { }

function foo(x: Object | null | undefined) { return x as Object }

function test_nullsafety() {
    // Handling of Object may be a bit different, so test it separately
    // let f = ... until inference in form ((p)=>expr)(a) is broken
    assert_ccexc(() => { let f = ((x: Object | null | undefined) => x as Object); f(null); });
    assert_ccexc(() => { let f = ((x: Object | null | undefined) => x as Object); f(undefined); });
    assert_ccexc(() => { let f = ((x: Object | null) => x as Object); f(null); });
    assert_ccexc(() => { let f = ((x: Object | undefined) => x as Object); f(undefined); });

    assert_ccexc(() => { let f = ((x: Object | null | undefined) => x as Object | undefined); f(null); });
    assert_ccexc(() => { let f = ((x: Object | null | undefined) => x as Object | null); f(undefined); });
    assert_ccexc(() => { let f = ((x: Object | null) => x as Object | undefined); f(null); });
    assert_ccexc(() => { let f = ((x: Object | undefined) => x as Object | null); f(undefined); });

    assert_ccexc(() => { let f = ((x: A | null | undefined) => x as A); f(null); });
    assert_ccexc(() => { let f = ((x: A | null | undefined) => x as A); f(undefined); });
    assert_ccexc(() => { let f = ((x: A | null) => x as A); f(null); });
    assert_ccexc(() => { let f = ((x: A | undefined) => x as A); f(undefined); });

    assert_ccexc(() => { let f = ((x: A | null | undefined) => x as A | undefined); f(null); });
    assert_ccexc(() => { let f = ((x: A | null | undefined) => x as A | null); f(undefined); });
    assert_ccexc(() => { let f = ((x: A | null) => x as A | undefined); f(null); });
    assert_ccexc(() => { let f = ((x: A | undefined) => x as A | null); f(undefined); });


    assert_nothrow(() => { let f = ((x: Object | null | undefined) => x as Object); f(new Object()); });
    assert_nothrow(() => { let f = ((x: Object | null | undefined) => x as Object); f(new Object); });
    assert_nothrow(() => { let f = ((x: Object | null) => x as Object); f(new Object()); });
    assert_nothrow(() => { let f = ((x: Object | undefined) => x as Object); f(new Object()); });

    assert_nothrow(() => { let f = ((x: Object | null | undefined) => x as Object | undefined); f(new Object()); });
    assert_nothrow(() => { let f = ((x: Object | null | undefined) => x as Object | null); f(new Object()); });
    assert_nothrow(() => { let f = ((x: Object | null) => x as Object | undefined); f(new Object()); });
    assert_nothrow(() => { let f = ((x: Object | undefined) => x as Object | null); f(new Object()); });

    assert_nothrow(() => { let f = ((x: A | null | undefined) => x as A); f(new A()); });
    assert_nothrow(() => { let f = ((x: A | null | undefined) => x as A); f(new A()); });
    assert_nothrow(() => { let f = ((x: A | null) => x as A); f(new A()); });
    assert_nothrow(() => { let f = ((x: A | undefined) => x as A); f(new A()); });

    assert_nothrow(() => { let f = ((x: A | null | undefined) => x as A | undefined); f(new A()); });
    assert_nothrow(() => { let f = ((x: A | null | undefined) => x as A | null); f(new A()); });
    assert_nothrow(() => { let f = ((x: A | null) => x as A | undefined); f(new A()); });
    assert_nothrow(() => { let f = ((x: A | undefined) => x as A | null); f(new A()); });
}

function test_unions() {
    assert_ccexc(() => { let f = ((x: A | B | C) => x as A); f(new C()); });
    assert_ccexc(() => { let f = ((x: A | B | C) => x as A | B); f(new C()); });
    assert_ccexc(() => { let f = ((x: A | B | C | null) => x as A | B); f(null); });
    assert_ccexc(() => { let f = ((x: A | B | C | undefined) => x as A | B); f(undefined); });

    assert_ccexc(() => { let f = ((x: A | null | undefined) => x as A | undefined); f(null); });
    assert_ccexc(() => { let f = ((x: A | null | undefined) => x as A | null); f(undefined); });
    assert_ccexc(() => { let f = ((x: A | null) => x as A | undefined); f(null); });
    assert_ccexc(() => { let f = ((x: A | undefined) => x as A | null); f(undefined); });

    assert_ccexc(() => { let f = ((x: A | B | C) => x as A); f(new C()); });
    assert_ccexc(() => { let f = ((x: A | B | C) => x as A | B); f(new C()); });
    assert_ccexc(() => { let f = ((x: A | B | C | null) => x as A | B); f(null); });
    assert_ccexc(() => { let f = ((x: A | B | C | undefined) => x as A | B); f(undefined); });

    assert_ccexc(() => { let f = ((x: A | null | undefined) => x as A | undefined); f(null); });
    assert_ccexc(() => { let f = ((x: A | null | undefined) => x as A | null); f(undefined); });
    assert_ccexc(() => { let f = ((x: A | null) => x as A | undefined); f(null); });
    assert_ccexc(() => { let f = ((x: A | undefined) => x as A | null); f(undefined); });
}

function main() {
    test_nullsafety();
    test_unions();
}

/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

class A {
    constructor(tag: string) { this.tag = tag }
    toString() { return this.tag }
    tag: string
}
class B extends A {
    constructor(tag: string) {
        super(tag)
    }
}

function test(f: Function, action: (f: Function) => Any, v: Any) {
    arktest.assertEQ(action(f), v)
}

function main() {
    test((a: A) => "" + a,
        (f) => f.unsafeCall(new B("tag")),
        "tag"
    )
    test((a: string, b: string) => a + b,
        (f) => f.unsafeCall("a", "b", "c"),
        "ab"
    )
    test((a?: int) => a == undefined ? -1 : a,
        (f) => f.unsafeCall(),
        -1
    )
    test((a?: int, b?: int) => (a == undefined ? 10 : a) + (b == undefined ? 20 : b),
        (f) => f.unsafeCall(),
        30
    )
    test((a: string, b?: string) => a + b,
        (f) => f.unsafeCall("a"),
        "aundefined"
    )
    test((a: string, b?: string) => b == undefined ? a + "!" : a + b,
        (f) => f.unsafeCall("a"),
        "a!"
    )
    test((...a: string[]) => a[0] + a[1],
        (f) => f.unsafeCall("a", "b"),
        "ab"
    )
    test((...a: A[]) => "" + a[0] + a[1],
        (f) => f.unsafeCall(new A("taga"), new B("tagb")),
        "tagatagb"
    )
    test((p1: A, ...a: A[]) => "" + p1,
        (f) => f.unsafeCall(new A("1")),
        "1"
    )
    test((p1: A, ...a: A[]) => "" + p1 + a[0] + a[1],
        (f) => f.unsafeCall(new A("1"), new A("2"), new A("3")),
        "123"
    )
}

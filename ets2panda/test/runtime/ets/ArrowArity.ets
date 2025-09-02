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

type S = string

function xassert(a: S, b: S) { arktest.assertEQ(a, b) }

function p0call(f: () => S, a: S[]) {
    return f()
}
function p1call(f: (p1: S) => S, a: S[]) {
    return f(a[0])
}
function p2call(f: (p1: S, p2: S) => S, a: S[]) {
    return f(a[0], a[1])
}
function p012call(f: () => S, a: S[]) {
    return p0call(f, a) + p1call(f, a) + p2call(f, a)
}
function p_12call(f: (p1: S) => S, a: S[]) {
    return p1call(f, a) + p2call(f, a)
}
function p__2call(f: (p1: S, p2: S) => S, a: S[]) {
    return p2call(f, a)
}

function r0__call(f: (p1?: S, p2?: S) => S, a: S[]) {
    return f() + f(a[0]) + f(a[0], a[1])
}
function r_1_call(f: (p1: S, p2?: S) => S, a: S[]) {
    return f(a[0]) + f(a[0], a[1])
}

function testBasicArity() {
    let args = ["a", "b"]
    let f0 = () => "/f0:"
    let f1 = (a1: S) => "/f1:" + a1
    let f2 = (a1: S, a2: S) => "/f2:" + a1 + a2

    xassert(p012call(f0, args), "/f0:/f0:/f0:")
    xassert(p_12call(f0, args), "/f0:/f0:")
    xassert(p_12call(f1, args), "/f1:a/f1:a")
    xassert(p__2call(f0, args), "/f0:")
    xassert(p__2call(f1, args), "/f1:a")
    xassert(p__2call(f2, args), "/f2:ab")

    xassert(r0__call(f0, args), "/f0:/f0:/f0:")
    xassert(r_1_call(f0, args), "/f0:/f0:")
    xassert(r_1_call(f1, args), "/f1:a/f1:a")
}
testBasicArity();

function testOptionalArity() {
    let args = ["a", "b"]
    let tou = (v?: string) => v ?? "u"
    let f0 = (a1?: S, a2?: S) => "/f0:" + tou(a1) + tou(a2)
    let f1 = (a1: S, a2?: S) => "/f1:" + a1 + tou(a2)
    let f2 = (a1: S, a2: S) => "/f2:" + a1 + a2

    xassert(p012call(f0, args), "/f0:uu/f0:au/f0:ab")
    xassert(p_12call(f0, args), "/f0:au/f0:ab")
    xassert(p_12call(f1, args), "/f1:au/f1:ab")
    xassert(p__2call(f0, args), "/f0:ab")
    xassert(p__2call(f1, args), "/f1:ab")
    xassert(p__2call(f2, args), "/f2:ab")

    xassert(r0__call(f0, args), "/f0:uu/f0:au/f0:ab")
    xassert(r_1_call(f0, args), "/f0:au/f0:ab")
    xassert(r_1_call(f1, args), "/f1:au/f1:ab")
}
testOptionalArity();

function testDfltArity() {
    let args = ["a", "b"]
    let f0 = (a1: S = "x", a2: S = "y") => "/f0:" + a1 + a2
    let f1 = (a1: S, a2: S = "y") => "/f1:" + a1 + a2
    let f2 = (a1: S, a2: S) => "/f2:" + a1 + a2

    xassert(p012call(f0, args), "/f0:xy/f0:ay/f0:ab")
    xassert(p_12call(f0, args), "/f0:ay/f0:ab")
    xassert(p_12call(f1, args), "/f1:ay/f1:ab")
    xassert(p__2call(f0, args), "/f0:ab")
    xassert(p__2call(f1, args), "/f1:ab")
    xassert(p__2call(f2, args), "/f2:ab")

    xassert(r0__call(f0, args), "/f0:xy/f0:ay/f0:ab")
    xassert(r_1_call(f0, args), "/f0:ay/f0:ab")
    xassert(r_1_call(f1, args), "/f1:ay/f1:ab")
}
testDfltArity()

function testDfltEvaluation() {
    let counter = "c"
    let inc = () => (counter += "x")
    let f = (a: string = inc()) => a
    xassert(f("a"), "a")
    xassert(f(), "cx")
    xassert(f(), "cxx")
    xassert(f(undefined), "cxxx")
}
testDfltEvaluation()

function testArrowExprCalls() {
    let x = (a1: S, a2?: S, a3?: S) => a1 + (a2 ?? "u") + (a3 ?? "u")
    xassert(x("a"), "auu")
    xassert(x("a", "b"), "abu")
    xassert(x("a", "b", "c"), "abc")

    xassert(((a?: S) => a ?? "u")(), "u")
    // xassert(((a?: S) => a ?? "u")("a"), "a") // #22952: broken
}
testArrowExprCalls()

class X<T> {
    constructor(v: (p?: T) => T) { this.fv = v }
    get f() { return this.fv }
    fv: (p?: T) => T
}
function gcall<T>(x: X<T>, a: T) {
    (x.f)();
    (x.f)(a);
    (x.fv)();
    (x.fv)(a);
}
function testGenerics() {
    let res: string = ":"
    let fn = (a?: string) => {
        res += (a ?? "D");
        return res;
    };
    gcall(new X<string>(fn), "a") // #22952: inference fails
    xassert(res, ":DaDa")
}
testGenerics();

function foo(a1: S, a2?: S) { return a1 + (a2 ?? "D") }
function testFuncRef() {
    let f = (a1: S, a2?: S) => foo(a1, a2) // #22952: foo is overloaded
    xassert(f("a"), "aD")
    xassert(f("a", undefined), "aD")
    xassert(f("a", "b"), "ab")
}
testFuncRef();

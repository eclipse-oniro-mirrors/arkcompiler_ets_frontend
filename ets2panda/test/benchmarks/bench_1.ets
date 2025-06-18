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

// ---------------------------------------------------------------------------------------------------------------------
// AccessBinaryTrees.ets
// ---------------------------------------------------------------------------------------------------------------------

class TreeNode {
  private left: TreeNode | null;
  private right: TreeNode | null;
  private item: int;

  constructor(left: TreeNode | null, right: TreeNode | null, item: int) {
    this.left = left;
    this.right = right;
    this.item = item;
  }

  public itemCheck(): int {
    if (this.left == null)
      return this.item;
    else
      return this.item + this.left!.itemCheck() - this.right!.itemCheck();
  }
}

export class AccessBinaryTrees {
  static readonly startDepth = 4;
  static readonly endDepth = 7;
  static readonly expected = -4;

  static bottomUpTree(item: int, depth: int): TreeNode {
   if (depth > 0) {
     return new TreeNode(
       AccessBinaryTrees.bottomUpTree(2*item - 1, depth-1),
       AccessBinaryTrees.bottomUpTree(2*item, depth-1),
       item
     );
   }
   return new TreeNode(null, null, item);
  }

  public run(): void {
    let ret: int = 0;

    for (let n: int = AccessBinaryTrees.startDepth; n <= AccessBinaryTrees.endDepth; n++) {
      let minDepth: int = AccessBinaryTrees.startDepth;
      let maxDepth: int = max(minDepth + 2, n);
      let stretchDepth: int = maxDepth + 1;
      let check: int = AccessBinaryTrees.bottomUpTree(0, stretchDepth).itemCheck();

      let longLivedTree = AccessBinaryTrees.bottomUpTree(0, maxDepth);

      for (let depth = minDepth; depth <= maxDepth; depth += 2) {
        let iterations: int = 1 << (maxDepth - depth + minDepth);

        check = 0;
        for (let i: int = 1; i <= iterations; i++) {
          check += AccessBinaryTrees.bottomUpTree(i, depth).itemCheck();
          check += AccessBinaryTrees.bottomUpTree(-i, depth).itemCheck();
        }
      }

      ret += longLivedTree.itemCheck();
    }

    arktest.assertEQ(ret, AccessBinaryTrees.expected,  "Incorrect result")
  }
}

// ---------------------------------------------------------------------------------------------------------------------
// RestTuple6.ets
// ---------------------------------------------------------------------------------------------------------------------

class A2<T> {}
class B2<T> {}

class C2 {
  foo<T, U>(...p: [A2<T>, B2<U>]): boolean {
      return p[0] == p[1]
  }

  moo<T, U>(a:int, ...p: [A2<T>, B2<U>]): boolean {
      return p[0] == p[1]
  }
}

// ---------------------------------------------------------------------------------------------------------------------
// GenericBridges_02.ets
// ---------------------------------------------------------------------------------------------------------------------

interface B3<T3> {
  f23(v: B3<T3>): B3<T3>;
}

class C3<T3 extends Numeric|string, Z3, T1 = string> implements B3<T3> {

  f13(v: T3): T3 {
    return v;
  }

  f23(v: C3<T3, Z3>): C3<T3, Z3> {
     return new C3<T3, Z3>();
  }

  f23(v: B3<T3>): B3<T3> {
     return this.f23(v as C3<T3, Z3>);
  }

  f33(v: T3|string): string {
    return "C3.f33";
  }

  f43(v: C3<T3, Z3>|Numeric): string {
    return "C3.f43";
  }

  f53(x: T1|Z3|C3<T3, Z3, T1>[]): string {
    return "C3.f53";
  }

  f63(x: C3<T3, Z3>, y: C3<T3, Z3>): string {
    return "C3.f63";
  }

  f73(x: T3, y: C3<T3, Z3>): string {
    return "C3.f73";
  }

  f83(x: string): string {
    return "C3.f83";
  }

  f93(z: Z3, y: T1): string {
    return "C3.f93";
  }
}

class D3<W3> extends C3<string, W3> {
  f13(v: string): string {
    return "D3.f13";
  }

  f13(v: Numeric|string|C3<Int, W3>): string {
    return this.f13(v as string);
  }

  f13(v: Int): Int {
    return 7;
  }

  f23(v: D3<W3>): D3<W3> {
    return new D3<W3>();
  }

  f33(v: string): string {
    return "D3.f33";
  }

  f43(v: D3<W3>): string {
    return "D3.f43";
  }

  f43 (x: int, y: int): int {
    return x + y;
  }

  f53(x: string|W3|C3<string, W3>[]): string {
    return "D3.f53";
  }

  f63(): string {
    return "D3.f63";
  }

//   f73(x: string, y: D3<W3>): string {
//     return "D3.f73";
//   }

  f83(x: string): string {
    return "D3.f83";
  }

  f93(z: W3, y: string): string {
    return "D3.f93-1";
  }

  f93(z: W3, y: Int): string {
    return "D3.f93-2";
  }
}

class F3 extends D3<string> {}

class G3<U3 extends Numeric|string> extends C3<U3, string, Int> {}

class E3<U3 extends Integral> extends C3<U3, string> {

  f13(v: U3): Integral {
    if (v instanceof Int) {
      return new Int(7);
    } else if (v instanceof Long) {
      return new Long(8);
    } else {
      return new Int(-1);
    }
  }

  f23(v: E3<U3>): E3<U3> {
     return new E3<U3>();
  }

  f33(){}

  f43(x:int, y: int): int { return x + y; }

//   f73(x: U3, y: E3<U3>): string {
//     return "E3.f73";
//   }
}

function foo13(c: C3<Int, string>) {
  arktest.assertEQ(c.f13(0), 7)
  arktest.assertEQ(c.f23(c).f13(0), 7)
  arktest.assertEQ(c.f53(""), "C3.f53")
  arktest.assertEQ(c.f63(c, c), "C3.f63")
}

function foo23(c: C3<Long, string>) {
 arktest.assertEQ(c.f13(0), 8)
 arktest.assertEQ(c.f23(c).f13(0), 8)
 arktest.assertEQ(c.f53(""), "C3.f53")
 arktest.assertEQ(c.f73(3, c), "E3.f73")
}

function ttt3(c: C3<string, string>): void {
  arktest.assertEQ(c.f13("ah"), "D3.f13")
  arktest.assertEQ(c.f23(c).f13("ah"), "D3.f13")
  arktest.assertEQ((c.f23(c as B3<string>) as C3<string, string>).f13("ah"), "D3.f13")
  arktest.assertEQ(c.f33("ah"), "D3.f33")
  arktest.assertEQ(c.f43(c), "D3.f43")
  arktest.assertEQ(c.f53("ah"), "D3.f53")
  arktest.assertEQ(c.f63(c, c), "C3.f63")
  arktest.assertEQ((c as D3<string>).f63(), "D3.f63")
  arktest.assertEQ(c.f73("ah", c), "D3.f73")
  arktest.assertEQ(c.f83(""), "D3.f83")
  arktest.assertEQ(c.f93("", ""), "C3.f93")
  arktest.assertEQ((c as D3<string>).f93("", 0), "D3.f93-2")
}

// ---------------------------------------------------------------------------------------------------------------------
// ArrowArity.ets
// ---------------------------------------------------------------------------------------------------------------------

type S4 = string

function xassert4(a: S4, b: S4) { arktest.assertEQ(a, b) }

function p0call4(f: () => S4, a: S4[]) {
    return f()
}
function p1call4(f: (p1: S4) => S4, a: S4[]) {
    return f(a[0])
}
function p2call4(f: (p1: S4, p2: S4) => S4, a: S4[]) {
    return f(a[0], a[1])
}
function p012call4(f: () => S4, a: S4[]) {
    return p0call4(f, a) + p1call4(f, a) + p2call4(f, a)
}
function p_12call4(f: (p1: S4) => S4, a: S4[]) {
    return p1call4(f, a) + p2call4(f, a)
}
function p__2call4(f: (p1: S4, p2: S4) => S4, a: S4[]) {
    return p2call4(f, a)
}

function r0__call4(f: (p1?: S4, p2?: S4) => S4, a: S4[]) {
    return f() + f(a[0]) + f(a[0], a[1])
}
function r_1_call4(f: (p1: S4, p2?: S4) => S4, a: S4[]) {
    return f(a[0]) + f(a[0], a[1])
}

function testBasicArity4() {
    let args = ["a", "b"]
    let f0 = () => "/f0:"
    let f1 = (a1: S4) => "/f1:" + a1
    let f2 = (a1: S4, a2: S4) => "/f2:" + a1 + a2

    xassert4(p012call4(f0, args), "/f0:/f0:/f0:")
    xassert4(p_12call4(f0, args), "/f0:/f0:")
    xassert4(p_12call4(f1, args), "/f1:a/f1:a")
    xassert4(p__2call4(f0, args), "/f0:")
    xassert4(p__2call4(f1, args), "/f1:a")
    xassert4(p__2call4(f2, args), "/f2:ab")

    xassert4(r0__call4(f0, args), "/f0:/f0:/f0:")
    xassert4(r_1_call4(f0, args), "/f0:/f0:")
    xassert4(r_1_call4(f1, args), "/f1:a/f1:a")
}
testBasicArity4();

function testOptionalArity4() {
    let args = ["a", "b"]
    let tou = (v?: string) => v ?? "u"
    let f0 = (a1?: S4, a2?: S4) => "/f0:" + tou(a1) + tou(a2)
    let f1 = (a1: S4, a2?: S4) => "/f1:" + a1 + tou(a2)
    let f2 = (a1: S4, a2: S4) => "/f2:" + a1 + a2

    xassert4(p012call4(f0, args), "/f0:uu/f0:au/f0:ab")
    xassert4(p_12call4(f0, args), "/f0:au/f0:ab")
    xassert4(p_12call4(f1, args), "/f1:au/f1:ab")
    xassert4(p__2call4(f0, args), "/f0:ab")
    xassert4(p__2call4(f1, args), "/f1:ab")
    xassert4(p__2call4(f2, args), "/f2:ab")

    xassert4(r0__call4(f0, args), "/f0:uu/f0:au/f0:ab")
    xassert4(r_1_call4(f0, args), "/f0:au/f0:ab")
    xassert4(r_1_call4(f1, args), "/f1:au/f1:ab")
}
testOptionalArity4();

function testDfltArity4() {
    let args = ["a", "b"]
    let f0 = (a1: S4 = "x", a2: S4 = "y") => "/f0:" + a1 + a2
    let f1 = (a1: S4, a2: S4 = "y") => "/f1:" + a1 + a2
    let f2 = (a1: S4, a2: S4) => "/f2:" + a1 + a2

    xassert4(p012call4(f0, args), "/f0:xy/f0:ay/f0:ab")
    xassert4(p_12call4(f0, args), "/f0:ay/f0:ab")
    xassert4(p_12call4(f1, args), "/f1:ay/f1:ab")
    xassert4(p__2call4(f0, args), "/f0:ab")
    xassert4(p__2call4(f1, args), "/f1:ab")
    xassert4(p__2call4(f2, args), "/f2:ab")

    xassert4(r0__call4(f0, args), "/f0:xy/f0:ay/f0:ab")
    xassert4(r_1_call4(f0, args), "/f0:ay/f0:ab")
    xassert4(r_1_call4(f1, args), "/f1:ay/f1:ab")
}
testDfltArity4()

function testDfltEvaluation4() {
    let counter = "c"
    let inc = () => (counter += "x")
    let f = (a: string = inc()) => a
    xassert4(f("a"), "a")
    xassert4(f(), "cx")
    xassert4(f(), "cxx")
    xassert4(f(undefined), "cxxx")
}
testDfltEvaluation4()

function testArrowExprCalls4() {
    let x = (a1: S4, a2?: S4, a3?: S4) => a1 + (a2 ?? "u") + (a3 ?? "u")
    xassert4(x("a"), "auu")
    xassert4(x("a", "b"), "abu")
    xassert4(x("a", "b", "c"), "abc")

    xassert4(((a?: S4) => a ?? "u")(), "u")
    // xassert4(((a?: S4) => a ?? "u")("a"), "a") // #22952: broken
}
testArrowExprCalls4()

class X4<T4> {
    constructor(v: (p?: T4) => T4) { this.fv = v }
    get f() { return this.fv }
    fv: (p?: T4) => T4
}
function gcall4<T4>(x: X4<T4>, a: T4) {
    (x.f)();
    (x.f)(a);
    (x.fv)();
    (x.fv)(a);
}
function testGenerics4() {
    let res: string = ":"
    let fn = (a?: string) => {
        res += (a ?? "D");
        return res;
    };
    gcall4(new X4<string>(fn), "a") // #22952: inference fails
    xassert4(res, ":DaDa")
}
testGenerics4();

function foo4(a1: S4, a2?: S4) { return a1 + (a2 ?? "D") }
function testFuncRef4() {
    let f = (a1: S4, a2?: S4) => foo4(a1, a2) // #22952: foo4 is overloaded
    xassert4(f("a"), "aD")
    xassert4(f("a", undefined), "aD")
    xassert4(f("a", "b"), "ab")
}
testFuncRef4();

// ---------------------------------------------------------------------------------------------------------------------
// OptionalChains.ets
// ---------------------------------------------------------------------------------------------------------------------

function assert_n5(v: Object | null | undefined) { arktest.assertTrue(v === null); }
function assert_u5(v: Object | null | undefined) { arktest.assertTrue(v === undefined); }
function assert_o5(v: Object | null | undefined) { arktest.assertTrue(v !== null && v !== undefined); }
function assert_npe5(f: () => void) {
    try {
        f();
    } catch (e) {
        return;
    }
    arktest.assertTrue(false,  "npe was not thrown")
}

class Link5 {
    m(): Link5 { return this; }
    f: Link5 = this;
    a: Link5[] = [(this)];
    c: () => Link5 = () => this

    om(): Link5 | null { return this.m() }
    of: Link5 | null = this.f;
    oa: Link5[] | null = this.a;
    oc: (() => Link5) | null = this.c;

    nm(): Link5 | null { return null }
    nf: Link5 | null = null;
    na: Link5[] | null = null;
    nc: (() => Link5) | null = null;

    static noevalFlag = true;
    noeval(): Link5 { if (Link5.noevalFlag) { throw new Error("never evaluated"); } return this; }
}

function test15(l: Link5 | null, nl: Link5 | null) {
    assert_o5(l?.m());
    assert_o5(l?.f);
    assert_o5(l?.a[0]);
    assert_o5(l?.c());
    assert_o5(l?.of!.f);

    assert_u5(nl?.m());
    assert_u5(nl?.f);
    assert_u5(nl?.a[0]);
    assert_u5(nl?.c());
    assert_u5(nl?.of!.f);
    assert_u5(nl?.nf!.f);

    nl?.m().noeval();
    nl?.f.noeval();
    nl?.a[0].noeval();
    nl?.c().noeval();
    nl?.of!.f.noeval();
    assert_npe5(() => { nl?.of!.f! });
}

function test25(l: Link5 | null, nl: Link5 | null) {
    assert_o5(l?.m().f.a[0].c());
    assert_o5(l?.f.m().c().a[0]);
    assert_o5(l?.a[0].c().f.m());
    assert_o5(l?.c().m().a[0].f);
    assert_o5(l?.c().m().of!.a[0].oc!().f);

    assert_u5(nl?.m().f.a[0].c());
    assert_u5(nl?.f.m().c().a[0]);
    assert_u5(nl?.a[0].c().f.m());
    assert_u5(nl?.c().m().a[0].f);
    assert_u5(nl?.c().m().of!.a[0].oc!().f);

    nl?.m().f.a[0].c().noeval();
    nl?.f.m().c().a[0].noeval();
    nl?.a[0].c().f.m().noeval();
    nl?.c().m().a[0].f.noeval();
    nl?.c().m().of!.a[0].oc!().f.noeval();
}

function test35(l: Link5 | null, nl: Link5 | null) {
    assert_o5(l?.om()?.of?.oa?.[0].oc?.());
    assert_o5(l?.of?.om()?.oc?.().oa?.[0]);
    assert_o5(l?.oa?.[0]?.oc?.().of?.om());
    assert_o5(l?.oc?.().om()?.oa?.[0].of);
    assert_o5(l?.oc?.().om()?.of!.oa?.[0].oc!().of);

    assert_u5(nl?.om()?.of?.oa?.[0].oc?.());
    assert_u5(nl?.of?.om()?.oc?.().oa?.[0]);
    assert_u5(nl?.oa?.[0]?.oc?.().of?.om());
    assert_u5(nl?.oc?.().om()?.oa?.[0].of);
    assert_u5(nl?.oc!().om()?.of!.oa![0].oc!().of);

    nl?.om()?.of?.oa?.[0].oc?.().noeval();
    nl?.of?.om()?.oc?.().oa?.[0].noeval();
    nl?.oa?.[0]?.oc?.().of?.om()?.noeval();
    nl?.oc?.().om()?.oa?.[0].of?.noeval();
    nl?.oc?.().om()?.of!.oa?.[0].oc!().of?.noeval();
}

function test45(l: Link5 | null, nl: Link5 | null) {
    assert_npe5(() => { nl?.of! });
    nl?.of!.f;
}

function test55(l: Link5 | null, nl: Link5 | null) {
    l?.f.a[0]?.f.c();
    nl?.f.a[0]?.f.c().noeval();
    assert_npe5(() => { nl?.f.a[0]?.f.c()! });
    assert_npe5(() => { (nl?.f?.a)?.[0].f! });
    assert_u5(l?.f.a[0].nf?.a[0].noeval()?.m());

    let u: Link5 | undefined = l?.f.oc?.().na?.[0].noeval().f?.oa?.[0];
}

// ---------------------------------------------------------------------------------------------------------------------
// UnionAsAndInstanceof.ets
// ---------------------------------------------------------------------------------------------------------------------

function assert_ccexc6(f: () => void) {
    try {
        f();
    } catch (e) {
        arktest.assertTrue(e instanceof ClassCastError)
        return;
    }
    arktest.assertTrue(false, "exception expected")
}

function assert_nothrow6(f: () => void) {
    try {
        f();
    } catch (e) {
        arktest.assertTrue(false, "unexpected exception")
    }
}

class A6 { }
class B6 { }
class C6 { }

function foo6(x: Object | null | undefined) { return x as Object }

function test_nullsafety6() {
    // Handling of Object may be a bit different, so test it separately
    // let f = ... until inference in form ((p)=>expr)(a) is broken
    assert_ccexc6(() => { let f = ((x: Object | null | undefined) => x as Object); f(null); });
    assert_ccexc6(() => { let f = ((x: Object | null | undefined) => x as Object); f(undefined); });
    assert_ccexc6(() => { let f = ((x: Object | null) => x as Object); f(null); });
    assert_ccexc6(() => { let f = ((x: Object | undefined) => x as Object); f(undefined); });

    assert_ccexc6(() => { let f = ((x: Object | null | undefined) => x as Object | undefined); f(null); });
    assert_ccexc6(() => { let f = ((x: Object | null | undefined) => x as Object | null); f(undefined); });
    assert_ccexc6(() => { let f = ((x: Object | null) => x as Object | undefined); f(null); });
    assert_ccexc6(() => { let f = ((x: Object | undefined) => x as Object | null); f(undefined); });

    assert_ccexc6(() => { let f = ((x: A6 | null | undefined) => x as A6); f(null); });
    assert_ccexc6(() => { let f = ((x: A6 | null | undefined) => x as A6); f(undefined); });
    assert_ccexc6(() => { let f = ((x: A6 | null) => x as A6); f(null); });
    assert_ccexc6(() => { let f = ((x: A6 | undefined) => x as A6); f(undefined); });

    assert_ccexc6(() => { let f = ((x: A6 | null | undefined) => x as A6 | undefined); f(null); });
    assert_ccexc6(() => { let f = ((x: A6 | null | undefined) => x as A6 | null); f(undefined); });
    assert_ccexc6(() => { let f = ((x: A6 | null) => x as A6 | undefined); f(null); });
    assert_ccexc6(() => { let f = ((x: A6 | undefined) => x as A6 | null); f(undefined); });


    assert_nothrow6(() => { let f = ((x: Object | null | undefined) => x as Object); f(new Object()); });
    assert_nothrow6(() => { let f = ((x: Object | null | undefined) => x as Object); f(new Object); });
    assert_nothrow6(() => { let f = ((x: Object | null) => x as Object); f(new Object()); });
    assert_nothrow6(() => { let f = ((x: Object | undefined) => x as Object); f(new Object()); });

    assert_nothrow6(() => { let f = ((x: Object | null | undefined) => x as Object | undefined); f(new Object()); });
    assert_nothrow6(() => { let f = ((x: Object | null | undefined) => x as Object | null); f(new Object()); });
    assert_nothrow6(() => { let f = ((x: Object | null) => x as Object | undefined); f(new Object()); });
    assert_nothrow6(() => { let f = ((x: Object | undefined) => x as Object | null); f(new Object()); });

    assert_nothrow6(() => { let f = ((x: A6 | null | undefined) => x as A6); f(new A6()); });
    assert_nothrow6(() => { let f = ((x: A6 | null | undefined) => x as A6); f(new A6()); });
    assert_nothrow6(() => { let f = ((x: A6 | null) => x as A6); f(new A6()); });
    assert_nothrow6(() => { let f = ((x: A6 | undefined) => x as A6); f(new A6()); });

    assert_nothrow6(() => { let f = ((x: A6 | null | undefined) => x as A6 | undefined); f(new A6()); });
    assert_nothrow6(() => { let f = ((x: A6 | null | undefined) => x as A6 | null); f(new A6()); });
    assert_nothrow6(() => { let f = ((x: A6 | null) => x as A6 | undefined); f(new A6()); });
    assert_nothrow6(() => { let f = ((x: A6 | undefined) => x as A6 | null); f(new A6()); });
}

function test_unions6() {
    assert_ccexc6(() => { let f = ((x: A6 | B6 | C6) => x as A6); f(new C6()); });
    assert_ccexc6(() => { let f = ((x: A6 | B6 | C6) => x as A6 | B6); f(new C6()); });
    assert_ccexc6(() => { let f = ((x: A6 | B6 | C6 | null) => x as A6 | B6); f(null); });
    assert_ccexc6(() => { let f = ((x: A6 | B6 | C6 | undefined) => x as A6 | B6); f(undefined); });

    assert_ccexc6(() => { let f = ((x: A6 | null | undefined) => x as A6 | undefined); f(null); });
    assert_ccexc6(() => { let f = ((x: A6 | null | undefined) => x as A6 | null); f(undefined); });
    assert_ccexc6(() => { let f = ((x: A6 | null) => x as A6 | undefined); f(null); });
    assert_ccexc6(() => { let f = ((x: A6 | undefined) => x as A6 | null); f(undefined); });

    assert_ccexc6(() => { let f = ((x: A6 | B6 | C6) => x as A6); f(new C6()); });
    assert_ccexc6(() => { let f = ((x: A6 | B6 | C6) => x as A6 | B6); f(new C6()); });
    assert_ccexc6(() => { let f = ((x: A6 | B6 | C6 | null) => x as A6 | B6); f(null); });
    assert_ccexc6(() => { let f = ((x: A6 | B6 | C6 | undefined) => x as A6 | B6); f(undefined); });

    assert_ccexc6(() => { let f = ((x: A6 | null | undefined) => x as A6 | undefined); f(null); });
    assert_ccexc6(() => { let f = ((x: A6 | null | undefined) => x as A6 | null); f(undefined); });
    assert_ccexc6(() => { let f = ((x: A6 | null) => x as A6 | undefined); f(null); });
    assert_ccexc6(() => { let f = ((x: A6 | undefined) => x as A6 | null); f(undefined); });
}

// ---------------------------------------------------------------------------------------------------------------------
// UncheckedCasts.ets
// ---------------------------------------------------------------------------------------------------------------------

function assert_ccexc7(f: () => void) {
    try {
        f();
    } catch (e) {
        arktest.assertTrue(e instanceof ClassCastError)
        return;
    }
    arktest.assertTrue(false, "exception expected")
}

class A7 { }
class B7 { }
class C7 { }
class X7<T7> { }

function erase7<T7>(x: Object | null | undefined): T7 { return x as T7; }

function test_substitution7() {
    assert_ccexc7(() => { erase7<Object>(null); })
    assert_ccexc7(() => { erase7<Object>(undefined); })
    assert_ccexc7(() => { erase7<A7>(null); })
    assert_ccexc7(() => { erase7<A7>(undefined); })

    assert_ccexc7(() => { erase7<Object | undefined>(null); })
    assert_ccexc7(() => { erase7<Object | null>(undefined); })
    assert_ccexc7(() => { erase7<A7 | undefined>(null); })
    assert_ccexc7(() => { erase7<A7 | null>(undefined); })

    assert_ccexc7(() => { erase7<A7>(undefined); })
    assert_ccexc7(() => { erase7<A7>(new Object()); })
    assert_ccexc7(() => { erase7<A7>(new B7()); })

    assert_ccexc7(() => { erase7<A7 | B7>(undefined); })
    assert_ccexc7(() => { erase7<A7 | B7>(new Object()); })
    assert_ccexc7(() => { erase7<A7 | B7>(new C7()); })

    assert_ccexc7(() => { erase7<FixedArray<A7>>(new B7[0]); })
}

class Erased7<T7> {
    constructor(x: Object | null | undefined) { this.t7 = x as T7; }
    t7: T7;
}

function test_substitution_memberexpr7() {
    assert_ccexc7(() => { new Erased7<Object>(null).t7; })
    assert_ccexc7(() => { new Erased7<Object>(undefined).t7; })
    assert_ccexc7(() => { new Erased7<A7>(null).t7; })
    assert_ccexc7(() => { new Erased7<A7>(undefined).t7; })

    assert_ccexc7(() => { new Erased7<Object | undefined>(null).t7; })
    assert_ccexc7(() => { new Erased7<Object | null>(undefined).t7; })
    assert_ccexc7(() => { new Erased7<A7 | undefined>(null).t7; })
    assert_ccexc7(() => { new Erased7<A7 | null>(undefined).t7; })

    assert_ccexc7(() => { new Erased7<A7>(undefined).t7; })
    assert_ccexc7(() => { new Erased7<A7>(new Object()).t7; })
    assert_ccexc7(() => { new Erased7<A7>(new B7()).t7; })

    assert_ccexc7(() => { new Erased7<A7 | B7>(undefined).t7; })
    assert_ccexc7(() => { new Erased7<A7 | B7>(new Object()).t7; })
    assert_ccexc7(() => { new Erased7<A7 | B7>(new C7()).t7; })

    assert_ccexc7(() => { new Erased7<FixedArray<A7>>(new B7[0]).t7; })
}

function cast_to_tparam7<T7 extends A7 | B7 | null>(x: Object | null | undefined) { x as T7; }

function test_constraint7() {
    assert_ccexc7(() => { cast_to_tparam7<A7>(undefined); })
    assert_ccexc7(() => { cast_to_tparam7<A7>(new Object()); })
    assert_ccexc7(() => { cast_to_tparam7<A7>(new C7()); })
}

function to_basetype7<T7>(x: Object | null | undefined) { return x as X7<T7>; }

function test_basetype7() {
    assert_ccexc7(() => { to_basetype7<A7>(null); })
    assert_ccexc7(() => { to_basetype7<A7>(undefined); })
    assert_ccexc7(() => { to_basetype7<A7>(new Object()); })
    assert_ccexc7(() => { to_basetype7<A7>(new C7()); })
}

// ---------------------------------------------------------------------------------------------------------------------
// partialTypeRuntime_2.ets
// ---------------------------------------------------------------------------------------------------------------------

class Class18 { fld: Number = 2; }

function test_18(): void {
    let class1_partial: Partial<Class18> = {fld: 3};
    arktest.assertEQ(class1_partial.fld, 3)
    class1_partial.fld = undefined;
    arktest.assertTrue(class1_partial.fld === undefined)
}

// -----------------------------------------------

class Class28 {
    fld: Number = 2;
    foo(a0: Partial<Class28>) {
        a0.fld = undefined;
    }
}

function test_28(): void {
    let class2_original: Class28 = new Class28();
    let class2_partial: Partial<Class28> = {fld: 3};
    arktest.assertEQ(class2_partial.fld, 3)
    class2_original.foo(class2_partial);
    arktest.assertTrue(class2_partial.fld === undefined)
}

// -----------------------------------------------

class Class38 {
    mmeb: Number = 2;
    foo(a0: Partial<Class58>){
        a0.mmeb = undefined;
    }
}

class Class48 extends Class38 {}

class Class58 extends Class48 {
    mmeb: Number = 2;
    foo(a0: Partial<Class38>){
        a0.mmeb = undefined;
    }
}

function test_38(): void {
    let class3_original: Class38 = new Class38();
    let class5_original: Class38 = new Class58();
    let class3_partial: Partial<Class38> = {mmeb: 8};
    let class5_partial: Partial<Class58> = {mmeb: 10};
    class3_original.foo(class5_partial);
    class5_original.foo(class3_partial);
    arktest.assertTrue(class3_partial.mmeb === undefined)
    arktest.assertTrue(class5_partial.mmeb === undefined)
}

// -----------------------------------------------

class Class68 {
    mmeb: Number = 2;
    foo(a0: Partial<Class78>){
        if(this.mmeb == 3){
            return;
        }

        this.bar(a0);
    }
    bar(a0: Partial<Class78>){
        this.mmeb = 3;
        this.foo(a0);
    }
}

class Class78 { fld: Number = 6;}

class Class88 {
    baz(a0: Partial<Class78>){
        a0.fld = undefined;
    }
}

function test_48(): void {
    let class7_partial: Partial<Class78> = {fld: 8};
    let class6_original: Class68 = new Class68();
    class6_original.foo(class7_partial);
}

// -----------------------------------------------

class Class98 { fld: Number = 8; }

class Class108 extends Class98 { memb: Number = 9; }

function test_58(): void {
    let class10_partial: Partial<Class108> = {memb: 7, fld: 5};

    arktest.assertEQ(class10_partial.memb, 7)
    arktest.assertEQ(class10_partial.fld, 5)

    class10_partial.memb = undefined;
    class10_partial.fld = undefined;

    arktest.assertTrue(class10_partial.memb === undefined)
    arktest.assertTrue(class10_partial.fld === undefined)
}

//---------------------------------------


// ---------------------------------------------------------------------------------------------------------------------
// SmartCast_01.ets
// ---------------------------------------------------------------------------------------------------------------------

class C33  {
  bar(): string {
    return "";
  }
}

function foo33(c: Object|null|undefined): string {
  if (c instanceof string)  {
    arktest.assertEQ(c.length, 11)
    c = "Case 1";
  } else if (c instanceof C33) {
    arktest.assertEQ(c.bar(), "Class C33")
    c = "Case 2";
  } else if (c instanceof Int) {
    arktest.assertEQ(c * 7, 49)
    c = "Case 3";
  } else if (c instanceof null) {
    arktest.assertEQ(c, null)
    c = "Case 4";
  } else {
    c = "Case 5";
  }

  arktest.assertEQ(c.length, 6)
  return c;
}

// ---------------------------------------------------------------------------------------------------------------------
// SmartCast_02.ets
// ---------------------------------------------------------------------------------------------------------------------

class A31 {
  bar(): string {
    return "Class A31";
  }
}

class B31 extends A31 {}

class C31 extends B31 {
  bar(): string {
    return "Class C31";
  }
}

function foo31(c: Int|String|A31|null|undefined): void {
  if (c instanceof String)  {
    arktest.assertEQ(c.length, 11)
  } else if (c instanceof C31) {
    arktest.assertEQ(c.bar(), "Class C31")
  } else if (c instanceof Int) {
    arktest.assertEQ(c * c, 49)
  } else if (c === undefined) {
     arktest.assertEQ(c, undefined)
  } else {
     arktest.assertEQ(c, null)
  }
}

// ---------------------------------------------------------------------------------------------------------------------
// SmartCast_03.ets
// ---------------------------------------------------------------------------------------------------------------------

class C32  {
  constructor() {}

  constructor(a: int) {
    this.x = a;
  }

  bar(): string {
    return "Class C32";
  }

  baz(): int {
    return this.x;
  }

  private x: int = 7;
}

function foo32(c: Object|null|undefined): string {
  if (c instanceof string && (c.length == 11 || c == "Test"))  {
    c = "Case 1";
  } else if (c instanceof C32 && c.baz() == 7) {
    arktest.assertEQ(c.bar(), "Class C32")
    c = "Case 2";
  } else if (c instanceof Int && c >= 0) {
    arktest.assertTrue(c >= 0)
    c = "Case 3";
  } else if (c instanceof null) {
    arktest.assertEQ(c, null)
    c = "Case 4";
  } else {
    c = "Case 5";
  }

  arktest.assertEQ(c.length, 6)
  return c;
}

// ---------------------------------------------------------------------------------------------------------------------
// SmartCast_04.ets
// ---------------------------------------------------------------------------------------------------------------------

function fooAnd34(x: String|null, y: String|null): string {
  if (x != null && y != null)  {
    return x + " " + y;
  } else if (x == null && y == null) {
    return "null";
  } else if (x != null && y == null) {
    return x;
  } else if (x == null && y != null) {
     return y;
  } else {
     throw new Error("Unreachable");
  }
}

function fooOr134(x: String|null, y: String|null): string {
  if (x != null || y != null)  {
    return "case 1";
  } else if (x == null && y == null) {
    return "null";
  } else {
     throw new Error("Unreachable");
  }
}

function fooOr234(x: String|null, y: String|null): string {
  if (x == null || y == null)  {
    return "case 1";
  } else if (x != null && y != null) {
    return x + " " + y;
  } else {
     throw new Error("Unreachable");
  }
}

// ---------------------------------------------------------------------------------------------------------------------
// SmartCast_05.ets
// ---------------------------------------------------------------------------------------------------------------------

class C35  {
  readonly a: boolean
  constructor(a_: boolean = false) {
    this.a = a_;
  }
}

function foo135(x: C35|null|undefined): string {
  if (x == null || !x.a)  {
    return x != null ? "false1" : "null";
  } else  {
    return x.a ? "true2" : "false2";
  }
}

function foo235(x: C35|null|undefined): string {
  if (x != null && x.a)  {
    return "true";
  } else  {
    return  x != null ? "false" : "null";
  }
}

function bar35(x: C35|null|undefined, y: boolean, z: boolean): string {
  if ((x instanceof C35 && y) || (x instanceof C35 && z))  {
    return (x.a ? "true1" : "false1") + y + z;
  } else  {
    return (x != null ? (x.a ? "true2" : "false2") : "null") + y + z;
  }
}

// ---------------------------------------------------------------------------------------------------------------------
// SmartCast_06.ets
// ---------------------------------------------------------------------------------------------------------------------

function foo36(x: int): string | undefined
{
  let rc: string|undefined = "default";

  label1: switch(x) {
    case 0:
       rc = "case 0";
       break;
    case 1: {
       let rc1: string|undefined = ():string => {return "case 1";}();
       label2: switch(rc) {
          case "default":
             rc = undefined;
             break label2;
          default:
             break label1;
       }
       rc = rc1;
       break;
    }
    case 2:
       rc = undefined;
       let rc2: string|null = ():string => {return "case 2";}();
       return rc2;
    case 3:
       rc = "case 3";
       break;
    case 4:
       rc = undefined;
       break;
    case 5:
       rc = "case 5";
       break;
    default:
       return rc != null ? rc :"case 4"
  }

  return rc;
}

// ---------------------------------------------------------------------------------------------------------------------
// SmartCast_08.ets
// ---------------------------------------------------------------------------------------------------------------------

function foo37(flag37: boolean): int {

  let x: int|undefined = 0;
  let y: int|undefined = 1;
  let z: int|undefined = 2;
  let w: int|undefined = 3;

  try {
    y = 1;
    if (flag37) {
      throw new Error();
    }
  } catch(ex) {
    if (ex instanceof NullPointerError) {
      z = 2;
    } else {
      w = undefined
    }
  }

  return x + y! + z + w!;
}

// ---------------------------------------------------------------------------------------------------------------------
// SmartCast_09.ets
// ---------------------------------------------------------------------------------------------------------------------

class A38 {
  prev_: A38|undefined = undefined;
  next_: A38|undefined = undefined;

  m() {
    const prev = this.prev_;
    const next = this.next_;

    if (prev) {
      this.prev_ = undefined;
      prev.next_ = next;
    }

    if (next) {
      this.next_ = undefined;
      next.prev_ = prev;
    }

  }
}

// ---------------------------------------------------------------------------------------------------------------------
// SmartCast_10.ets
// ---------------------------------------------------------------------------------------------------------------------

interface It39<T39> {
}

class Cl39<T39> implements It39<T39> {

  constructor(p: T39) {
    this.x = p;
  }

  static resolve<U>(value: U|It39<U>): Cl39<U> {
    if (value instanceof Cl39) {
        return value as Cl39<U>;
    }
    return new Cl39<U>(value as U);
  }

  x: T39;

  print(): string {
    if (this.x == undefined) {
      return "value is " + this.x;
    }
    else if (this.x instanceof string) {
      return "string: '" + this.x + "'";
    } else {
      return "number = " + this.x;
    }
  }
}

// ---------------------------------------------------------------------------------------------------------------------
// SmartCast_11.ets
// ---------------------------------------------------------------------------------------------------------------------

class C41 {}

function foo41(arg?: boolean): C41 | undefined {
    if (arg == undefined) {
        return undefined;
    }
    return arg == false ? undefined : new C41();
}

function bar41(arg?: boolean): C41 | undefined {
    if (arg == true) return new C41();
    return arg == false ? undefined : new C41();
}

function baz41(arg?: boolean): int {
    if (arg == undefined) return 0;
    arktest.assertEQ(typeof arg, "boolean")
    return 1;
}

// ---------------------------------------------------------------------------------------------------------------------
// SmartCast_13.ets
// ---------------------------------------------------------------------------------------------------------------------

class C43 {}

function foo43(arg?: number): C43 | undefined {
  if (arg == 5) return new C43();
  return arg == 3 ? undefined : new C43();
}

// ---------------------------------------------------------------------------------------------------------------------
// SmartCast_14.ets
// ---------------------------------------------------------------------------------------------------------------------

class A44<T44> {
    A44(a44?: T44 | number[]) {
        if (a44 instanceof Error) {
            return this.D(a44);
        } else if (a44 instanceof undefined) {
            return this.B(a44);
        } else if (a44 instanceof number[]) {
            return this.E(a44);
        } else {
            return this.C(a44);
        }
    }

    B(b: undefined) {
        return "undefined";
    }

    C(c: T44) {
        return "Generic";
    }

    D(d: Error) {
        return "Error";
    }

    E(e: number[]) {
        return "number[]"
    }
}

let a44 = new A44<Error | number>();
arktest.assertEQ(a44.A44(Error()), "Error");
arktest.assertEQ(a44.A44(undefined), "undefined");
arktest.assertEQ(a44.A44(1), "Generic");
arktest.assertEQ(a44.A44([1, 2, 3]), "number[]")

// ---------------------------------------------------------------------------------------------------------------------
// SmartCast_15.ets
// ---------------------------------------------------------------------------------------------------------------------

// Issue #22779 - smart cast in return statement
function isEmpty45(text?: string): boolean {
    return text === undefined || text === null || text.length === 0
}

// Some other examples
function foo45(text?: string): boolean {
    let a: boolean = (text === undefined || text === null || text.length == 0) ? true : false
    let b: boolean = text === undefined || text === null || text.length == 0
    let c: boolean = !(text !== undefined) || text === null || text.length == 0
    let d: boolean = (text !== undefined && text !== null) ? text.length == 0 : true
    let e: boolean = (text != undefined && text != null) ? text.length == 0 : true
    return a && b && c && d && e
}

// ---------------------------------------------------------------------------------------------------------------------
// SmartCast_16.ets
// ---------------------------------------------------------------------------------------------------------------------

class C45<out T45> {
  readonly fld: T45
  constructor(p: T45) {
      this.fld = p
  }
}

type NestedC45 = String | C45<NestedC45> | Error


function main(): void {
    // -----------------------------------------------------------------------------------------------------------------
    // AccessBinaryTrees.ets
    // -----------------------------------------------------------------------------------------------------------------

    let a = new AccessBinaryTrees;
    a.run();

    // -----------------------------------------------------------------------------------------------------------------
    // ArrayLiteral.ets
    // -----------------------------------------------------------------------------------------------------------------

    let a1: byte = 2;
    let b1: short = 20000;
    let c1: int = 2000000;
    let d1: long = 200000000000;
    let e1: float = 2.2f;
    let f1: double = 2.2222222222;
    let g1: double[] = [a1, b1, c1, d1, e1, f1];
    arktest.assertEQ(g1[0], 2)
    arktest.assertEQ(g1[1], 20000)
    arktest.assertEQ(g1[2], 2000000)
    arktest.assertEQ(g1[3], 200000000000)
    arktest.assertEQ(g1[4], (Double.toFloat(2.2)))
    arktest.assertEQ(g1[5], 2.2222222222)

    const h1: byte = 2;
    const i1: short = 2;
    const j1: int = 2;
    const k1: long = 2;
    const l1: float = 2.0f;
    const m1: double = 2.0;
    const n1: byte[] = [h1, Short.toByte(i1), Int.toByte(j1), Long.toByte(k1), Float.toByte(l1), Double.toByte(m1)];
    arktest.assertEQ(n1[0], 2)
    arktest.assertEQ(n1[1], 2)
    arktest.assertEQ(n1[2], 2)
    arktest.assertEQ(n1[3], 2)
    arktest.assertEQ(n1[4], 2)
    arktest.assertEQ(n1[5], 2)

    let o1: Object[] = [1, 1.1, "testStr", new Int(2), d1, k1];
    arktest.assertEQ(o1[0] as Int, 1)
    arktest.assertEQ(o1[1] as Double, 1.1)
    arktest.assertTrue((o1[2] as String).equals("testStr"))
    arktest.assertEQ(o1[3] as Int, 2)
    arktest.assertEQ(o1[4] as Long, 200000000000)
    arktest.assertEQ(o1[5] as Long, 2)

    let p1: long[] = [new Int(3), new Short(2 as short), new Long(4)];
    arktest.assertEQ(p1[0], 3)
    arktest.assertEQ(p1[1], 2)
    arktest.assertEQ(p1[2], 4)

    // -----------------------------------------------------------------------------------------------------------------
    // RestTuple6.ets
    // -----------------------------------------------------------------------------------------------------------------

    let a12: [A2<Long>, B2<Error>] = [new A2<Long>, new B2<Error>]
    arktest.assertTrue((new C2()).foo<Long, Error>(...a12) == false)
    arktest.assertTrue((new C2()).moo<Long, Error>(12, ...a12) == false)
    arktest.assertTrue((new C2()).foo<Long, Error>(...[new A2<Long>, new B2<Error>]) == false)
    arktest.assertTrue((new C2()).moo<Long, Error>(12, ...[new A2<Long>, new B2<Error>]) == false)
    arktest.assertTrue((new C2()).foo<Long, Error>(...[new A2<Long>, new B2<Error>] as [A2<Long>, B2<Error>]) == false)
    arktest.assertTrue((new C2()).moo<Long, Error>(12, ...[new A2<Long>, new B2<Error>] as [A2<Long>, B2<Error>]) == false)
    arktest.assertTrue((new C2()).foo<Long, Error>(new A2<Long>, new B2<Error>) == false)
    arktest.assertTrue((new C2()).moo<Long, Error>(12, new A2<Long>, new B2<Error>) == false)

    // -----------------------------------------------------------------------------------------------------------------
    // GenericBridges_02.ets
    // -----------------------------------------------------------------------------------------------------------------

    ttt3(new D3<string>())
    let c3: C3<Int, string> = new E3<Int>();
    foo13(c3);
    foo23(new E3<Long>());

    // -----------------------------------------------------------------------------------------------------------------
    // OptionalChains.ets
    // -----------------------------------------------------------------------------------------------------------------

    test15(new Link5(), null)
    test25(new Link5(), null)
    test35(new Link5(), null)
    test45(new Link5(), null)
    test55(new Link5(), null)

    // -----------------------------------------------------------------------------------------------------------------
    // UnionAsAndInstanceof.ets
    // -----------------------------------------------------------------------------------------------------------------

    test_nullsafety6();
    test_unions6();

    // -----------------------------------------------------------------------------------------------------------------
    // UncheckedCasts.ets
    // -----------------------------------------------------------------------------------------------------------------

    test_substitution7();
    test_substitution_memberexpr7();
    test_constraint7();
    test_basetype7();

    // -----------------------------------------------------------------------------------------------------------------
    // partialTypeRuntime_2.ets
    // -----------------------------------------------------------------------------------------------------------------

    test_18();
    test_28();
    test_38();
    test_48();
    test_58();

    // -----------------------------------------------------------------------------------------------------------------
    // SmartCast_01.ets
    // -----------------------------------------------------------------------------------------------------------------

    arktest.assertEQ(foo33("Test string"), "Case 1")
    arktest.assertEQ(foo33(new Int(7)), "Case 3")
    arktest.assertEQ(foo33(new C33()), "Case 2")
    arktest.assertEQ(foo33(null), "Case 4")
    arktest.assertEQ(foo33(undefined), "Case 5")
    arktest.assertEQ(foo33(new Number(3.0)), "Case 5")

    // -----------------------------------------------------------------------------------------------------------------
    // SmartCast_02.ets
    // -----------------------------------------------------------------------------------------------------------------

    foo31("Test string");
    foo31(new Int(7));
    foo31(new C31());
    foo31(null);
    foo31(undefined);

    // -----------------------------------------------------------------------------------------------------------------
    // SmartCast_03.ets
    // -----------------------------------------------------------------------------------------------------------------

    arktest.assertEQ(foo32("Test string"), "Case 1")
    arktest.assertEQ(foo32("Test"), "Case 1")
    arktest.assertEQ(foo32("Test string 2"), "Case 5")
    arktest.assertEQ(foo32("test"), "Case 5")

    arktest.assertEQ(foo32(new Int(5)), "Case 3")
    arktest.assertEQ(foo32(new Int(0)), "Case 3")
    arktest.assertEQ(foo32(new Int(-5)), "Case 5")

    arktest.assertEQ(foo32(new C32(7)), "Case 2")
    arktest.assertEQ(foo32(new C32()), "Case 2")
    arktest.assertEQ(foo32(new C32(17)), "Case 5")

    arktest.assertEQ(foo32(null), "Case 4")

    arktest.assertEQ(foo32(undefined), "Case 5")
    arktest.assertEQ(foo32(new Number(3.0)), "Case 5")

    // -----------------------------------------------------------------------------------------------------------------
    // SmartCast_04.ets
    // -----------------------------------------------------------------------------------------------------------------

    arktest.assertEQ(fooAnd34("Test", "string"), "Test string")
    arktest.assertEQ(fooAnd34("Test", null), "Test")
    arktest.assertEQ(fooAnd34(null, "string"), "string")
    arktest.assertEQ(fooAnd34(null, null), "null")

    arktest.assertEQ(fooOr134("Test", "string"), "case 1")
    arktest.assertEQ(fooOr134("Test", null), "case 1")
    arktest.assertEQ(fooOr134(null, "string"), "case 1")
    arktest.assertEQ(fooOr134(null, null), "null")

    arktest.assertEQ(fooOr234("Test", "string"), "Test string")
    arktest.assertEQ(fooOr234("Test", null), "case 1")
    arktest.assertEQ(fooOr234(null, "string"), "case 1")
    arktest.assertEQ(fooOr234(null, null), "case 1")

    // -----------------------------------------------------------------------------------------------------------------
    // SmartCast_05.ets
    // -----------------------------------------------------------------------------------------------------------------

    arktest.assertEQ(foo135(null), "null")
    arktest.assertEQ(foo235(null), "null")
    arktest.assertEQ(bar35(null, true, true), "nulltruetrue")
    arktest.assertEQ(bar35(null, true, false), "nulltruefalse")
    arktest.assertEQ(bar35(null, false, true), "nullfalsetrue")
    arktest.assertEQ(bar35(null, false, false), "nullfalsefalse")

    arktest.assertEQ(foo135(undefined), "null")
    arktest.assertEQ(foo235(undefined), "null")
    arktest.assertEQ(bar35(undefined, true, true), "nulltruetrue")
    arktest.assertEQ(bar35(undefined, true, false), "nulltruefalse")
    arktest.assertEQ(bar35(undefined, false, true), "nullfalsetrue")
    arktest.assertEQ(bar35(undefined, false, false), "nullfalsefalse")

    let c = new C35();
    arktest.assertEQ(foo135(c), "false1")
    arktest.assertEQ(foo235(c), "false")
    arktest.assertEQ(bar35(c, true, true), "false1truetrue")
    arktest.assertEQ(bar35(c, true, false), "false1truefalse")
    arktest.assertEQ(bar35(c, false, true), "false1falsetrue")
    arktest.assertEQ(bar35(c, false, false), "false2falsefalse")

    c = new C35(true);
    arktest.assertEQ(foo135(c), "true2")
    arktest.assertEQ(foo235(c), "true")
    arktest.assertEQ(bar35(c, true, true), "true1truetrue")
    arktest.assertEQ(bar35(c, true, false), "true1truefalse")
    arktest.assertEQ(bar35(c, false, true), "true1falsetrue")
    arktest.assertEQ(bar35(c, false, false), "true2falsefalse")

    // -----------------------------------------------------------------------------------------------------------------
    // SmartCast_06.ets
    // -----------------------------------------------------------------------------------------------------------------

    arktest.assertEQ(foo36(0), "case 0")
    arktest.assertEQ(foo36(1), "case 1")
    arktest.assertEQ(foo36(2), "case 2")
    arktest.assertEQ(foo36(3), "case 3")
    arktest.assertEQ(foo36(4), "case 4")
    arktest.assertEQ(foo36(5), "case 5")
    arktest.assertEQ(foo36(7), "default")

    // -----------------------------------------------------------------------------------------------------------------
    // SmartCast_07.ets
    // -----------------------------------------------------------------------------------------------------------------

    let x363: int | boolean | string  = 7;
    arktest.assertEQ(x363, 7)
    x363--;
    arktest.assertEQ(x363, 6)
    x363 = x363 == 7;
    arktest.assertTrue(!x363)
    x363 = !x363;
    arktest.assertEQ(x363, true)
    x363 = "x363 = " + x363.toString();
    arktest.assertEQ(x363, "x363 = true")

    // -----------------------------------------------------------------------------------------------------------------
    // SmartCast_08.ets
    // -----------------------------------------------------------------------------------------------------------------

    let flag37: boolean = false;
    arktest.assertEQ(foo37(flag37), 6)
    try {
        foo37(!flag37);
    } catch(ex) {
        flag37 = true;
    }
    arktest.assertTrue(flag37)

    // -----------------------------------------------------------------------------------------------------------------
    // SmartCast_10.ets
    // -----------------------------------------------------------------------------------------------------------------

    arktest.assertEQ(Cl39.resolve(undefined).print(), "value is undefined")
    arktest.assertEQ(Cl39.resolve("test").print(), "string: 'test'")
    arktest.assertEQ(Cl39.resolve(5.5).print(), "number = 5.5")
    arktest.assertEQ(Cl39.resolve(new Int(8)).print(), "number = 8")

    arktest.assertEQ(Cl39.resolve(new Cl39<null>(null)).print(), "value is null")
    arktest.assertEQ(Cl39.resolve(new Cl39<string>("TEST")).print(), "string: 'TEST'")
    arktest.assertEQ(Cl39.resolve(new Cl39<number>(7.7)).print(), "number = 7.7")
    arktest.assertEQ(Cl39.resolve(new Cl39<Int>(new Int(-8))).print(), "number = -8")

    // -----------------------------------------------------------------------------------------------------------------
    // SmartCast_11.ets
    // -----------------------------------------------------------------------------------------------------------------

    arktest.assertTrue(foo41(true) instanceof C41)
    arktest.assertEQ(foo41(false), undefined)
    arktest.assertEQ(foo41(), undefined)

    arktest.assertTrue(bar41(true) instanceof C41)
    arktest.assertEQ(bar41(false), undefined)
    arktest.assertTrue(bar41() instanceof C41)

    arktest.assertEQ(baz41(true), 1)
    arktest.assertEQ(baz41(false), 1)
    arktest.assertEQ(baz41(), 0)

    // -----------------------------------------------------------------------------------------------------------------
    // SmartCast_12.ets
    // -----------------------------------------------------------------------------------------------------------------

    let resolve42: ((value: string) => void) | null = null;

    let p42 = new Promise<string>((_resolve: (value: string)=> void): void => {
        resolve42 = _resolve;
    });

    resolve42!("abc");                         // no smart cast!

    let x42: Number|String|undefined = "test1";
    let y42: Number|String|undefined = "test2"
    let z42: Number|String|undefined = 7

    let lam42: () => void = () => {
        let y42: Number|String|undefined = 2;   // hides outer declaration!
        x42 = z42;
        let tmp: number = y42;                  // smart cast is used
        arktest.assertEQ(tmp, 2);
    };

    if (x42 instanceof string) {
        let tmp: string = x42 as string;         // no smart cast!
        arktest.assertEQ(tmp, "test1");
    }

    lam42();

    arktest.assertEQ(x42, 7);

    if (y42 instanceof string) {
        let tmp: string = y42;                   // smart cast is used
        arktest.assertEQ(tmp, "test2");
    }

    let w42: number = z42;                       // smart cast is used
    arktest.assertEQ(w42, 7)

    // -----------------------------------------------------------------------------------------------------------------
    // SmartCast_13.ets
    // -----------------------------------------------------------------------------------------------------------------

    arktest.assertTrue(foo43(5) instanceof C43)
    arktest.assertEQ(foo43(3), undefined)
    arktest.assertTrue(foo43(undefined) instanceof C43)

    // -----------------------------------------------------------------------------------------------------------------
    // SmartCast_15.ets
    // -----------------------------------------------------------------------------------------------------------------

    arktest.assertEQ(isEmpty45(), true)
    arktest.assertEQ(isEmpty45(""), true)
    arktest.assertEQ(isEmpty45("a"), false)
    arktest.assertEQ(foo45(), true)

    // -----------------------------------------------------------------------------------------------------------------
    // SmartCast_16.ets
    // -----------------------------------------------------------------------------------------------------------------

    let x45: NestedC45 = new C45<C45<Error>>(new C45<Error>(new Error()))
    let ok45 = x45 instanceof C45 && x45.fld instanceof C45 && x45.fld.fld instanceof Error
    let aaa = ok45 ? 0 : 1

}

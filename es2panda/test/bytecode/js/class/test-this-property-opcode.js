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

// Codegen golden: `this.name` by-name access on the current-frame `this` folds to
// ldthisbyname/stthisbyname (receiver = GetThis(sp), implicit). Derived constructors,
// optional chains, arrow-captured `this` and private members fall back to the original path.
// Every remaining `this` MATERIALIZATION (call receiver, argument, bare `this`,
// computed-key base) becomes the 1-byte ldthis instead of ldlexvar 0x0,0x0, but only
// when `this` is lexical-bound (an inner arrow captures it): a plain-parameter `this`
// stays on the copy-propagatable `lda <param>` path.

class Base {
    x = 0
}

class Derived extends Base {
    n = 0
    // derived-ctor body write: must FALL BACK (GetThis(sp) stale after super())
    constructor() { super(); this.n = 1 }
    // method read: ldthisbyname
    read() { return this.x }
    // method write: stthisbyname
    write(v) { this.x = v }
    // optional chain on this: FALL BACK (preserve nullish short-circuit)
    opt() { return this?.x }
    // plain-parameter this (no capture): receiver stays `lda a2` + ldobjbyname + callthis0
    call() { this.read() }
    // plain-parameter this: bare `this` stays `lda a2`
    self() { return this }
    // plain-parameter this: argument stays the a2 register
    pass(f) { return f(this) }
    // plain-parameter this: computed base stays the a2 register (by-name folding excluded)
    comp(k) { return this[k] }
    // arrow-captured this: FALL BACK to ldlexvar 0xN,0x0 (captured env, not frame this)
    arr() { const f = () => this.x; return f() }
    // ---- ldthis golden coverage: the inner arrow makes `this` lexical-bound, so the
    // level-0 materializations below load the frame this via the 1-byte ldthis ----
    // bare this with capture: ldthis
    mat() { const f = () => this; return this }
    // call receiver with capture: ldthis materialization for callthis0
    matcall() { const f = () => this; this.read() }
    // this as argument with capture: ldthis
    matarg(f) { const g = () => this; return f(this) }
    // computed-key base with capture: ldthis
    matcomp(k) { const f = () => this; return this[k] }
}

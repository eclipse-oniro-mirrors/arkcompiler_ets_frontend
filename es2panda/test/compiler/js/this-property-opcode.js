/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/modules/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Runtime equivalence for the ldthisbyname/stthisbyname folding.
// `this.name` by-name access on the current-frame `this` folds to ldthisbyname/stthisbyname;
// derived constructors (this rebound by super()), arrow-captured `this`, optional chains and
// private members fall back. All paths must produce identical results to the unoptimized code.

class Base {
    constructor() { this.x = 0 }
}
class Derived extends Base {
    constructor() { super(); this.n = 41 }   // derived-ctor body write: FALL BACK, must not crash
    read() { return this.x }                  // ldthisbyname
    write(v) { this.x = v }                   // stthisbyname
    bump() { this.x = this.x + 1 }            // ldthisbyname + stthisbyname
    opt() { return this?.x }                  // optional chain: FALL BACK
}

const d = new Derived();
print(d.n === 41);          // ctor fallback path executed correctly
print(d.x === 0);           // base ctor write
d.write(99);
print(d.read() === 99);     // method write + read
d.bump();
print(d.read() === 100);    // compound ld+st
print(d.opt() === 100);     // optional-chain fallback

// getter on this.x — receiver identity + call order must match (same IC path)
class WithGetter {
    constructor() { this._c = 0 }
    get x() { this._c++; return this._c }
    read() { return this.x }   // ldthisbyname triggers getter
}
const g = new WithGetter();
print(g.read() === 1);
print(g.read() === 2);

// arrow capturing outer this — FALL BACK, must still resolve correctly
class Holder {
    constructor() { this.v = 7 }
    outer() { const inner = () => this.v; return inner() }
}
print(new Holder().outer() === 7);

// Phase 2a runtime equivalence: `this` MATERIALIZATION via ldthis (frame thisObj) instead of
// ldlexvar 0x0,0x0 (lexical slot) — value-identical outside derived constructors.
// Covers the call receiver, this-as-argument, bare this, computed key, and the two resume-frame
// classes (generator across yield, async across await) where the this must survive re-entry.
class Materializer {
    constructor() { this.v = 5 }
    call() { return this.bump2() }              // ldthis receiver + callthis0
    bump2() { this.v += 2; return this.v }      // ldthisbyname + stthisbyname
    self() { return this }                      // bare ldthis
    pass(f) { return f(this) }                  // ldthis as argument
    comp(k) { return this[k] }                  // ldthis + ldobjbyvalue
}
const m = new Materializer();
print(m.call() === 7);
print(m.self() === m);
print(m.pass((o) => o.v) === 7);
print(m.comp('v') === 7);

class GenThis {
    constructor() { this.n = 40 }
    *gen() { yield this.n; yield this.n + 1 }   // this across yield (resume frame)
}
const gi = new GenThis().gen();
print(gi.next().value === 40);
print(gi.next().value === 41);

class AsyncThis {
    constructor() { this.n = 40 }
    async foo() { await Promise.resolve(1); return this.n + 2 }   // this across await
    async bar() { return this.foo().then((v) => v + this.n) }     // nested: ldthis after resume
}
new AsyncThis().bar().then((v) => print(v === 82));

// strict-mode plain function: this = undefined when called unqualified; optional chain on it
// must short-circuit (undefined) exactly as with the lexical-slot load.
function strictOpt() { return this?.missing }
strictOpt();
print(strictOpt.call(null) === undefined);

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

// A getter and setter that share the same key are merged into a single
// definegettersetterbyvalue at compile time; both must still work at runtime.

class C {
    constructor() { this._v = 0 }
    get foo() { return this._v }
    set foo(v) { this._v = v }
}
const c = new C();
print(c.foo);
c.foo = 42;
print(c.foo);
const dc = Object.getOwnPropertyDescriptor(C.prototype, "foo");
print(typeof dc.get, typeof dc.set);
print(dc.get.name, dc.set.name);

const o = {
    _w: 5,
    get bar() { return this._w },
    set bar(v) { this._w = v }
};
print(o.bar);
o.bar = 99;
print(o.bar);
const dobj = Object.getOwnPropertyDescriptor(o, "bar");
print(typeof dobj.get, typeof dobj.set);

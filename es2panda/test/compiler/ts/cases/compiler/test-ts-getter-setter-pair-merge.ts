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

// The getter/setter pair merge must also apply to TS input after type
// transformation: a typed accessor pair merges into one
// definegettersetterbyvalue, while a same-named method between the accessors
// keeps them unmerged (the setter keeps its source position).

class C {
    private v: number = 0;
    get value(): number { return this.v; }
    set value(v: number) { this.v = v; }
}
const c = new C();
print(c.value);
c.value = 42;
print(c.value);
const dc = Object.getOwnPropertyDescriptor(C.prototype, "value");
print(typeof dc.get, typeof dc.set);

class NoMerge {
    get x(): number { return 1; }
    x(): string { return "m"; }
    set x(v: number) { }
}
const dn = Object.getOwnPropertyDescriptor(NoMerge.prototype, "x");
print(typeof dn.get, typeof dn.set, typeof dn.value);

const o = {
    w: 5,
    get bar(): number { return this.w; },
    set bar(v: number) { this.w = v; },
};
o.bar = 7;
print(o.bar);
const dob = Object.getOwnPropertyDescriptor(o, "bar");
print(typeof dob.get, typeof dob.set);

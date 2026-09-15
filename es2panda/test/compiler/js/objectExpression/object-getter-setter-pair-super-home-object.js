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

// super in both accessor bodies is the only shape that observes a missing
// home-object reload before the second definemethod of a merged pair: the
// setter's super store would target the wrong object without it.

class Base {
    get v() { return "base-get" }
    set v(x) { this.bset = x }
}
class Derived extends Base {
    get v() { return "d:" + super.v }
    set v(x) { super.v = x; this.wrote = true }
}
const d = new Derived();
print(d.v);
d.v = 7;
print(d.bset, d.wrote);
const dd = Object.getOwnPropertyDescriptor(Derived.prototype, "v");
print(typeof dd.get, typeof dd.set);

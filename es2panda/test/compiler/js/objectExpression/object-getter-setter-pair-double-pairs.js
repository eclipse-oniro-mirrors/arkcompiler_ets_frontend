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

// Two same-named pairs in one literal: the literal-buffer pass retires the
// first pair and the scan must merge only the surviving second pair. A data
// property between two pairs retires the first pair the same way.

const o = {
    get a() { return 1 },
    set a(v) { },
    get a() { return 2 },
    set a(v) { this.av = v }
};
const d = Object.getOwnPropertyDescriptor(o, "a");
print(typeof d.get, typeof d.set, d.get());
o.a = 9;
print(o.av);

const p = {
    get b() { return 1 },
    set b(v) { },
    b: 7,
    get b() { return 3 },
    set b(v) { this.bv = v }
};
print(p.b);
p.b = 5;
print(p.bv);
const dp = Object.getOwnPropertyDescriptor(p, "b");
print(typeof dp.get, typeof dp.set);

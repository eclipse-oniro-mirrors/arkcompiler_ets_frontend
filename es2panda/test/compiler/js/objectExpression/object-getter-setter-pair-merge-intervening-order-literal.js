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

// Object literal: merging the pair across an intervening differently-named
// (non-constant, loop-compiled) property must keep the insertion order of
// keys - the pair key's slot is fixed by the getter's first install and the
// setter is only a redefine that does not move it.

function side() { return 3 }
const o = {
    get k() { return 1 },
    v: side(),
    set k(x) { this.kv = x }
};
print(Object.keys(o).join(","));
const d = Object.getOwnPropertyDescriptor(o, "k");
print(typeof d.get, typeof d.set, o.k);
o.k = 9;
print(o.kv, o.v);

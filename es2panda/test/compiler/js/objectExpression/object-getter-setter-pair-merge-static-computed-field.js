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

// A computed static field between the static accessors does not stop the
// scan (class fields install outside the member loop): the pair merges and
// the field store still lands after both installs, so the data property
// keeps winning exactly as with the single-instruction emission.

const k = "x";
class C {
    static get x() { return "g" }
    static [k] = 9;
    static set x(v) { }
}
print(typeof C.x, C.x);
const d = Object.getOwnPropertyDescriptor(C, "x");
print(typeof d.get, typeof d.set, typeof d.value);
C.x = 4;
print(C.x);

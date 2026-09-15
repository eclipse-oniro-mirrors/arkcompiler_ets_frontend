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

// Object literal: a same-named method between the getter and setter keeps
// the final descriptor an accessor with get undefined and a real setter
// (matches master and Node).

const o = {
    get a() { return 1 },
    a() { return "m" },
    set a(v) { }
};
const d = Object.getOwnPropertyDescriptor(o, "a");
print(typeof d.get, typeof d.set, typeof d.value);

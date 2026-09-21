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

// Object literal: a duplicate getter stops the scan for the first getter;
// the adjacent second getter merges with the setter, and the second getter's
// value wins.

const o = {
    get a() { return 1 },
    get a() { return 2 },
    set a(v) { }
};
const d = Object.getOwnPropertyDescriptor(o, "a");
print(typeof d.get, typeof d.set);
print(o.a);

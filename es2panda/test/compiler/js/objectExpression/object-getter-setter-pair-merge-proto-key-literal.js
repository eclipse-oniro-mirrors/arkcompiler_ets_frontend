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

// Object literal: __proto__ as an accessor key is a plain property name; the
// own accessor shadows the inherited prototype getter.

const o = {
    get __proto__() { return 42 },
    set __proto__(v) { }
};
const d = Object.getOwnPropertyDescriptor(o, "__proto__");
print(typeof d.get, typeof d.set);
print(("value" in d) ? "data" : "accessor");
print(o.__proto__);

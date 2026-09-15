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

// A same-named method between the getter and setter keeps the pair unmerged:
// the setter keeps its source position and re-installs the accessor after
// the method definition, so the final descriptor matches master and Node.

class NoMergeMethod {
    get x() { return 1 }
    x() { return "m" }
    set x(v) { }
};
const d = Object.getOwnPropertyDescriptor(NoMergeMethod.prototype, "x");
print(typeof d.get, typeof d.set, typeof d.value);

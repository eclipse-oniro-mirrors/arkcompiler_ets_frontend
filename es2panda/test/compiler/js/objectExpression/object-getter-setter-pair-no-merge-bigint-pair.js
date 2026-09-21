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

// A BigInt key has no static name: the pair falls back to the single-path
// forms and both accessors still answer under the string form of the key.

class BigIntPair {
    get 1n() { return 1 }
    set 1n(v) { }
};
const d = Object.getOwnPropertyDescriptor(BigIntPair.prototype, "1");
print(typeof d.get, typeof d.set);
print(BigIntPair.prototype[1]);

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

// Private accessors compile through the private literal buffer, so pair
// merging never applies; both accessors must still work at runtime.

class PrivPair {
    #v = 1;
    get #x() { return this.#v }
    set #x(v) { this.#v = v }
    read() { return this.#x }
    write(v) { this.#x = v }
};
const p = new PrivPair();
print(p.read());
p.write(7);
print(p.read());

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

// A private-identifier key between the pair stops the scan; the private
// getter keeps working through its own brand.

class PrivateBetween {
    get x() { return 1 }
    get #y() { return 2 }
    set x(v) { }
    #read() { return this.#y }
};
const d = Object.getOwnPropertyDescriptor(PrivateBetween.prototype, "x");
print(typeof d.get, typeof d.set);

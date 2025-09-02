/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

class A {
    fld: [number, string] = [42.0, 'a']
}
class B {
    fld: [number, string] = [33.0, 'b']
}
let u1: A|B = new A
let u2: A|B = new B

function main() {
    arktest.assertEQ(u1.fld[0], 42.0)
    arktest.assertEQ(u2.fld[1], 'b')
}

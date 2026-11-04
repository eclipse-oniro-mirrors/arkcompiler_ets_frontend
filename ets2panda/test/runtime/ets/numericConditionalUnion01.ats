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

function check(c: boolean): void {
    let a: byte = 10 as byte;
    let b: short = 20 as short;
    let inferred = c ? a : b;
    let explicit: byte | short = c ? a : b;
    if (c) {
        arktest.assertTrue(inferred instanceof Byte);
        arktest.assertTrue(explicit instanceof Byte);
        arktest.assertEQ(inferred as byte, a);
    } else {
        arktest.assertTrue(inferred instanceof Short);
        arktest.assertTrue(explicit instanceof Short);
        arktest.assertEQ(inferred as short, b);
    }
}
function main() { check(true); check(false); }

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
    let a: double = 2.5;
    let b: byte = 127 as byte;
    let inferred = c ? a : b;
    let explicit: double | byte = c ? a : b;
    if (c) {
        arktest.assertTrue(inferred instanceof Double);
        arktest.assertTrue(explicit instanceof Double);
        arktest.assertEQ(inferred as double, a);
    } else {
        arktest.assertTrue(inferred instanceof Byte);
        arktest.assertTrue(explicit instanceof Byte);
        arktest.assertEQ(inferred as byte, b);
    }
}
function main() { check(true); check(false); }

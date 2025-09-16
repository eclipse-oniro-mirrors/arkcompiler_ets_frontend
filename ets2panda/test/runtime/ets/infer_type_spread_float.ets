/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

function main() {
    let a: float[] = [2 as float, 3 as float]
    let r1 = [1 as float, ...a, 4 as float]
    arktest.assertTrue(r1.length == 4 && r1[0] == 1 && r1[1] == 2 && r1[2] == 3 && r1[3] == 4)

    let b: byte[] = [2 as byte, 3 as byte]
    let r2 = [1 as byte, ...b, 4 as byte]
    arktest.assertTrue(r2.length == 4 && r2[0] == 1 && r2[1] == 2 && r2[2] == 3 && r2[3] == 4)

    let c: short[] = [2 as short, 3 as short]
    let r3 = [1 as short, ...c, 4 as short]
    arktest.assertTrue(r3.length == 4 && r3[0] == 1 && r3[1] == 2 && r3[2] == 3 && r3[3] == 4)

    let d: long[] = [2 as long, 3 as long]
    let r4 = [1 as long, ...d, 4 as long]
    arktest.assertTrue(r4.length == 4 && r4[0] == 1 && r4[1] == 2 && r4[2] == 3 && r4[3] == 4)
}

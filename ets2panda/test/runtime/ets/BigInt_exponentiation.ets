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

function main() {
    let a = new BigInt(22);
    let b = new BigInt(11);
    let aa = -2n;
    let bb = 31n;
    let c = a ** b;
    c **= 2n;
    arktest.assertEQ(a ** b, new BigInt("584318301411328"));
    arktest.assertEQ(c, new BigInt("341427877364219557396646723584"));
    arktest.assertEQ(aa ** bb, new BigInt("-2147483648"));
}
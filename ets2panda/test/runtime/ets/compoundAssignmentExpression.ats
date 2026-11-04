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
    let s: FixedArray<string> = ["s1", "s2"];
    let res: FixedArray<int> = [0, 0, 0, 0, 0];
    res[0] -= Double.toInt(s[1 % 2].length);
    res[(res.length - 4) as Int] += 1;
    res[2 % (res.length as int)] &= Double.toInt(s[1].length);
    res[3 % (res.length as int)] |= Double.toInt(s[1 % 2].length);
    res[2 % (res.length as int) + s.length] *= Double.toInt(s[1 % 2].length);

    arktest.assertEQ(res[0], -2);
    arktest.assertEQ(res[1], 1);
    arktest.assertEQ(res[2], 0);
    arktest.assertEQ(res[3], 2);
    arktest.assertEQ(res[4], 0);
}

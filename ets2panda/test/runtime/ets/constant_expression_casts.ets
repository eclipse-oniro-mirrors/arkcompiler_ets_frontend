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

enum CastEnum: byte {
    One = 3 as byte,
    Two,
}

enum ReviewEnum: byte {
    A = 1 as int,
    B,
}

function main(): void {
    let tuple: [number, string, string, string, string, string] = [42.0, "abc", "def", "ghi", "jkl", "mno"];
    const index0 = 0;
    const index1 = 1;

    arktest.assertEQ(tuple[0 as int], 42.0);
    arktest.assertEQ(tuple[1 as short], "abc");
    arktest.assertEQ(tuple[index0 as int], 42.0);
    arktest.assertEQ(tuple[index1 as short], "abc");
    arktest.assertEQ(tuple[1 as int as long], "abc");
    arktest.assertEQ(tuple[5 as int as long], "mno");
    arktest.assertEQ(tuple[(0 as int) + 1], "abc");
    arktest.assertEQ(tuple[1 as int + 0 as int], "abc");
    arktest.assertEQ(tuple[1 as int + 1 as int], "def");
    arktest.assertEQ(CastEnum.Two.valueOf(), 4);
    arktest.assertEQ(ReviewEnum.B.valueOf(), 2);
}

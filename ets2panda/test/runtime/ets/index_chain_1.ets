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

function testArr(a: int[]): void {
    let x = a[1] = a[2] = 1;
    arktest.assertEQ(a[0], 6);
    arktest.assertEQ(a[1], 1);
    arktest.assertEQ(a[2], 1);
    arktest.assertEQ(a[3], 9);
}

let resSetter: string = ""
let resIdx: string = ""

class TestSetIdx {
    $_get(idx: number): number {
        arktest.assertTrue(false);
        return 0;
    }
    $_set(idx: number, val: number) {
        resSetter += idx;
        arktest.assertEQ(val, 201);
    }
}

function getIdx(a0: number): number {
    resIdx += a0;
    return a0;
}

function testNoGetCall() {
    let testCls = new TestSetIdx()
    testCls[getIdx(9)] = testCls[getIdx(8)] = testCls[getIdx(7)] = 201;
    resSetter += "_";
    resIdx += "_";
    let a: number = 201.0;
    let b = a = testCls[getIdx(13)] = testCls[getIdx(14)] = a = a = testCls[getIdx(15)] = a;
    arktest.assertEQ(b, 201.0);
}

function main(): int {
    resSetter = ""
    resIdx = ""

    testArr([6,7,8,9]);
    testNoGetCall();
    arktest.assertEQ(resSetter, "789_151413");
    arktest.assertEQ(resIdx, "987_131415");

    return 0;
}

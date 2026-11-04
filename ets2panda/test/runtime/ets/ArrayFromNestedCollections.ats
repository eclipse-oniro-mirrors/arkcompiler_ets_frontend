/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

function main() {
    const set1 = new Set<number>([1, 2]);
    const set2 = new Set<number>([3, 4]);
    const parentSet = new Set<Set<number>>([set1, set2]);

    const arr = Array.from(parentSet, (s) => Array.from(s));
    arktest.assertEQ(arr.length, 2);
    arktest.assertEQ(arr[0][0], 1);
    arktest.assertEQ(arr[0][1], 2);
    arktest.assertEQ(arr[1][0], 3);
    arktest.assertEQ(arr[1][1], 4);

    const arr1 = new Array<number>(1, 2);
    const arr2 = new Array<number>(3, 4);
    const parentArr = new Array<Array<number>>(arr1, arr2);

    const ret = Array.from(parentArr, (a) => Array.from(a));
    arktest.assertEQ(ret.length, 2);
    arktest.assertEQ(ret[0][0], 1);
    arktest.assertEQ(ret[0][1], 2);
    arktest.assertEQ(ret[1][0], 3);
    arktest.assertEQ(ret[1][1], 4);
}


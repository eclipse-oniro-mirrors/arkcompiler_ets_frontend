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

function fn<T>(expected: T[], ...args: T[]) {
    for (let i = 0; i < args.length; i++) {
        arktest.assertEQ(args[i], expected[i]);
    }
}

function main() {
    let arg1: int[] = [1, 2]
    let arg2: FixedArray<int> = [3, 4]
    let arg3: [int, int] = [5, 6]
    fn([1, 2, 3, 4], 1, 2, 3, 4);
    fn([1, 2], ...arg1);
    fn([1, 2], ...[...arg1]);
    fn([1, 2, 3, 4, 1, 2], 1, 2, 3, 4, ...arg1);
    fn([1, 2, 3, 4, 7, 5, 6, 8], ...arg1, ...arg2, 7, ...arg3, 8);
}


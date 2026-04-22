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

function of(...items: FixedArray<int>): int {
    return 0
}

function of(...items: FixedArray<number>): int {
    return items.length.toInt()
}

function main() {
    let a1: FixedArray<number> = [-1, -2, -3, -4, -5]
    let a2: FixedArray<int> = [1, 2, 3, 4, 5]

    arktest.assertEQ(of(...a1), 5);
    arktest.assertEQ(of(...a2), 0);
}


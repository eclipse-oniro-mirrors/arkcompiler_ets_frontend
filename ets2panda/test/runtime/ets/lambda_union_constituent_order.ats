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

function acceptComparator(comparator?: ((a: number, b: number) => int) | undefined): void {}

function main(): void {
    acceptComparator((a, b) => (a - b).toInt());

    const levels: number[] = [3.0, 1.0, 2.0];
    levels.sort((a, b) => (a - b).toInt());
    arktest.assertEQ(levels[0], 1.0);
    arktest.assertEQ(levels[1], 2.0);
    arktest.assertEQ(levels[2], 3.0);
}

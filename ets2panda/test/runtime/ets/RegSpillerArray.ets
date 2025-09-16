/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

function main(): void {
    const failures: int = 0;
    const test1First : boolean[] = [true];
    const test1Second : boolean[] = [true];
    const test2First : boolean[] = [true, true];
    const test2Second : boolean[] = [false, false];
    const test3First : boolean[] = [];
    const test3Second : boolean[] = [];
    const test4First : boolean[] = [true, false];
    const test4Second : boolean[] = [];
    const test5First : boolean[] = [];
    const test5Second : boolean[] = [false, true, false];

    const test1Expected : boolean[] = [true, true];
    const test2Expected : boolean[] = [true, true, false, false];
    const test3Expected : boolean[] = [];
    const test4Expected : boolean[] = [true, false];
    const test5Expected : boolean[] = [false, true, false];

    arktest.assertEQ(failures, 0)

    arktest.assertEQ(test1First[0], true)
    arktest.assertEQ(test1Second[0], true)

    arktest.assertEQ(test2First[0], true)
    arktest.assertEQ(test2First[1], true)
    arktest.assertEQ(test2Second[0], false)
    arktest.assertEQ(test2Second[1], false)

    arktest.assertEQ(test3First.length, 0)
    arktest.assertEQ(test3Second.length, 0)

    arktest.assertEQ(test4First[0], true)
    arktest.assertEQ(test4First[1], false)
    arktest.assertEQ(test4Second.length, 0)

    arktest.assertEQ(test5First.length, 0)
    arktest.assertEQ(test5Second[0], false)
    arktest.assertEQ(test5Second[1], true)
    arktest.assertEQ(test5Second[2], false)

    arktest.assertEQ(test1Expected[0], true)
    arktest.assertEQ(test1Expected[1], true)

    arktest.assertEQ(test2Expected[0], true)
    arktest.assertEQ(test2Expected[1], true)
    arktest.assertEQ(test2Expected[2], false)
    arktest.assertEQ(test2Expected[3], false)

    arktest.assertEQ(test3Expected.length, 0)

    arktest.assertEQ(test4Expected[0], true)
    arktest.assertEQ(test4Expected[1], false)

    arktest.assertEQ(test5Expected[0], false)
    arktest.assertEQ(test5Expected[1], true)
    arktest.assertEQ(test5Expected[2], false)
}

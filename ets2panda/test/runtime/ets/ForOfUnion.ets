/*
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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

function check1() {
    let a8 : string|Int[] = "abc"
    let str = "";
    for (let character : string|Int of a8) {
        if (character instanceof string) {
            str += character;
        }
    }

    arktest.assertEQ(str, "abc")
}

function check2() {
    let a9 : string|Int[] = [1,2] as Int[]
    let sum = 0;
    for (let val : string|Int of a9) {
        let b9 = val
        if (b9 instanceof Int) {
            sum += b9;
        }
    }

    arktest.assertEQ(sum, 3)
}

function fooStr() : string|Int[] {
    let a : string|Int[] = "abc";
    return a;
}

function check3() {
    let a8 : string|Int[] = fooStr();
    let target : string|Int = 0;
    for (let character : String|Int of a8) {
        target = character;
    }

    arktest.assertTrue(target instanceof string)
}

function fooInt() : string|Int[] {
    let a : string|Int[] = [1,2] as Int[];
    return a;
}

function check4() {
    let a8 : string|Int[] = fooInt();
    let target : string|Int = 0;
    for (let val : String|Int of a8) {
        target = val;
    }

    arktest.assertTrue((target instanceof Int) && (target == 2))
}

function main() {
    check1();
    check2();
    check3();
    check4();
}

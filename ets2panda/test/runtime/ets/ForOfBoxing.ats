/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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
    let sum = 0;
    let a : number[] = [1,2,3]
    let idx : number | string;
    for (idx of a) {
        let b = idx
        if (b instanceof Number) {
            sum += b;
        } else {
            sum += 0;
        }
    }

    arktest.assertEQ(sum, 6)
}

function check2() {
    let str = "";
    let a2 : string[] = ["ddd"]
    let idx2 : string | boolean;
    for (idx2 of a2) {
        let c2 = idx2
        if (c2 instanceof string) {
            str += c2;
        }
    }

    arktest.assertEQ(str, "ddd")
}

function check3() {
    let str = "";
    let a3 : (int|string)[] = ["1,2,3"]
    let idx3 : int|String;
    for (idx3 of a3) {
        let c3 = idx3
        if (idx3 instanceof string) {
            str += c3;
        }
    }

    arktest.assertEQ(str, "1,2,3")
}

function check4() {
    let sum = 0;
    let a4 : number[] = [888, 999];
    for (let idx : number | null of a4) {
        let b4 = idx;
        if (b4 instanceof Number) {
            sum += b4;
        }
    }

    arktest.assertEQ(sum, 1887)
}

function check5() {
    let sum = 0;
    let a5 : number[] = [11, 22];
    for (let idx5 : Number of a5) {
        let b5 = idx5
        sum += b5;
    }

    arktest.assertEQ(sum, 33)
}

function check6() {
    let sum = 0;
    let a6 : number[] = [500, 600];
    for (let idx6 : Double | null of a6) {
        let b6 = idx6
        if (b6 instanceof Number) {
            sum += b6;
        }
    }

    arktest.assertEQ(sum, 1100)
}

function check7() {
    let str = "";
    let a7 : string = "ffff"
    for (let idx7 : string of a7) {
        let b7 = idx7
        str += b7;
    }

    arktest.assertEQ(str, "ffff")
}
function main() {
    check1();
    check2();
    check3();
    check4();
    check5();
    check6();
    check7();
}

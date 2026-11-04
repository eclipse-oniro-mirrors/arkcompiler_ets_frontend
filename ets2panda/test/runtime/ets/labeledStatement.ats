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

function labeledTest01(): int {
    let value: int = 0;
    let result: int = 0;

    label1:
    for (let i = 0; i < 10; i++) {
        switch (value) {
            case 0:
            case 1:
                result += 2;
                ++i;
                continue label1;
            case 2:
                result = 1;
                break;
            default:
                result = 3400;
        }
    }

    return result;
}

function labeledTest02(): int {
    let value: int = 0;
    let result: int = 0;

    for (let i = 0; i < 10; i++) {
        label1:
        for (let j = 0; j < 10; j++) {
            if (i + j > 10) {
                result += i * j;
                break label1;
            } else {
                continue label1;
            }
        }
    }

    return result;
}

function labeledTest03(): int {
    let result: int = 1;
    let i = 0;
    let j = 0;

    while (i < 100) {

        label1:
        for (let k = 0; k < 5; k++) {
            if (result < 10) {
                result += (k + 1) * (i + 1);

                ++i;
                k = i;
                break label1;
            }
        }

        label2:
        for (let l = 0; l < 79; l++) {
            result -= l / (i + 2);

            if (result > 13) {
                break label2;
            }
        }

        i++;

        return result;
    }

    return result;
}

function labeledTest04(): int {
    let result: int = 0;
    let i = 0;

    label1:
    do {
        i = i + 1;
        result = result + i;
        if (result > 10000) {
            break label1;
        }

    } while (i < 142);

    return result - 11;
}

function labeledTest05(): int {
    let h = 0;
    loop1:
    for (let i = 0; i < 3; i++) {
        loop2:
        for (let j = 0; j < 3; j++) {
            if (i == 1 && j == 1) {
                h += 20;
                break loop1;
            } else {
                h += 100;
                continue loop2;
            }
        }
    }
    return h;
}

function main(): void {
    let test1 = labeledTest01();
    arktest.assertEQ(test1, 10)

    let test2 = labeledTest02();
    arktest.assertEQ(test2, 200)

    let test3 = labeledTest03();
    arktest.assertEQ(test3, -999)

    let test4 = labeledTest04();
    arktest.assertEQ(test4, 10000)

    let test5 = labeledTest05();
    arktest.assertEQ(test5, 420)
}

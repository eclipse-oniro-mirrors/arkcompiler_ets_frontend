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

// test about variable captures
function foo(c: () => void): void {
    c();
}

function test_captures(): void {
    const num_const = 1
    let num = 2

    // capture in lambda
    foo() {
        arktest.assertEQ(num_const, 1)
        arktest.assertEQ(num, 2)
        {
            num++
            arktest.assertEQ(num_const, 1)
            arktest.assertEQ(num, 3)
        }

        let x: ()=>void = () => {
            num++  // This capture will cause crash issue(#I80K99)
        }
        x();

        if (true) {
            num++;
            arktest.assertEQ(num, 4)
            arktest.assertEQ(num_const, 1)
        }

        for (let i = 0; i < 2; ++i) {
            num++;
            arktest.assertEQ(num_const, 1)
        }

        arktest.assertEQ(num, 6)

        while (true) {
            if (num < 7) {
                num++
            } else {
                break;
            }
        }
        arktest.assertEQ(num, 7)

        foo() {
            num++;
            arktest.assertEQ(num, 8)
        }
    };
}

function main() {
    test_captures();
}

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

function main(): void {
    let f = (): void => {
        let sum = 0;
        outer: for (let v of [1, 2, 3, 4, 5]) {
            if (v % 2 === 0) {
                continue outer;
            }
            sum += v;
        }
        arktest.assertEQ(sum, 9);
    };
    f();

    let g = (): void => {
        outer: for (let a of [1, 2]) {
            for (let b of [10, 20]) {
                if (b === 20) {
                    continue outer;
                }
            }
        }
    };
    g();

    let h = (): void => {
        let count = 0;
        top: for (let a of [1, 2, 3]) {
            for (let b of [4, 5]) {
                for (let c of [6, 7]) {
                    if (c === 7) {
                        continue top;
                    }
                    count++;
                }
            }
        }
    };
    h();
}

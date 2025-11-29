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

// This test snippet originates from the subsystem’s XTS test suite.
// It is extracted to reproduce and verify the core behavior in isolation.
async function Test(p:number): Promise<Array<number>> {
    let arr: Array<number> = [1.1, 1.2]
    return new Promise<Array<number>>((resolve: (value: Array<number>) => void): void => {
        resolve(arr);
    })
}

async function foo() {
    let myArr: Array<number> = [1.1, 1.2]
    for (const p of myArr) {
        await Test(p).then((data: Array<number>) => {
            arktest.assertEQ(p + data.length, p + 2)
            console.info(data.length)
        })
    }
}

function main() {
    await foo()
}
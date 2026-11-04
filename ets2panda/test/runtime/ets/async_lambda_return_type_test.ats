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

let money_box: number[] = Array.from<number>([10.0, 5.0, 20.0, 50.0, 100.0, 200.0, 500.0])
let lock: AsyncLock = AsyncLock.request("lock_1")
let len: number = 0

async function initialize() {
    len = await lock.lockAsync<number>(async () => {
        return money_box.length
    })
}

function main() {
    initialize().then(() => {
        arktest.assertEQ(len, 7)
    })
}

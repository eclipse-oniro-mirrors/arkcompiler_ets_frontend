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
class ReceiverTest {
    value: int

    constructor(value: int) {
        this.value = value
    }
}

class ReceiverTest1 {
    value: int

    constructor(value: int) {
        this.value = value
    }
}

class ReceiverTest2 {
    value: int

    constructor(value: int) {
        this.value = value
    }
}

get valuePlusOne(this: ReceiverTest): int {
    return this.value + 1
}

set valueNew(this: ReceiverTest, newValue: int) {
    this.value = newValue
}

get valuePlusOneGetter(this: ReceiverTest1): int {
    return this.value + 1
}

set valueNewSetter(this: ReceiverTest2, newValue: int) {
    this.value = newValue
}

function main(): void {
    let r: ReceiverTest = new ReceiverTest(1)
    arktest.assertEQ(r.valuePlusOne, 2)
    r.valueNew = 3
    arktest.assertEQ(r.valuePlusOne, 4)

    let r1: ReceiverTest1 = new ReceiverTest1(5)
    arktest.assertEQ(r1.valuePlusOneGetter, 6)

    let r2: ReceiverTest2 = new ReceiverTest2(7)
    r2.valueNewSetter = 8
    arktest.assertEQ(r2.value, 8)
}

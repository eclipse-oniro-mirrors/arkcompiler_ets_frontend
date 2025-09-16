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

class A {
    public field: number
    constructor constructor2(...numbers: [number, number, number]) {
        this.field = numbers[0] + numbers[1] + numbers[2]
    }

    constructor constructor1(a: int, ...numbers: [number, number, number]) {
        this.field = a + numbers[0] + numbers[1] + numbers[2]
    }

    overload constructor{ constructor1, constructor2 }
}

function main() {
    let tuple: [number, number, number] = [30, 40, 50]

    let res: A = new A(10, 20, 30)
    let res2: A = new A(10, 20, 30, 40)

    let res3: A = new A(...tuple)
    let res4: A = new A(10, ...tuple)

    let res5: A = new A(...([30, 40, 50] as [number, number, number]))
    let res6: A = new A(10, ...([30, 40, 50] as [number, number, number]))

    arktest.assertEQ(res.field, 60)
    arktest.assertEQ(res2.field, 100)
    arktest.assertEQ(res3.field, 120)
    arktest.assertEQ(res4.field, 130)
    arktest.assertEQ(res5.field, 120)
    arktest.assertEQ(res6.field, 130)
}

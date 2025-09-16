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


class A {
    foo () : int[] {
        return [7]
    }
}

function foo() : int[]{
    return [1, 2] 
}

function getStringArr(): string[] {
    return ["A"]
}

function main() {
    let a = new A()
    let arr1: int[] = [...a.foo(), ...foo()]
    arktest.assertEQ(arr1.length, 3)
    arktest.assertEQ(arr1[0], 7)
    arktest.assertEQ(arr1[1], 1)
    arktest.assertEQ(arr1[2], 2)

    let arr2: string[] = [...getStringArr()]
    arktest.assertEQ(arr2.length, 1)
    arktest.assertEQ(arr2[0], "A")

    let getArr0 = (): string[] => {
        return ["a", "b"]
    }
    let arr3 : string[] = [...arr2, ...getArr0(), "B"]
    let expectedArr3: string[] = ["A", "a", "b", "B"]
    arktest.assertEQ(arr3.length, expectedArr3.length)
    for (let i = 0; i < arr3.length; i++) {
        arktest.assertEQ(arr3[i], expectedArr3[i])
    }

    let getArr1 = (): boolean[] => {
        return [true, false, true]
    }
    let arr4: boolean[] = [...getArr1(), ...getArr1()]
    let expectedArr4: boolean[] = [true, false, true, true, false, true]
    arktest.assertEQ(arr4.length, expectedArr4.length)
    for (let i = 0; i < arr4.length; i++) {
        arktest.assertEQ(arr4[i], expectedArr4[i])
    }

    let arr5: (string|boolean|int)[] = [...a.foo(), ...getArr0(), ...getStringArr(), ...getArr1()]
    let expectedArr5: (string|boolean|int)[] = [7, "a", "b", "A", true, false, true]
    arktest.assertEQ(arr5.length, expectedArr5.length)
    for (let i = 0; i < arr5.length; i++) {
        arktest.assertEQ(arr5[i], expectedArr5[i])
    }

}


/*
 * Copyright (c) 2023 - 2024-2025 Huawei Device Co., Ltd.
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

let a = 0;

function foo(): int {
    a++;
    return 1;
}

function bar(): int {
    a++;
    return 2;
}

class Residence {
    numberOfRooms: int = 1;
}

class Person {
    residence: Residence = new Residence();
}

function aux(): Person | null {
  return null;
}

function main(): void {
    a = 0;
    let test = false;
    let john: Person | null = aux();

    try {
        let residence = (john as Person).residence;
    } catch (e: ClassCastError) {
        test = true;
    }
    arktest.assertEQ(test, true)

    test = false;
    arktest.assertEQ(test, false)

    try {
        let numbers: int = john!.residence.numberOfRooms;
    } catch (e: NullPointerError) {
        test = true;
    }
    arktest.assertEQ(test, true)

    test = false;
    arktest.assertEQ(test, false)
    try {
        let numbers: int = foo() + bar() + john!.residence.numberOfRooms;
    } catch (e: NullPointerError) {
        test = true;
    }
    arktest.assertEQ(test, true)
    arktest.assertEQ(a, 2) // foo and bar were evaluated

    john = new Person();

    let numbers: int = john.residence.numberOfRooms;
    arktest.assertEQ(numbers, 1)

    numbers = foo() + bar() + john.residence.numberOfRooms;
    arktest.assertEQ(numbers, 4)
}

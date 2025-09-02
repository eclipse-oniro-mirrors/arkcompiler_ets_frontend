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
    private _data: number = 0

    get data(): number {
        return this._data
    }

    set data(input: number|string|boolean) {
        if (input instanceof number) {
            this._data = input
        } else if (input instanceof string) {
            this._data = new Number(input)
        } else {
            this._data = input ? 1 : 0
        }
    }
}

function testAssignment() {
    let obj: A = new A()
    arktest.assertEQ(obj.data, 0)

    obj.data = 2
    arktest.assertEQ(obj.data, 2)

    obj.data = '123'
    arktest.assertEQ(obj.data, 123)

    obj.data = true
    arktest.assertEQ(obj.data, 1)

    obj.data = false
    arktest.assertEQ(obj.data, 0)
}

function testObjectLiteral() {
    let obj: A = {}
    arktest.assertEQ(obj.data, 0)

    obj = { data: 2 }
    arktest.assertEQ(obj.data, 2)

    obj = { data: '123' }
    arktest.assertEQ(obj.data, 123)

    obj = { data: true }
    arktest.assertEQ(obj.data, 1)

    obj = { data: false }
    arktest.assertEQ(obj.data, 0)
}

function main(): void {
    testAssignment()
    testObjectLiteral()
}

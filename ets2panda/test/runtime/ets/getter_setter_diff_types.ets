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

// Issue #25267 Unable to use different getter/setter signature
class A {
    private _data: string = ''

    get data(): string {
        return this._data
    }

    set data(input: string|null) {
        this._data = (input === null) ? "null" : input
    }
}

function testAssignment() {
    let obj: A = new A()
    arktest.assertEQ(obj.data, '')

    obj.data = 'abc'
    arktest.assertEQ(obj.data, 'abc')

    obj.data = ''
    arktest.assertEQ(obj.data, '')

    obj.data = null
    arktest.assertEQ(obj.data, 'null')
}

function testObjectLiteral() {
    let obj: A = {}
    arktest.assertEQ(obj.data, '')

    obj = { data: 'abc' }
    arktest.assertEQ(obj.data, 'abc')

    obj = { data: '' }
    arktest.assertEQ(obj.data, '')

    obj = { data: null }
    arktest.assertEQ(obj.data, 'null')
}

function main(): void {
    testAssignment()
    testObjectLiteral()
}

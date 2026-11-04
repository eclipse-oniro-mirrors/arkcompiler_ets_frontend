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

class A<T> {
    fld: T
    constructor(p: T) {
        this.fld = p
    }
}

class B {
    fld: string = 'b'
}

class C {
    fld: Int = 11
}

let u1: A<string>|B = new A<string>('a')
let u2: A<string>|B = new B

let u3: A<Int>|C = new A<Int>(10)
let u4: A<Int>|C = new C

function main() {
    arktest.assertEQ(u1.fld, 'a')
    arktest.assertEQ(u2.fld, 'b')

    arktest.assertEQ(u3.fld, 10)
    arktest.assertEQ(u4.fld, 11)
}

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
    fld: Int = 10
}

let u1: B|C = new B
let u2: B|C = new C

let u3: A<string>|A<Int> = new A<Int>(11)
let u4: A<string>|A<Int> = new A<string>('a')

function main() {
    arktest.assertEQ(u1.fld, 'b')
    arktest.assertEQ(u2.fld, 10)

    arktest.assertEQ(u3.fld, 11)
    arktest.assertEQ(u4.fld, 'a')
}

/* @@? 38:22 Error TypeError: Member type must be the same for all union objects. */
/* @@? 39:22 Error TypeError: Member type must be the same for all union objects. */
/* @@? 41:22 Error TypeError: Member type must be the same for all union objects. */
/* @@? 42:22 Error TypeError: Member type must be the same for all union objects. */

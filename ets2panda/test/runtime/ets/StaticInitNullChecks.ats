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

/*
   These tests should be not be affected by compile-time
   nullsafety checks. Please adjust the code if compiler has actually
   succeeded to figure out there is an unitialized value access.
*/ 

class A {
    static x: object = B.getx()
    static getx() { return A.x }
}

class B {
    static x: object = A.getx()
    static getx() { return B.x }
}

arktest.expectThrow(() => { A.getx() }, (e) => e instanceof NullPointerError)
arktest.expectThrow(() => { A.getx() }, (e) => e instanceof LinkerUnresolvedClassError)
arktest.expectThrow(() => { B.getx() }, (e) => e instanceof LinkerUnresolvedClassError)

class C {
    static x: string

    static {
        cb()
        C.x = "a"
    }
}

let cb: () => void
cb = () => { C.x }

arktest.expectThrow(() => { new C() }, (e) => e instanceof NullPointerError)
arktest.expectThrow(() => { new C() }, (e) => e instanceof LinkerUnresolvedClassError)

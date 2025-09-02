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
    foo2(a: int) { }
    foo1() {
        return 1
    }
    overload foo{ foo1, foo2 }
}

class B {
    foo2(a: int) { }
    foo1() {
        return 2
    }
    overload foo{ foo1, foo2 }
}

type AAA = A | B

function foo<T extends AAA>(a: T) {
    return /* @@ label1 */a.foo()
}

/* @@@ label1 Error TypeError: Overload declaration cannot be called by union. */

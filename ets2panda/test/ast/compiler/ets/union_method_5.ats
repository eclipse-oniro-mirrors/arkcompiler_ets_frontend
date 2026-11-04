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
    foo(v: string) { console.log("A:" + v) }
}

class B extends A {
    foo(v: string) { console.log("B:" + v) }
}

class C {
    foo(v: string) { console.log("C:" + v) }
}

class D extends C {}

function test1(v: A|C) {
    v.foo("123")
}

function test2(v: B|C) {
    v.foo("123")
}

function test3(v: B|D) {
    v.foo("123")
}

test1(new A())
test1(new C())
test2(new B())
test3(new D())

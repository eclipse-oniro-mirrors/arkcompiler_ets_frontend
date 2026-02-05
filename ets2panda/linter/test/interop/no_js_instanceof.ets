/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

import { Foo, foo, CreatePerson, a , b, MyNamespace } from "./no_js_instanceof_file.js"

class Foo1 {}

let foo1 = new Foo1()

if(foo1 instanceof Foo1) {

}

if(foo instanceof Foo) {
    
}

if(foo1 instanceof Foo) {

}

if(foo instanceof Foo1) {

}

let person: CreatePerson = CreatePerson('xc', 18)

if(person instanceof CreatePerson) {

}

function test1(): void {
    if(person instanceof CreatePerson) {

    }
}

const test2 = (): void => {
    if(person instanceof CreatePerson) {

    }
}

class Test3 {
    init(): void {
        if(person instanceof CreatePerson) {

        }
    }
}

if(a instanceof Array) {

}

if(b() instanceof Array) {

}

const myDog: MyNamespace.Dog = new MyNamespace.Dog('Buddy');

if (myDog instanceof MyNamespace.Dog) {
  console.log("This is a Dog!");
}
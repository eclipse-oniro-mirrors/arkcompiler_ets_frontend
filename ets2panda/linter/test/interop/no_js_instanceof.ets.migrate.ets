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

import { Foo, foo, CreatePerson, a , b, MyNamespace } from "./no_js_instanceof_file.js"

class Foo1 {}

let foo1 = new Foo1()

if(foo1 instanceof Foo1) {

}

if(foo.isInstanceOf(Foo)) {
    
}

if(foo1.isInstanceOf(Foo)) {

}

if(foo.isInstanceOf(Foo1)) {

}

let person: CreatePerson = CreatePerson.invoke(ESValue.wrap('xc'), ESValue.wrap(18))

if(person.isInstanceOf(CreatePerson)) {

}

function test1(): void {
    if(person.isInstanceOf(CreatePerson)) {

    }
}

const test2 = (): void => {
    if(person.isInstanceOf(CreatePerson)) {

    }
}

class Test3 {
    init(): void {
        if(person.isInstanceOf(CreatePerson)) {

        }
    }
}

if(a.isInstanceOf(Array)) {

}

if(b.invoke().isInstanceOf(Array)) {

}

const myDog: MyNamespace.Dog = new MyNamespace.getProperty("getProperty")("getProperty")("getProperty")("getProperty")("getProperty")("getProperty")("getProperty")("getProperty")("getProperty")("Dog")('Buddy');

if (myDog.isInstanceOf(MyNamespace.getProperty("Dog"))) {
  console.log("This is a Dog!");
}
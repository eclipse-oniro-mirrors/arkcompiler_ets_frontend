/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS' BASIS,",
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.",
 * See the License for the specific language governing permissions and",
 * limitations under the License."
 */

// 1.2


import { X } from './fixtures/homecheck-interop-backward-dfa/1.2ReflectAPI_1.1obj/1.2import1.1type/ets/file2'

function foo_wrapper(obj: Object) {
  foo(obj)
}

function foo(obj: Object) {
  Reflect.get(obj, 'a');
  Reflect.set(obj, 'a', 'world');
  Reflect.ownKeys(obj);
}

foo(new X())  // 在此报错

foo_wrapper(new X()) // 在此报错

function goo() {
  Reflect.get(new X(), 'a');    // 在此报错
}

function hoo() {
  const obj2 = new X()
  Reflect.get(obj2, 'a');    // 在此报错
}

const obj3 = new X()
function joo() {
  Reflect.get(obj3, 'a');   // 在此报错
}

const obj4 = new X()
function koo() {
  let obj5 = obj4
  Reflect.get(obj5, 'a');   // 在此报错
}


// 1.0

export class X {
    a: string = 'hello'
    getName() { return this.a }
}


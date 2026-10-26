/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// 1.0

import { X } from './fixtures/homecheck-interop-backward-dfa/1.1ReflecfAPI_1.2obj/1.1import1.2type/ets/file1'

function foo_wrapper(obj: ESObject) {
  foo(obj)
}

function foo(obj: ESObject) {
  Reflect.apply(obj.getName, {a : 12});
  Reflect.defineProperty(obj, 'newField', { value: 7 });
  Reflect.deleteProperty(obj, "a");
  Reflect.getOwnPropertyDescriptor(obj, "a");
  Reflect.ownKeys(obj);
  Reflect.isExtensible(obj);
  Reflect.get(obj, 'a');
  Reflect.set(obj, 'a', 'world');
}
foo(new X())  // 在此报错

foo_wrapper(new X()) // 在此报错

function goo() {
  Reflect.get(new X(), 'a');  // 在此报错
}

function hoo() {
  const obj2 = new X()  // 在此报错
  Reflect.deleteProperty(obj2, "a");
}

const obj3 = new X() // 在此报错
function joo() {
  Reflect.deleteProperty(obj3, "a");
}

const obj4 = new X()
function koo() {
  let obj5 = obj4   // 在此报错
  Reflect.deleteProperty(obj5, "a");
}
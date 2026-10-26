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


import { X } from './fixtures/homecheck-interop-backward-dfa/1.2ObjectAPI_1.1obj/1.2import1.1type/ets/file2'

function foo_wrapper(obj: Object) {
  foo(obj)
}

function foo(obj: Object) {
  Object.assign({}, obj);
  Object.entries(obj);
  Object.keys(obj);
  Object.values(obj);
}
foo(new X())  // 在此报错

foo_wrapper(new X()) // 在此报错

function goo() {
  Object.keys(new X());   // 在此报错
}

function hoo() {
  const obj2 = new X()
  Object.keys(obj2);    // 在此报错
}

const obj3 = new X()
function joo() {
  Object.keys(obj3);  // 在此报错
}

const obj4 = new X()
function koo() {
  let obj5 = obj4
  Object.keys(obj5);  // 在此报错
}


// 1.0

export class X { a = 1 }


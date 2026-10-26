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

class B {
  v = 2
}

function test4() {
  let obj: B = {v: 2};
  let _ = obj instanceof B;
}

@Entry
@Component
struct Index {
  build() {
  }
}

class B2 {
  v = 2
}

function test4_2() {
  let obj: B = {v: 2};
  let obj3 = obj;
  let _ = obj2 instanceof B;
}

@Entry
@Component
struct Index2 {
  build() {
  }
}

// Additional cases for branch coverage

// Import with null exportInfo / arkExport not Local
import { obj as someValue } from './fixtures/homecheck-object-literal/ets/object';

// Top-level var with 'this' name pattern
class ClassC {
  this: number = 1;
  val: string = '';
}

// Object literal where class field lookup returns null
class StaleSig {
  old: number = 0;
}
let staleObj: StaleSig = { old: 1, missing: 2 };

// Object literal with non-assign stmt in initializer
class NoInit {
  val: number;
  constructor() {
    this.val = 42;
  }
}
let noInitObj: NoInit = { val: 1 };

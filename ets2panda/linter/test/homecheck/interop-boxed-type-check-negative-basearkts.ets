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

import { a, b, c } from './fixtures/homecheck-interop-boxed-type/exportEts2'




import { ClassBoxedInField,
    ClassBoxedInMethod,
    ClassNoBoxed, Node } from './fixtures/homecheck-interop-boxed-type/exportTs';

import { fooWithBoxed,
    fooReturnBoxed,
    fooReturnCallBoxed,
    fooReturnCallNoBoxed } from './fixtures/homecheck-interop-boxed-type/exportEts1';

import { myNamespaceA,
    myNamespaceB,
    myNamespaceC,
    myNamespaceD } from './fixtures/homecheck-interop-boxed-type/exportTs';

import defaultFoo from './fixtures/homecheck-interop-boxed-type/exportJs';

import { boxedObj, boxedMethod, ClassWithBoxedObj, classObj } from './fixtures/homecheck-interop-boxed-type/exportJs';

import path from 'path';
import { hilog } from '@kit.PerformanceAnalysisKit';

// Additional cases for branch coverage

// Boxed type with number assignment
let boxedNum: Number = 42;

// Boxed type with string assignment
let boxedStr: String = "hello";

// Boxed type with boolean assignment
let boxedBool: Boolean = true;

// Boxed type in function parameter
function acceptBoxed(param: Number): void {
  console.log(param.valueOf());
}
acceptBoxed(42);

// Boxed type in return
function returnBoxed(): Number {
  return 42;
}

// Boxed type in class field
class BoxedClass {
  count: Number = 0;
  name: String = "";
  flag: Boolean = false;
}

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

import {exportFunc} from './Test';

function useExportFunc() {
  exportFunc(1,2);
}

export function func1() {
}

interface Interface1 {
  param1: int;
}

interface Interface2 {
  iMethod1();
}

interface Interface3 extends Interface2{
  iMethod2();
}

class ClassA {
  clsA1: int;
}

class ClassB {
  clsB1: int;
}

class ClassC extends ClassB {
  clsC1: int;
}

class ClassE {
  name: string = 'hello';
  age: int = 10;
  constructor(n: string, a: int) {
    this.name = n;
    this.age = a;
  }
}

class ClassE1 {
  static name = 'hello';
}

class ClassF {
  cMethod() {
  }
}

class ClassF1 {
  static staticMethod(): string {
    return "this is a static method";
  }
}

function main() {
  console.log(' exportFunc: ' + exportFunc(1,2));
  let n1 = new ClassE('hello1', 10);
  console.log(' n1: ' + n1.name);
  console.log(' ClassE1: ' + ClassE1.name);
  console.log(ClassF1.staticMethod());
}
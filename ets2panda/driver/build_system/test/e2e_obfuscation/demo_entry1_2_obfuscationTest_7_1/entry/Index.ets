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

class Class1 {
  name: string = 'Alice';
  age: number = 1;
}

class Class2 {
  field2: int;
}

class Class3 {
  static field3: int;
}

class Class4 {
  method4() {
  }
}

class Class5 {
  static method5() {
  }
}

class Class6 extends Class3 {
  static field6: int;
}

interface Interface1 {
  field1: int;
}

interface Interface2 {
  iMethod1();
}

interface Interface3 extends Interface2 {
  iMethod2();
}

function main() {
  console.log((new Class1()).name);
}
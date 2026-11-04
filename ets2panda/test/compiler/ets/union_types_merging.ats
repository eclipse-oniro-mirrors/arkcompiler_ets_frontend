/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

function main(): void {
  let union1: (A1 | A2 | A3 | A4 | A5);
  let union2: (B5 | B4 | B3 | B2 | B1);
  let union3: (A1 | B5 | A2 | B4 | A3 | B3 | A4 | B2 | A5 | B1);
  let union4: (A1 | B5 | A2 | B4 | A3 | object | B3 | A4 | B2 | A5 | B1);

  union1 = new A1();
  union2 = new B1();
  union3 = new A1();
  union3 = new B1();
  union4 = new Object();
}

class A1 {}
class A2 extends A1 {}
class A3 extends A2 {}
class A4 extends A3 {}
class A5 extends A4 {}

class B1 {}
class B2 extends B1 {}
class B3 extends B2 {}
class B4 extends B3 {}
class B5 extends B4 {}
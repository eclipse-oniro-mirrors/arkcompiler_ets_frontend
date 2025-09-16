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

class A {
  field1: number = 10;
  foo() { }
}

class B extends A {
  bfield1: number = 10;
  bfoo() { }
}

function getProperty<K extends (A | number | (keyof B))>(key: K) { }

type keyofA = keyof A;
type keyofB = keyof B;
type keyofUnion = keyofA | keyofB | number | A | B;

function main(): void {
  let a: A = new A();
  let b: B = new B();

  let x1: keyofUnion = "field1";
  let x2: keyofUnion = "foo";
  let x3: keyofUnion = "field1";
  let x4: keyofUnion = "foo";
  let x5: keyofUnion = 100;
  let x6: keyofUnion = a;
  let x7: keyofUnion = b;

  getProperty<keyofB>("field1")
  getProperty<keyofB>("bfoo")
  getProperty<A>(a)
}

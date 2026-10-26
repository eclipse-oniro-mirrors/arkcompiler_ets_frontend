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

interface InterfaceA {}
class ClassA implements InterfaceA {}
class ClassB extends ClassA {}
class ClassC extends ClassB {}
class ClassD extends ClassC {}

@Entry
@Component
struct MyComponent {
  @State obj: ClassA = getClassA();    // @Observed ClassC, ClassB, ClassA
  @Consume('theme') obj1: ClassA = getClassB();   // @Observed ClassB, ClassA
  @State obj2: ClassB = getClassB();   // @Observed ClassB, ClassA
  @State obj3 = getClassA();           // @Observed ClassC, ClassB, ClassA
  @State obj4: ClassB = getObjectLiteral();    // @Observed ClassB, ClassA
  @State positive1 = getObjectLiteral();    // nothing to do
  @State positive2 = foo();    // nothing to do

  build() {
  }
}
let cond1 = true
let cond2 = false
function getClassA(): ClassA {
  if (cond1) {
    return {};
  }
  if (cond2) {
    return new ClassC();
  }
  return new ClassA();
}

function getClassB() {
  if (cond1) {
    return {};
  }
  return new ClassB();
}

function getObjectLiteral() {
  if (cond1) {
    return {};
  }
  if (cond2) {
    return 123;
  }
  return 456;
}

function foo() {
  return 456;
}

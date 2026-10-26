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

class ClassB extends ClassA {}
class ClassC extends ClassB {}


@Entry
@Component
struct MyComponent {
  @State obj: ClassA = new ClassA();      // issue on obj
  @State obj1: ClassA = new ClassB();     // @Observed ClassB
  @State obj2: ClassA = {};               // issue on obj2 ???
  @State obj3: ClassB = {a: 1};           // @Observed ClassB
  @State obj4: ClassC = get();            // issue on obj4
  @State obj5: ClassC = getClassA();      // issue on obj5
  @State obj6: ClassC = getClassB();      // issue on obj6
  @State obj7: ClassA = getObjectLiteral();    // issue on obj7 ？？？

  build() {
  }
}

function getClassA();

function getClassB() {
    return new ClassA();
}

function getObjectLiteral() {
    return {}
}
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


class ClassA {}
class ClassB extends ClassA {}

@Entry
@Component
struct MultiStepInit {
  @State obj: ClassA = (() => {
    const temp = new ClassB();
    return temp;
  })();
  build() {}
}

@Entry
@Component
struct UnionField {
  @State obj1: ClassA | ClassB = {} as ClassA; // union type field with object literal
  build() {}
}

interface IObj {}
@Entry
@Component
struct InterfaceField {
  @State obj2: IObj = {} as IObj; // interface field type with object literal
  build() {}
}




class ClassA2 {}

function getNumber(): number {
  return 42;
}

@Entry
@Component
struct NonClassReturn {
  @State obj: ClassA2 = getNumber() as unknown as ClassA2; // non-class return type
  build() {}
}

class GrandParent {}
class Parent extends GrandParent {}
class Child extends Parent {}
class GrandChild extends Child {}

@Entry
@Component
struct MultiLevelInheritance {
  @State obj2: GrandParent = new GrandChild(); // multi-level inheritance
  build() {}
}




interface IFace {
  name: string;
}

class ImplClass implements IFace {
  name: string = '';
}

@Entry
@Component
struct ObjectLiteralInterface {
  @State obj1: IFace = { name: 'test' }; // object literal with interface field type
  build() {}
}

@Entry
@Component
struct LocalChain2 {
  @State obj2: ImplClass = (() => {
    const a = new ImplClass();
    const b = a; // local-to-local assignment chain
    return b;
  })();
  build() {}
}




class ObservedClass {}

class ProviderClass {
  getObserved(): ObservedClass {
    return new ObservedClass();
  }
  getNumber(): number {
    return 42;
  }
}

class GrandA {}
class GrandB extends GrandA {}
class GrandC extends GrandB {}

@Entry
@Component
struct NonClassReturnInvoke3 {
  @State obj1: ObservedClass = new ProviderClass().getObserved(); // invoke expr with class return
  @State obj2: ObservedClass = new ProviderClass().getNumber() as unknown as ObservedClass; // non-class return
  build() {}
}

@Entry
@Component
struct MultiLevelInheritanceComponent3 {
  @State obj3: GrandA = new GrandC(); // multi-level inheritance (3 levels)
  build() {}
}

@Entry
@Component
struct AnonymousObjectWithCast3 {
  @State obj4: ObservedClass = {} as ObservedClass; // anonymous object with cast
  build() {}
}

@Entry
@Component
struct UnionTypeField3 {
  @State obj5: ObservedClass | GrandA = {} as ObservedClass; // union type field with object literal
  build() {}
}

interface IFaceObs {}
@Entry
@Component
struct InterfaceObjectLiteral3 {
  @State obj6: IFaceObs = { name: 'test' }; // interface field with object literal
  build() {}
}


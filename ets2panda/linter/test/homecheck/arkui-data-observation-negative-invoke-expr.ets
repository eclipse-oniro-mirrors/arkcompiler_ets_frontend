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


// Additional cases for branch coverage

// Invoke method not found in scene
class MissingMethod {}
function unknownFunc() { return new MissingMethod(); }
@Entry
@Component
struct NotFoundInvoke {
  @State obj: MissingMethod = unknownFunc();
  build() {}
}

// Method returning non-class type (number)
class ClassForNumReturn {}
function getNum(): number { return 42; }
@Entry
@Component
struct NonClassReturnType {
  @State obj: ClassForNumReturn = getNum() as unknown as ClassForNumReturn;
  build() {}
}

// Class with no super classes (single level)
class NoSuper {}
@Entry
@Component
struct NoSuperClass {
  @State obj: NoSuper = new NoSuper();
  build() {}
}

// Field with primitive type (number)
@Entry
@Component
struct PrimitiveField {
  @State count: number = 42;
  build() {}
}

// Object literal with missing field
class PartialObj {
  a: number = 0;
  b: number = 0;
}
@Entry
@Component
struct MissingFieldObj {
  @State obj: PartialObj = { a: 1 }; // missing field b
  build() {}
}

// Cross-project class (SDK)
@Entry
@Component
struct SdkImport {
  @State obj: Object = new Object();
  build() {}
}

// Additional cases for branch coverage

// Circular type reference
class CircA { ref: CircA = this; }
@Entry
@Component
struct CircRef {
  @State obj: CircA = new CircA();
  build() {}
}

// Anonymous class with non-OBJECT category
class AnonContainer {
  createAnon(): CircA {
    return new class extends CircA {}();
  }
}
@Entry
@Component
struct AnonStruct {
  @State obj: CircA = new AnonContainer().createAnon();
  build() {}
}

// Cross-project field class
@Entry
@Component
struct SdkField {
  @State obj: Object = { toString: () => 'test' };
  build() {}
}

// Class with no heritage (single class, no super)
class Standalone {}
@Entry
@Component
struct NoInherit {
  @State obj: Standalone = {} as Standalone;
  build() {}
}

// Test: field without decorator (no @State/@Prop)
class NoDeco {}
@Entry
@Component
struct NoDecoratorField {
  plainField: NoDeco = new NoDeco();
  build() {}
}

// Test: constant assignment to @State field
@Entry
@Component
struct ConstAssign {
  @State count: number = 42;
  build() {}
}

// Test: cast expr where castOp is not Local (object literal cast)
class CastTarget {}
@Entry
@Component
struct CastObjLiteral {
  @State obj: CastTarget = {} as CastTarget;
  build() {}
}

// Test: class with non-CLASS category (interface)
interface IObj {}
@Entry
@Component
struct InterfaceField2 {
  @State obj: IObj = { name: 'test' };
  build() {}
}

// Test: cross-project class (SDK Object)
@Entry
@Component
struct SdkObjectField {
  @State obj: Object = new Object();
  build() {}
}

// Test: anonymous class with non-OBJECT category
class StructCategory {
  build() {}
}
@Entry
@Component
struct StructAnon {
  @State obj: StructCategory = new class extends StructCategory {}();
  build() {}
}

// Test: union type field with non-ClassType member
@Entry
@Component
struct UnionWithPrimitive {
  @State val: number | string = 42;
  build() {}
}

// Test: @State field with interface type (non-CLASS category)
interface IFace2 {}
class ImplClass2 implements IFace2 {}
@Entry
@Component
struct InterfaceStateField {
  @State obj: IFace2 = new ImplClass2();
  build() {}
}

// Test: @State field assigned with cast of object literal
class CastClass {}
@Entry
@Component
struct CastObjLiteralState {
  @State obj: CastClass = {} as CastClass;
  build() {}
}

// Test: @State field with SDK class (cross-project)
@Entry
@Component
struct SdkClassState {
  @State obj: Object = new Object();
  build() {}
}

// Test: @State field calling unknown function
class UnknownReturn {}
function getUnknown(): UnknownReturn {
  return new UnknownReturn();
}
@Entry
@Component
struct UnknownFuncState {
  @State obj: UnknownReturn = getUnknown();
  build() {}
}

// Test: @State field with function returning non-class
class NonClassTarget {}
function getPrimitive(): number {
  return 42;
}
@Entry
@Component
struct PrimitiveReturnState {
  @State obj: NonClassTarget = getPrimitive() as unknown as NonClassTarget;
  build() {}
}

// Test: recursive getClassByType with circular type
class SelfRef {
  self: SelfRef = this;
}
@Entry
@Component
struct SelfRefState {
  @State obj: SelfRef = new SelfRef();
  build() {}
}

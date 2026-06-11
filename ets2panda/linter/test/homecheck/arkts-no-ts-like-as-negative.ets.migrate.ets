/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

import { I, A, B, C, importedBase, ImportedParent1 } from './fixtures/BaseClass';

interface BaseI {}
interface BaseI1 {}

class ParentC implements BaseI {
  par1?: string = 'parentC';
}

class ParentC1 implements BaseI {
  par1?: string = 'parentC1';
}

class SonC extends ParentC {}

class SonC1 extends ParentC {
  par2?: number = 2.0;
}

class SonC2 extends ParentC {
  par2?: number = 2.0;
}

function takeParent(p: ParentC): void {}

let objP: ParentC = new ParentC();
let objI: BaseI = objP;
let asObjP1: ParentC1 = objI as ParentC1; // arkts-no-ts-like-as
let objI1: BaseI | BaseI1 = objP;
let asObjP11: ParentC1 = objI1 as ParentC1; // arkts-no-ts-like-as
let sonObj: SonC = objI as SonC; // arkts-no-ts-like-as

let childAsInterface: BaseI = new SonC();
let parObj1: ParentC = childAsInterface as ParentC;
let sameObj: ParentC = objP as ParentC;
if (childAsInterface instanceof SonC) {
  let guarded: SonC = childAsInterface as SonC;
}

let siblingInterface: BaseI = new SonC1();
let sibling: SonC2 = siblingInterface as SonC2; // arkts-no-ts-like-as

function argCast(arg: BaseI): void {
  takeParent(arg as ParentC); // arkts-no-ts-like-as
}

function returnCast(result: BaseI): ParentC {
  return result as ParentC;
}

function arrayPopCast(): ParentC {
  let arr: BaseI[] = [new ParentC1()];
  return arr.pop() as ParentC; // arkts-no-ts-like-as
}

function mapGetCast(): ParentC {
  let map: Map<string, BaseI> = new Map<string, BaseI>();
  map.set('x', new ParentC1());
  return map.get('x') as ParentC; // arkts-no-ts-like-as
}

function numericCast(a: number | string): number | string {
  a = (a as number) + 1; // arkts-no-ts-like-as
  !(a as number); // arkts-no-ts-like-as
  (a as number) ?? 1; // arkts-no-ts-like-as
  return a;
}

interface FlowBaseI {}
interface FlowBaseI1 {}

class FlowParent implements FlowBaseI {
  name: string = 'parent';
}

class FlowParent1 implements FlowBaseI {
  name: string = 'parent1';
}

class FlowSon extends FlowParent {}

class FlowSon1 extends FlowParent {
  score: number = 1;
}

class FlowSon2 extends FlowParent {
  score: number = 2;
}

type FlowParentAlias = FlowParent1;

let globalParent: FlowParent = new FlowParent();
let globalBase: FlowBaseI = globalParent;
let globalCast: FlowParent1 = globalBase as FlowParent1; // arkts-no-ts-like-as
let globalAliasCast: FlowParentAlias = globalBase as FlowParentAlias; // arkts-no-ts-like-as

function takeFlowParent(value: FlowParent): void {}

function paramSource(value: FlowBaseI): void {
  let castValue: FlowParent1 = value as FlowParent1; // arkts-no-ts-like-as
}

function callParamSource(): void {
  let value: FlowBaseI = new FlowParent();
  paramSource(value);
}

function returnSource(): FlowBaseI {
  return new FlowParent();
}

function returnCast(): FlowParent1 {
  return returnSource() as FlowParent1; // arkts-no-ts-like-as
}

function argumentCast(value: FlowBaseI): void {
  takeFlowParent(value as FlowParent); // arkts-no-ts-like-as
}

function arrayPushCast(value: FlowBaseI): void {
  let values: FlowParent[] = [];
  values.push(value as FlowParent); // arkts-no-ts-like-as
}

function setAddCast(value: FlowBaseI): void {
  let values: Set<FlowParent> = new Set<FlowParent>();
  values.add(value as FlowParent); // arkts-no-ts-like-as
}

function mapSetCast(value: FlowBaseI): void {
  let values: Map<string, FlowParent> = new Map<string, FlowParent>();
  values.set('key', value as FlowParent); // arkts-no-ts-like-as
}

function arrPopCast(): FlowParent {
  let values: FlowBaseI[] = [new FlowParent1()];
  return values.pop() as FlowParent; // arkts-no-ts-like-as
}

function mapGetCastFlow(): FlowParent {
  let values: Map<string, FlowBaseI> = new Map<string, FlowBaseI>();
  values.set('key', new FlowParent1());
  return values.get('key') as FlowParent; // arkts-no-ts-like-as
}

function safeArrayPopCast(): FlowParent {
  let values: FlowBaseI[] = [new FlowSon()];
  return values.pop() as FlowParent;
}

class FlowHolder {
  fieldParent: FlowParent = new FlowParent();
  static staticParent: FlowParent = new FlowParent();
  fieldBase: FlowBaseI = this.fieldParent;
  static staticBase: FlowBaseI = FlowHolder.staticParent;
  fieldCast: FlowParent1 = this.fieldBase as FlowParent1; // arkts-no-ts-like-as

  methodFieldCast(): void {
    let localCast: FlowParent1 = this.fieldBase as FlowParent1; // arkts-no-ts-like-as
  }

  static methodStaticCast(): void {
    let localCast: FlowParent1 = FlowHolder.staticBase as FlowParent1; // arkts-no-ts-like-as
  }
}

let siblingBase: FlowBaseI = new FlowSon1();
let siblingCast: FlowSon2 = siblingBase as FlowSon2; // arkts-no-ts-like-as

function setForEachCast(): void {
  let values: Set<FlowBaseI> = new Set<FlowBaseI>();
  values.add(new FlowParent1());
  values.forEach((value: FlowBaseI) => {
    let result: FlowParent = value as FlowParent; // arkts-no-ts-like-as
  });
}

let importedCast: ImportedParent1 = importedBase as ImportedParent1; // arkts-no-ts-like-as

function numericOperation(value: number | string): number | string {
  (value as number)++; // arkts-no-ts-like-as
  (value as number)--; // arkts-no-ts-like-as
  ++(value as number); // arkts-no-ts-like-as
  --(value as number); // arkts-no-ts-like-as
  (value as number) += 5; // arkts-no-ts-like-as
  (value as number) -= 5; // arkts-no-ts-like-as
  (value as number) *= 5; // arkts-no-ts-like-as
  (value as number) /= 5; // arkts-no-ts-like-as
  (value as number) %= 5; // arkts-no-ts-like-as
  (value as number) **= 5; // arkts-no-ts-like-as
  value = (value as number) + 1; // arkts-no-ts-like-as
  value = (value as number) - 1; // arkts-no-ts-like-as
  value = (value as number) * 5; // arkts-no-ts-like-as
  value = (value as number) / 5; // arkts-no-ts-like-as
  value = (value as number) % 5; // arkts-no-ts-like-as
  !(value as number); // arkts-no-ts-like-as
  ~(value as number); // arkts-no-ts-like-as
  (value as number) ?? 1; // arkts-no-ts-like-as
  return value;
}

function foo(x: number | string): number | string {
  let f = () => {
    x = 10;
  }
  if (typeof x === 'string') {
    f();
    return (x as string).length;
  } else {
    return (x as number)++; // arkts-no-ts-like-as
  }
}

class Test {
    a: number | string = 123;
    b: number = (this.a as number)--; // arkts-no-ts-like-as

    goo() {
      let x: number | boolean = 123;
      let y = ++(x as number); // arkts-no-ts-like-as
      --(x as number); // arkts-no-ts-like-as
    }
}

namespace test {
  class NumericOperationTest {
    a: number | string = 123;

    foo(x: number | boolean): number {
      if ((x as number)++ > 5) {} // arkts-no-ts-like-as
    }

    goo() {
      let b = this.foo(++(this.a as number)) // arkts-no-ts-like-as
      for (const i = 0; i < --(b as number); i++) {} // arkts-no-ts-like-as
    }

    other() {
      (this.a as number) += 5; // arkts-no-ts-like-as
      let c = 3;
      (c as number) -= 5; // arkts-no-ts-like-as
      let d = (c as number) *=5; // arkts-no-ts-like-as
      (this.a as number) /= 5; // arkts-no-ts-like-as
      (c as number) %= 5; // arkts-no-ts-like-as
      (c as number) **= 5; // arkts-no-ts-like-as
    }
  }
}

interface GuardBaseI {}

class GuardParent implements GuardBaseI {
  name: string = 'parent';
}

class GuardParent1 implements GuardBaseI {
  name: string = 'parent1';
}

class GuardSon extends GuardParent {}

function negativeGuard(value: GuardBaseI): void {
  if (!(value instanceof GuardParent1)) {
    let guarded: GuardParent1 = value as GuardParent1; // arkts-no-ts-like-as
  }
}

function orGuard(value: GuardBaseI, flag: boolean): void {
  if (flag || value instanceof GuardParent1) {
    let guarded: GuardParent1 = value as GuardParent1; // arkts-no-ts-like-as
  }
}

function checkedForDifferentTarget(value: GuardBaseI): void {
  if (value instanceof GuardSon) {
    let guarded: GuardParent1 = value as GuardParent1; // arkts-no-ts-like-as
  }
}

function binaryOrIf(): void {
    let a: A = new A();
    let i: I = a;;
    if (i instanceof A || true) {
        let b: B = i as B; // arkts-no-ts-like-as
    }
}

function binaryOrElse(): void {
    let a: A = new A();
    let i: I = a;;
    if (i instanceof A || true) {
        return;
    } else {
        let b: B = i as B; // arkts-no-ts-like-as
    }
}

function binaryOrNotIf(): void {
    let a: A = new A();
    let i: I = a;;
    if (!(i instanceof A || true)) {
        let b: B = i as B; // arkts-no-ts-like-as
    }
}

function binaryOrNotElse(): void {
    let a: A = new A();
    let i: I = a;;
    if (!(i instanceof A || true)) {
        return;
    } else {
        let b: B = i as B; // arkts-no-ts-like-as
    }
}

function binaryNotOrIf(): void {
    let a: A = new A();
    let i: I = a;;
    if (!(i instanceof A) || true) {
        let b: B = i as B; // arkts-no-ts-like-as
    }
}

function binaryNotOrElse(): void {
    let a: A = new A();
    let i: I = a;;
    if (!(i instanceof A) || true) {
        return;
    } else {
        let b: B = i as B; // arkts-no-ts-like-as
    }
}

function binaryOr(): void {
    let a: A = new A();
    let i: I = a;;
    if ((i instanceof B) || (i instanceof A)) {
        let b: B = i as B; // arkts-no-ts-like-as
    }
}

function binaryNot(): void {
    let a: A = new A();
    let i: I = a;;
    if (!(i instanceof B) && !(i instanceof A)) {
        let b: B = i as B; // arkts-no-ts-like-as
    }
}

class TestField {
    b: B = i as B;
}

function testReturn1(): I {
    let a: A = new A();
    let i: I = a;
    return i;
}

function testReturn2(): I {
    return testReturn1();
}

function testReturn3(): I {
    return testReturn2();
}

function testReturn4(): void {
    let i = testReturn1();
    let b: B = i as B; // arkts-no-ts-like-as
}

function testReturn5(): void {
    let i = testReturn2();
    let b: B = i as B; // arkts-no-ts-like-as
}

function testReturn6(): void {
    let i = testReturn3();
    let b: B = i as B; // arkts-no-ts-like-as
}

function testParam1(i: I): void {
    let b: B = i as B; // arkts-no-ts-like-as
}

function testParam2(i: I): void {
    testParam3(i);
}

function testParam3(i: I): void {
    let b: B = i as B; // arkts-no-ts-like-as
}

function testParam4(i: I): void {
    testParam5(i);
}

function testParam5(i: I): void {
    testParam6(i);
}

function testParam6(i: I): void {
    let b: B = i as B; // arkts-no-ts-like-as
}

function callMethod(): void {
    let a: A = new A();
    let i: I = a;
    testParam1(i);
    testParam2(i);
    testParam4(i);
}

class XField {
  classObj: A = new A();
  obj: I = this.classObj;

  test(): void {
    let b: B = this.obj as B; // arkts-no-ts-like-as
  }
}

function testField(): void {
  let b: B = C.c as B;
}

let aGlobal: A = new A();
let iGlobal: I = aGlobal;

function fooGlobal(): void {
    let b: B = iGlobal as B; // arkts-no-ts-like-as
}

let iImport = i;
let bImport = iImport as B;

let aLocal: A = new A();
let iLocal: I = aLocal;
let bLocal: B = iLocal as B; // arkts-no-ts-like-as

function testLocal(): void {
    let a: I = new A();
    let i: I = a;
    let b: B = i as B; // arkts-no-ts-like-as
}

class Base{}
class ABase extends Base{}
class BBase extends Base{}

let aBase: ABase = new ABase();
let bBase: Base = aBase;
let cBase = bBase as BBase; // arkts-no-ts-like-as

let aUnion:ABase = new ABase();
let bUnion: Base|ABase = aUnion;
let cUnion = bUnion as BBase; // arkts-no-ts-like-as

function fooParam(a: Base){
  let b = a as ABase; // arkts-no-ts-like-as
}
let aParam = new BBase();
fooParam(aParam);

function fooReturn(): Base{
  return new ABase();
}
let bReturn = fooReturn() as BBase; // arkts-no-ts-like-as

let aLiteral: ABase = {};
let bLiteral: Base = aLiteral;
let cLiteral = bLiteral as BBase; // arkts-no-ts-like-as

class Animal2 {
  static a: ABase = new ABase();
  b: Base = Animal2.a;
}
let animal2 = new Animal2();
let bAnimal = animal2.b as BBase; // arkts-no-ts-like-as

function fooReturnLiteral(): Base{
  return {};
}
let bReturnLiteral = fooReturnLiteral() as BBase; // arkts-no-ts-like-as

let aGlobal2: ABase = new ABase();
let bGlobal2: Base = aGlobal2;

function fooGlobal2() {
  let b = bGlobal2 as BBase; // arkts-no-ts-like-as
}

interface BaseI {}
interface AI extends BaseI{}
interface BI extends BaseI{}

let aInterface: AI = {};
let bInterface: BaseI = aInterface;
let cInterface = bInterface as BI; // arkts-no-ts-like-as

let aUnionInterface: AI = {};
let bUnionInterface: BaseI | BI = aUnionInterface;
let cUnionInterface = bUnionInterface as BI; // arkts-no-ts-like-as

function fooParamInterface(a: BaseI){
  let b = a as AI; // arkts-no-ts-like-as
}
let aParamInterface:BI = {};
fooParamInterface(aParamInterface);

function fooReturnInterface(): BaseI{
  let a: AI = {};
  return a;
}
let bReturnInterface = fooReturnInterface() as BI; // arkts-no-ts-like-as

class Person {
  b: AI = {}
  a: BaseI = this.b;
}
let person = new Person();

let bPerson = person.a as BI; // arkts-no-ts-like-as

class Base1{}
class A1 extends Base1{}
class B1 extends A1{}

let aExtend = new B1();
let bExtend: Base1 = aExtend;
let cExtend = bExtend as A1;

let aExtend2 = new A1();
let bExtend2: Base1 = aExtend2;
let cExtend2 = bExtend2 as B1; // arkts-no-ts-like-as

interface RelabelBaseI {}
class RelabelParentC implements RelabelBaseI {}
class RelabelParentC1 implements RelabelBaseI {}
class RelabelParentC2 implements RelabelBaseI {}
class RelabelParentC3 implements RelabelBaseI {}
class RelabelSonC extends RelabelParentC {}
class RelabelSonC1 extends RelabelParentC {}
class RelabelSonC2 extends RelabelParentC {}
class RelabelSonC3 extends RelabelParentC {}

function takeRelabelParentC(value: RelabelParentC): void {}
function takeRelabelParentC1(value: RelabelParentC1): void {}
function takeRelabelParentC2(value: RelabelParentC2): void {}
function takeRelabelParentC3(value: RelabelParentC3): void {}
function takeRelabelSonC(value: RelabelSonC): void {}
function takeRelabelSonC1(value: RelabelSonC1): void {}
function takeRelabelSonC2(value: RelabelSonC2): void {}
function takeRelabelSonC3(value: RelabelSonC3): void {}

let relabelParentSource: RelabelBaseI = new RelabelParentC();
takeRelabelParentC1(relabelParentSource as RelabelParentC1); // arkts-no-ts-like-as
takeRelabelParentC2(relabelParentSource as RelabelParentC2); // arkts-no-ts-like-as
takeRelabelParentC3(relabelParentSource as RelabelParentC3); // arkts-no-ts-like-as
takeRelabelSonC(relabelParentSource as RelabelSonC); // arkts-no-ts-like-as
takeRelabelSonC1(relabelParentSource as RelabelSonC1); // arkts-no-ts-like-as
takeRelabelSonC2(relabelParentSource as RelabelSonC2); // arkts-no-ts-like-as
takeRelabelSonC3(relabelParentSource as RelabelSonC3); // arkts-no-ts-like-as

let relabelLiteralSource: RelabelBaseI = {};
takeRelabelParentC(relabelLiteralSource as RelabelParentC); // arkts-no-ts-like-as
takeRelabelParentC1(relabelLiteralSource as RelabelParentC1); // arkts-no-ts-like-as
takeRelabelParentC2(relabelLiteralSource as RelabelParentC2); // arkts-no-ts-like-as
takeRelabelParentC3(relabelLiteralSource as RelabelParentC3); // arkts-no-ts-like-as
takeRelabelSonC(relabelLiteralSource as RelabelSonC); // arkts-no-ts-like-as
takeRelabelSonC1(relabelLiteralSource as RelabelSonC1); // arkts-no-ts-like-as
takeRelabelSonC2(relabelLiteralSource as RelabelSonC2); // arkts-no-ts-like-as
takeRelabelSonC3(relabelLiteralSource as RelabelSonC3); // arkts-no-ts-like-as

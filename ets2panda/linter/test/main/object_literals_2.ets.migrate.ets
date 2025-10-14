/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

interface GeneratedObjectLiteralInterface_1 {
    n: number;
    s: string;
}
let obj: GeneratedObjectLiteralInterface_1 = {n: 42, s: 'foo'} // OK in TypeScript, CTE in ArkTS: unknown type of obj.

class C {
  n: number = 0;
  s: string = "";
}

let c1: C = {n: 42, s: 'foo'} // Declaration + initialization: type of the literla is inferred from the type of c1.

let c2: C;
c2 = {n: 42, s: 'foo'}; // Initialization after declaration: type of the literal is inferred from the type of c2.

let c3: Object = {n: 42, s: 'foo'} as C as Object; // Type of the literal is inferred from the 'as' cast.
console.log(c3 instanceof C); // NB! Output is true in ArkTS, but is false in TS.

function foo(c: C) {
  console.log('foo is called');
}

foo({n: 42, s: 'foo'}); // Parsing as an argument: type of the literal is inferred from the type of parameter 'c'

function bar(): C {
  return {n: 42, s: 'foo'}; // Returning from function: type of the literal is inferred from the bar's return type.
}

let cc: C[] = [{n: 1, s: '1'}, {n: 2, s: '2'}]; // Type of the literal is inferred from the type of the array.

class D {
  b: boolean = false;
  c: C = {n: 0, s: ""};
}

let d: D = {
  b: true,
  c: { // Initialization of a field with a literal: type of the literal is inferred from the definition of class D.
    n: 42,
    s: 'foo'
  }
}

// Restrictions of classes that can be initialized with literal
// Default initializable class.
class C1 {
  n: number = 0;
  s?: string;
}

let c4: C1 = {n: 42}; // OK in TS, OK in ArkTS, c.s is null

class C2 {
  s: string;
  constructor(s: string) {
    this.s = "s = " + s;
  }
}

let c5: C2 = {s: 'foo'} // OK in TS, CTE in ArkTS

// All class fields are accessible at the point of initialization.
class C3 {
  private n: number = 0;
  public s: string = '';
}

// CTE in TypeScript, CTE in ArkTS // let c6: C3 = {n: 42, s: 'foo'},

class C4 {
  readonly n: number = 0;
  readonly s: string = '';
}

let c7: C4 = {n: 42, s: 'foo'}; // OK in TS, CTE in ArkTS

// Class is non-abstract
abstract class A {}
let a: A = {}; // OK in TS, CTE in ArkTS

// Class declares no methods, apart from optionally declared constructors and setters.
class C5 {
  n: number = 0;
  s: string = '';
  f() {
    console.log('C5.f is called');
  }
}

let c8: C5 = {n: 42, s: 'foo', f: () => {}} // OK in TS, CTE in ArkTS

// NB! If a class has getters/setters the semantics of initialization differs:
class C6 {
  n: number = 0;
  _s: string = '';
  get s(): string { return this._s; }
  set s(s: string) { this._s = s; }
}

let c9: C6 = {n: 42, _s: 'foo', s: 'bar'}
console.log(c9.s); // TS: 'bar', ArkTS: 'bar'
console.log(c9._s); // TS: 'foo', ArkTS: 'bar'

// Extra fields are not allowed (eventually it means that it's not possible to assign literals to Object / object):
class C7 {
  n: number = 0;
  s: string = '';
}
// TS: CTE, ArtTS: CTE // let c10: C7 = {n: 42, s: '', extra: true},
let o1: Object = {s: '', n: 42} // OK in TS, CTE in ArkTS: no fields 'n' and 's' in Object
let o2: object = {n: 42, s: ''} // OK in TS, CTE in ArkTS: no fields 'n' and 's' in object

// If initialized class is inherited from another class, the base class must also be literal-initializable,
// and initialization should happen from the 'glattened' literal:
class Base {
  n: number = 0;
}

class Derived extends Base {
  s: string = '';
}

let d2: Derived = {n: 42, s: ''};

// Interface should not declare methods, only properties are allowed.
interface I {
  n: number;
  s: string;
  f(): void;
}

let i: I = {n: 42, s: '', f: () => {console.log('I.f is called')}} // OK in TypeScript, CTE in ArkTS

// Interface properties of reference types must be default-initializable:
interface I2 {
  n: number;
  s: string; // Assuming that 'string' is an alias for 'String', and there is String() constructor (what is true).
}

let i2: I2 = {n: 42, s: ''};

interface CompilerOptions {
  strict?: boolean;
  sourcePath?: string;
  targetPath?: string;
}

const options: CompilerOptions = { // OK, as 'targetPath' field is optional
  strict: true,
  sourcePath: './src',
};

// Function parameter with union type.
function funcWithUnionParam(x: C | number): void { }
funcWithUnionParam({ n: 1, s: '2' }) // OK, union type is supported

// issue 13022: property with union type
class UnionProperty {
    a: number | string = 123;
    b?: boolean | number;
}
let u: UnionProperty = { a: 1 }; // OK, union type is supported
u = { a: '2' }; // OK, union type is supported
u = { a: 3, b: true }; // OK, union type is supported

// issue 13022: optional property
class OptionalProp {
  a?: number;
}
let o: OptionalProp = {};
o = {a: 1}; // OK

class OptionalProp2 {
    a?: number;
    b: string;
}
function optProp(a: OptionalProp2) {}
optProp({b: ''}); // OK
optProp({a: 0, b: '1'}); // OK

// Property with inheritance
class E1 {
  x: number;
  y: Base;
}
let e1 : E1 = {
  x: 1,
  y: new Derived()
}

// Property with inheritance through generic type parameter
class E2<T> {
  x: number;
  y: T;
}
let e2 : E2<Base> = {
  x: 1,
  y: new Derived()
}

// Type alias chain to interface
interface ITypeAlias<T> { a: number; b: T }
type ITA<T> = ITypeAlias<T>;
type ITA2<K> = ITA<K>;
let ti: ITA2<string> = { // OK, 'ITA2' is an alias to interface 'ITypeAlias'
    a: 12,
    b: '34'
}

// Type alias chain to class
class CTypeAlias<T> {
    a: number;
    b: T;
}
type CTA<T> = CTypeAlias<T>;
type CTA2<K> = CTA<K>;
let tc: CTA2<string> = { // OK, 'CTA' is an alias to class 'CTypeAlias'
    a: 4,
    b: '4'
}

// issue 13114: Const enum value converted to string/number type.
const enum ATTRIBUTE {
  ROW = 'Row',
  COLUMN = 'Column',
  COLUMN_REVERSE = 'ColumnReverse',
};
const enum GROUP {
  MAIN_DIRECTION = 'MAIN_DIRECTION',
};
enum Orientation {
  Horizontal,
  Vertical
}
class ContainerModuleItem {
  groupName: string = '';
  attributeList: string[] = [];
  attribute: ATTRIBUTE = ATTRIBUTE.COLUMN;
  orientation: number = 0;
}
const FLEX_MODULE: ContainerModuleItem[] = [
  {
    groupName: GROUP.MAIN_DIRECTION,
    attributeList: [ATTRIBUTE.COLUMN, ATTRIBUTE.ROW, ATTRIBUTE.COLUMN_REVERSE],
    attribute: ATTRIBUTE.ROW,
    orientation: Orientation.Horizontal
  }
];

interface I3 {}

class CCl implements I3 {}

class CCl2 extends CCl implements I3 {}

interface I4 {
        a: I3;
        b: I3;
        c: CCl;
        d: CCl2;
}

class DCl {
        constructor(a: I4) {}
}

let c: I4 = {a: new CCl(), b: new CCl2(), c: new CCl2(), d: new CCl2()}

new DCl({a: new CCl(), b: new CCl2(), c: new CCl2(), d: new CCl2()})

let oo1: Object = {}

let oo2: Object = {a: 12}


interface ABC {
    foo: number[]
}

const foo = [];

let a: ABC = {
    foo // should not be scanned
}

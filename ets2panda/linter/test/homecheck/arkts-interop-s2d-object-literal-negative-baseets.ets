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

import { X, Y, foo, bar, createY } from './fixtures/homecheck-interop-s2d-object-literal/ets/ExportEts2';
import { M, N, O, lX, lY, lA } from './fixtures/homecheck-interop-s2d-object-literal/ets/ExportEts1';

interface A {
  x: X
}

interface B {
  b: number;
  c: {
    x: X
  }
}

class C {
  x: X;
  constructor(arg: X) {
    this.x = arg;
  }
}

function test1(x: X): void {}
function test2(a: A): void {}

function declareLocalWithObjectLiteral(): void {
  let x: X = { name: 'abc' };
  let y: Y = { data: 123 };
  let a: A = { x: { name: 'abc' }};
  let b: B = { b: 123, c: { x: { name: 'abc' }}};
  let c: C = new C({ name: 'abc' });
}

function callArgWithObjectLiteral(): void {
  foo({ name: 'efg' });
  const param1 = { data: 456 };
  let b = bar(456, param1);
  test1({ name: 'efg' });
  test2({ x: { name: 'abc' }});
}

function callArgFromOtherFile(): void {
  foo(lX);
  let b = bar(456, lY);
  test1(lX);
  test2(lA);
  test2(lB);
}

let lB = { x: { name: 'efg' }};

function returnWithObjectLiteral(): X {
  return { name: 'qwe' };
}

function returnWithNestedObjectLiteral(): B {
  let b = { b: 123, c: { x: { name: 'abc' }}};
  return b;
}

class Test {
  a: A = { x: { name: 'test' }};
  x: X = new X('xyz');
}

function positiveCase: void {
  let xx: X = new X('xyz');
  foo(new X('xyz'));
  let a: A = { x: xx };

  let y: Y = createY(789);
  bar(789, createY(789));

  let m: M = { m: 'mmm' };
  let n: N = { n: 123 };
  let o: O = { o: 123, c: { n: 123 }}

  let p: P = { p: 123 };
}

function positiveReturnWithInterface(): Y {
  return createY(0);
}

function positiveReturnWithClass(): X {
  return new X('xyz');
}

export interface P {
  P: number;
}

class Node {
  left: Node;
}

// Additional test cases for branch coverage

// Test: object literal in arrow function return
const arrowReturnX = (): X => {
  return { name: 'arrow' };
};

// Test: object literal in method return
class MethodReturn {
  getX(): X {
    return { name: 'method' };
  }
}

// Test: object literal in function expression return
const funcExprReturn = function(): X {
  return { name: 'expr' };
};

// Test: object literal without type annotation (inferred type)
let untypedObj = { name: 'untyped' };

// Test: object literal assigned to variable without type annotation
let inferredX = { name: 'inferred' };

// Test: nested object literal with deeply nested structure
interface DeepNested {
  level1: {
    level2: {
      level3: X;
    }
  }
}
let deep: DeepNested = {
  level1: {
    level2: {
      level3: { name: 'deep' }
    }
  }
};

// Test: object literal as extra argument (beyond param count)
function extraArgTest(): void {
  foo({ name: 'extra' }, { name: 'extra2' });
}

// Test: object literal with non-object property (early exit in processNested)
let mixedObj: B = { b: 123, c: { x: { name: 'mixed' } } };

// Test: object literal in conditional expression
let condObj: X = true ? { name: 'cond' } : { name: 'else' };

// Test: object literal property that is not PropertyAssignment (e.g., shorthand)
let shorthand = { name };
let xVal: X = { name };

// Test: function with no return type annotation returning object literal
function noReturnType() {
  return { name: 'noType' };
}

// Test: object literal in binary expression (assignment)
let assignX: X;
assignX = { name: 'assign' };

// Test: object literal in as expression
let asX = { name: 'as' } as X;

// Test: call with object literal where param has no valueDeclaration
function callWithNoValueDecl(): void {
  test1({ name: 'noValDecl' });
}
// Additional cases for branch coverage

// Assignment with this reference (right op is ArkThisRef)
class TestThis {
  x: X;
  constructor() {
    this.x = this;
  }
}

// Assignment where left type is not ClassType (number)
let numAssign: number = 123;

// Assignment where right type class lookup returns null
let brokenAssign: X = { name: 'test' };

// Constructor return value (should be skipped)
class TestConstructor {
  x: X;
  constructor(arg: X) {
    this.x = arg;
  }
}

// Return type is not ClassType (number return)
function returnNumber(): number {
  return 42;
}

// Circular class field reference
interface CircularX { x: CircularX; }
class CircularImpl implements CircularX {
  x: CircularX = this;
}

// Field with primitive type (not ClassType)
class PrimitiveField {
  count: number = 0;
}

// Test: this = thisRef assignment (should be skipped)
class ThisRefAssign {
  x: X;
  constructor() {
    this.x = this;
  }
}

// Test: assignment where left type is not ClassType (number)
function assignNonClassType(): void {
  let num: number = 123;
}

// Test: invoke with null method (unknown function call)
function callUnknownFunc(): void {
  unknownFunc({ name: 'test' });
}

// Test: invoke with non-ClassType param
function callWithPrimitive(): void {
  foo(42);
}

// Test: invoke with extra args beyond param count
function callWithExtraArgs(): void {
  foo({ name: 'extra' }, { name: 'extra2' }, { name: 'extra3' });
}

// Test: method returning non-ClassType (number)
function returnNonClass(): number {
  return 42;
}

// Test: method with no return statement
function noReturn(): void {
  let x = 1;
}

// Test: circular class reference
class CircRefA {
  ref: CircRefB = {} as CircRefB;
}
class CircRefB {
  ref: CircRefA = {} as CircRefA;
}
let circTest: CircRefA = { ref: { ref: { ref: {} } } };

// Test: field with primitive type
class PrimitiveFieldClass {
  count: number = 0;
  name: string = '';
}

// Test: this = thisRef in regular method (not constructor)
class ThisRefMethod {
  x: X;
  setThis(): void {
    this.x = this;
  }
}

// Test: call to unknown method (method === null)
function callUnknown(): void {
  nonExistentFunc({ name: 'test' });
}

// Test: invoke with primitive param (param type not ClassType)
function fooWithPrimitive(): void {
  foo(42);  // foo expects X, but 42 is number
}

// Test: invoke with primitive arg (arg type not ClassType)
function barWithPrimitive(): void {
  bar(42, { data: 123 });
}

// Test: constructor method (should be skipped in checkReturnWithObjectLiteral)
class ConstructorTest {
  x: X;
  constructor(arg: X) {
    this.x = arg;
  }
}

// Test: method returning non-ClassType (number)
function returnNumber2(): number {
  return 42;
}

// Test: method returning void
function returnVoid(): void {
  let x = 1;
}

// Test: circular class reference via fields
class CircA {
  b: CircB = {} as CircB;
}
class CircB {
  a: CircA = {} as CircA;
}
let circTest2: CircA = { b: { a: { b: {} } } };

// Test: field with number type (fieldType not ClassType)
class NumberField {
  count: number = 0;
}

// Test: field assigned with number (rightOp not ClassType)
class NumberFieldAssign {
  count: number = 42;
}

// Test: return statement with non-ClassType value
function returnNonClassValue(): X {
  return 42 as unknown as X;
}

// Test: return null from X-returning function (valueType not ClassType)
function returnNull(): X {
  return null;
}

// Test: return undefined from X-returning function
function returnUndefined(): X {
  return undefined;
}

// Test: call with fewer args than params (index >= args.length)
function callWithFewerArgs(): void {
  bar(42);  // bar expects (number, Y) but only 1 arg
}

// Test: assignment where left type is from .d.ts SDK (class not in scene as ArkClass)
import { Callback } from '@ohos.base';
let cb: Callback<number> = () => {};

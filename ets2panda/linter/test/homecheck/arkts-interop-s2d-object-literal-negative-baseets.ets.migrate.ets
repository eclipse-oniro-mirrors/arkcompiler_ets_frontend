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
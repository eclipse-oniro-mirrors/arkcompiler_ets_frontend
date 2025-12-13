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

function test1() {
  let a: number, b: number, c: number;
  [a, b, c] = [1, 2, 3];

  arktest.assertEQ(a, 1);
  arktest.assertEQ(b, 2);
  arktest.assertEQ(c, 3);
}

function test2() {
  let a: number, b: number, c: number;
  [a, b, ] = [1, 2, 3];

  arktest.assertEQ(a, 1);
  arktest.assertEQ(b, 2);
  arktest.assertEQ(c, 0);
}

function test3() {
  let a: number;
  [a, , ] = [1, 2, 3]; // should work
  arktest.assertEQ(a, 1);
}

function test4() {
  let a: number, c: number;
  [a, , c] = [1, 2, 3]; // should work
  arktest.assertEQ(a, 1);
  arktest.assertEQ(c, 3);
}

function test5() {
  let a: number, c: number;
  let y = [a,  , c] = [1,2,3];

  arktest.assertEQ(y[0], 1);
  arktest.assertEQ(y[1], 2);
  arktest.assertEQ(y[2], 3);
}

function foo() : number[] {
  return [1,2,3];
}

function testRangeError() {
  let a: number, b: number, c: number, d: number;
  try {
    [a, b, c, d] = foo(); // ArrayIndexOutOfBoundsError
  } catch (e) {
    arktest.assertEQ(a, 1);
    arktest.assertEQ(b, 2);
    arktest.assertEQ(c, 3);
    arktest.assertEQ(d, 0);
  }
}

function testTooFewVariables() {
  let a: number, b: number, c: number;
  [a, b, c] = [1, 2, 3, 4, 5, 6];

  arktest.assertEQ(a, 1);
  arktest.assertEQ(b, 2);
  arktest.assertEQ(c, 3);
}

function testUnionTypes() {
  let a: number|undefined, b: number, c: number, d: number;
  [a, b, c] = [1, 2, 3];

  arktest.assertEQ(a, 1);
  arktest.assertEQ(b, 2);
  arktest.assertEQ(c, 3);
}

function testArrayIdentifiers() {
  let a: number|undefined, b: number, c: number, d: number;

  let fixedArray: FixedArray<number> = [1,2,3];
  [b, c, d] = fixedArray;
  arktest.assertEQ(a, undefined);
  arktest.assertEQ(b, 1);
  arktest.assertEQ(c, 2);
  arktest.assertEQ(d, 3);

  let arr: number[] = [4, 5, 6];
  [a,b,c] = arr;
  arktest.assertEQ(a, 4);
  arktest.assertEQ(b, 5);
  arktest.assertEQ(c, 6);
}

function main() {
  test1();
  test2();
  test3();
  test4();
  test5();
  testRangeError();
  testTooFewVariables();
  testArrayIdentifiers();
}

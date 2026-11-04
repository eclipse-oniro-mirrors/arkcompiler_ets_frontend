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

function testTuple1() {
  let tuple1: [number, byte] = [1, 2];

  let x: number;
  let y: byte;
  [x, y] = tuple1;

  arktest.assertEQ(x, 1);
  arktest.assertEQ(y, 2);
}

class A {}

function testTuple2() {
  let num: Int, str: String, obj: A;
  let tuple: [Int, String, A] = [1, "2", new A()];

  [num, str, obj ] = tuple;

  arktest.assertEQ(num, 1);
  arktest.assertEQ(str, "2");
  arktest.assertTrue(obj instanceof A);
}

function foo(a: number, b: string) : void {}
function bar(t: [number, string]) : void {}

function testTuple3() {
  let tuple1: [number, string] = [1, "a"];
  let tuple2: readonly [number, string] = [1, "abc"];

  let x: number;
  let y: string;
  [x, y] = tuple1;

  arktest.assertEQ(x, 1);
  arktest.assertEQ(y, "a");

  [x, y] = tuple2;
  arktest.assertEQ(x, 1);
  arktest.assertEQ(y, "abc");
}

function testTuple4() {
  let tuple1: [number, string] = [1, "a"];

  let x: number;
  let y: string = "";

  [,] = tuple1;
  arktest.assertEQ(x, 0);
  arktest.assertEQ(y, "");

  [x,] = tuple1;
  arktest.assertEQ(x, 1);
  arktest.assertEQ(y, "");

  [,y] = tuple1;
  arktest.assertEQ(y, "a");
}

function testTuple5() {
  let a: number;
  let b: string;
  let y = [a,b] = [1, "str"];

  arktest.assertEQ(a, 1);
  arktest.assertEQ(b, "str");
  arktest.assertEQ(y[0], 1);
  arktest.assertEQ(y[1], "str");
}

function main() {
  testTuple1();
  testTuple2();
  testTuple3();
  testTuple4();
  testTuple5();
}

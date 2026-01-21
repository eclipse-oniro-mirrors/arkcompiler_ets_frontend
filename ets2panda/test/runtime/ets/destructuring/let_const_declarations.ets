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

let MY_GLOBAL = false;

function foo(): FixedArray<number> {
  return [4,5,6];
}

function bar(): [number, string] {
    return [42, "abc"];
}

function getNum(): Int {
  return 1024;
}

function setGlobal(): Int {
  MY_GLOBAL = true;
  return 0;
}

class A {
  i: number = 0;
  array: Array<number> = [1, 2, 3];
  tuple: [number, string] = [42, "abc"];
  get a() : number[] {
    return [this.i++, this.i++];
  }
}

function main() {
  let [a, b, c] = [1, 2, 3];
  arktest.assertEQ(a, 1);
  arktest.assertEQ(b, 2);
  arktest.assertEQ(c, 3);

  let [d, e, f] = foo();
  arktest.assertEQ(d, 4);
  arktest.assertEQ(e, 5);
  arktest.assertEQ(f, 6);

  const [g, h, i] = [7, 8, 9];
  arktest.assertEQ(g, 7);
  arktest.assertEQ(h, 8);
  arktest.assertEQ(i, 9);

  MY_GLOBAL = false;
  arktest.assertFalse(MY_GLOBAL);
  let [j, k, l] = [10, 11, getNum(), setGlobal()];
  arktest.assertEQ(j, 10);
  arktest.assertEQ(k, 11);
  arktest.assertEQ(l, 1024);
  arktest.assertTrue(MY_GLOBAL);

  // Tuple test cases
  let tuple:[number, string] = [1,"asd"];
  let [num, str] = tuple;
  arktest.assertEQ(num, 1);
  arktest.assertEQ(str, "asd");

  const [num2, str2] = bar();
  arktest.assertEQ(num2, 42);
  arktest.assertEQ(str2, "abc");

  let [aa, bb, cc] = new A().array;
  arktest.assertEQ(aa, 1);
  arktest.assertEQ(bb, 2);
  arktest.assertEQ(cc, 3);

  let [xx, yy] = new A().tuple;
  arktest.assertEQ(xx, 42);
  arktest.assertEQ(yy, "abc");

  let o = new A();
  let [a1, a2] = o.a;
  arktest.assertEQ(a1, 0);
  arktest.assertEQ(a2, 1);

  let isCaught = 0;
  try {
    let [b1, b2, b3, b4] = foo();
  } catch (e) {
    isCaught = 1;
  }
  arktest.assertEQ(isCaught, 1);

  let [m, n] = foo();
  arktest.assertEQ(m, 4);
  arktest.assertEQ(n, 5);
}

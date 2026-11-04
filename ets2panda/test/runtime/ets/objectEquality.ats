/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

class A {}

function main(): int {
  classEquality();
  arrayEquality();
  functiontypeEquality();
  nullEquality();
  return 0;
}

function classEquality(): void {
  let a = new A;
  let b = a;
  arktest.assertEQ(a, b)
  b = new A;
  arktest.assertEQ(a, b)
}

function arrayEquality(): void {
  let a : int[] = null;
  let b = a;
  arktest.assertEQ(a, b)
}

function functiontypeEquality(): void {
  let a : (x : double, y : double) => double;
  let b = a;
  arktest.assertEQ(a, b)
}

function nullEquality(): void {
  let a = new A;
  let hit : int = 0;

  if (a == null) {
    arktest.assertTrue(false)
  } else {
    hit = 1;
  }
  arktest.assertEQ(hit, 1)

  if (a != null) {
    hit = 2;
  } else {
    arktest.assertTrue(false)
  }
  arktest.assertEQ(hit, 2)

  if (null == a) {
    arktest.assertTrue(false)
  } else {
    hit = 3;
  }
  arktest.assertEQ(hit, 3)

  if (null != a) {
    hit = 4;
  } else {
    arktest.assertTrue(false)
  }
  arktest.assertEQ(hit, 4)


  a = null;

  if (a != null) {
    arktest.assertTrue(false)
  } else {
    hit = 1;
  }
  arktest.assertEQ(hit, 1)

  if (a == null) {
    hit = 2;
  } else {
    arktest.assertTrue(false)
  }
  arktest.assertEQ(hit, 2)

  if (null != a) {
    arktest.assertTrue(false)
  } else {
    hit = 3;
  }
  arktest.assertEQ(hit, 3)

  if (null == a) {
    hit = 4;
  } else {
    arktest.assertTrue(false)
  }
  arktest.assertEQ(hit, 4)
}

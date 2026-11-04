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

function foo(a: int = 3, b: int = 5): int {
  return a * 10 + b;
}

class C {
  x: int;
  y: int;

  constructor (a: int = 3, b: int = 5) {
    this.x = a;
    this.y = b;
  }
}

function main(): void {

  let res = foo(5, 7);
  arktest.assertEQ(res, 57)

  res = foo(7);
  arktest.assertEQ(res, 75)

  res = foo();
  arktest.assertEQ(res, 35)

  let c0 = new C();
  arktest.assertTrue(c0.x == 3 && c0.y == 5)

  let c1 = new C(7);
  arktest.assertTrue(c1.x == 7 && c1.y == 5)

  let c2 = new C(5, 7);
  arktest.assertTrue(c2.x == 5 && c2.y == 7)
}

/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

enum Color { Red, Green, Blue }

class A {
  public c: Color;
  public constructor(c_: Color) {
    this.c = c_;
  }
}

function colorToInt(c: Color): int {
  return c as int;
}

function main(): void {
  let red: Color = Color.Red;
  arktest.assertEQ(red, Color.Red);
  arktest.assertEQ(colorToInt(red), 0);

  let red2: Color = red;
  arktest.assertEQ(red, red2);
  arktest.assertEQ(Color.Red, red2);

  let red_int: int = red as int;
  arktest.assertEQ(red_int, 0);
  arktest.assertEQ(red_int, Color.Red as int);

  let a: A = new A(red);
  arktest.assertEQ(a.c, Color.Red);

  switch(red) {
    case Color.Green:
      arktest.assertTrue(false)
      break;
    case Color.Red:
      arktest.assertTrue(true)
      break;
    default:
      arktest.assertTrue(false)
  }

  switch(Color.Blue) {
    case Color.Green:
    case Color.Red:
      arktest.assertTrue(false)
      break;
    case Color.Blue:
      arktest.assertTrue(true)
      break;
    default:
      arktest.assertTrue(false)
  }
}


/* @@? 26:10 Warning Warning W0030: Enum cast is deprecated. Cast support from 'Color' to 'Int' will be removed. */
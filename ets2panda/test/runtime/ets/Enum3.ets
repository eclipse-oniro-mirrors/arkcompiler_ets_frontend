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

enum Color { Red, Green, Blue }

function main(): void {
  let x: int = Color.Red.valueOf();
  arktest.assertEQ(x, 0)

  let blue = Color.Blue;
  let str = blue.getName();
  arktest.assertEQ("Blue", str)
  str = blue.toString();
  arktest.assertEQ("2", str)


  let values = Color.values();
  arktest.assertEQ(values.length, 3)
  arktest.assertEQ(values[0], Color.Red)
  arktest.assertEQ(values[1], Color.Green)
  arktest.assertEQ(values[2], Color.Blue)

  let red: Color = Color.Red;

  try {
    red = Color.getValueOf("Red");
  } catch (e) {}

  arktest.assertEQ(red, Color.Red)

  try {
    let yellow: Color = Color.getValueOf("Yellow");
    arktest.assertTrue(false)
  } catch (e: Exception) {
    arktest.assertTrue((e as Object).toString().startsWith("No enum constant Color.Yellow"))
  } catch (e) {}

  let one: int = 1;
  let green = one as Color;
  arktest.assertEQ(green, Color.Green)

  try {
    let x = 5 as Color;
    arktest.assertTrue( false)
  } catch (e: Error) {
    arktest.assertTrue( (e as Object).toString().startsWith("Error: No enum Color with value 5"))
  }

  arktest.assertEQ(2 as Color as int, 2)
  arktest.assertEQ(Color.Blue as int as Color, Color.Blue)
  arktest.assertEQ((Color.Red as int + 1) as Color, (Color.Blue as int - 1) as Color)
}

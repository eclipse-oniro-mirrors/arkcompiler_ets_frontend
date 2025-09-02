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

enum Color { Green = "red" , Red = "blue", Blue = "green" }

function main(): void {

  try {
    let name = Color.Blue.getName();
    let value:string = Color.getValueOf("Blue").toString();
    arktest.assertEQ(name, "Blue")
    arktest.assertEQ(value, "green")
  } catch (e) {
    arktest.assertTrue((e as Object).toString().startsWith("No enum constant Color.Blue"))
  }

  let x = Color.Red.valueOf();
  arktest.assertEQ(x, "blue")

  let blue = Color.Blue;
  let str = blue.getName();
  arktest.assertEQ( "Blue", str)
  arktest.assertEQ( blue.valueOf(), blue.toString())

  let values = Color.values();
  arktest.assertEQ(values.length, 3)
  arktest.assertEQ(values[1], Color.Red)
  arktest.assertEQ(values[0], Color.Green)
  arktest.assertEQ(values[2], Color.Blue)

  let red1: Color = Color.Red;
  let red2: Color = Color.Green;
  arktest.assertEQ(red2 as string, "red")
  arktest.assertEQ(red2.valueOf(), "red")
  arktest.assertEQ(red2.getName(), "Green")
  arktest.assertEQ(red2.toString(), "red")

  try {
    red2 = Color.getValueOf("Red");
  } catch (e) {
    arktest.assertTrue( (e as Object).toString().startsWith("No enum constant Color.Red"))
  }

  arktest.assertEQ(red1 as string, "blue")
  arktest.assertEQ(red1.valueOf(), "blue")
  arktest.assertEQ(red1.getName(), "Red")
  arktest.assertEQ(red1.toString(), "blue")

  arktest.assertEQ(red2 as string, "blue")
  arktest.assertEQ(red2.valueOf(), "blue")
  arktest.assertEQ(red2.getName(), "Red")
  arktest.assertEQ(red2.toString(), "blue")

  arktest.assertEQ(red1, Color.Red)
  arktest.assertEQ(red2, Color.Red)
  arktest.assertEQ(red2, red1)

  let ord: string = "green";
  blue = ord as Color;
  arktest.assertEQ(blue as string, "green")
  arktest.assertEQ(blue.valueOf(), "green")
  arktest.assertEQ(blue.getName(), "Blue")
  arktest.assertEQ(blue.toString(), "green")

  try {
    let yellow: Color = Color.getValueOf("Yellow");
    arktest.assertTrue(false)
  } catch (e: Exception) {
    arktest.assertTrue((e as Object).toString().startsWith("No enum constant Color.Yellow"))
  } catch (e) {}

  ord = "red";
  let green: Color = ord as Color;
  arktest.assertEQ( green, Color.Green)

  try {
    let x = "a" as Color;
    arktest.assertTrue( false)
  } catch (e: Error) {
    arktest.assertTrue( (e as Object).toString().startsWith("Error: No enum Color with value a"))
  }

  arktest.assertEQ( "red" as Color as string, "red")
  arktest.assertEQ( Color.Blue as string as Color, Color.Blue)
  try {
    arktest.assertEQ( (Color.Green as string) as Color, Color.getValueOf("red"))
  } catch (e) {
  }
}

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

enum Color { 
	Red = "aaa",
	Yellow = "bbb",
}
enum ColorInt {
	A = 1111,
	B = 2222
}

function foo(b : boolean) : int | Color | string | ColorInt {
	if (b) {
		return 1111;
	}
	return Color.Red;
}

function foo1(b : boolean) : int | Color | string | ColorInt {
	if (b) {
		return "bbb";
	}
	return ColorInt.B;
}

function testOptional(b ?: ColorInt | int | string) : int {
    const v1 : int = b ? b as int : 0;
    return v1;
}

function main() {
  const v1: Color = foo(false) as Color
	const v2: ColorInt = foo(true) as ColorInt
	const v3: string = foo(false) as string
	const v4: int = foo1(false) as int
	const v5: Color = foo1(true) as Color
	const v6: ColorInt = foo1(false) as ColorInt

	arktest.assertEQ(v1, Color.Red)
	arktest.assertEQ(v2, ColorInt.A)
	arktest.assertEQ(v3, "aaa")
	arktest.assertEQ(v4, 2222)
	arktest.assertEQ(v5, Color.Yellow)
	arktest.assertEQ(v6, ColorInt.B)

    const a : Color | string | boolean = Color.Red
    const b : string | Color | boolean = Color.Red
    arktest.assertEQ(a, Color.Red)
    arktest.assertEQ(b, Color.Red)

    arktest.assertEQ(testOptional(ColorInt.A), 1111)
    arktest.assertEQ(testOptional(), 0);
}

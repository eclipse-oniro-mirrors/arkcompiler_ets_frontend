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

enum Color { Red, Green, Blue }

class ColorEnumDelegate {
    private static values_: Color[] = [Color.Red, Color.Green, Color.Blue];
    private static names_: String[] = ["Red", "Green", "Blue"];

    public static values(): Color[] { return ColorEnumDelegate.values_; }

    public static ordinal(x: Color): int { return x as int; }

    public static toString(x: Color): String { return ColorEnumDelegate.names_[x as int]; }

    public static valueOf1(name: String): Color {
        for (let i = 0; i < ColorEnumDelegate.values_.length; i++) {
            if (ColorEnumDelegate.names_[i] == name) { return ColorEnumDelegate.values_[i]; }
        }

        arktest.assertTrue(false, "No enum constant Color")
        return Color.Red;
    }

    public static valueOf2(ordinal: int): Color {
        if (0 <= ordinal && ordinal < ColorEnumDelegate.values_.length) {
            return ColorEnumDelegate.values_[ordinal];
        }

        arktest.assertTrue(false, "No enum constant Color")
        return Color.Red;
    }

    static overload valueOf{ valueOf1, valueOf2 }
}

function main(): void {
    let red: Color = Color.Red;
    arktest.assertEQ(ColorEnumDelegate.toString(red), "Red")

    arktest.assertEQ(ColorEnumDelegate.ordinal(red), 0)
    arktest.assertEQ(ColorEnumDelegate.ordinal(Color.Green), 1)

    arktest.assertEQ(ColorEnumDelegate.valueOf(2), Color.Blue)
    arktest.assertEQ(ColorEnumDelegate.valueOf(ColorEnumDelegate.ordinal(Color.Red)), red)

    arktest.assertEQ(ColorEnumDelegate.valueOf("Green"), Color.Green)
    arktest.assertEQ(ColorEnumDelegate.valueOf(ColorEnumDelegate.toString(Color.Blue)), Color.Blue)
}

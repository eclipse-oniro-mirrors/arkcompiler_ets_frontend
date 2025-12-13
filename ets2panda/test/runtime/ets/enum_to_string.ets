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

enum Enum: double {
    Red = 1.5,
    Green = 10.3,
    Blue = 7.6,
    First = -214748.3648,
    Second = 123.45,
    Third = 2147483.647,
    One = 1.0,
    one = 1.0,
    oNe = 1.0
}

function main(): void {
    arktest.assertTrue(Enum.Red.valueOf() instanceof double)
    arktest.assertEQ(Enum.Red.toString(), new Double(1.5).toString())
    arktest.assertEQ(Enum.Red.toString(), Enum.Red.valueOf().toString())
    arktest.assertTrue(Enum.Green.valueOf() instanceof double)
    arktest.assertEQ(Enum.Green.toString(), new Double(10.3).toString())
    arktest.assertEQ(Enum.Green.toString(), Enum.Green.valueOf().toString())
    arktest.assertTrue(Enum.Blue.valueOf() instanceof double)
    arktest.assertEQ(Enum.Blue.toString(), new Double(7.6).toString())
    arktest.assertEQ(Enum.Blue.toString(), Enum.Blue.valueOf().toString())
    arktest.assertTrue(Enum.First.valueOf() instanceof double)
    arktest.assertEQ(Enum.First.toString(), new Double(-214748.3648).toString())
    arktest.assertEQ(Enum.First.toString(), Enum.First.valueOf().toString())
    arktest.assertTrue(Enum.Second.valueOf() instanceof double)
    arktest.assertEQ(Enum.Second.toString(), new Double(123.45).toString())
    arktest.assertEQ(Enum.Second.toString(), Enum.Second.valueOf().toString())
    arktest.assertTrue(Enum.Third.valueOf() instanceof double)
    arktest.assertEQ(Enum.Third.toString(), new Double(2147483.647).toString())
    arktest.assertEQ(Enum.Third.toString(), Enum.Third.valueOf().toString())
    arktest.assertTrue(Enum.One.valueOf() instanceof double)
    arktest.assertEQ(Enum.One.toString(), new Double(1.0).toString())
    arktest.assertEQ(Enum.One.toString(), Enum.One.valueOf().toString())
    arktest.assertTrue(Enum.one.valueOf() instanceof double)
    arktest.assertEQ(Enum.one.toString(), new Double(1.0).toString())
    arktest.assertEQ(Enum.one.toString(), Enum.one.valueOf().toString())
    arktest.assertTrue(Enum.oNe.valueOf() instanceof double)
    arktest.assertEQ(Enum.oNe.toString(), new Double(1.0).toString())
    arktest.assertEQ(Enum.oNe.toString(), Enum.oNe.valueOf().toString())
}

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

enum ByteEnum: byte { A = 1 }
enum ShortEnum: short { A = 1 }
enum IntEnum: int { A = 1 }
enum LongEnum: long { A = 1 }
enum FloatEnum: float { A = 1.0f }
enum DoubleEnum: double { A = 1.0 }
enum StringEnum { A = "a" }

@interface TestAnno {
    byteField: ByteEnum = ByteEnum.A
    shortField: ShortEnum = ShortEnum.A
    intField: IntEnum = IntEnum.A
    longField: LongEnum = LongEnum.A
    floatField: FloatEnum = FloatEnum.A
    doubleField: DoubleEnum = DoubleEnum.A
    stringField: StringEnum = StringEnum.A
    byteArr: ByteEnum[] = [ByteEnum.A, ByteEnum.A]
    shortArr: ShortEnum[] = [ShortEnum.A, ShortEnum.A]
    intArr: IntEnum[] = [IntEnum.A, IntEnum.A]
    longArr: LongEnum[] = [LongEnum.A, LongEnum.A]
    floatArr: FloatEnum[] = [FloatEnum.A, FloatEnum.A]
    doubleArr: DoubleEnum[] = [DoubleEnum.A, DoubleEnum.A]
    stringArr: StringEnum[] = [StringEnum.A, StringEnum.A]
}

@TestAnno({
    byteField: ByteEnum.A,
    shortField: ShortEnum.A,
    intField: IntEnum.A,
    longField: LongEnum.A,
    floatField: FloatEnum.A,
    doubleField: DoubleEnum.A,
    stringField: StringEnum.A,
    byteArr: [ByteEnum.A, ByteEnum.A],
    shortArr: [ShortEnum.A, ShortEnum.A],
    intArr: [IntEnum.A, IntEnum.A],
    longArr: [LongEnum.A, LongEnum.A],
    floatArr: [FloatEnum.A, FloatEnum.A],
    doubleArr: [DoubleEnum.A, DoubleEnum.A],
    stringArr: [StringEnum.A, StringEnum.A]
})
class A {}

function main(): int {
    arktest.assertTrue(true)
    return 0
}

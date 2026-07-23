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

enum ByteEnum: byte {
    FIRST = 127 - 1,
    SECOND,
    THIRD,
}

enum ShortEnum: short {
    FIRST = 32767 - 1,
    SECOND,
    THIRD,
}

function main(): void {
    let firstByte = ByteEnum.FIRST;
    let secondByte = ByteEnum.SECOND;
    let thirdByte = ByteEnum.THIRD;
    let byteV1: byte = 127 - 1
    let byteV2: byte = byteV1
    byteV2++
    let byteV3: byte = byteV2
    byteV3++
    arktest.assertTrue(firstByte instanceof ByteEnum)
    arktest.assertTrue(secondByte instanceof ByteEnum)
    arktest.assertTrue(thirdByte instanceof ByteEnum)
    arktest.assertTrue(firstByte.valueOf() == byteV1)
    arktest.assertTrue(secondByte.valueOf() == byteV2)
    arktest.assertTrue(thirdByte.valueOf() == byteV3)

    let firstShort = ShortEnum.FIRST;
    let secondShort = ShortEnum.SECOND;
    let thirdShort = ShortEnum.THIRD;
    let shortV1: short = 32767 - 1
    let shortV2: short = shortV1
    shortV2++
    let shortV3: short = shortV2
    shortV3++
    arktest.assertTrue(firstShort instanceof ShortEnum)
    arktest.assertTrue(secondShort instanceof ShortEnum)
    arktest.assertTrue(thirdShort instanceof ShortEnum)
    arktest.assertTrue(firstShort.valueOf() == shortV1)
    arktest.assertTrue(secondShort.valueOf() == shortV2)
    arktest.assertTrue(thirdShort.valueOf() == shortV3)
}

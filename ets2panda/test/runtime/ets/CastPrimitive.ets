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

function main(): void {
  let a : double = 2147483649.0;    // 2^31 + 1
  arktest.assertEQ(Double.toFloat(a), 2147483648)  // rounded
  arktest.assertEQ(Double.toLong(a), 2147483649)
  arktest.assertEQ(Double.toInt(a), 2147483647  )  // 2^31 - 1 == MAX_INT == 0xFFFFFFFF
  arktest.assertEQ(Double.toShort(a), -1         ) // 0xFFFF
  arktest.assertEQ(Double.toByte(a), -1           )// 0xFF

  a = -2147483649.0                 // 2^31 + 1
  arktest.assertEQ(Double.toFloat(a), -2147483648) // rounded
  arktest.assertEQ(Double.toLong(a), -2147483649)
  arktest.assertEQ(Double.toInt(a), -2147483648)   // -2^31 == MIN_INT == 0x10000000
  arktest.assertEQ(Double.toShort(a), 0)
  arktest.assertEQ(Double.toByte(a), 0)

  let b : float = 70000.9921875f;
  arktest.assertEQ(Float.toDouble(b), 70000.9921875)
  arktest.assertEQ(Float.toLong(b), 70000  )         // rounded, 70000 == 0x11170
  arktest.assertEQ(Float.toInt(b), 70000)
  arktest.assertEQ(Float.toShort(b), 4464)           // 4464 == 0x1170
  arktest.assertEQ(Float.toByte(b), 112)             // 112 == 0x70

  let c : long = 1193046;              // 1193046 == 0x123456
  arktest.assertEQ(Long.toInt(c), 1193046)
  arktest.assertEQ(Long.toShort(c), 13398)          // 13398 == 0x3456
  arktest.assertEQ(Long.toChar(c), c'\u3456')
  arktest.assertEQ(Long.toByte(c), 86)              // 86 == 0x56

  let d : int = 126977;                // 65537 == 0x1F001
  arktest.assertEQ(Int.toShort(d), -4095)          // -4095 == 0xF001
  arktest.assertEQ(Int.toChar(d), c'\uF001')
  arktest.assertEQ(Int.toByte(d), 1)               // 1 == 0x01

  let e : short = -30875;              // -30875 == 0x8765
  arktest.assertEQ(Short.toDouble(e), -30875.0)
  arktest.assertEQ(Short.toFloat(e), -30875.0)
  arktest.assertEQ(Short.toLong(e), -30875)          // -30875 == 0xFFFFFFFFFFFF8765
  arktest.assertEQ(Short.toInt(e), -30875)           // -30875 == 0xFFFF8765
  arktest.assertEQ(Short.toChar(e), c'\u8765')
  arktest.assertEQ(Short.toByte(e), 101)             // 101 == 0x65

  let f : char = c'\uF001';
  arktest.assertEQ(Char.toDouble(f), 61441.0)
  arktest.assertEQ(Char.toFloat(f), 61441.0)
  arktest.assertEQ(Char.toLong(f), 61441)            // 61441 == 0x000000000000F001
  arktest.assertEQ(Char.toInt(f), 61441)             // 61441 == 0x0000F001
  arktest.assertEQ(Char.toShort(f), (0xf001).toShort()) // -4095 == 0xF001
  arktest.assertEQ(Char.toShort(f), -4095)

  let g : byte = -128;
  arktest.assertEQ(Byte.toDouble(g), -128.0)
  arktest.assertEQ(Byte.toFloat(g), -128.0)
  arktest.assertEQ(Byte.toLong(g), -128)
  arktest.assertEQ(Byte.toInt(g), -128)
  arktest.assertEQ(Byte.toShort(g), -128)
  arktest.assertEQ(Byte.toChar(g), c'\uFF80')
  arktest.assertEQ(Int.toByte(-128), -128)
  arktest.assertEQ(Int.toByte(-129), 127)

  let i : boolean = true;
  arktest.assertEQ(i, true)

  i = false;
  arktest.assertEQ(i, false)

  arktest.assertEQ(Double.toByte(4294967296.0), -1)
  arktest.assertEQ(Double.toShort(4294967296.0), -1)
  arktest.assertEQ(Double.toInt(4294967296.0), Int.MAX_VALUE)
  arktest.assertEQ(Double.toLong(4294967296.0), 4294967296)
  arktest.assertEQ(Double.toByte(-4294967296.0), 0)
  arktest.assertEQ(Double.toShort(-4294967296.0), 0)
  arktest.assertEQ(Double.toInt(-4294967296.0), Int.MIN_VALUE)
  arktest.assertEQ(Double.toLong(-4294967296.0), -4294967296)

  return;
}

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

function test(t: boolean): boolean | long[] {
  if (t) {
    return true
  }
  let c: long[] = [4, 6]
  return c
}
function test1(t: int): Object | null {
  if (t) {
    return new Object();
  }
  return null
}
function test2(t: int): Object | undefined {
  if (!t) {
    return new Object();
  }
  return undefined
}

enum Color { Red, Green, Blue }

function main() {
  // for identifiers
  // ---------------
  // ids with numbers (primitive, boxed and objects)

  let x00: boolean
  let x01: byte
  let x02: char
  let x03: short
  let x04: int
  let x05: long
  let x06: float
  let x07: double
  let x08: bigint = 123456789n
  arktest.assertEQ(typeof x00, "boolean")
  arktest.assertEQ(typeof x01, "byte")
  arktest.assertEQ(typeof x02, "char")
  arktest.assertEQ(typeof x03, "short")
  arktest.assertEQ(typeof x04, "int")
  arktest.assertEQ(typeof x05, "long")
  arktest.assertEQ(typeof x06, "float")
  arktest.assertEQ(typeof x07, "number")
  arktest.assertEQ(typeof x08, "bigint")

  let x000: Boolean = new Boolean()
  let x001: Byte = new Byte()
  let x002: Char = new Char()
  let x003: Short = new Short()
  let x004: Int = new Int()
  let x005: Long = new Long()
  let x006: Float = new Float()
  let x007: Double = new Double()
  let x008: BigInt = new BigInt()
  arktest.assertEQ(typeof x000, "boolean")
  arktest.assertEQ(typeof x001, "byte")
  arktest.assertEQ(typeof x002, "char")
  arktest.assertEQ(typeof x003, "short")
  arktest.assertEQ(typeof x004, "int")
  arktest.assertEQ(typeof x005, "long")
  arktest.assertEQ(typeof x006, "float")
  arktest.assertEQ(typeof x007, "number")
  arktest.assertEQ(typeof x008, "bigint")

  let x0 = new Boolean()
  let x1 = new Byte()
  let x2 = new Char()
  let x3 = new Short()
  let x4 = new Int()
  let x5 = new Long()
  let x6 = new Float()
  let x7 = new Double()
  let x8 = new BigInt()
  arktest.assertEQ(typeof x0, "boolean")
  arktest.assertEQ(typeof x1, "byte")
  arktest.assertEQ(typeof x2, "char")
  arktest.assertEQ(typeof x3, "short")
  arktest.assertEQ(typeof x4, "int")
  arktest.assertEQ(typeof x5, "long")
  arktest.assertEQ(typeof x6, "float")
  arktest.assertEQ(typeof x7, "number")
  arktest.assertEQ(typeof x8, "bigint")

  // ---------------
  // ids with literals

  let l0 = "123"
  let l1 = null
  let l2 = undefined
  let l3 = true
  let l4 = false
  arktest.assertEQ(typeof l0, "string")
  arktest.assertEQ(typeof l1, "object")
  arktest.assertEQ(typeof l2, "undefined")
  arktest.assertEQ(typeof l3, "boolean")
  arktest.assertEQ(typeof l4, "boolean")

  // ---------------
  // ids with objects and enums

  let o0 = new String("1")
  let o1: Int[] = [1, 2]
  let o2 = test(true)
  let o3 = test(false)
  let o4: Object | null = test1(1)
  let o5: Object | null = test1(0)
  let o6: Object | undefined = test2(1)
  let o7: Object | undefined = test2(0)
  let o8: (a: boolean) => boolean | long[] = test
  arktest.assertEQ(typeof o0, "string")
  arktest.assertEQ(typeof o1, "object")
  arktest.assertEQ(typeof o2, "boolean")
  arktest.assertEQ(typeof o3, "object")
  arktest.assertEQ(typeof o4, "object")
  arktest.assertEQ(typeof o5, "object")
  arktest.assertEQ(typeof o6, "undefined")
  arktest.assertEQ(typeof o7, "object")
  arktest.assertEQ(typeof o8, "function")

  let e0 = Color.Red
  let e1: Color = Color.Green
  arktest.assertEQ(typeof e0, "number")
  arktest.assertEQ(typeof e1, "number")

  // ===============
  // for expressions
  // ---------------
  // exprs with numbers (objects)

  arktest.assertEQ(typeof new Boolean(), "boolean")
  arktest.assertEQ(typeof new Byte(), "byte")
  arktest.assertEQ(typeof new Char(), "char")
  arktest.assertEQ(typeof new Short(), "short")
  arktest.assertEQ(typeof new Int(), "int")
  arktest.assertEQ(typeof new Long(), "long")
  arktest.assertEQ(typeof new Float(), "float")
  arktest.assertEQ(typeof new Number(), "number")
  arktest.assertEQ(typeof new BigInt(), "bigint")

  // ---------------
  // exprs with literals
  arktest.assertEQ(typeof "123", "string")
  arktest.assertEQ(typeof null, "object")
  arktest.assertEQ(typeof undefined, "undefined")
  arktest.assertEQ(typeof true, "boolean")
  arktest.assertEQ(typeof false, "boolean")
  arktest.assertEQ(typeof 1n, "bigint")

  // ---------------
  // exprs with objects and enums
  arktest.assertEQ(typeof new String(), "string")
  arktest.assertEQ(typeof [1, 2], "object")
  arktest.assertEQ(typeof test(true), "boolean")
  arktest.assertEQ(typeof test(false), "object")
  arktest.assertEQ(typeof test1(1), "object")
  arktest.assertEQ(typeof test1(0), "object")
  arktest.assertEQ(typeof test2(1), "undefined")
  arktest.assertEQ(typeof test1(0), "object")
  arktest.assertEQ(typeof ("123" + "124"), "string")
  x004 = 9
  x4 = 6
  arktest.assertEQ(typeof (x4 + x004), "int")
  x8 = 1n
  x08 = new BigInt(10)
  arktest.assertEQ(typeof (x8 + x08), "bigint")
}

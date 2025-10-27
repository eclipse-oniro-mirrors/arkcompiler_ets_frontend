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

export declare const byte_1: byte = -127 as byte, byte_2: byte = 126 as byte;
export declare const byte_12: byte = -127 + 1, byte_22: byte = 126 - 1;
export declare const byte_13: byte = Byte.MIN_VALUE, byte_23: byte = 126;
export declare const byte_14: byte = new Byte().unboxed()
export declare const int_1 = 1 + 2 + 3 + 4
const v = 42
export declare const int_12 = v
export declare const int_15: int
export declare const int_16: int = 12345
export declare const int_17 = 12345
export declare const int_18: number = 12345
declare const x1: int = 5
declare const x2: int = -5
declare const y1: float = 5.55f
declare const y2: double = -5.55
declare const x3: int = 0x5
declare const x4: int = 0b101
declare const x5: string = "abc"
declare const x6: "abc" = "abc"
declare const x7 = null
declare const x8 = undefined
// fllowing pass without CTE
declare const x11 = 5
declare const x21 = -5
declare const y11 = 5.55
declare const y21 = -5.55
declare const x31 = 0x5
declare const x41 = 0b101
declare const x51 = "abc"

/* @@? 16:37 Error TypeError: Initializers are not allowed in ambient contexts: byte_1 */
/* @@? 16:66 Error TypeError: Initializers are not allowed in ambient contexts: byte_2 */
/* @@? 17:38 Error TypeError: Initializers are not allowed in ambient contexts: byte_12 */
/* @@? 17:38 Error TypeError: Type 'Int' cannot be assigned to type 'Byte' */
/* @@? 17:64 Error TypeError: Type 'Int' cannot be assigned to type 'Byte' */
/* @@? 17:64 Error TypeError: Initializers are not allowed in ambient contexts: byte_22 */
/* @@? 18:38 Error TypeError: Initializers are not allowed in ambient contexts: byte_13 */
/* @@? 18:70 Error TypeError: Initializers are not allowed in ambient contexts: byte_23 */
/* @@? 19:38 Error TypeError: Initializers are not allowed in ambient contexts: byte_14 */
/* @@? 24:36 Error TypeError: Initializers are not allowed in ambient contexts: int_16 */
/* @@? 26:39 Error TypeError: Initializers are not allowed in ambient contexts: int_18 */
/* @@? 27:25 Error TypeError: Initializers are not allowed in ambient contexts: x1 */
/* @@? 28:25 Error TypeError: Initializers are not allowed in ambient contexts: x2 */
/* @@? 29:27 Error TypeError: Initializers are not allowed in ambient contexts: y1 */
/* @@? 30:28 Error TypeError: Initializers are not allowed in ambient contexts: y2 */
/* @@? 31:25 Error TypeError: Initializers are not allowed in ambient contexts: x3 */
/* @@? 32:25 Error TypeError: Initializers are not allowed in ambient contexts: x4 */
/* @@? 33:28 Error TypeError: Initializers are not allowed in ambient contexts: x5 */
/* @@? 34:27 Error TypeError: Initializers are not allowed in ambient contexts: x6 */
/* @@? 35:20 Error TypeError: A 'const' initializer in an ambient context must be a string or numeric literal: x7 */
/* @@? 36:20 Error TypeError: A 'const' initializer in an ambient context must be a string or numeric literal: x8 */

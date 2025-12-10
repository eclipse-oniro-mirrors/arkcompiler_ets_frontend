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

// Octal - Err: Invalid BigInt number. Can't use bigint with octal numbers
let b1: bigint = BigInt(0o777n);
let b2: bigint = 0o644n;
let b3: bigint = 0O1_23n;
let b4: bigint = BigInt(0O1_23n);

let b5 = 0o66_5n;
let b6 = BigInt(0O1_23n);

// Binary - Err: Invalid BigInt number. Can't use bigint with binary numbers
let c1: bigint =  BigInt(0b1010n);
let c2: bigint = 0B1101n
let c3: bigint =  BigInt(0b1111_0000n)

let c4 = 0B0101n
let c5 = BigInt(0b1_1010010n)

// Hexadecimal - Err: Invalid BigInt number. Can't use bigint with Hexadecimal numbers
let d1: bigint =  BigInt(0xCFEn);
let d2: bigint = 0xBAD3n
let d3: bigint =  BigInt(0XAB_CD_EFn)
 
let d4 = 0xFFn
let d5 = BigInt(0XCF_CD_EAn)
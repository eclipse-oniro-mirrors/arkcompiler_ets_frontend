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

// Bigint with decimals
let a1: bigint = 1907n
let a2: bigint = -34536n
let a3: bigint = BigInt(1_020_000n)

// Bigint with decimals not type given
let a4 = 12653n
let a5 = -1907n
let a6 = BigInt(1_000_000n)

// Bigint type but normal decimal
let a7: bigint =  BigInt(1907)
let a9: bigint = BigInt(1_000_000)

// Extra cases
let a10 = BigInt(5n/2n)
let a11: bigint = 1_0_0n

// Hexadecimal, octa, binary but not bigint
let a12 = 0xCFE
let a13 = 0b101
let a14 = 0o34
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


let i = 1;
let j = 2;
let b1 = true;
let b2 = false;
let r1 = new Object();
let r2: Object | null = null;

// nullish
let r3 = r1 ?? r2;
let r4 = r1 ?? r2 ?? r3;

// logical (and, or)
let b01 = b1 && b2;
let b02 = b1 || b2;
let b03 = b1 && b2 && b01 && b02;
let b04 = b1 || b2 || b01 || b02;
let b05 = b1 && b2 || b01 || b02;
let b06 = b1 || b2 && b01 && b02;

// bitwise (and, or, xor)
let i01 = i & j;
let i02 = i ^ j;
let i03 = i | j;
let i04 = i & j ^ i & j | i ^ j;

// equality (eq, neq, strict eq, strict neq)
let b07 = i != j;
let b08 = i == j;
let b09 = r1 !== r2;
let b10 = r1 === r2;

// relational (less, greater, less eq, greater eq), instanceof
let b12 = i < j;
let b13 = i > j;
let b14 = i <= j;
let b15 = i >= j;
let b16 = r1 instanceof Object;

// shift (left, right, unsigned right)
let b17 = i >> j;
let b18 = i << j;
let b19 = i >>> j;
let b20 = i >> j << i >>> j;

// additive (add, sub)
let i05 = i + j;
let i06 = i - j;
let i07 = i + j - i + j;

// multiplicative (mul, div, mod)
let i08 = i * j;
let i09 = i / j;
let i10 = i % j;
let i11 = i * j / i % j * i / j;

// mixed, precedence
let b21 = i * j >>> i - j;
let b22 = i > j && b1 || b2;
let b23 = i * j + i >> j < i != b1 & b2 ^ b1 | b2 && b1 || b2;
let b24 = b1 || b2 && b1 | b2 ^ b1 & b2 == i >= j << i - j % i;
let b25 = r1 ?? b1 != b2 | i <= j;

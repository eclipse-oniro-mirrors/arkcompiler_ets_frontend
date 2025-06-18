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

const INT_MIN:int = -2147483648;
const DivInt = /*@@ DivInt */ INT_MIN / -1;
const ModInt = /*@@ ModInt */ INT_MIN % -1;
const MulInt = /*@@ MulInt */ INT_MIN * -1;
const SubInt = /*@@ SubInt */ INT_MIN - 1;
const AddInt = /*@@ AddInt */ INT_MIN + INT_MIN;

const BYTE_MIN:Byte = -128;
const BYTE_NEG_ONE:Byte = -1;
const BYTE_POS_ONE:Byte = 1;
const DivByte = /*@@ DivByte */ BYTE_MIN / BYTE_NEG_ONE;
const ModByte = /*@@ ModByte */ BYTE_MIN % BYTE_NEG_ONE;
const MulByte = /*@@ MulByte */ BYTE_MIN * BYTE_NEG_ONE;
const SubByte = /*@@ SubByte */ BYTE_MIN - BYTE_POS_ONE;
const AddByte = /*@@ AddByte */ BYTE_MIN + BYTE_MIN;

// Negation is allowed since parser treats -128 as -(128). 
const NegInt = -INT_MIN;
const NegByte = -BYTE_MIN;

/* @@@ DivInt Error TypeError: Arithmetic operation causes an overflow. */
/* @@@ ModInt Error TypeError: Arithmetic operation causes an overflow. */
/* @@@ MulInt Error TypeError: Arithmetic operation causes an overflow. */
/* @@@ SubInt Error TypeError: Arithmetic operation causes an overflow. */
/* @@@ AddInt Error TypeError: Arithmetic operation causes an overflow. */
/* @@@ DivByte Error TypeError: Arithmetic operation causes an overflow. */
/* @@@ ModByte Error TypeError: Arithmetic operation causes an overflow. */
/* @@@ MulByte Error TypeError: Arithmetic operation causes an overflow. */
/* @@@ SubByte Error TypeError: Arithmetic operation causes an overflow. */
/* @@@ AddByte Error TypeError: Arithmetic operation causes an overflow. */

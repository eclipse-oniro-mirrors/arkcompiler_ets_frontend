/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

export const floatInf: /* @@ dst_type1 */float = /* @@ value1 */1.0 / 0.0
export const byteInf:  /* @@ dst_type2 */byte = /* @@ value2 */1.0 / 0.0
export const shortInf: /* @@ dst_type3 */short = /* @@ value3 */1.0 / 0.0




/* @@@ dst_type1 Error Semantic error ESE0344: Invalid destination type for floating-point constant value */
/* @@@ value1 Error Semantic error ESE0318: Type 'Double' cannot be assigned to type 'Float' */
/* @@@ dst_type2 Error Semantic error ESE0344: Invalid destination type for floating-point constant value */
/* @@@ value2 Error Semantic error ESE0318: Type 'Double' cannot be assigned to type 'Byte' */
/* @@@ dst_type3 Error Semantic error ESE0344: Invalid destination type for floating-point constant value */
/* @@@ value3 Error Semantic error ESE0318: Type 'Double' cannot be assigned to type 'Short' */
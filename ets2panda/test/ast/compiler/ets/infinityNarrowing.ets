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


/* @@? 16:42 Error TypeError: Invalid destination type for floating-point constant value  */
/* @@? 16:65 Error TypeError: Type 'Double' cannot be assigned to type 'Float'  */
/* @@? 17:42 Error TypeError: Invalid destination type for floating-point constant value  */
/* @@? 17:64 Error TypeError: Type 'Double' cannot be assigned to type 'Byte'  */
/* @@? 18:42 Error TypeError: Invalid destination type for floating-point constant value  */
/* @@? 18:65 Error TypeError: Type 'Double' cannot be assigned to type 'Short'  */


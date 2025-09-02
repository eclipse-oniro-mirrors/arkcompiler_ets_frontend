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

export const floatInf: float = /* @@ label2 */1.0 / 0.0
export const byteInf: byte = /* @@ label */1.0 / 0.0
export const shortInf: short = /* @@ label1 */1.0 / 0.0

/* @@@ label Error TypeError: Type 'Double' cannot be assigned to type 'Byte'  */
/* @@@ label1 Error TypeError: Type 'Double' cannot be assigned to type 'Short'  */
/* @@@ label2 Error TypeError: Type 'Double' cannot be assigned to type 'Float'  */
/* @@@ label2 Error TypeError: Value is out of range  */
/* @@@ label1 Error TypeError: Floating-point value cannot be converted  */
/* @@@ label Error TypeError: Floating-point value cannot be converted  */

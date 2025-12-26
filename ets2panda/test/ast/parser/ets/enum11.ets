/*
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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

enum Color { Red }

function main(): void {
  let arr: Color[] = [Color.Red];

  let ordinal: int = arr[0] as int;
  let red = ordinal as Color;
  let ordinalFail: int = /* @@ label */Color as int;
}


/* @@? 21:22 Error Semantic error ESE0030: Enum cast is deprecated. Cast support from 'Color' to 'Int' will be removed. */
/* @@? 22:13 Error Semantic error ESE0030: Enum cast is deprecated. Cast support from 'Int' to 'Color' will be removed. */
/* @@@ label Error Semantic error ESE0144: Enum name 'Color' used in the wrong context */
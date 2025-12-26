/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

enum E {
  a, b, c
}

enum EE {
  a, b, c, d, e
}

function foo(e: E, i: int) {
  i = e  // OK if widening, otherwise - CTE
  i = e as int  // CTE
  e = i  // CTE
  e = 1 as E  // CTE
  e = E.fromValue(i)  // RTE, if i is not equal to one of enum constants, otherwise OK
  e = 1  // CTE
}


/* @@? 26:7 Error Semantic error ESE0030: Enum cast is deprecated. Cast support from 'E' to 'Int' will be removed. */
/* @@? 27:7 Error Semantic error ESE0318: Type 'Int' cannot be assigned to type 'E' */
/* @@? 28:7 Error Semantic error ESE0030: Enum cast is deprecated. Cast support from 'Int' to 'E' will be removed. */
/* @@? 30:7 Error Semantic error ESE0318: Type 'Int' cannot be assigned to type 'E' */
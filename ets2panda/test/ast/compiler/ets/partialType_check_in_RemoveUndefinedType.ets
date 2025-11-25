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

function genericFunc<T>(p: Partial<T>): void {
  if (p !== undefined) {
  }
}
function main() {
  genericFunc<{a: number, b: string}>({a: 1})
}


/* @@? 16:35 Error Semantic error ESE124644: T in Partial<T> must be a class or an interface type. */
/* @@? 21:3 Error Semantic error ESE655063: Generic function genericFunc requires explicit type arguments when used as a value. */
/* @@? 21:15 Error Semantic error ESE0062: need to specify target type for class composite */
/* @@? 21:38 Error Semantic error ESE0062: need to specify target type for class composite */
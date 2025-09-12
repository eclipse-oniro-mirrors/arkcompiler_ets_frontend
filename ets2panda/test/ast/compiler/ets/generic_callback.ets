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

 function identity<A>(a: A): A {
  return a;
}
let x = [1, 2, 3].map(identity)[0];

/* @@? 19:9 Error TypeError: No matching call signature for map((<A>(p1: A) => A)) */
/* @@? 19:9 Error TypeError: Indexed access is not supported for such expression type. */
/* @@? 19:9 Error TypeError: No matching call signature for map((<A>(p1: A) => A)) */
/* @@? 19:9 Error TypeError: Indexed access is not supported for such expression type. */
/* @@? 19:23 Error TypeError: Type '(<A>(p1: A) => A)' is not compatible with type '((p1: Int, p2: Int, p3: Array<Int>) => A)' at index 1 */
/* @@? 19:23 Error TypeError: Type '(<A>(p1: A) => A)' is not compatible with type '((p1: Int, p2: Int, p3: ReadonlyArray<Int>) => A)' at index 1 */
/* @@? 19:23 Error TypeError: Type '(<A>(p1: A) => A)' is not compatible with type '((p1: Int, p2: Int, p3: Array<Int>) => A)' at index 1 */
/* @@? 19:23 Error TypeError: Type '(<A>(p1: A) => A)' is not compatible with type '((p1: Int, p2: Int, p3: ReadonlyArray<Int>) => A)' at index 1 */

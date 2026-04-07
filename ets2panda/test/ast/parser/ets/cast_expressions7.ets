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

class A {}
class B extends A {}

function main(): void {
  let b: B[] = [new B()];
  let a: A[] = /* @@ label1 */b as A[];
  let b2: B[] = /* @@ label2 */a as B[];

  let int_a: int[] = [1,2,3];
  let int_a2 = /* @@ label3 */int_a as int[];

  let long_a: long[] = /* @@ label4 */int_a as long[];
}

/* @@@ label1 Warning Warning W369037: Cast expression `as Array<A>` is type unsafe. 'CastClassError' exception can be thrown at runtime */
/* @@@ label2 Warning Warning W369037: Cast expression `as Array<B>` is type unsafe. 'CastClassError' exception can be thrown at runtime */
/* @@@ label3 Warning Warning W670214: This cast can be excessive because it always succeeds */
/* @@@ label4 Warning Warning W0326: Attempting to cast type 'Array<Int>' to 'Array<Long>' can throw 'CastClassError' exception at runtime */

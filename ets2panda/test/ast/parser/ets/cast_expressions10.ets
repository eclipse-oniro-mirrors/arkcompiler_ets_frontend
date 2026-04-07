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

function main(): void {
  let Int_a: Int[][] = [[new Int(1)],[new Int(1)],[new Int(1)]];
  let Int_a2: Int[][] = /* @@ label1 */Int_a as Int[][];
  let Object_a: Object[][] = /* @@ label2 */Int_a as Object[][];
  let Int_a3: Int[][] = /* @@ label3 */Object_a as Int[][];

  let Object_: Object = /* @@ label4 */Int_a as Object;
  let Int_a4: Int[][] = /* @@ label5 */Object_ as Int[][];

  let Long_a: Long[][] = /* @@ label6 */Int_a as Long[][];
}


/* @@@ label1 Warning Warning W670214: This cast can be excessive because it always succeeds */
/* @@@ label2 Warning Warning W369037: Cast expression `as Array<Array<Object>>` is type unsafe. 'CastClassError' exception can be thrown at runtime */
/* @@@ label3 Warning Warning W369037: Cast expression `as Array<Array<Int>>` is type unsafe. 'CastClassError' exception can be thrown at runtime */
/* @@@ label4 Warning Warning W670214: This cast can be excessive because it always succeeds */
/* @@@ label5 Warning Warning W369037: Cast expression `as Array<Array<Int>>` is type unsafe. 'CastClassError' exception can be thrown at runtime */
/* @@@ label6 Warning Warning W0326: Attempting to cast type 'Array<Array<Int>>' to 'Array<Array<Long>>' can throw 'CastClassError' exception at runtime */

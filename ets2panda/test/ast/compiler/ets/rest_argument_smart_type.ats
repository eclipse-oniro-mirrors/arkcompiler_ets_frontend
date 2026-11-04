/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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


const C1: number = 4;
const C2: number = 2;

function foo(x: number | undefined): number {
  let m: number = 3;
  let s: number | undefined = x ?? C1;
  s = undefined; 
  return /* @@ label1 */Math.min(/* @@ label2 */s, m, C2);
}

/* @@@ label1 Error Semantic error ESE0127: No matching call signature for min(undefined, Double, Double) */
/* @@@ label2 Error Semantic error ESE0323: Type 'undefined' is not compatible with rest parameter type 'Double' at index 1 */



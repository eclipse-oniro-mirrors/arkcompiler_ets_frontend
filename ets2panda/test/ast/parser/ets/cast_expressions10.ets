/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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
  let Int_a2: Int[][] = Int_a as Int[][];
  let Object_a: Object[][] = Int_a as Object[][];
  let Int_a3: Int[][] = Object_a as Int[][];

  let Object_: Object = Int_a as Object;
  let Int_a4: Int[][] = Object_ as Int[][];

  let Long_a: Long[][] = Int_a as Long[][];
}

/* @@? 19:30 Error Semantic error ESE0326: Cannot cast type 'Array<Array<Int>>' to 'Array<Array<Object>>' */
/* @@? 20:25 Error Semantic error ESE0326: Cannot cast type 'Array<Array<Object>>' to 'Array<Array<Int>>' */
/* @@? 25:26 Error Semantic error ESE0326: Cannot cast type 'Array<Array<Int>>' to 'Array<Array<Long>>' */
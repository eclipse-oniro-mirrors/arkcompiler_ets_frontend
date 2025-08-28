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

'use static';

const numbers = [1, 2, 3];
const moreNumbers = [0, ...numbers, 4, 5]; 
const part1 = [1, 2];
const part2 = [3, 4];
const combined = [...part1, ...part2, 5]; 
const original = [10, 20, 30];
const copy = [...original];

export class Test {
   sum(...numbers: number[]): number {
      return 0;
   }
   args = [1.5, 2.5, 3.0];
   result = this.sum(...this.args);
   testNums = [1, 2, 3];
   moreTestNums  = [0, ...this.testNums, 4, 5]; 
}
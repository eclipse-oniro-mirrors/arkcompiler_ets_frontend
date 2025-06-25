/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

function main(): int {
   let z: Any = new Number(1);
   let z1 = undefined;
   let z2 = null;
   let z3: Object = new String("0");
   let z4: Object = new Number(1);
   let x = new String(z)
   let x1 = new String(z1)
   let x2 = new String(z2)
   let x3 = new String(z3)
   let x4 = new String(z4)
   let x5 = new String(null)
   let x6 = new String(undefined)
   let actual: String[] = [new String(z), new String(z1), new String(z2), new String(z3), new String(z4), new String(null), new String(undefined)]
   let expected: String[] = ["1", "undefined", "null", "0", "1", "null", "undefined"]
   for (let i = 0; i < expected.length; i++) {
      if (actual[i] != expected[i]) {
         console.log("Failed: actual is \"" + actual[i] + "\" expected is \"" + expected[i] + "\"")
         return 1;
      }
   }
   return 0;
}

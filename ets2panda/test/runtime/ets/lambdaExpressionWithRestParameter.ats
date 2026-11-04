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

type LambdaTypeTest = (prefix: string, ...args: string[]) => string;


function main(): void {

  let lam: LambdaTypeTest = (prefix: string, ...args: string[]): string => {
    let ret = prefix;
    for (let arg of args) {
      ret += arg;
    }
    return ret;
  }

  let x: string = lam("Test 1:", " case 1,", " case 2,", " case 3.");
  arktest.assertEQ(x, "Test 1,  case 1, case 2, case 3.")

  let arr: string[] = [" case 4,", " case 5,", " case 6."];

  let y = lam("Test 2:", ...arr);
  arktest.assertEQ(y, "Test 2,  case 4, case 5, case 6.")
}

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

export class BitopsBitwiseAnd {
  n: int = 600000;
  static readonly expected: long = 0;

  public run(): void {
    let bitwiseAndValue: long = 4294967296;
    for (let i: int = 0; i < this.n; i++) {
      bitwiseAndValue = bitwiseAndValue & i;
    }

    arktest.assertEQ(bitwiseAndValue, BitopsBitwiseAnd.expected,  "Incorrect result");
  }
}

function main(): void {
  let a = new BitopsBitwiseAnd;
  a.run();
}

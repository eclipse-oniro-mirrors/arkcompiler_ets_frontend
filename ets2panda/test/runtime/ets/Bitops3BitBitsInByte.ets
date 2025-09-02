/*
 * Copyright (c) 2022-2025 Huawei Device Co., Ltd.
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

export class Bitops3BitBitsInByte {
  private static fast3bitlookup(b: int): int {
    let c: int;
    let bi3b: int = 0xE994; // 0b1110 1001 1001 0100; // 3 2 2 1  2 1 1 0
    c = 3 & (bi3b >> ((b << 1) & 14));
    c += 3 & (bi3b >> ((b >> 2) & 14));
    c += 3 & (bi3b >> ((b >> 5) & 6));
    return c;
  }

  private n1: int = 500;
  private n2: int = 256;
  private static readonly expected: int = 512000;

  public run(): void {
    let sum: int = 0;
    for (let x: int = 0; x < this.n1; x++) {
      for (let y: int = 0; y < this.n2; y++) {
        sum += Bitops3BitBitsInByte.fast3bitlookup(y);
      }
    }
    arktest.assertEQ(sum, Bitops3BitBitsInByte.expected,  "Incorrect result")
  }
}

function main(): void {
  let a = new Bitops3BitBitsInByte;
  a.run();
}

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

export class BitopsBitsInByte {
  private static bitsinbyte(b: int): int {
    let m: int = 1;
    let c: int = 0;
    while (m < 0x100) {
      if ((b & m) != 0) {
        c++;
      }
      m <<= 1;
    }
    return c;
  }

  private n1: int = 350;
  private n2: int = 256;
  private static readonly expected: int = 358400;

  public run(): void {
    let sum: int = 0;
    for (let x: int = 0; x < this.n1; x++) {
      for (let y: int = 0; y < this.n2; y++) {
        sum += BitopsBitsInByte.bitsinbyte(y);
      }
    }
    arktest.assertEQ(sum, BitopsBitsInByte.expected,  "Incorrect result")
  }
}

function main(): void {
  let a = new BitopsBitsInByte;
  a.run();
}



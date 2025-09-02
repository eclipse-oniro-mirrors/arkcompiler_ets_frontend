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


export class BitopsNSieveBits {
  static primes(isPrime: int[], n1: int, n2: int): void {
    let i: int;
    let m: int = n2 << n1;
    let size: int = m + 31 >> 5;

    for (i = 0; i < size; i++) {
      isPrime[i] = -1;
    }

    for (i = 2; i < m; i++) {
      if ((isPrime[i >> 5] & 1 << (i & 31)) != 0) {
        for (let j: int = i + i; j < m; j += i) {
          isPrime[j >> 5] &= ~(1 << (j & 31));
        }
      }
    }
  }

  private static sieve(n1: int, n2: int): int[] {
    // Not parsed new int[...]
    let isPrime: int[] = new int[(n2 << n1) + 31 >> 5];
    BitopsNSieveBits.primes(isPrime, n1, n2);
    return isPrime;
  }

  n1: int = 4;
  n2: int = 10000;
  static readonly expected: long = -1286749544853;

  public run(): void {
    let result: int[] = BitopsNSieveBits.sieve(this.n1, this.n2);
    let sum: long = 0;
    for (let i: int = 0; i < result.length; ++i) {
      sum += result[i];
    }

    arktest.assertEQ(sum, BitopsNSieveBits.expected,  "Incorrect result")
  }
}

function main(): void {
  let a = new BitopsNSieveBits;
  a.run();
}



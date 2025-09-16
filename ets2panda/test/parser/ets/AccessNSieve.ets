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

export class AccessNSieve {
  static readonly n1: int = 3;
  static readonly n2: int = 10000;
  static readonly expected: int = 14302;
  static isPrime: boolean[] = [];

  public setup(): void {
    AccessNSieve.isPrime = new boolean[(1 << AccessNSieve.n1) * AccessNSieve.n2 + 1];
  }

  private static nsieve(m: int): int {
    let count: int = 0;

    for (let i: int = 2; i <= m; i++) {
      AccessNSieve.isPrime[i] = true;
    }

    for (let i: int = 2; i <= m; i++) {
      if (AccessNSieve.isPrime[i]) {
        for (let k: int = i + i; k <= m; k += i) {
          AccessNSieve.isPrime[k] = false;
        }
        count++;
      }
    }
    return count;
  }

  public static sieve(): int {
    let sum: int = 0;
    for (let i: int = 1; i <= AccessNSieve.n1; i++) {
      let m: int = (1 << i) * AccessNSieve.n2;
      sum += AccessNSieve.nsieve(m);
    }
    return sum;
  }

  public run(): void {
    let ret: int = AccessNSieve.sieve();
    arktest.assertEQ(ret, AccessNSieve.expected,  "Invalid result");
  }
}

function main(): void {
  let a = new AccessNSieve;
  a.setup();
  a.run();
}

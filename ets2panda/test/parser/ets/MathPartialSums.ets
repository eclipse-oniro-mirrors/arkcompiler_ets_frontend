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

export class MathPartialSums {
  static readonly n1: int = 1024;
  static readonly n2: int = 16384;
  static readonly expected: double = 60.08994194659945;
  static g_flag: int = 0;

  private static partial(n: int): double {
    let a1: double = 0.0;
    let a2: double = 0.0;
    let a3: double = 0.0;
    let a4: double = 0.0;
    let a5: double = 0.0;
    let a6: double = 0.0;
    let a7: double = 0.0;
    let a8: double = 0.0;
    let a9: double = 0.0;

    let twothirds: double = 2.0 / 3.0;
    let alt: double = -1.0;
    let k2: double = 0.0;
    let k3: double = 0.0;
    let sk: double = 0.0;
    let ck: double = 0.0;
    let res: double = 0.0;

    for (let k: long = 1; k <= n; k++) {
      k2 = k * k;
      k3 = k2 * k;
      sk = sin(k);
      ck = cos(k);
      alt = -alt;

      a1 += power(twothirds, k - 1);
      a2 += power(k, -0.5);
      a3 += 1.0 / (k * k + 1.0);
      a4 += 1.0 / (k3 * sk * sk);
      a5 += 1.0 / (k3 * ck * ck);
      a6 += 1.0 / k;
      a7 += 1.0 / k2;
      a8 += alt / k;
      a9 += alt / (2 * k - 1);
    }

    res = a1 + a2 + a3 + a4 + a5;

    if (res > 0) {
      MathPartialSums.g_flag = 1;
    } else {
      MathPartialSums.g_flag = 2;
    }

    // NOTE: We don't try to validate anything from pow(),  sin() or cos()
    // because those aren't well-specified in ECMAScript.
    return a6 + a7 + a8 + a9;
  }

  public static run(): void {
    let sum: double = 0;
    for (let j: int = MathPartialSums.n1; j <= MathPartialSums.n2; j *= 2) {
      sum += MathPartialSums.partial(j);
    }

    arktest.assertEQ(sum, MathPartialSums.expected,  "Incorrect result");
    arktest.assertEQ(MathPartialSums.g_flag == 1 || MathPartialSums.g_flag, 2,  "Incorrect g_flag");
  }
}

function main(): void {
  MathPartialSums.run();
}

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

export class ControlFlowRecursive {
  static readonly n1: int = 3;
  static readonly n2: int = 5;
  static readonly expected: int = 57775;

  private static ack(m: int, n: int): int {
    if (m == 0) {
      return n + 1;
    }
    if (n == 0) {
      return ControlFlowRecursive.ack(m - 1, 1);
    }
    return ControlFlowRecursive.ack(m - 1, ControlFlowRecursive.ack(m, n - 1));
  }

  private static fib(n: int): int {
    if (n < 2) {
      return 1;
    }
    return ControlFlowRecursive.fib(n - 2) + ControlFlowRecursive.fib(n - 1);
  }

  private static tak(x: int, y: int, z: int): int {
    if (y >= x) {
      return z;
    }
    return ControlFlowRecursive.tak(ControlFlowRecursive.tak(x - 1, y, z), ControlFlowRecursive.tak(y - 1, z, x), ControlFlowRecursive.tak(z - 1, x, y));
  }

  public static run(): void {
    let result: int = 0;
    for (let j: int = ControlFlowRecursive.n1; j <= ControlFlowRecursive.n2; ++j) {
      result += ControlFlowRecursive.ack(3, j);
      result += ControlFlowRecursive.fib(17 + j);
      result += ControlFlowRecursive.tak(3 * j + 3, 2 * j + 2, j + 1);
    }

    arktest.assertEQ(result, ControlFlowRecursive.expected,  "Incorrect result");
  }
}

function main(): void {
  ControlFlowRecursive.run();
}

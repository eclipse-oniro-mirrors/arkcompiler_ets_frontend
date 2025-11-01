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

import { PI } from "std/math/consts";

export class Morph3d {
  private static readonly param: int = 120;
  private static readonly n: int = 15;

  static a: double[] = [];

  public setup(): void {
    // Not parsed new double[...]
    Morph3d.a = new double[Morph3d.param * Morph3d.param * 3];
    for (let i: int = 0; i < Morph3d.param * Morph3d.param * 3; i++) {
      Morph3d.a[i] = 0;
    }
  }

  private morph(f: double): void {
    const paramPI2: double = PI * 8 / Morph3d.param;
    const f30: double = -(50 * sin(f * PI * 2));

    for (let i: int  = 0; i < Morph3d.param; ++i) {
      for (let j: int = 0; j < Morph3d.param; ++j) {
        Morph3d.a[3 * (i * Morph3d.param + j) + 1] = sin((j - 1) * paramPI2) * -f30;
      }
    }
  }

  public run(): void { //TBD: throws RuntimeException
    let loops: int = Morph3d.n;
    for (let i: int = 0; i < loops; i++) {
      this.morph(i / loops);
    }

    let testOutput: double = 0.0;
    for (let i: int = 0; i < Morph3d.param; i++) {
      testOutput += Morph3d.a[3 * (i * Morph3d.param + i) + 1];
    }
    let epsilon: double = 1e-13;

    arktest.assertTrue(abs(testOutput) < epsilon,  "Incorrect result")
  }
}


function main(): void {
  let a = new Morph3d;
  a.setup();
  a.run();
}


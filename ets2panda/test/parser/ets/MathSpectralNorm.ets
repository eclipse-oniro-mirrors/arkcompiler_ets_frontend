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

export class MathSpectralNorm {
  private static fnA(i: double, j: double): double {
    return 1 / ((i + j) * (i + j + 1) / 2 + i + 1);
  }

  private static fnAu(u: double[], v: double[]): void {
    for (let i: int = 0; i < v.length; i++) {
      let t: double = 0;
      for (let j: int = 0; j < u.length; j++) {
        t += MathSpectralNorm.fnA(i, j) * u[j];
      }
      v[i] = t;
    }
  }

  private static fnAtu(u: double[], v: double[]): void {
    for (let i: int = 0; i < v.length; i++) {
      let t: double = 0;
      for (let j: int = 0; j < u.length; j++) {
        t += MathSpectralNorm.fnA(j, i) * u[j];
      }
      v[i] = t;
    }
  }

  private static fnAtAu(u: double[], v: double[], w: double[]): void {
    MathSpectralNorm.fnAu(u, w);
    MathSpectralNorm.fnAtu(w, v);
  }

  private static spectralnorm(n: int): double {
    let i: int;
    // Not parsed new double[n]
    let u: double[] = new double[n];
    // Not parsed new double[n]
    let w: double[] = new double[n];
    // Not parsed new double[n]
    let v: double[] = new double[n];
    let vbv: double = 0;
    let vv: double = 0;

    for (i = 0; i < n; i++) {
      u[i] = 1;
      v[i] = w[i] = 0;
    }

    for (i = 0; i < 10; i++) {
      MathSpectralNorm.fnAtAu(u, v, w);
      MathSpectralNorm.fnAtAu(v, u, w);
    }

    for (i = 0; i < n; i++) {
      vbv += u[i] * v[i];
      vv  += v[i] * v[i];

    }

    return sqrt(vbv / vv);
  }

  n1: int = 6;
  n2: int = 48;
  private static readonly expected: double = 5.086694231303284;

  public run(): void {
    let total: double = 0;

    for (let i: int = this.n1; i <= this.n2; i *= 2) {
      total += MathSpectralNorm.spectralnorm(i);
    }

    arktest.assertEQ(total, MathSpectralNorm.expected,  "Incorrect result");
  }
}

function main(): void {
  let a = new MathSpectralNorm;
  a.run();
}

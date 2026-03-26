/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import { BENCH_CHAIN_SYMBOL_L2, L2MakeToken } from "./bench_l2";

export interface BenchRunnerL3 {
  run(seed: number): string;
}

export class BenchRunnerL3Impl implements BenchRunnerL3 {
  run(seed: number): string {
    return "L3-" + seed.toString();
  }
}

export const BENCH_CHAIN_SYMBOL_L3 = BENCH_CHAIN_SYMBOL_L2 + "-L3";

export const benchObjL3 = {
  id: 3,
  label: BENCH_CHAIN_SYMBOL_L3,
  score: 30,
  ratio: 0.3,
  tag: "L3",
};

export function L3MakeToken(): string {
  const impl = new BenchRunnerL3Impl();
  const base = L2MakeToken();
  const r0 = BENCH_CHAIN_SYMBOL_L3;
  const r1 = BENCH_CHAIN_SYMBOL_L3 + "-0";
  const r2 = BENCH_CHAIN_SYMBOL_L3 + "-1";
  const r3 = BENCH_CHAIN_SYMBOL_L3 + "-2";
  const r4 = BENCH_CHAIN_SYMBOL_L3 + "-3";
  const r5 = BENCH_CHAIN_SYMBOL_L3 + "-4";
  const r6 = BENCH_CHAIN_SYMBOL_L3 + "-5";
  const r7 = BENCH_CHAIN_SYMBOL_L3 + "-6";
  const r8 = BENCH_CHAIN_SYMBOL_L3 + "-7";
  return impl.run(benchObjL3.id) + "|" + base + "|" + r0 + "|" + r1 + "|" + r2 + "|" + r3 + "|" + r4 + "|" + r5 +
         "|" + r6 + "|" + r7 + "|" + r8;
}

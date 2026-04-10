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

import { BENCH_CHAIN_SYMBOL_L3, L3MakeToken } from "./bench_l3";

export interface BenchRunnerL4 {
  run(seed: number): string;
}

export class BenchRunnerL4Impl implements BenchRunnerL4 {
  run(seed: number): string {
    return "L4-" + seed.toString();
  }
}

export const BENCH_CHAIN_SYMBOL_L4 = BENCH_CHAIN_SYMBOL_L3 + "-L4";

export const benchObjL4 = {
  id: 4,
  label: BENCH_CHAIN_SYMBOL_L4,
  score: 40,
  ratio: 0.4,
  tag: "L4",
  depth: 3,
};

export function L4MakeToken(): string {
  const impl = new BenchRunnerL4Impl();
  const base = L3MakeToken();
  const r0 = BENCH_CHAIN_SYMBOL_L4;
  const r1 = BENCH_CHAIN_SYMBOL_L4 + "-0";
  const r2 = BENCH_CHAIN_SYMBOL_L4 + "-1";
  const r3 = BENCH_CHAIN_SYMBOL_L4 + "-2";
  const r4 = BENCH_CHAIN_SYMBOL_L4 + "-3";
  const r5 = BENCH_CHAIN_SYMBOL_L4 + "-4";
  const r6 = BENCH_CHAIN_SYMBOL_L4 + "-5";
  const r7 = BENCH_CHAIN_SYMBOL_L4 + "-6";
  const r8 = BENCH_CHAIN_SYMBOL_L4 + "-7";
  const r9 = BENCH_CHAIN_SYMBOL_L4 + "-8";
  const r10 = BENCH_CHAIN_SYMBOL_L4 + "-9";
  const r11 = BENCH_CHAIN_SYMBOL_L4 + "-10";
  const r12 = BENCH_CHAIN_SYMBOL_L4 + "-11";
  const r13 = BENCH_CHAIN_SYMBOL_L4 + "-12";
  const r14 = BENCH_CHAIN_SYMBOL_L4 + "-13";
  const r15 = BENCH_CHAIN_SYMBOL_L4 + "-14";
  const r16 = BENCH_CHAIN_SYMBOL_L4 + "-15";
  return impl.run(benchObjL4.id) + "|" + base + "|" + r0 + "|" + r1 + "|" + r2 + "|" + r3 + "|" + r4 + "|" + r5 +
         "|" + r6 + "|" + r7 + "|" + r8 + "|" + r9 + "|" + r10 + "|" + r11 + "|" + r12 + "|" + r13 + "|" + r14 +
         "|" + r15 + "|" + r16;
}

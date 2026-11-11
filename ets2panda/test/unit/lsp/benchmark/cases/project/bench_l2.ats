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

import { BENCH_CHAIN_SYMBOL_L1, L1MakeToken } from "./bench_l1";

export interface BenchRunnerL2 {
  run(seed: number): string;
}

export class BenchRunnerL2Impl implements BenchRunnerL2 {
  run(seed: number): string {
    return "L2-" + seed.toString();
  }
}

export const BENCH_CHAIN_SYMBOL_L2 = BENCH_CHAIN_SYMBOL_L1 + "-L2";

export const benchObjL2 = {
  id: 2,
  label: BENCH_CHAIN_SYMBOL_L2,
  score: 20,
  ratio: 0.2,
};

export function L2MakeToken(): string {
  const impl = new BenchRunnerL2Impl();
  const base = L1MakeToken();
  const a = BENCH_CHAIN_SYMBOL_L2;
  const b = BENCH_CHAIN_SYMBOL_L2 + "-a";
  const c = BENCH_CHAIN_SYMBOL_L2 + "-b";
  const d = BENCH_CHAIN_SYMBOL_L2 + "-c";
  const e = BENCH_CHAIN_SYMBOL_L2 + "-d";
  return impl.run(benchObjL2.id) + "|" + base + "|" + a + "|" + b + "|" + c + "|" + d + "|" + e;
}

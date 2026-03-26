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

export interface BenchRunnerL1 {
  run(seed: number): string;
}

export class BenchRunnerL1Impl implements BenchRunnerL1 {
  run(seed: number): string {
    return "L1-" + seed.toString();
  }
}

export const BENCH_CHAIN_SYMBOL_L1 = "BENCH_CHAIN_SYMBOL_L1";

export const benchObjL1 = {
  id: 1,
  label: BENCH_CHAIN_SYMBOL_L1,
  score: 10,
};

export function L1MakeToken(): string {
  const impl = new BenchRunnerL1Impl();
  const a = BENCH_CHAIN_SYMBOL_L1;
  const b = BENCH_CHAIN_SYMBOL_L1 + "-a";
  const c = BENCH_CHAIN_SYMBOL_L1 + "-b";
  return impl.run(benchObjL1.id) + ":" + a + ":" + b + ":" + c;
}

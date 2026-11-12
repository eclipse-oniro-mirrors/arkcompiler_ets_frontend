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

import { FeatureAdmin, FeatureEntity, FeatureRegistry } from "./domain";
import { BuildStableKey, NormalizeTag } from "./helpers";

export function createAdmin(name: string): FeatureAdmin {
  const stable = BuildStableKey(name, "default");
  return new FeatureAdmin(100, stable, NormalizeTag("Feature-Default"));
}

export function createBatch(names: string[]): FeatureEntity[] {
  const out: FeatureEntity[] = [];
  let id = 1;
  for (const name of names) {
    out.push(new FeatureAdmin(id, name, NormalizeTag("batch")));
    id++;
  }
  return out;
}

export function registerDefaults(registry: FeatureRegistry): number {
  const a = createAdmin("Mika");
  const b = createAdmin("Nora");
  registry.add(a);
  registry.add(b);
  return registry.all().length;
}

export function calculateHealthScore(registry: FeatureRegistry): number {
  const count = registry.all().length;
  if (count == 0) {
    return 0;
  }
  return count * 10;
}

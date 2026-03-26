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

import { FeatureAdmin, FeatureEntity, FeatureRegistry, FindByName } from "./domain";
import { createAdmin, createBatch } from "./services";
import { CreateSymbolRegistry, SymbolRegistry } from "./registry";
import { BuildStableKey, NormalizeTag } from "./helpers";

export const SMALL_SHARED_ANCHOR = "SMALL_SHARED_ANCHOR";

export interface SmallCasePoint {
  id: number;
  key: string;
  score: number;
}

export class SharedSmallScenario {
  private points: SmallCasePoint[] = [];
  private registry: FeatureRegistry = new FeatureRegistry();
  private symbols: SymbolRegistry | null = null;

  init(seed: number): void {
    this.points = BuildSmallPoints(seed);
    const anchorAdmin: FeatureAdmin = createAdmin("SmallAnchor");
    anchorAdmin.featureTag = NormalizeTag("Small-Shared-Anchor");
    this.registry.add(anchorAdmin);
    this.symbols = CreateSymbolRegistry(createBatch(["Kai", "Lia", "Mio"]));
  }

  findAnchorName(): string {
    const hit: FeatureEntity | null = FindByName(this.registry, "default::SmallAnchor");
    return hit != null ? hit.name : "none";
  }

  completionPrefix(index: number): string {
    const point = this.points[index % this.points.length];
    return point.key + ":" + point.score.toString();
  }

  definitionTarget(): string {
    return BuildStableKey("small-definition", "projectA");
  }

  referencesTarget(): string {
    return SMALL_SHARED_ANCHOR + "|" + this.findAnchorName();
  }

  summary(): string {
    const symbolCount = this.symbols != null ? this.symbols.size().toString() : "0";
    return this.definitionTarget() + "|" + this.referencesTarget() + "|" + symbolCount;
  }
}

export function BuildSmallPoints(seed: number): SmallCasePoint[] {
  const points: SmallCasePoint[] = [];
  for (let i = 0; i < 16; i++) {
    points.push({
      id: seed + i,
      key: BuildStableKey("small_key_" + i.toString(), "small"),
      score: seed + i * 2,
    });
  }
  return points;
}

export function SmallAnchorUse0(): string { return SMALL_SHARED_ANCHOR + "_0"; }
export function SmallAnchorUse1(): string { return SMALL_SHARED_ANCHOR + "_1"; }
export function SmallAnchorUse2(): string { return SMALL_SHARED_ANCHOR + "_2"; }
export function SmallAnchorUse3(): string { return SMALL_SHARED_ANCHOR + "_3"; }
export function SmallAnchorUse4(): string { return SMALL_SHARED_ANCHOR + "_4"; }
export function SmallAnchorUse5(): string { return SMALL_SHARED_ANCHOR + "_5"; }
export function SmallAnchorUse6(): string { return SMALL_SHARED_ANCHOR + "_6"; }
export function SmallAnchorUse7(): string { return SMALL_SHARED_ANCHOR + "_7"; }

export function RunSharedSmallScenario(seed: number): string {
  const scenario = new SharedSmallScenario();
  scenario.init(seed);
  const refs = SmallAnchorUse0() + "|" + SmallAnchorUse1() + "|" + SmallAnchorUse2();
  const quick = scenario.completionPrefix(3);
  return scenario.summary() + "|" + refs + "|" + quick;
}

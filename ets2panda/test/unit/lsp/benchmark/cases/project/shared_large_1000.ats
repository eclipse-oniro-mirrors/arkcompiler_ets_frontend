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
import { createAdmin, createBatch, calculateHealthScore } from "./services";
import { CreateSymbolRegistry, SymbolRegistry } from "./registry";
import { BuildStableKey, NormalizeTag } from "./helpers";

export const LARGE_SHARED_ANCHOR = "LARGE_SHARED_ANCHOR";

export interface SharedLargeItem {
  id: number;
  key: string;
  weight: number;
  tag: string;
  owner: string;
}

export interface SharedLargeIndex {
  id: number;
  key: string;
}

export interface NodeResolver {
  Resolve(input: string): number;
}

export abstract class BaseResolver implements NodeResolver {
  protected baseWeight: number;

  constructor(baseWeight: number) {
    this.baseWeight = baseWeight;
  }

  Resolve(input: string): number {
    return this.baseWeight + input.length;
  }

  abstract Kind(): string;
}

export class KeyResolver extends BaseResolver {
  constructor() {
    super(10);
  }

  // Override: same method name in child class with specialized behavior.
  Resolve(input: string): number {
    return super.Resolve(input) + 100;
  }

  Kind(): string {
    return "key";
  }
}

export class TagResolver extends BaseResolver {
  constructor() {
    super(20);
  }

  // Override: same method name in child class with specialized behavior.
  Resolve(input: string): number {
    return super.Resolve(NormalizeTag(input)) + 200;
  }

  Kind(): string {
    return "tag";
  }
}

export class CompositeResolver extends BaseResolver {
  private keyResolver: KeyResolver = new KeyResolver();
  private tagResolver: TagResolver = new TagResolver();

  constructor() {
    super(30);
  }

  // Override: merges parent behavior with child-owned strategy calls.
  Resolve(input: string): number {
    return super.Resolve(input) + this.keyResolver.Resolve(input) + this.tagResolver.Resolve(input);
  }

  Kind(): string {
    return "composite";
  }
}

// Overload signatures used by definition/quick-info/completion benches.
export function ResolveByOverload(input: string): number;
export function ResolveByOverload(input: number): number;
export function ResolveByOverload(input: SharedLargeItem): number;
export function ResolveByOverload(input: string | number | SharedLargeItem): number {
  if (typeof input == "string") {
    return input.length;
  }
  if (typeof input == "number") {
    return input;
  }
  return input.weight + input.id;
}

export function ResolveWithPolymorphism(resolver: BaseResolver, key: string): string {
  const score = resolver.Resolve(key);
  return resolver.Kind() + ":" + score.toString();
}

export class SharedLargeStore {
  private items: SharedLargeItem[] = [];
  private keyToIndex: Map<string, number> = new Map();

  add(item: SharedLargeItem): void {
    this.keyToIndex.set(item.key, this.items.length);
    this.items.push(item);
  }

  size(): number {
    return this.items.length;
  }

  findByKey(key: string): SharedLargeItem | null {
    const index = this.keyToIndex.get(key);
    if (index == null) {
      return null;
    }
    return this.items[index];
  }

  at(index: number): SharedLargeItem | null {
    if (index < 0 || index >= this.items.length) {
      return null;
    }
    return this.items[index];
  }

  makeIndex(): SharedLargeIndex[] {
    const out: SharedLargeIndex[] = [];
    for (const item of this.items) {
      out.push({ id: item.id, key: item.key });
    }
    return out;
  }
}

export function MakeAnchorAdmin(): FeatureAdmin {
  const targetAdminValue: FeatureAdmin = createAdmin("LargeAnchor");
  targetAdminValue.featureTag = NormalizeTag("Shared-Large-Anchor");
  targetAdminValue.traceId = "trace-large-anchor";
  return targetAdminValue;
}

export function BuildCrossFileState(): string {
  const registry = new FeatureRegistry();
  const targetAdminValue = MakeAnchorAdmin();
  registry.add(targetAdminValue);
  const users = createBatch(["A", "B", "C", "D", "E"]);
  const symbolRegistry: SymbolRegistry = CreateSymbolRegistry(users);
  const hit: FeatureEntity | null = FindByName(registry, targetAdminValue.name);
  const hitName = hit != null ? hit.name : "none";
  const score = calculateHealthScore(registry);
  return targetAdminValue.featureTag + ":" + hitName + ":" + score.toString() + ":" + symbolRegistry.size().toString();
}

export function BuildLargeKey(index: number): string {
  return BuildStableKey("k" + index.toString(), "large_group_" + (index % 19).toString());
}

export function BuildLargeItem(seed: number, index: number): SharedLargeItem {
  return {
    id: seed + index,
    key: BuildLargeKey(index),
    weight: seed + index * 3,
    tag: NormalizeTag("TAG_" + index.toString()),
    owner: "owner_" + (index % 13).toString(),
  };
}

export function BuildLargeStore(seed: number, count: number): SharedLargeStore {
  const store = new SharedLargeStore();
  for (let i = 0; i < count; i++) {
    store.add(BuildLargeItem(seed, i));
  }
  return store;
}

export class LargeScenarioRunner {
  private store: SharedLargeStore = new SharedLargeStore();
  private checksum: number = 0;

  load(seed: number): void {
    this.store = BuildLargeStore(seed, 320);
    this.checksum = 0;
    for (let i = 0; i < 80; i++) {
      const item = this.store.at(i);
      if (item != null) {
        this.checksum += item.weight + item.key.length + item.owner.length;
      }
    }
  }

  completionTarget(prefix: string): string[] {
    const out: string[] = [];
    for (let i = 0; i < 24; i++) {
      out.push(prefix + "_" + BuildLargeKey(i));
    }
    return out;
  }

  definitionTarget(): string {
    return BuildStableKey("large-definition-target", "projectA");
  }

  referencesTarget(): string {
    const mid = this.store.at(42);
    const key = mid != null ? mid.key : "none";
    return LARGE_SHARED_ANCHOR + "|" + key + "|" + LARGE_SHARED_ANCHOR;
  }

  quickInfoTarget(): string {
    const first = this.store.at(0);
    return first != null ? first.tag + "|" + first.owner : "none";
  }

  renameTarget(): string {
    return LARGE_SHARED_ANCHOR + "_rename_target";
  }

  summary(): string {
    return this.definitionTarget() + "|" + this.referencesTarget() + "|" + this.quickInfoTarget() + "|" +
      this.checksum.toString();
  }
}

// Shared anchors for references/rename/definition.
export function AnchorUse0(): string { return LARGE_SHARED_ANCHOR + "_0"; }
export function AnchorUse1(): string { return LARGE_SHARED_ANCHOR + "_1"; }
export function AnchorUse2(): string { return LARGE_SHARED_ANCHOR + "_2"; }
export function AnchorUse3(): string { return LARGE_SHARED_ANCHOR + "_3"; }
export function AnchorUse4(): string { return LARGE_SHARED_ANCHOR + "_4"; }
export function AnchorUse5(): string { return LARGE_SHARED_ANCHOR + "_5"; }
export function AnchorUse6(): string { return LARGE_SHARED_ANCHOR + "_6"; }
export function AnchorUse7(): string { return LARGE_SHARED_ANCHOR + "_7"; }
export function AnchorUse8(): string { return LARGE_SHARED_ANCHOR + "_8"; }
export function AnchorUse9(): string { return LARGE_SHARED_ANCHOR + "_9"; }
export function AnchorUse10(): string { return LARGE_SHARED_ANCHOR + "_10"; }
export function AnchorUse11(): string { return LARGE_SHARED_ANCHOR + "_11"; }
export function AnchorUse12(): string { return LARGE_SHARED_ANCHOR + "_12"; }
export function AnchorUse13(): string { return LARGE_SHARED_ANCHOR + "_13"; }
export function AnchorUse14(): string { return LARGE_SHARED_ANCHOR + "_14"; }
export function AnchorUse15(): string { return LARGE_SHARED_ANCHOR + "_15"; }

// Dense but non-bloated list used by completion/quick-info/references benches.
export const LARGE_REFERENCE_POOL: string[] = [
  "LARGE_SHARED_ANCHOR_ref_000",
  "LARGE_SHARED_ANCHOR_ref_001",
  "LARGE_SHARED_ANCHOR_ref_002",
  "LARGE_SHARED_ANCHOR_ref_003",
  "LARGE_SHARED_ANCHOR_ref_004",
  "LARGE_SHARED_ANCHOR_ref_005",
  "LARGE_SHARED_ANCHOR_ref_006",
  "LARGE_SHARED_ANCHOR_ref_007",
  "LARGE_SHARED_ANCHOR_ref_008",
  "LARGE_SHARED_ANCHOR_ref_009",
  "LARGE_SHARED_ANCHOR_ref_010",
  "LARGE_SHARED_ANCHOR_ref_011",
  "LARGE_SHARED_ANCHOR_ref_012",
  "LARGE_SHARED_ANCHOR_ref_013",
  "LARGE_SHARED_ANCHOR_ref_014",
  "LARGE_SHARED_ANCHOR_ref_015",
  "LARGE_SHARED_ANCHOR_ref_016",
  "LARGE_SHARED_ANCHOR_ref_017",
  "LARGE_SHARED_ANCHOR_ref_018",
  "LARGE_SHARED_ANCHOR_ref_019",
  "LARGE_SHARED_ANCHOR_ref_020",
  "LARGE_SHARED_ANCHOR_ref_021",
  "LARGE_SHARED_ANCHOR_ref_022",
  "LARGE_SHARED_ANCHOR_ref_023",
  "LARGE_SHARED_ANCHOR_ref_024",
  "LARGE_SHARED_ANCHOR_ref_025",
  "LARGE_SHARED_ANCHOR_ref_026",
  "LARGE_SHARED_ANCHOR_ref_027",
  "LARGE_SHARED_ANCHOR_ref_028",
  "LARGE_SHARED_ANCHOR_ref_029",
  "LARGE_SHARED_ANCHOR_ref_030",
  "LARGE_SHARED_ANCHOR_ref_031",
  "LARGE_SHARED_ANCHOR_ref_032",
  "LARGE_SHARED_ANCHOR_ref_033",
  "LARGE_SHARED_ANCHOR_ref_034",
  "LARGE_SHARED_ANCHOR_ref_035",
  "LARGE_SHARED_ANCHOR_ref_036",
  "LARGE_SHARED_ANCHOR_ref_037",
  "LARGE_SHARED_ANCHOR_ref_038",
  "LARGE_SHARED_ANCHOR_ref_039",
];

export function RunLargeScenario(seed: number): string {
  const runner = new LargeScenarioRunner();
  runner.load(seed);

  const completionHead = runner.completionTarget("head");
  const completionTail = runner.completionTarget("tail");
  const tempItem = BuildLargeItem(seed, 7);
  const overloadValue =
    ResolveByOverload(tempItem) + ResolveByOverload("entry") + ResolveByOverload(seed);
  const keyMsg = ResolveWithPolymorphism(new KeyResolver(), completionHead[0]);
  const tagMsg = ResolveWithPolymorphism(new TagResolver(), completionTail[1]);
  const compositeMsg = ResolveWithPolymorphism(new CompositeResolver(), BuildLargeKey(5));
  const refs = AnchorUse0() + "|" + AnchorUse1() + "|" + AnchorUse2() + "|" + AnchorUse3();
  const poolKey = LARGE_REFERENCE_POOL[seed % LARGE_REFERENCE_POOL.length];
  const summary = runner.summary();
  return summary + "|" + refs + "|" + poolKey + "|" + completionHead[0] + "|" + completionTail[1] + "|" +
    runner.renameTarget() + "|" + overloadValue.toString() + "|" + keyMsg + "|" + tagMsg + "|" + compositeMsg;
}

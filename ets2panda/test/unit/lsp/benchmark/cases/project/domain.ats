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

export interface FeatureEntity {
  id: number;
  name: string;
  featureTag: string;
}

export class FeatureAdmin implements FeatureEntity {
  id: number;
  name: string;
  featureTag: string;
  traceId: string;

  constructor(id: number, name: string, featureTag: string) {
    this.id = id;
    this.name = name;
    this.featureTag = featureTag;
    this.traceId = "trace-" + id.toString();
  }
}

export class RegistrySnapshot {
  readonly count: number;
  readonly firstName: string;

  constructor(count: number, firstName: string) {
    this.count = count;
    this.firstName = firstName;
  }
}

export class FeatureRegistry {
  private entities: FeatureEntity[] = [];

  add(entity: FeatureEntity): void {
    this.entities.push(entity);
  }

  all(): FeatureEntity[] {
    return this.entities;
  }

  snapshot(): RegistrySnapshot {
    const first = this.entities.length > 0 ? this.entities[0].name : "";
    return new RegistrySnapshot(this.entities.length, first);
  }
}

export function CreateDefaultEntities(): FeatureEntity[] {
  return [
    new FeatureAdmin(1, "alpha", "search"),
    new FeatureAdmin(2, "beta", "ads"),
    new FeatureAdmin(3, "gamma", "storage"),
  ];
}

export function FindByName(registry: FeatureRegistry, name: string): FeatureEntity | null {
  for (const entity of registry.all()) {
    if (entity.name == name) {
      return entity;
    }
  }
  return null;
}

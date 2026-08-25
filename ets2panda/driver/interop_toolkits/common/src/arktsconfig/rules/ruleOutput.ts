/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

import type { DependencyItem } from '../arktsconfig';

export interface RuleOutput {
  readonly paths: ReadonlyMap<string, readonly string[]>;
  readonly dependencies: ReadonlyMap<string, DependencyItem>;
}

export interface MutableRuleOutput {
  readonly paths: Map<string, string[]>;
  readonly dependencies: Map<string, DependencyItem>;
}

export function createRuleOutput(): MutableRuleOutput {
  return { paths: new Map(), dependencies: new Map() };
}

export function mergeRuleOutput(target: MutableRuleOutput, source: RuleOutput): void {
  for (const [key, values] of source.paths) {
    appendUniquePath(target.paths, key, values);
  }
  for (const [key, value] of source.dependencies) {
    if (!target.dependencies.has(key)) {
      target.dependencies.set(key, value);
    }
  }
}

export function appendUniquePath(target: Map<string, string[]>, key: string, values: readonly string[]): void {
  const current = target.get(key) ?? [];
  const seen = new Set(current);
  for (const value of values) {
    if (!seen.has(value)) {
      current.push(value);
      seen.add(value);
    }
  }
  target.set(key, current);
}

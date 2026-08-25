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

import * as context from '../context';
import { DependencyNode } from './graph';
import * as common from '@interop-toolkits/common';

/**
 * Result of a single-language (partial) resolution pass.
 *
 * `nodes` contains every file reachable within this resolver's language, plus
 * sentinel placeholders for files that cross the language boundary. `sentinels`
 * lists the keys of those boundary files so the orchestrator can hand them to
 * the other resolver.
 */
export interface PartialResolvedDependencyMap {
  nodes: Map<string, DependencyNode>;
  sentinels: string[];
}

/**
 * Base class for a single-language resolver. Concrete implementations
 * (`DynamicResolver`, `StaticResolver`) drive their respective compilers and
 * mark cross-language references as sentinels.
 */
export abstract class PartialResolver {
  private resolverContext?: context.Context;

  get context(): context.Context {
    if (!this.resolverContext) {
      throw new common.errors.InternalError('Resolver context is not initialized');
    }
    return this.resolverContext;
  }

  setContext(context: context.Context): void {
    this.resolverContext = context;
  }

  /** Resolve the dependency subgraph reachable (within this language) from the given entries. */
  abstract resolve(entryFiles: string[]): PartialResolvedDependencyMap;
}

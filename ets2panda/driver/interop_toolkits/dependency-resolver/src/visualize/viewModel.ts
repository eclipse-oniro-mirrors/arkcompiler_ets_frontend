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

import * as path from 'path';
import * as common from '@interop-toolkits/common';
import type * as context from '../context';
import { NodeType } from '../resolver/graph';
import type { DependencyGraph } from '../resolver/graph';

/** How a file is classified for coloring in the dependency graph. */
export type FileKind = 'static' | 'dynamic';

/** A single file, drawn as a circle or a square sentinel in the visualization. */
export interface GraphFileView {
  /** Stable, unique id (normalized absolute path). */
  id: string;
  /** Short display label (file basename). */
  label: string;
  /** Full normalized path, shown in the tooltip. */
  path: string;
  /** Classification that drives the fill color. */
  kind: FileKind;
  /** Whether the file was discovered as a cross-language sentinel. */
  isSentinel: boolean;
  /** Whether the file belongs to an SDK or interop SDK. */
  isSdk?: boolean;
  /** Id of the module this file belongs to. */
  moduleId: string;
  /** Ids of files this file directly depends on. */
  dependencies: string[];
}

/** A group of files, drawn as a labeled box around its circles. */
export interface GraphModuleView {
  id: string;
  label: string;
  language?: string;
}

/**
 * Serializable, render-agnostic description of a dependency graph.
 *
 * This is the interchange format shared by the JSON and HTML outputs: the JSON
 * format prints it verbatim, and the HTML renderer draws it.
 */
export interface GraphViewModel {
  modules: GraphModuleView[];
  files: GraphFileView[];
}

/** Module id used for files that cannot be attributed to a project module. */
export const EXTERNAL_MODULE_ID = '(external)';
export const STATIC_SDK_MODULE_ID = 'static sdk';
export const DYNAMIC_SDK_MODULE_ID = 'dynamic sdk';
export const STATIC_INTEROP_SDK_MODULE_ID = 'static interop sdk';
export const DYNAMIC_INTEROP_SDK_MODULE_ID = 'dynamic interop sdk';

function kindFromNodeType(type: NodeType): FileKind {
  switch (type) {
    case NodeType.STATIC:
      return 'static';
    case NodeType.DYNAMIC:
      return 'dynamic';
  }
  throw new Error('This line should never be reachable');
}

function kindFromLanguage(language: common.fileUtils.Language): FileKind {
  return language === common.fileUtils.Language.DYNAMIC ? 'dynamic' : 'static';
}

/**
 * Build a {@link GraphViewModel} from a resolved {@link DependencyGraph}.
 *
 * When a {@link ProjectManifest} is supplied, files are grouped into their
 * owning modules; otherwise every file falls into a single external group.
 */
export function graphToViewModel(graph: DependencyGraph, context?: context.Context): GraphViewModel {
  const modules = new Map<string, GraphModuleView>();
  const files: GraphFileView[] = [];

  const resolveFile = (filePath: string, nodeType: NodeType): { moduleId: string; kind: FileKind; isSdk: boolean } => {
    if (!context) {
      return { moduleId: EXTERNAL_MODULE_ID, kind: kindFromNodeType(nodeType), isSdk: false };
    }
    const meta = context.fileManager.queryFileMeta(filePath);
    if (meta === undefined) {
      return { moduleId: EXTERNAL_MODULE_ID, kind: kindFromNodeType(nodeType), isSdk: false };
    }
    const module = moduleViewFromMeta(meta);
    modules.set(module.id, module);
    return {
      moduleId: module.id,
      kind: kindFromLanguage(meta.language),
      isSdk: meta.owner === common.fileManager.Owner.SDK || meta.owner === common.fileManager.Owner.INTEROP_SDK,
    };
  };

  for (const [key, node] of graph.nodes) {
    const { moduleId, kind, isSdk } = resolveFile(key, node.type);
    files.push({
      id: key,
      label: path.basename(key),
      path: key,
      kind,
      isSentinel: node.isSentinel,
      isSdk,
      moduleId,
      dependencies: [...node.dependencies],
    });
  }

  ensureModule(modules, files);
  return { modules: [...modules.values()], files };
}

function moduleViewFromMeta(meta: common.fileManager.FileMeta): GraphModuleView {
  if (meta.owner === common.fileManager.Owner.MODULE && meta.module !== undefined) {
    return {
      id: meta.module.moduleName,
      label: meta.module.moduleName,
      language: meta.module.language,
    };
  }
  const id = sdkModuleId(meta);
  return { id, label: id, language: meta.language };
}

function sdkModuleId(meta: common.fileManager.FileMeta): string {
  if (meta.owner === common.fileManager.Owner.SDK) {
    return meta.language === common.fileUtils.Language.STATIC ? STATIC_SDK_MODULE_ID : DYNAMIC_SDK_MODULE_ID;
  }
  return meta.language === common.fileUtils.Language.DYNAMIC
    ? STATIC_INTEROP_SDK_MODULE_ID
    : DYNAMIC_INTEROP_SDK_MODULE_ID;
}

/**
 * Build a {@link GraphViewModel} from the lightweight CLI resolve result.
 *
 * Used until the full cross-language resolver is wired into the CLI: it yields
 * a structurally correct graph (entry plus its direct dependencies) that the
 * renderer can draw and that enriches automatically once real node metadata
 * becomes available.
 */
export function resolveResultToViewModel(result: { entry: string; dependencies: string[] }): GraphViewModel {
  const entryId = common.fileUtils.normalizePath(result.entry);
  const depIds = result.dependencies.map((dep) => common.fileUtils.normalizePath(dep));

  const files: GraphFileView[] = [
    {
      id: entryId,
      label: path.basename(entryId),
      path: entryId,
      kind: 'static',
      isSentinel: false,
      moduleId: EXTERNAL_MODULE_ID,
      dependencies: depIds,
    },
    ...depIds.map((dep) => ({
      id: dep,
      label: path.basename(dep),
      path: dep,
      kind: 'static' as FileKind,
      isSentinel: false,
      moduleId: EXTERNAL_MODULE_ID,
      dependencies: [] as string[],
    })),
  ];

  const modules = new Map<string, GraphModuleView>();
  ensureModule(modules, files);
  return { modules: [...modules.values()], files };
}

/** Guarantee that every module referenced by a file has a module entry. */
function ensureModule(modules: Map<string, GraphModuleView>, files: GraphFileView[]): void {
  for (const file of files) {
    if (!modules.has(file.moduleId)) {
      modules.set(file.moduleId, { id: file.moduleId, label: file.moduleId });
    }
  }
}

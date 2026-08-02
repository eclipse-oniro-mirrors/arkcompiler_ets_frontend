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

import { promises as fs, type Stats } from 'node:fs';
import * as path from 'node:path';

import JSON5 from 'json5';

import {
  GlueGenDiagnosticError,
  GlueGenError,
  GlueGenErrorCode,
  GlueGenInternalError,
  errorMessage,
} from '../../errors';
import { LogData } from '../../logger';
import type { GlueGenContext } from '../../pipeline/context';
import type { StageScope } from '../../pipeline';
import { hasEtsSourceExtension } from '../../utils/staticSource';
import type { InteropTarget } from './interopTarget';
import { reachableModulesOf, type ModuleInfo, type ModuleTable } from './moduleTable';

interface ResolveModulesInput {
  readonly 'resolve-modules': ModuleTable;
}

interface InteropContribution {
  readonly packageName: string;
  readonly target: InteropTarget;
}

interface InteropDependencyEntries {
  readonly package?: readonly string[];
  readonly source?: Readonly<
    Record<
      string,
      {
        readonly static?: readonly string[];
      }
    >
  >;
}

/** Only the module-level fields consumed by gluegen; other interop fields are ignored. */
interface InteropConfigFile {
  readonly interopEntries?: {
    readonly static?: readonly string[];
    readonly dependency?: InteropDependencyEntries;
  };
}

export async function resolveInteropConfig(
  _scope: StageScope<GlueGenContext, readonly []>,
  inputs: ResolveModulesInput,
): Promise<ReadonlyMap<string, InteropTarget>> {
  try {
    return await buildInteropTargets(inputs['resolve-modules']);
  } catch (error) {
    if (error instanceof GlueGenError) {
      throw error;
    }
    const data = new LogData({
      code: GlueGenErrorCode.INTERNAL_FAILURE,
      description: 'Gluegen could not resolve the module interop configuration.',
      cause: errorMessage(error, 'unknown interop configuration failure'),
    });
    throw new GlueGenInternalError(data);
  }
}

async function buildInteropTargets(table: ModuleTable): Promise<ReadonlyMap<string, InteropTarget>> {
  const targets = new Map<string, InteropTarget>();
  const modules = reachableModulesOf(table);
  const modulesByPackage = new Map(modules.map((module) => [module.packageName, module]));
  for (const module of modules) {
    const contributions = await resolveModuleInteropConfig(modulesByPackage, module);
    for (const contribution of contributions) {
      mergeTarget(targets, contribution.packageName, contribution.target);
    }
  }
  return targets;
}

async function resolveModuleInteropConfig(
  modulesByPackage: ReadonlyMap<string, ModuleInfo>,
  module: ModuleInfo,
): Promise<readonly InteropContribution[]> {
  const configPath = module.interopConfigPath;
  if (configPath === undefined) {
    return [];
  }

  const config: InteropConfigFile = await readInteropConfig(module.packageName, configPath);
  const entries = config.interopEntries;
  if (entries === undefined) {
    return [];
  }

  const contributions: InteropContribution[] = [];
  await addOwnInteropContribution(contributions, entries.static, module);
  await addDependencyInteropContributions(contributions, entries.dependency, modulesByPackage, module, configPath);
  return contributions;
}

async function addDependencyInteropContributions(
  contributions: InteropContribution[],
  dependency: InteropDependencyEntries | undefined,
  modulesByPackage: ReadonlyMap<string, ModuleInfo>,
  module: ModuleInfo,
  configPath: string,
): Promise<void> {
  if (dependency === undefined) {
    return;
  }
  addPackageInteropContributions(contributions, dependency.package ?? [], modulesByPackage, module, configPath);
  await addSourceInteropContributions(contributions, dependency.source ?? {}, modulesByPackage, module, configPath);
}

async function addOwnInteropContribution(
  contributions: InteropContribution[],
  values: readonly string[] | undefined,
  module: ModuleInfo,
): Promise<void> {
  const ownFiles = await resolveStaticFiles(values ?? [], module);
  if (ownFiles.length > 0) {
    contributions.push({
      packageName: module.packageName,
      target: { kind: 'items', files: ownFiles },
    });
  }
}

function addPackageInteropContributions(
  contributions: InteropContribution[],
  packageNames: readonly string[],
  modulesByPackage: ReadonlyMap<string, ModuleInfo>,
  module: ModuleInfo,
  configPath: string,
): void {
  for (const packageName of packageNames) {
    const targetModule = requireInteropModule(modulesByPackage, module.packageName, configPath, packageName);
    contributions.push({
      packageName,
      target: { kind: 'package', moduleInfo: targetModule },
    });
  }
}

async function addSourceInteropContributions(
  contributions: InteropContribution[],
  sources: NonNullable<InteropDependencyEntries['source']>,
  modulesByPackage: ReadonlyMap<string, ModuleInfo>,
  module: ModuleInfo,
  configPath: string,
): Promise<void> {
  for (const [packageName, selected] of Object.entries(sources)) {
    const targetModule = requireInteropModule(modulesByPackage, module.packageName, configPath, packageName);
    const files = await resolveStaticFiles(selected.static ?? [], targetModule);
    if (files.length > 0) {
      contributions.push({
        packageName,
        target: { kind: 'items', files },
      });
    }
  }
}

async function readInteropConfig(packageName: string, configPath: string): Promise<InteropConfigFile> {
  try {
    const content = await fs.readFile(configPath, 'utf8');
    return JSON5.parse(content) as InteropConfigFile;
  } catch (error) {
    throw new GlueGenDiagnosticError(
      new LogData({
        code: GlueGenErrorCode.INVALID_INTEROP_CONFIG,
        description: `The interop configuration for package "${packageName}" ` + 'could not be read or parsed.',
        cause: errorMessage(error, 'unknown interop configuration failure'),
        position: configPath,
        solutions: ['Check that the file exists and contains valid JSON5.'],
        moreInfo: { packageName },
      }),
    );
  }
}

async function resolveStaticFiles(values: readonly string[], module: ModuleInfo): Promise<readonly string[]> {
  if (values.length === 0) {
    return [];
  }

  const files = new Set<string>();
  for (const value of values) {
    const filePath = path.resolve(module.modulePath, value);
    if (files.has(filePath)) {
      continue;
    }
    await validateStaticFile(filePath, module);
    files.add(filePath);
  }
  return [...files];
}

async function validateStaticFile(filePath: string, module: ModuleInfo): Promise<void> {
  validateStaticFileExtension(filePath, module);
  const stats = await statStaticFile(filePath, module);
  validateStaticFileType(stats, filePath, module);
  validateStaticFileLocation(filePath, module);
}

function validateStaticFileExtension(filePath: string, module: ModuleInfo): void {
  if (!hasEtsSourceExtension(filePath)) {
    throw new GlueGenDiagnosticError(
      new LogData({
        code: GlueGenErrorCode.INVALID_INTEROP_CONFIG,
        description:
          `Package "${module.packageName}" declares a static interop file ` + 'with an unsupported extension.',
        cause: 'Static interop files must use a lowercase .ets or .d.ets extension.',
        position: filePath,
        solutions: ['Rename the file or correct its path in the interop configuration.'],
        moreInfo: { packageName: module.packageName },
      }),
    );
  }
}

async function statStaticFile(filePath: string, module: ModuleInfo): Promise<Stats> {
  try {
    return await fs.stat(filePath);
  } catch (error) {
    throw new GlueGenDiagnosticError(
      new LogData({
        code: GlueGenErrorCode.INVALID_INTEROP_CONFIG,
        description: `A static interop file declared by package ` + `"${module.packageName}" cannot be accessed.`,
        cause: errorMessage(error, 'unknown interop configuration failure'),
        position: filePath,
        solutions: ['Check that the file exists and is readable.'],
        moreInfo: { packageName: module.packageName },
      }),
    );
  }
}

function validateStaticFileType(stats: Stats, filePath: string, module: ModuleInfo): void {
  if (!stats.isFile()) {
    throw new GlueGenDiagnosticError(
      new LogData({
        code: GlueGenErrorCode.INVALID_INTEROP_CONFIG,
        description: `A static interop path declared by package ` + `"${module.packageName}" is not a regular file.`,
        position: filePath,
        solutions: ['Point the interop configuration to an .ets or .d.ets file.'],
        moreInfo: { packageName: module.packageName },
      }),
    );
  }
}

function validateStaticFileLocation(filePath: string, module: ModuleInfo): void {
  if (!isPathInside(module.modulePath, filePath)) {
    throw new GlueGenDiagnosticError(
      new LogData({
        code: GlueGenErrorCode.INVALID_INTEROP_CONFIG,
        description: `Package "${module.packageName}" declares a static interop file ` + 'outside its module root.',
        cause: `Module root: ${module.modulePath}`,
        position: filePath,
        solutions: ['Move the file under the module root or correct the configured path.'],
        moreInfo: { packageName: module.packageName },
      }),
    );
  }
}

/**
 * Validates that a package named in interopConfig is in the Main Module dependency graph.
 * This guard can be removed once IDE-side schema validation guarantees valid package references.
 */
function requireInteropModule(
  modulesByPackage: ReadonlyMap<string, ModuleInfo>,
  ownerPackageName: string,
  interopConfigPath: string,
  packageName: string,
): ModuleInfo {
  const target = modulesByPackage.get(packageName);
  if (target === undefined) {
    throw new GlueGenDiagnosticError(
      new LogData({
        code: GlueGenErrorCode.INVALID_INTEROP_CONFIG,
        description: `Package "${ownerPackageName}" does not have a dependency named ` + `"${packageName}".`,
        cause: `No package named "${packageName}" is reachable in the Main Module ` + 'dependency graph.',
        position: interopConfigPath,
        solutions: [`Add "${packageName}" to the module dependencies, or remove the ` + 'interop reference.'],
        moreInfo: {
          packageName: ownerPackageName,
          dependencyName: packageName,
        },
      }),
    );
  }
  return target;
}

function mergeTarget(targets: Map<string, InteropTarget>, packageName: string, incoming: InteropTarget): void {
  const current = targets.get(packageName);
  if (current?.kind === 'package') {
    return;
  }
  if (incoming.kind === 'package' || current === undefined) {
    targets.set(packageName, incoming);
    return;
  }
  targets.set(packageName, {
    kind: 'items',
    files: [...new Set([...current.files, ...incoming.files])],
  });
}

function isPathInside(parentPath: string, childPath: string): boolean {
  const relativePath = path.relative(parentPath, childPath);
  return (
    relativePath !== '' &&
    relativePath !== '..' &&
    !relativePath.startsWith(`..${path.sep}`) &&
    !path.isAbsolute(relativePath)
  );
}

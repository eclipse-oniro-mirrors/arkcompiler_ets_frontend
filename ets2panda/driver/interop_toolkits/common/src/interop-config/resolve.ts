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

import { InteropConfigError, errorMessage } from './errors';
import { Extension } from '../fileUtils';
import type { InteropTarget } from './types';
import type { InteropConfigModuleInfo, ModuleTable } from './types';

/** Resolves a module's dependency names to their immutable table entries. */
export function dependencyModulesOf(
  table: ModuleTable,
  module: InteropConfigModuleInfo,
): readonly InteropConfigModuleInfo[] {
  return module.dependencies
    .map((packageName) => table.byPackage.get(packageName))
    .filter((dependency): dependency is InteropConfigModuleInfo => dependency !== undefined);
}

/** Finds a module and all its direct and transitive dependencies once, in traversal order. */
export function reachableModulesOf(
  table: ModuleTable,
  root: InteropConfigModuleInfo = table.mainModule,
): readonly InteropConfigModuleInfo[] {
  const modules: InteropConfigModuleInfo[] = [];
  const visited = new Set<string>();
  const visit = (module: InteropConfigModuleInfo): void => {
    if (visited.has(module.packageName)) {
      return;
    }
    visited.add(module.packageName);
    modules.push(module);
    dependencyModulesOf(table, module).forEach(visit);
  };
  visit(root);
  return modules;
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
        readonly dynamic?: readonly string[];
      }
    >
  >;
}

/** Only the module-level fields consumed by gluegen; other interop fields are ignored. */
interface InteropConfigFile {
  readonly interopEntries?: {
    readonly static?: readonly string[];
    readonly dynamic?: readonly string[];
    readonly dependency?: InteropDependencyEntries;
  };
}

export async function resolveInteropConfig(table: ModuleTable): Promise<ReadonlyMap<string, InteropTarget>> {
  const targets = new Map<string, InteropTarget>();
  const modules = reachableModulesOf(table);
  for (const module of modules) {
    const contributions = await resolveModuleInteropConfig(table.byPackage, module);
    for (const contribution of contributions) {
      mergeTarget(targets, contribution.packageName, contribution.target);
    }
  }
  return targets;
}

async function resolveModuleInteropConfig(
  modulesByPackage: ReadonlyMap<string, InteropConfigModuleInfo>,
  module: InteropConfigModuleInfo,
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
  await addOwnInteropContribution(contributions, entries.static, entries.dynamic, module);
  await addDependencyInteropContributions(contributions, entries.dependency, modulesByPackage, module, configPath);
  return contributions;
}

async function addDependencyInteropContributions(
  contributions: InteropContribution[],
  dependency: InteropDependencyEntries | undefined,
  modulesByPackage: ReadonlyMap<string, InteropConfigModuleInfo>,
  module: InteropConfigModuleInfo,
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
  staticValues: readonly string[] | undefined,
  dynamicValues: readonly string[] | undefined,
  module: InteropConfigModuleInfo,
): Promise<void> {
  const staticFiles = await resolveStaticFiles(staticValues ?? [], module);
  const dynamicFiles = await resolveDynamicFiles(dynamicValues ?? [], module);
  if (staticFiles.length > 0 || dynamicFiles.length > 0) {
    contributions.push({
      packageName: module.packageName,
      target: { kind: 'items', staticFiles, dynamicFiles },
    });
  }
}

function addPackageInteropContributions(
  contributions: InteropContribution[],
  packageNames: readonly string[],
  modulesByPackage: ReadonlyMap<string, InteropConfigModuleInfo>,
  module: InteropConfigModuleInfo,
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
  modulesByPackage: ReadonlyMap<string, InteropConfigModuleInfo>,
  module: InteropConfigModuleInfo,
  configPath: string,
): Promise<void> {
  for (const [packageName, selected] of Object.entries(sources)) {
    const targetModule = requireInteropModule(modulesByPackage, module.packageName, configPath, packageName);
    const staticFiles = await resolveStaticFiles(selected.static ?? [], targetModule);
    const dynamicFiles = await resolveDynamicFiles(selected.dynamic ?? [], targetModule);
    if (staticFiles.length > 0 || dynamicFiles.length > 0) {
      contributions.push({
        packageName,
        target: { kind: 'items', staticFiles, dynamicFiles },
      });
    }
  }
}

async function readInteropConfig(packageName: string, configPath: string): Promise<InteropConfigFile> {
  try {
    const content = await fs.readFile(configPath, 'utf8');
    return JSON5.parse(content) as InteropConfigFile;
  } catch (error) {
    throw new InteropConfigError({
      description: `The interop configuration for package "${packageName}" ` + 'could not be read or parsed.',
      cause: errorMessage(error, 'unknown interop configuration failure'),
      position: configPath,
      solutions: ['Check that the file exists and contains valid JSON5.'],
      moreInfo: { packageName },
    });
  }
}

async function resolveStaticFiles(
  values: readonly string[],
  module: InteropConfigModuleInfo,
): Promise<readonly string[]> {
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

async function resolveDynamicFiles(
  values: readonly string[],
  module: InteropConfigModuleInfo,
): Promise<readonly string[]> {
  if (values.length === 0) {
    return [];
  }

  const files = new Set<string>();
  for (const value of values) {
    const filePath = path.resolve(module.modulePath, value);
    if (files.has(filePath)) {
      continue;
    }
    await validateDynamicFile(filePath, module);
    files.add(filePath);
  }
  return [...files];
}

async function validateStaticFile(filePath: string, module: InteropConfigModuleInfo): Promise<void> {
  validateStaticFileExtension(filePath, module);
  const stats = await statStaticFile(filePath, module);
  validateStaticFileType(stats, filePath, module);
  validateStaticFileLocation(filePath, module);
}

async function validateDynamicFile(filePath: string, module: InteropConfigModuleInfo): Promise<void> {
  validateDynamicFileExtension(filePath, module);
  const stats = await statDynamicFile(filePath, module);
  validateDynamicFileType(stats, filePath, module);
  validateDynamicFileLocation(filePath, module);
}

function validateStaticFileExtension(filePath: string, module: InteropConfigModuleInfo): void {
  const hasEtsSourceExtension = (filePath: string): boolean => {
    const ext = path.extname(filePath);
    return ext === Extension.ETS;
  };
  if (!hasEtsSourceExtension(filePath)) {
    throw new InteropConfigError({
      description: `Package "${module.packageName}" declares a static interop file ` + 'with an unsupported extension.',
      cause: 'Static interop files must use a lowercase .ets or .d.ets extension.',
      position: filePath,
      solutions: ['Rename the file or correct its path in the interop configuration.'],
      moreInfo: { packageName: module.packageName },
    });
  }
}

function validateDynamicFileExtension(filePath: string, module: InteropConfigModuleInfo): void {
  const hasDynamicSourceExtension = (file: string): boolean => {
    const ext = path.extname(file);
    return ext === Extension.TS || ext === Extension.ETS;
  };
  if (!hasDynamicSourceExtension(filePath)) {
    throw new InteropConfigError({
      description:
        `Package "${module.packageName}" declares a dynamic interop file ` + 'with an unsupported extension.',
      cause: 'Dynamic interop files must use a lowercase .ts, .d.ts, .ets, or .d.ets extension.',
      position: filePath,
      solutions: ['Rename the file or correct its path in the interop configuration.'],
      moreInfo: { packageName: module.packageName },
    });
  }
}

async function statStaticFile(filePath: string, module: InteropConfigModuleInfo): Promise<Stats> {
  try {
    return await fs.stat(filePath);
  } catch (error) {
    throw new InteropConfigError({
      description: `A static interop file declared by package ` + `"${module.packageName}" cannot be accessed.`,
      cause: errorMessage(error, 'unknown interop configuration failure'),
      position: filePath,
      solutions: ['Check that the file exists and is readable.'],
      moreInfo: { packageName: module.packageName },
    });
  }
}

async function statDynamicFile(filePath: string, module: InteropConfigModuleInfo): Promise<Stats> {
  try {
    return await fs.stat(filePath);
  } catch (error) {
    throw new InteropConfigError({
      description: `A dynamic interop file declared by package ` + `"${module.packageName}" cannot be accessed.`,
      cause: errorMessage(error, 'unknown interop configuration failure'),
      position: filePath,
      solutions: ['Check that the file exists and is readable.'],
      moreInfo: { packageName: module.packageName },
    });
  }
}

function validateStaticFileType(stats: Stats, filePath: string, module: InteropConfigModuleInfo): void {
  if (!stats.isFile()) {
    throw new InteropConfigError({
      description: `A static interop path declared by package ` + `"${module.packageName}" is not a regular file.`,
      position: filePath,
      solutions: ['Point the interop configuration to an .ets or .d.ets file.'],
      moreInfo: { packageName: module.packageName },
    });
  }
}

function validateDynamicFileType(stats: Stats, filePath: string, module: InteropConfigModuleInfo): void {
  if (!stats.isFile()) {
    throw new InteropConfigError({
      description: `A dynamic interop path declared by package ` + `"${module.packageName}" is not a regular file.`,
      position: filePath,
      solutions: ['Point the interop configuration to a .ts or .d.ts file.'],
      moreInfo: { packageName: module.packageName },
    });
  }
}

function validateStaticFileLocation(filePath: string, module: InteropConfigModuleInfo): void {
  if (!isPathInside(module.modulePath, filePath)) {
    throw new InteropConfigError({
      description: `Package "${module.packageName}" declares a static interop file ` + 'outside its module root.',
      cause: `Module root: ${module.modulePath}`,
      position: filePath,
      solutions: ['Move the file under the module root or correct the configured path.'],
      moreInfo: { packageName: module.packageName },
    });
  }
}

function validateDynamicFileLocation(filePath: string, module: InteropConfigModuleInfo): void {
  if (!isPathInside(module.modulePath, filePath)) {
    throw new InteropConfigError({
      description: `Package "${module.packageName}" declares a dynamic interop file ` + 'outside its module root.',
      cause: `Module root: ${module.modulePath}`,
      position: filePath,
      solutions: ['Move the file under the module root or correct the configured path.'],
      moreInfo: { packageName: module.packageName },
    });
  }
}

/**
 * Validates that a package named in interopConfig is present in the project's dependent module list.
 * This guard can be removed once IDE-side schema validation guarantees valid package references.
 */
function requireInteropModule(
  modulesByPackage: ReadonlyMap<string, InteropConfigModuleInfo>,
  ownerPackageName: string,
  interopConfigPath: string,
  packageName: string,
): InteropConfigModuleInfo {
  const target = modulesByPackage.get(packageName);
  if (target === undefined) {
    throw new InteropConfigError({
      description: `Package "${ownerPackageName}" does not have a dependency named ` + `"${packageName}".`,
      cause: `No package named "${packageName}" exists in the dependent module list.`,
      position: interopConfigPath,
      solutions: [`Add "${packageName}" to the oh-package.json5, or remove the interop reference.`],
      moreInfo: {
        packageName: ownerPackageName,
        dependencyName: packageName,
      },
    });
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
    staticFiles: [...new Set([...current.staticFiles, ...incoming.staticFiles])],
    dynamicFiles: [...new Set([...current.dynamicFiles, ...incoming.dynamicFiles])],
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

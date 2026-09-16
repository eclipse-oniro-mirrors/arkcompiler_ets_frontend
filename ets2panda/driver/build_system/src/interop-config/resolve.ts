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

import { readFileSync, statSync, type Stats } from 'node:fs';
import * as path from 'node:path';

import JSON5 from 'json5';

import { InteropConfigError, errorMessage } from './errors';
import { AggregateUserError, InternalError, type ErrorMessage } from './errors';
import { createInteropConfigHost, type InteropConfigHost } from './host';
import { parseInteropConfig, type InteropConfigFile, type InteropDependencyEntries } from './schema';
import { ETS_SUFFIX, TS_SUFFIX, JS_SUFFIX, LANGUAGE_VERSION } from '../pre_define';
import type {
  InteropTarget,
  InteropFileEntry,
  InteropBuildOption,
  ProjectInteropConfigFile,
  ResolvedProjectInteropConfigFile,
} from './types';
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

/** Describes one kind of interop entry: its validation rules and diagnostic wording. */
interface EntryKind {
  /** The entry kind as worded mid-sentence in diagnostics. */
  readonly name: 'static' | 'dynamic';
  /** File extensions (per `path.extname`) accepted for this kind. */
  readonly extensions: readonly string[];
  /** The accepted extension list as worded in diagnostics. */
  readonly extensionsLabel: string;
  /** The language a source file of this kind is compiled as. */
  readonly language: LANGUAGE_VERSION;
}

const STATIC_ENTRIES: EntryKind = {
  name: 'static',
  extensions: [ETS_SUFFIX],
  extensionsLabel: '.ets or .d.ets',
  language: LANGUAGE_VERSION.ARKTS_1_2,
};

const DYNAMIC_ENTRIES: EntryKind = {
  name: 'dynamic',
  extensions: [TS_SUFFIX, ETS_SUFFIX, JS_SUFFIX],
  extensionsLabel: '.ts, .d.ts, .ets, .d.ets, or .js',
  language: LANGUAGE_VERSION.ARKTS_1_1,
};

/**
 * The interop config that declared the entries, plus the host exposing build facts about them.
 *
 * File validation errors are collected in `errors` across the whole resolution
 * and reported together once it finishes.
 */
interface ResolveContext {
  readonly configPath: string;
  readonly host: InteropConfigHost | undefined;
  readonly errors: InteropConfigError[];
  /** Set when the entries select files from a dependency's source, not the declaring module's own entries. */
  readonly dependencyPackageName?: string;
}

export const DEFAULT_INTEROP_BUILD_OPTION: InteropBuildOption = { stripInteropMapping: false };

/**
 * Resolves the projection interop config from the top-level interop-config.json5,
 * with defaults applied to every consumed field.
 *
 * `interopConfigPath === undefined` means hvigor did not pass a project configuration,
 * so the default options are used. A present `interopBuildOption` is optional and every
 * field defaults independently (`stripInteropMapping` defaults to `false`).
 */
export function resolveProjectionInteropConfig(
  interopConfigPath: string | undefined,
): ResolvedProjectInteropConfigFile {
  if (interopConfigPath === undefined) {
    return { interopBuildOption: DEFAULT_INTEROP_BUILD_OPTION };
  }
  const config: ProjectInteropConfigFile = readProjectInteropConfig(interopConfigPath);
  const stripInteropMapping = config.interopBuildOption?.stripInteropMapping ?? false;
  return { interopBuildOption: { stripInteropMapping } };
}

function readProjectInteropConfig(configPath: string): ProjectInteropConfigFile {
  try {
    const content = readFileSync(configPath, 'utf8');
    return JSON5.parse(content) as ProjectInteropConfigFile;
  } catch (error) {
    throw new InteropConfigError({
      description: 'The project interop configuration could not be read or parsed.',
      cause: errorMessage(error, 'unknown interop configuration failure'),
      position: configPath,
      solutions: ['Check that the file exists and contains valid JSON5.'],
    });
  }
}

/**
 * Synchronous variant of the interop-toolkits resolver: the build_system pipeline that
 * consumes it (`processBuildConfig`) runs synchronously inside the mode constructor.
 *
 * A custom host supplies build-environment facts and is merged over the default
 * host the way a customized `ts.CompilerHost` is; without a host, host-based
 * cross-validation is skipped. All invalid entries are reported together: a
 * single failure throws an `InteropConfigError`, multiple failures throw an
 * `AggregateUserError` wrapping them.
 */
export function resolveInteropConfig(
  table: ModuleTable,
  host?: Partial<InteropConfigHost>,
): ReadonlyMap<string, InteropTarget> {
  const effectiveHost = host === undefined ? undefined : createInteropConfigHost(host);
  const errors: InteropConfigError[] = [];
  const targets = new Map<string, InteropTarget>();
  for (const module of reachableModulesOf(table)) {
    const contributions = moduleContributions(table.byPackage, module, effectiveHost, errors);
    for (const contribution of contributions) {
      mergeTarget(targets, contribution.packageName, contribution.target);
    }
  }
  if (errors.length === 1) {
    throw errors[0];
  }
  if (errors.length > 1) {
    throw new AggregateUserError(errors);
  }
  return targets;
}

function moduleContributions(
  modulesByPackage: ReadonlyMap<string, InteropConfigModuleInfo>,
  module: InteropConfigModuleInfo,
  host: InteropConfigHost | undefined,
  errors: InteropConfigError[],
): readonly InteropContribution[] {
  const configPath = module.interopConfigPath;
  if (configPath === undefined) {
    return [];
  }

  const config: InteropConfigFile = readInteropConfig(module.packageName, configPath);
  const entries = config.interopEntries;
  if (entries === undefined) {
    return [];
  }

  const context: ResolveContext = { configPath, host, errors };
  return [
    ...ownContributions(entries, module, context),
    ...dependencyContributions(entries.dependency, modulesByPackage, module, context),
  ];
}

function ownContributions(
  entries: NonNullable<InteropConfigFile['interopEntries']>,
  module: InteropConfigModuleInfo,
  context: ResolveContext,
): readonly InteropContribution[] {
  const staticFiles = resolveEntryFiles(STATIC_ENTRIES, entries.static ?? [], module, context);
  const dynamicFiles = resolveEntryFiles(DYNAMIC_ENTRIES, entries.dynamic ?? [], module, context);
  if (staticFiles.length === 0 && dynamicFiles.length === 0) {
    return [];
  }
  return [{ packageName: module.packageName, target: { kind: 'items', staticFiles, dynamicFiles } }];
}

function dependencyContributions(
  dependency: InteropDependencyEntries | undefined,
  modulesByPackage: ReadonlyMap<string, InteropConfigModuleInfo>,
  module: InteropConfigModuleInfo,
  context: ResolveContext,
): readonly InteropContribution[] {
  if (dependency === undefined) {
    return [];
  }
  return [
    ...packageContributions(dependency.package ?? [], modulesByPackage, module, context),
    ...sourceContributions(dependency.source ?? {}, modulesByPackage, module, context),
  ];
}

function packageContributions(
  packageNames: readonly string[],
  modulesByPackage: ReadonlyMap<string, InteropConfigModuleInfo>,
  module: InteropConfigModuleInfo,
  context: ResolveContext,
): readonly InteropContribution[] {
  return packageNames.map((packageName) => ({
    packageName,
    target: { kind: 'package', moduleInfo: requireInteropModule(modulesByPackage, module, context, packageName) },
  }));
}

function sourceContributions(
  sources: NonNullable<InteropDependencyEntries['source']>,
  modulesByPackage: ReadonlyMap<string, InteropConfigModuleInfo>,
  module: InteropConfigModuleInfo,
  context: ResolveContext,
): readonly InteropContribution[] {
  const contributions: InteropContribution[] = [];
  for (const [packageName, selected] of Object.entries(sources)) {
    const targetModule = requireInteropModule(modulesByPackage, module, context, packageName);
    const sourceContext: ResolveContext = { ...context, dependencyPackageName: packageName };
    const staticFiles = resolveEntryFiles(STATIC_ENTRIES, selected.static ?? [], targetModule, sourceContext);
    const dynamicFiles = resolveEntryFiles(DYNAMIC_ENTRIES, selected.dynamic ?? [], targetModule, sourceContext);
    if (staticFiles.length === 0 && dynamicFiles.length === 0) {
      continue;
    }
    contributions.push({ packageName, target: { kind: 'items', staticFiles, dynamicFiles } });
  }
  return contributions;
}

function readInteropConfig(packageName: string, configPath: string): InteropConfigFile {
  let content: string;
  try {
    content = readFileSync(configPath, 'utf8');
  } catch (error) {
    throw new InteropConfigError({
      description: `The interop configuration for package "${packageName}" could not be read.`,
      cause: errorMessage(error, 'unknown interop configuration failure'),
      position: configPath,
      solutions: ['Check that the file exists and is readable.'],
      moreInfo: { packageName },
    });
  }
  return parseInteropConfig(content, packageName, configPath);
}

/** Resolves configured entry values into validated file entries, collecting failures. */
function resolveEntryFiles(
  kind: EntryKind,
  values: readonly string[],
  module: InteropConfigModuleInfo,
  context: ResolveContext,
): readonly InteropFileEntry[] {
  const files = new Map<string, InteropFileEntry>();
  const seen = new Set<string>();
  for (const value of values) {
    const filePath = path.resolve(module.modulePath, value);
    if (seen.has(filePath)) {
      continue;
    }
    seen.add(filePath);
    const error = validateEntryFile(kind, filePath, value, module, context);
    if (error === undefined) {
      files.set(filePath, { packageName: module.packageName, relativePath: value, filePath });
    } else {
      context.errors.push(error);
    }
  }
  return [...files.values()];
}

/**
 * Runs every check for one configured entry and returns the first failure.
 * Returning the error lets the caller keep validating the remaining entries.
 */
function validateEntryFile(
  kind: EntryKind,
  filePath: string,
  configuredValue: string,
  module: InteropConfigModuleInfo,
  context: ResolveContext,
): InteropConfigError | undefined {
  const extensionError = validateEntryExtension(kind, filePath, module, context);
  if (extensionError !== undefined) {
    return extensionError;
  }
  const stats = statEntryFile(kind, filePath, module, context);
  if (stats instanceof InteropConfigError) {
    return stats;
  }
  return (
    validateEntryFileType(kind, stats, module, context) ??
    validateEntryLocation(kind, filePath, module, context) ??
    validateEntryLanguage(kind, filePath, configuredValue, module, context)
  );
}

function validateEntryExtension(
  kind: EntryKind,
  filePath: string,
  module: InteropConfigModuleInfo,
  context: ResolveContext,
): InteropConfigError | undefined {
  const extension = path.extname(filePath);
  if (kind.extensions.some((accepted) => accepted === extension)) {
    return undefined;
  }
  const kindWord = kind.name.charAt(0).toUpperCase() + kind.name.slice(1);
  return entryError(module, context, {
    description: `Package "${module.packageName}" declares a ${kind.name} interop file with an unsupported extension.`,
    cause: `${kindWord} interop files must use a lowercase ${kind.extensionsLabel} extension.`,
    solutions: ['Rename the file or correct its path in the interop configuration.'],
  });
}

function statEntryFile(
  kind: EntryKind,
  filePath: string,
  module: InteropConfigModuleInfo,
  context: ResolveContext,
): Stats | InteropConfigError {
  try {
    return statSync(filePath);
  } catch (error) {
    return entryError(module, context, {
      description: `A ${kind.name} interop file declared by package "${module.packageName}" cannot be accessed.`,
      cause: errorMessage(error, 'unknown interop configuration failure'),
      solutions: ['Check that the file exists and is readable.'],
    });
  }
}

function validateEntryFileType(
  kind: EntryKind,
  stats: Stats,
  module: InteropConfigModuleInfo,
  context: ResolveContext,
): InteropConfigError | undefined {
  if (stats.isFile()) {
    return undefined;
  }
  return entryError(module, context, {
    description: `A ${kind.name} interop path declared by package "${module.packageName}" is not a regular file.`,
    solutions: [`Point the interop configuration to a ${kind.extensionsLabel} file.`],
  });
}

function validateEntryLocation(
  kind: EntryKind,
  filePath: string,
  module: InteropConfigModuleInfo,
  context: ResolveContext,
): InteropConfigError | undefined {
  if (isPathInside(module.modulePath, filePath)) {
    return undefined;
  }
  return entryError(module, context, {
    description: `Package "${module.packageName}" declares a ${kind.name} interop file outside its module root.`,
    cause: `Module root: ${module.modulePath}`,
    solutions: ['Move the file under the module root or correct the configured path.'],
  });
}

/**
 * Rejects an interop entry whose declared kind conflicts with the language the
 * source code declares, e.g. a dynamic source file listed as a static entry.
 */
function validateEntryLanguage(
  kind: EntryKind,
  filePath: string,
  configuredValue: string,
  module: InteropConfigModuleInfo,
  context: ResolveContext,
): InteropConfigError | undefined {
  if (context.host === undefined) {
    return undefined;
  }
  const actualLanguage = context.host.getLanguageFromSourceCode(filePath);
  if (!conflictsWithLanguage(kind.language, actualLanguage)) {
    return undefined;
  }
  const declaredIn =
    context.dependencyPackageName === undefined ? '' : ` in dependency ${context.dependencyPackageName}`;
  return entryError(module, context, {
    description: 'Invalid interop configuration.',
    cause:
      `${configuredValue}${declaredIn} is a ${languageName(actualLanguage)} source file, ` +
      `but is configured as a ${languageName(kind.language)} interop entry.`,
    solutions: [`Move the entry to the "${languageName(actualLanguage)}" interop entries.`],
    moreInfo: { filePath },
  });
}

type EntryErrorInit = Omit<ErrorMessage, 'position'>;

/** Builds an interop-config error stamped with the declaring config and package. */
function entryError(
  module: InteropConfigModuleInfo,
  context: ResolveContext,
  init: EntryErrorInit,
): InteropConfigError {
  return new InteropConfigError({
    ...init,
    position: context.configPath,
    moreInfo: { packageName: module.packageName, ...init.moreInfo },
  });
}

/**
 * Validates that a package named in interopConfig is present in the project's dependent module list.
 * This guard can be removed once IDE-side schema validation guarantees valid package references.
 */
function requireInteropModule(
  modulesByPackage: ReadonlyMap<string, InteropConfigModuleInfo>,
  module: InteropConfigModuleInfo,
  context: ResolveContext,
  packageName: string,
): InteropConfigModuleInfo {
  const target = modulesByPackage.get(packageName);
  if (target !== undefined) {
    return target;
  }
  throw entryError(module, context, {
    description: `Package "${module.packageName}" does not have a dependency named "${packageName}".`,
    cause: `No package named "${packageName}" exists in the dependent module list.`,
    solutions: [`Add "${packageName}" to the oh-package.json5, or remove the interop reference.`],
    moreInfo: { dependencyName: packageName },
  });
}

/** A hybrid classification is ambiguous rather than contradictory, so it conflicts with nothing. */
function conflictsWithLanguage(declared: LANGUAGE_VERSION, actual: LANGUAGE_VERSION): boolean {
  return (
    (declared === LANGUAGE_VERSION.ARKTS_1_2 && actual === LANGUAGE_VERSION.ARKTS_1_1) ||
    (declared === LANGUAGE_VERSION.ARKTS_1_1 && actual === LANGUAGE_VERSION.ARKTS_1_2)
  );
}

/** Maps the version-like enum values to the words used in user-facing diagnostics. */
function languageName(language: LANGUAGE_VERSION): string {
  switch (language) {
    case LANGUAGE_VERSION.ARKTS_1_2:
      return 'static';
    case LANGUAGE_VERSION.ARKTS_1_1:
      return 'dynamic';
  }
  throw new InternalError(`Unknown language: ${language}`);
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
    staticFiles: mergeFileEntries(current.staticFiles, incoming.staticFiles),
    dynamicFiles: mergeFileEntries(current.dynamicFiles, incoming.dynamicFiles),
  });
}

function mergeFileEntries(
  current: readonly InteropFileEntry[],
  incoming: readonly InteropFileEntry[],
): readonly InteropFileEntry[] {
  const byFilePath = new Map<string, InteropFileEntry>();
  for (const entry of [...current, ...incoming]) {
    if (!byFilePath.has(entry.filePath)) {
      byFilePath.set(entry.filePath, entry);
    }
  }
  return [...byFilePath.values()];
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

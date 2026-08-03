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

import { promises as fs } from 'node:fs';
import * as path from 'node:path';

import type { BuildConfig } from '../../../contracts';
import { hasEtsSourceExtension, hasUseStaticDirectiveInFile } from '../../../utils/staticSource';
import { dependencyModulesOf, reachableModulesOf, type ModuleInfo, type ModuleTable } from '../../configuration';
import type { DependencyItem } from '../arktsconfig';
import { collectFiles, pathExists } from '../../../utils/fileTree';
import type { ArkTSConfigRule, GenerationContext } from './arktsconfigRule';
import {
  appendUniquePath,
  createRuleOutput,
  mergeRuleOutput,
  type MutableRuleOutput,
  type RuleOutput,
} from './ruleOutput';

const ARKTS_1_1 = '1.1';
const ARKTS_1_2 = '1.2';
const ARKTS_HYBRID = 'hybrid';
const SHARED_BUNDLE_TYPES = new Set(['shared', 'appPlugin']);

interface DeclFileItem {
  readonly declPath: string;
  readonly filePath?: string;
  readonly ohmUrl: string;
}

interface DeclFilesConfig {
  readonly files: Readonly<Record<string, DeclFileItem>>;
}

interface DeclDependencyState {
  readonly target: Map<string, DependencyItem>;
  readonly module: ModuleInfo;
  readonly fileEntries: readonly (readonly [string, DeclFileItem])[];
  readonly hasDynamicEntry: boolean;
  readonly context: GenerationContext;
  readonly entryFile: string;
  readonly processedFiles: Set<string>;
  readonly claimedTransformedKeys: Set<string>;
}

export class ModuleRule implements ArkTSConfigRule {
  async generate(context: GenerationContext): Promise<RuleOutput> {
    const modules = reachableModulesOf(context.moduleTable);
    const fragments = new Map<string, RuleOutput>();
    for (const module of modules) {
      fragments.set(module.packageName, await this.generateLocalFragment(module, context));
    }

    const output = createRuleOutput();
    this.mergeLocalFragments(output, modules, fragments);
    await this.applyDependencyEdges(output, modules, fragments, context);
    return output;
  }

  private mergeLocalFragments(
    output: MutableRuleOutput,
    modules: readonly ModuleInfo[],
    fragments: ReadonlyMap<string, RuleOutput>,
  ): void {
    for (const module of modules) {
      const fragment = fragments.get(module.packageName);
      if (fragment !== undefined) {
        mergeRuleOutput(output, fragment);
      }
    }
  }

  private async applyDependencyEdges(
    output: MutableRuleOutput,
    modules: readonly ModuleInfo[],
    fragments: ReadonlyMap<string, RuleOutput>,
    context: GenerationContext,
  ): Promise<void> {
    for (const owner of modules) {
      await this.applyOwnerDependencyEdges(output, owner, fragments, context);
    }
  }

  private async applyOwnerDependencyEdges(
    output: MutableRuleOutput,
    owner: ModuleInfo,
    fragments: ReadonlyMap<string, RuleOutput>,
    context: GenerationContext,
  ): Promise<void> {
    for (const dependency of dependencyModulesOf(context.moduleTable, owner)) {
      const fragment = fragments.get(dependency.packageName);
      if (fragment !== undefined) {
        await this.applyDependencyEdge(output, owner, dependency, fragment);
      }
    }
  }

  private async generateLocalFragment(module: ModuleInfo, context: GenerationContext): Promise<RuleOutput> {
    const fragment = createRuleOutput();
    switch (module.language) {
      case ARKTS_1_1:
        await this.addDynamicContributions(fragment, module, false, context);
        break;
      case ARKTS_1_2:
        await this.addStaticContributions(fragment, module, false);
        break;
      case ARKTS_HYBRID: {
        const hasStaticEntry = await this.isStaticEntryFile(module);
        await this.addStaticContributions(fragment, module, true, hasStaticEntry);
        await this.addDynamicContributions(fragment, module, hasStaticEntry, context);
        break;
      }
      default:
        break;
    }
    return fragment;
  }

  private async addStaticContributions(
    fragment: MutableRuleOutput,
    module: ModuleInfo,
    inspectFileMarker: boolean,
    hasStaticEntry = true,
  ): Promise<void> {
    const entryFile = await resolvedEntryFilePath(module);
    if (entryFile !== '') {
      await this.addStaticFilePath(fragment, module, entryFile, inspectFileMarker);
    }

    const sourceRoots = await this.existingSourceRoots(module);
    await this.addStaticSourceFiles(fragment, module, sourceRoots, inspectFileMarker);

    if (!hasStaticEntry || entryFile === '') {
      return;
    }
    // Keep "./" as the lowest-priority bare-package fallback for compatibility.
    appendUniquePath(fragment.paths, module.packageName, [...[...sourceRoots].reverse(), module.modulePath]);
    appendUniquePath(fragment.paths, `${module.packageName}/Index`, [entryFile]);
  }

  private async existingSourceRoots(module: ModuleInfo): Promise<string[]> {
    const sourceRoots: string[] = [];
    for (const sourceRoot of module.sourceRoots) {
      if ((await pathExists(sourceRoot)) && (await fs.stat(sourceRoot)).isDirectory()) {
        sourceRoots.push(sourceRoot);
      }
    }
    return sourceRoots;
  }

  private async addStaticSourceFiles(
    fragment: MutableRuleOutput,
    module: ModuleInfo,
    sourceRoots: readonly string[],
    inspectFileMarker: boolean,
  ): Promise<void> {
    for (const sourceRoot of sourceRoots) {
      // Workaround: skip "./" source root. avoid scanning "build" directory.
      if (sourceRoot === module.modulePath) {
        continue;
      }
      for (const filePath of await collectFiles(sourceRoot, (candidate) =>
        this.acceptsStaticFile(candidate, inspectFileMarker),
      )) {
        await this.addStaticFilePath(fragment, module, filePath, false);
      }
    }
  }

  private async acceptsStaticFile(candidate: string, inspectFileMarker: boolean): Promise<boolean> {
    if (!hasEtsSourceExtension(candidate)) {
      return false;
    }
    return !inspectFileMarker || (await this.isStaticSourceFile(candidate));
  }

  /**
   * Adds one file's package-relative path mapping.
   *
   * @param inspectFileMarker Whether this function must verify the first-line
   * 'use static' marker. Pass false when the module is ArkTS 1.2 or the caller
   * has already filtered the file as static.
   */
  private async addStaticFilePath(
    fragment: MutableRuleOutput,
    module: ModuleInfo,
    filePath: string,
    inspectFileMarker: boolean,
  ): Promise<void> {
    if (inspectFileMarker && !(await this.isStaticSourceFile(filePath))) {
      return;
    }
    const relativePath = toUnixPath(path.relative(module.modulePath, filePath));
    const keyWithoutExtension = removeSourceExtension(relativePath);
    appendUniquePath(fragment.paths, `${module.packageName}/${keyWithoutExtension}`, [filePath]);
  }

  private async addDynamicContributions(
    fragment: MutableRuleOutput,
    module: ModuleInfo,
    hasStaticEntry: boolean,
    context: GenerationContext,
  ): Promise<void> {
    if (module.declFilesPath === undefined || !(await pathExists(module.declFilesPath))) {
      return;
    }
    const config = JSON.parse(await fs.readFile(module.declFilesPath, 'utf8')) as DeclFilesConfig;
    await this.addDeclFileDependencies(fragment.dependencies, module, config.files, !hasStaticEntry, context);
  }

  private async addDeclFileDependencies(
    target: Map<string, DependencyItem>,
    module: ModuleInfo,
    files: Readonly<Record<string, DeclFileItem>>,
    hasDynamicEntry: boolean,
    context: GenerationContext,
  ): Promise<void> {
    const fileEntries = Object.entries(files);
    const entryFile = await moduleEntryFile(module);
    const state: DeclDependencyState = {
      target,
      module,
      fileEntries,
      hasDynamicEntry,
      context,
      entryFile,
      processedFiles: new Set<string>(),
      claimedTransformedKeys: new Set<string>(),
    };

    this.addTransformedDeclFileDependencies(state);
    this.addRemainingDeclFileDependencies(state);
  }

  private addTransformedDeclFileDependencies(state: DeclDependencyState): void {
    for (const root of normalizedSourceRoots(state.module)) {
      this.addSourceRootDeclFileDependencies(state, root);
    }
  }

  private addSourceRootDeclFileDependencies(state: DeclDependencyState, root: string): void {
    const prefix = root.endsWith('/') ? root : `${root}/`;
    for (const [file, fileItem] of state.fileEntries) {
      const normalizedFile = toUnixPath(file);
      if (!normalizedFile.startsWith(prefix)) {
        continue;
      }
      state.processedFiles.add(file);
      const transformedKey = `${state.module.packageName}/${normalizedFile.slice(prefix.length)}`;
      if (state.claimedTransformedKeys.has(transformedKey)) {
        continue;
      }
      state.claimedTransformedKeys.add(transformedKey);
      this.addOneDeclFile(
        state.target,
        state.module,
        normalizedFile,
        transformedKey,
        this.isDynamicEntry(state, normalizedFile),
        this.createDeclDependencyItem(fileItem, state.module, state.context),
      );
    }
  }

  private addRemainingDeclFileDependencies(state: DeclDependencyState): void {
    for (const [file, fileItem] of state.fileEntries) {
      if (state.processedFiles.has(file)) {
        continue;
      }
      const normalizedFile = toUnixPath(file);
      this.addOneDeclFile(
        state.target,
        state.module,
        normalizedFile,
        undefined,
        this.isDynamicEntry(state, normalizedFile),
        this.createDeclDependencyItem(fileItem, state.module, state.context),
      );
    }
  }

  private isDynamicEntry(state: DeclDependencyState, file: string): boolean {
    return state.hasDynamicEntry && file === state.entryFile;
  }

  private addOneDeclFile(
    target: Map<string, DependencyItem>,
    module: ModuleInfo,
    file: string,
    transformedKey: string | undefined,
    isEntryFile: boolean,
    item: DependencyItem,
  ): void {
    const legacyKey = `${module.packageName}/${file}`;
    target.set(legacyKey, item);
    if (transformedKey !== undefined && transformedKey !== legacyKey) {
      target.set(transformedKey, item);
    }
    if (isEntryFile) {
      target.set(module.packageName, item);
    }
  }

  private createDeclDependencyItem(file: DeclFileItem, module: ModuleInfo, context: GenerationContext): DependencyItem {
    return {
      language: 'js',
      path: file.declPath,
      ohmUrl: this.generateModuleOhmUrl(file.ohmUrl, module, context.moduleTable, context.buildConfig),
      ...(file.filePath === undefined ? {} : { sourceFilePath: file.filePath }),
    };
  }

  private generateModuleOhmUrl(
    ohmUrl: string,
    module: ModuleInfo,
    table: ModuleTable,
    buildConfig: BuildConfig,
  ): string {
    const parts = ohmUrl.split('&');
    this.rewriteBundlePart(parts, module, table, buildConfig);
    this.rewriteModulePart(parts, module, table);
    this.rewriteVersionPart(parts, module);
    return parts.join('&');
  }

  private rewriteBundlePart(parts: string[], module: ModuleInfo, table: ModuleTable, buildConfig: BuildConfig): void {
    const bundleName = this.bundleNameFor(module, table, buildConfig);
    if (bundleName !== undefined) {
      parts[2] = bundleName;
    }
  }

  private bundleNameFor(module: ModuleInfo, table: ModuleTable, buildConfig: BuildConfig): string | undefined {
    if (this.isSharedBundle(module)) {
      return module.bundleName;
    }
    if (this.isSharedBundle(table.mainModule) && buildConfig.moduleType === 'shared') {
      return buildConfig.bundleName;
    }
    return undefined;
  }

  private isSharedBundle(module: ModuleInfo): boolean {
    return module.bundleType !== undefined && SHARED_BUNDLE_TYPES.has(module.bundleType);
  }

  private rewriteModulePart(parts: string[], module: ModuleInfo, table: ModuleTable): void {
    if (
      module.moduleType === 'shared' &&
      module.packageName !== table.mainModule.packageName &&
      module.moduleName !== undefined
    ) {
      parts[1] = module.moduleName;
    }
  }

  private rewriteVersionPart(parts: string[], module: ModuleInfo): void {
    if (module.moduleType === 'har' && module.packageVersion !== undefined) {
      parts[parts.length - 1] = module.packageVersion;
    }
  }

  private async applyDependencyEdge(
    output: MutableRuleOutput,
    owner: ModuleInfo,
    dependency: ModuleInfo,
    dependencyFragment: RuleOutput,
  ): Promise<void> {
    const alias = this.aliasForDependency(owner, dependency);
    this.addDependencyAliases(output.dependencies, dependencyFragment.dependencies, dependency.packageName, alias);
    await this.addBinaryDependency(output.dependencies, dependency, alias);
  }

  private addDependencyAliases(
    target: Map<string, DependencyItem>,
    source: ReadonlyMap<string, DependencyItem>,
    packageName: string,
    alias: string | undefined,
  ): void {
    if (alias === undefined) {
      return;
    }
    const prefix = `${packageName}/`;
    for (const [key, item] of source) {
      if (key !== packageName && !key.startsWith(prefix)) {
        continue;
      }
      const aliasKey = `${alias}${key.slice(packageName.length)}`;
      if (!target.has(aliasKey)) {
        target.set(aliasKey, item);
      }
    }
  }

  private async addBinaryDependency(
    target: Map<string, DependencyItem>,
    dependency: ModuleInfo,
    alias: string | undefined,
  ): Promise<void> {
    if (!this.isBinaryHarDependency(dependency) || target.has(dependency.packageName)) {
      return;
    }
    if ((await resolvedEntryFilePath(dependency)) === '') {
      return;
    }
    const item: DependencyItem = {
      language: 'ets',
      path: dependency.abcPath,
      ohmUrl: dependency.packageName,
      mainFile: 'Index',
    };
    target.set(dependency.packageName, item);
    this.addBinaryDependencyAlias(target, alias, item);
  }

  private isBinaryHarDependency(dependency: ModuleInfo): dependency is ModuleInfo & { readonly abcPath: string } {
    return (
      this.hasStaticContributions(dependency) && dependency.moduleType === 'har' && dependency.abcPath !== undefined
    );
  }

  private addBinaryDependencyAlias(
    target: Map<string, DependencyItem>,
    alias: string | undefined,
    item: DependencyItem,
  ): void {
    if (alias !== undefined && !target.has(alias)) {
      target.set(alias, item);
    }
  }

  private hasStaticContributions(module: ModuleInfo): boolean {
    return module.language === ARKTS_1_2 || module.language === ARKTS_HYBRID;
  }

  private aliasForDependency(owner: ModuleInfo, dependency: ModuleInfo): string | undefined {
    for (const [alias, packageName] of Object.entries(owner.originalPackageNameMap)) {
      if (packageName === dependency.packageName) {
        return alias === packageName ? undefined : alias;
      }
    }
    return undefined;
  }

  private async isStaticEntryFile(module: ModuleInfo): Promise<boolean> {
    const entryFile = await resolvedEntryFilePath(module);
    return entryFile !== '' && (await this.isStaticSourceFile(entryFile));
  }

  private async isStaticSourceFile(filePath: string): Promise<boolean> {
    try {
      return await hasUseStaticDirectiveInFile(filePath);
    } catch {
      return false;
    }
  }
}

function normalizedSourceRoots(module: ModuleInfo): readonly string[] {
  return [...module.sourceRoots]
    .map((root) => toUnixPath(path.relative(module.modulePath, root)))
    .map((root) => root.replace(/^\.\//, ''))
    .filter((root) => root !== '' && root !== '.')
    .reverse();
}

async function moduleEntryFile(module: ModuleInfo): Promise<string> {
  const entryFile = await resolvedEntryFilePath(module);
  if (entryFile === '') {
    return '';
  }
  return removeSourceExtension(toUnixPath(path.relative(module.modulePath, entryFile)));
}

/** A directory-valued entryFile denotes a module without an entry file. */
async function resolvedEntryFilePath(module: ModuleInfo): Promise<string> {
  if (module.entryFile === '') {
    return '';
  }
  try {
    if ((await fs.stat(module.entryFile)).isDirectory()) {
      return '';
    }
  } catch {
    // Preserve the configured value; its consumer owns unreadable entry handling.
  }
  return module.entryFile;
}

function removeSourceExtension(filePath: string): string {
  return filePath.replace(/\.(?:d\.)?[^/.]+$/, '');
}

function toUnixPath(value: string): string {
  return value.replace(/\\/g, '/');
}

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

import { promises as fs } from 'node:fs';
import * as path from 'node:path';

import type { DependencyItem } from '../arktsconfig';
import type { DynamicInteropContext, DynamicInteropFile, ModuleInfo } from '../context';
import type { ArkTSConfigRule } from './arktsconfigRule';
import { collectFiles } from './fileTree';
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

export interface ModuleRuleInput {
  readonly modules: readonly ModuleInfo[];
  readonly modulesByPackage: ReadonlyMap<string, ModuleInfo>;
  readonly interopContexts: ReadonlyMap<string, DynamicInteropContext>;
  readonly mainModule: ModuleInfo;
  readonly bundleName: string;
  readonly moduleType?: string;
}

export class ModuleRule implements ArkTSConfigRule {
  public constructor(private readonly input: ModuleRuleInput) {}

  public async generate(): Promise<RuleOutput> {
    const fragments = new Map<string, MutableRuleOutput>();
    for (const module of this.input.modules) {
      fragments.set(module.packageName, await this.generateLocalFragment(module));
    }
    const output = createRuleOutput();
    for (const module of this.input.modules) {
      const fragment = fragments.get(module.packageName);
      if (fragment !== undefined) {
        mergeRuleOutput(output, fragment);
      }
    }
    for (const owner of this.input.modules) {
      for (const dependencyName of owner.dependencies) {
        const dependency = this.input.modulesByPackage.get(dependencyName);
        const fragment = dependency === undefined ? undefined : fragments.get(dependency.packageName);
        if (dependency !== undefined && fragment !== undefined) {
          await this.applyDependencyEdge(output, owner, dependency, fragment);
        }
      }
    }
    return output;
  }

  private async generateLocalFragment(module: ModuleInfo): Promise<MutableRuleOutput> {
    const fragment = createRuleOutput();
    switch (module.language) {
      case ARKTS_1_1:
        await this.addDynamicContributions(fragment, module, false);
        break;
      case ARKTS_1_2:
        await this.addStaticContributions(fragment, module, false);
        break;
      case ARKTS_HYBRID: {
        const hasStaticEntry = await this.isStaticEntryFile(module);
        await this.addStaticContributions(fragment, module, true, hasStaticEntry);
        await this.addDynamicContributions(fragment, module, hasStaticEntry);
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
    const sourceRoots: string[] = [];
    for (const sourceRoot of module.sourceRoots) {
      if (await isDirectory(sourceRoot)) {
        sourceRoots.push(sourceRoot);
      }
    }
    for (const sourceRoot of sourceRoots) {
      if (sourceRoot === module.modulePath) {
        continue;
      }
      for (const filePath of await collectFiles(sourceRoot, async (candidate) => {
        return candidate.endsWith('.ets') && (!inspectFileMarker || (await isStaticSourceFile(candidate)));
      })) {
        await this.addStaticFilePath(fragment, module, filePath, false);
      }
    }
    if (entryFile === '') {
      return;
    }
    appendUniquePath(fragment.paths, module.packageName, [...sourceRoots].reverse().concat(module.modulePath));
    if (hasStaticEntry) {
      appendUniquePath(fragment.paths, `${module.packageName}/Index`, [entryFile]);
    }
  }

  private async addStaticFilePath(
    fragment: MutableRuleOutput,
    module: ModuleInfo,
    filePath: string,
    inspectFileMarker: boolean,
  ): Promise<void> {
    if (inspectFileMarker && !(await isStaticSourceFile(filePath))) {
      return;
    }
    const relativePath = toUnixPath(path.relative(module.modulePath, filePath));
    appendUniquePath(fragment.paths, `${module.packageName}/${removeSourceExtension(relativePath)}`, [filePath]);
  }

  private async addDynamicContributions(
    fragment: MutableRuleOutput,
    module: ModuleInfo,
    hasStaticEntry: boolean,
  ): Promise<void> {
    const dynamicContext = this.input.interopContexts.get(module.packageName);
    if (dynamicContext === undefined) {
      return;
    }
    const entries = Object.entries(dynamicContext.files);
    const entryFile = await moduleEntryFile(module);
    const processedFiles = new Set<string>();
    const claimedTransformedKeys = new Set<string>();
    for (const root of normalizedSourceRoots(module)) {
      const prefix = root.endsWith('/') ? root : `${root}/`;
      for (const [file, fileItem] of entries) {
        const normalizedFile = toUnixPath(file);
        if (!normalizedFile.startsWith(prefix)) {
          continue;
        }
        processedFiles.add(file);
        const transformedKey = `${module.packageName}/${normalizedFile.slice(prefix.length)}`;
        if (claimedTransformedKeys.has(transformedKey)) {
          continue;
        }
        claimedTransformedKeys.add(transformedKey);
        this.addOneDynamicFile(
          fragment.dependencies,
          module,
          normalizedFile,
          transformedKey,
          !hasStaticEntry && normalizedFile === entryFile,
          this.createDynamicDependencyItem(fileItem, module),
        );
      }
    }
    for (const [file, fileItem] of entries) {
      if (processedFiles.has(file)) {
        continue;
      }
      const normalizedFile = toUnixPath(file);
      this.addOneDynamicFile(
        fragment.dependencies,
        module,
        normalizedFile,
        undefined,
        !hasStaticEntry && normalizedFile === entryFile,
        this.createDynamicDependencyItem(fileItem, module),
      );
    }
  }

  private addOneDynamicFile(
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

  private createDynamicDependencyItem(file: DynamicInteropFile, module: ModuleInfo): DependencyItem {
    return {
      language: 'js',
      path: file.declPath,
      ohmUrl: this.generateModuleOhmUrl(file.ohmUrl, module),
      sourceFilePath: file.filePath,
    };
  }

  private generateModuleOhmUrl(ohmUrl: string, module: ModuleInfo): string {
    const parts = ohmUrl.split('&');
    const bundleName = this.bundleNameFor(module);
    if (bundleName !== undefined) {
      parts[2] = bundleName;
    }
    if (
      module.moduleType === 'shared' &&
      module.packageName !== this.input.mainModule.packageName &&
      module.moduleName !== undefined
    ) {
      parts[1] = module.moduleName;
    }
    if (module.moduleType === 'har' && module.packageVersion !== undefined) {
      parts[parts.length - 1] = module.packageVersion;
    }
    return parts.join('&');
  }

  private bundleNameFor(module: ModuleInfo): string | undefined {
    if (module.bundleType !== undefined && SHARED_BUNDLE_TYPES.has(module.bundleType)) {
      return module.bundleName;
    }
    if (
      this.input.mainModule.bundleType !== undefined &&
      SHARED_BUNDLE_TYPES.has(this.input.mainModule.bundleType) &&
      this.input.moduleType === 'shared'
    ) {
      return this.input.bundleName;
    }
    return undefined;
  }

  private async applyDependencyEdge(
    output: MutableRuleOutput,
    owner: ModuleInfo,
    dependency: ModuleInfo,
    fragment: RuleOutput,
  ): Promise<void> {
    const alias = this.aliasForDependency(owner, dependency);
    this.addAliasedDependencies(output.dependencies, dependency.packageName, alias, fragment.dependencies);
    await this.addBinaryDependency(output.dependencies, dependency, alias);
  }

  private addAliasedDependencies(
    target: Map<string, DependencyItem>,
    packageName: string,
    alias: string | undefined,
    dependencies: ReadonlyMap<string, DependencyItem>,
  ): void {
    if (alias === undefined) {
      return;
    }
    const prefix = `${packageName}/`;
    for (const [key, item] of dependencies) {
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
    if (
      (dependency.language !== ARKTS_1_2 && dependency.language !== ARKTS_HYBRID) ||
      dependency.moduleType !== 'har' ||
      dependency.abcPath === undefined ||
      target.has(dependency.packageName) ||
      (await resolvedEntryFilePath(dependency)) === ''
    ) {
      return;
    }
    const item: DependencyItem = {
      language: 'ets',
      path: dependency.abcPath,
      ohmUrl: dependency.packageName,
      mainFile: 'Index',
    };
    target.set(dependency.packageName, item);
    if (alias !== undefined && !target.has(alias)) {
      target.set(alias, item);
    }
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
    return entryFile !== '' && (await isStaticSourceFile(entryFile));
  }
}

async function isDirectory(filePath: string): Promise<boolean> {
  try {
    return (await fs.stat(filePath)).isDirectory();
  } catch {
    return false;
  }
}

async function isStaticSourceFile(filePath: string): Promise<boolean> {
  try {
    const file = await fs.open(filePath, 'r');
    try {
      const buffer = Buffer.allocUnsafe(256);
      const { bytesRead } = await file.read(buffer, 0, buffer.length, 0);
      const firstLine = buffer.toString('utf8', 0, bytesRead).split(/\r?\n/, 1)[0]?.trim();
      return firstLine === "'use static'";
    } finally {
      await file.close();
    }
  } catch {
    return false;
  }
}

async function resolvedEntryFilePath(module: ModuleInfo): Promise<string> {
  if (module.entryFile === '') {
    return '';
  }
  try {
    if ((await fs.stat(module.entryFile)).isDirectory()) {
      return '';
    }
  } catch {
    return module.entryFile;
  }
  return module.entryFile;
}

async function moduleEntryFile(module: ModuleInfo): Promise<string> {
  const entryFile = await resolvedEntryFilePath(module);
  return entryFile === '' ? '' : removeSourceExtension(toUnixPath(path.relative(module.modulePath, entryFile)));
}

function normalizedSourceRoots(module: ModuleInfo): readonly string[] {
  return [...module.sourceRoots]
    .map((root) => toUnixPath(path.relative(module.modulePath, root)))
    .map((root) => root.replace(/^\.\//, ''))
    .filter((root) => root !== '' && root !== '.')
    .reverse();
}

function removeSourceExtension(filePath: string): string {
  return filePath.replace(/\.(?:d\.)?[^/.]+$/, '');
}

function toUnixPath(value: string): string {
  return value.replace(/\\/g, '/');
}

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

import type { BuildConfig, ModuleConfig } from '../../contracts';
import { GlueGenError, GlueGenErrorCode, GlueGenInternalError, errorMessage } from '../../errors';
import { LogData } from '../../logger';
import type { GlueGenContext } from '../../pipeline/context';
import type { StageScope } from '../../pipeline';
import {
  copyStringMap,
  copyStrings,
  invalidBuildConfig,
  requireAbsolute,
  requireNonEmptyString,
  requireOptionalString,
  requireString,
  requireStringArray,
  resolveDependencyNames,
} from './moduleConfig';

export function validate(scope: StageScope<GlueGenContext, readonly []>): void {
  const { context } = scope;
  try {
    validateBuildConfig(context.buildConfig);
  } catch (error) {
    if (error instanceof GlueGenError) {
      throw error;
    }
    const data = new LogData({
      code: GlueGenErrorCode.INTERNAL_FAILURE,
      description: 'Gluegen could not validate the build configuration.',
      cause: errorMessage(error, 'unknown configuration failure'),
    });
    throw new GlueGenInternalError(data);
  }
}

function validateBuildConfig(buildConfig: BuildConfig): void {
  requireAbsolute(buildConfig.projectRootPath, 'projectRootPath');
  requireNonEmptyString(buildConfig.declgenBridgeConfigPath, 'declgenBridgeConfigPath');

  const modules = requireModuleDescriptors(buildConfig);
  requireSingleMainModuleDescriptor(modules, buildConfig.packageName);
  modules.forEach(validateModule);

  const byPackage = buildDependencyViews(modules);
  validateDependencyReferences(byPackage);
  detectCyclicDependencies(byPackage);
  requireMainPackage(byPackage, buildConfig.packageName);
}

function requireModuleDescriptors(buildConfig: BuildConfig): readonly ModuleConfig[] {
  const modules = buildConfig.dependentModuleList;
  if (!Array.isArray(modules)) {
    throw invalidBuildConfig('Build configuration field "dependentModuleList" must be an array.');
  }
  return modules;
}

function requireSingleMainModuleDescriptor(modules: readonly ModuleConfig[], mainPackageName: string): void {
  const mainModuleDescriptors = modules.filter((module) => module.packageName === mainPackageName);
  if (mainModuleDescriptors.length > 1) {
    throw invalidBuildConfig(`Main package "${mainPackageName}" has multiple module descriptors.`);
  }
}

function buildDependencyViews(modules: readonly ModuleConfig[]): ReadonlyMap<string, ModuleDependencyView> {
  const byPackage = new Map<string, ModuleDependencyView>();
  for (const module of modules) {
    if (byPackage.has(module.packageName)) {
      throw invalidBuildConfig(`Package "${module.packageName}" has multiple module descriptors.`);
    }
    const originalPackageNameMap = copyStringMap(
      module.originalPackageNameMap ?? {},
      `originalPackageNameMap for ${module.packageName}`,
    );
    const dependencies = resolveDependencyNames(
      copyStrings(module.dependencies ?? [], `dependencies for ${module.packageName}`),
      originalPackageNameMap,
    );
    byPackage.set(module.packageName, { dependencies });
  }
  return byPackage;
}

function validateDependencyReferences(byPackage: ReadonlyMap<string, ModuleDependencyView>): void {
  for (const [packageName, view] of byPackage) {
    for (const dependency of view.dependencies) {
      if (!byPackage.has(dependency)) {
        throw invalidBuildConfig(`Package "${packageName}" declares unknown dependency "${dependency}".`);
      }
    }
  }
}

function requireMainPackage(byPackage: ReadonlyMap<string, ModuleDependencyView>, mainPackageName: string): void {
  if (!byPackage.has(mainPackageName)) {
    throw invalidBuildConfig(`Main package "${mainPackageName}" is missing from dependentModuleList.`);
  }
}

function validateModule(module: ModuleConfig): void {
  requireNonEmptyString(module.packageName, 'packageName');
  requireNonEmptyString(module.modulePath, 'modulePath');
  requireStringArray(module.sourceRoots, 'sourceRoots', true);
  requireString(module.entryFile, 'entryFile');
  requireOptionalString(module.moduleName, 'moduleName');
  requireOptionalString(module.abcPath, 'abcPath');
  requireOptionalString(module.declFilesPath, 'declFilesPath');
  requireOptionalString(module.language, 'language');
  requireOptionalString(module.interopConfigPath, 'interopConfigPath');
}

interface ModuleDependencyView {
  readonly dependencies: readonly string[];
}

function detectCyclicDependencies(byPackage: ReadonlyMap<string, ModuleDependencyView>): void {
  const visiting = new Set<string>();
  const visited = new Set<string>();

  const visit = (packageName: string, path: readonly string[]): void => {
    if (visited.has(packageName)) {
      return;
    }
    if (visiting.has(packageName)) {
      const cycleStart = path.indexOf(packageName);
      const cycle = [...path.slice(cycleStart), packageName].join(' -> ');
      throw invalidBuildConfig(`Package dependency cycle detected: ${cycle}.`);
    }
    visiting.add(packageName);
    const module = byPackage.get(packageName);
    if (module !== undefined) {
      for (const dependency of module.dependencies) {
        visit(dependency, [...path, packageName]);
      }
    }
    visiting.delete(packageName);
    visited.add(packageName);
  };

  for (const packageName of byPackage.keys()) {
    visit(packageName, []);
  }
}

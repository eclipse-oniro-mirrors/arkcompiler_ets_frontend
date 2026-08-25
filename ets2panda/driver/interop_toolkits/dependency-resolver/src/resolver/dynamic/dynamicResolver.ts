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

import * as ts from 'typescript';
import * as path from 'path';
import * as common from '@interop-toolkits/common';
import { InvalidSdkAliasError } from '../../errors';
import { NodeType } from '../graph';
import type { DependencyNode } from '../graph';
import { PartialResolver } from '../partialResolver';
import type { PartialResolvedDependencyMap } from '../partialResolver';

const SDK_PREFIX = 'ohos|system|kit|arkts';

export interface AliasItem {
  originalAPIName: string;
  isStatic: boolean;
}

export type SdkAliasMap = Record<string, Record<string, AliasItem>>;

/**
 * Resolves the dependency graph of the project's dynamic ArkTS files using the
 * TypeScript compiler's module resolution. References that point at static
 * files are recorded as static nodes with `isSentinel` set so the static
 * resolver can stitch them in later. References that resolve into the static
 * interop SDK are relocated to the same-named static SDK declaration first.
 */
export class DynamicResolver extends PartialResolver {
  private readonly moduleResolutionHost: ts.ModuleResolutionHost;
  private readonly moduleResolutionCache: ts.ModuleResolutionCache;
  private readonly specifierResolutionCache: Map<string, string | undefined>;
  private readonly sdkAliasMap?: Map<string, Map<string, AliasItem>>;

  constructor(
    projectRootPath: string,
    private compilerOptions: ts.CompilerOptions,
    sdkAliasMap?: SdkAliasMap,
  ) {
    super();
    this.moduleResolutionHost = ts.sys;
    const getCanonicalFileName = ts.sys.useCaseSensitiveFileNames
      ? (fileName: string): string => fileName
      : (fileName: string): string => fileName.toLowerCase();
    this.moduleResolutionCache = ts.createModuleResolutionCache(
      projectRootPath,
      getCanonicalFileName,
      this.compilerOptions,
    );
    this.specifierResolutionCache = new Map<string, string | undefined>();
    if (sdkAliasMap !== undefined) {
      validateSdkAliasMap(sdkAliasMap);
      this.sdkAliasMap = new Map(
        Object.entries(sdkAliasMap).map(([key, value]) => [key, new Map(Object.entries(value))]),
      );
    }
  }

  override resolve(entryFiles: string[]): PartialResolvedDependencyMap {
    const nodes = new Map<string, DependencyNode>();
    const sentinels = new Set<string>();
    const processedFiles = new Set<string>();

    for (const file of entryFiles) {
      // file is guaranteed to be normalized and absolute by the manifest.
      this.resolveFile(file, nodes, sentinels, processedFiles);
    }

    return { nodes, sentinels: [...sentinels] };
  }

  /** Parse a single dynamic file and record its direct dependencies. */
  private resolveFile(
    filePath: string,
    nodes: Map<string, DependencyNode>,
    sentinels: Set<string>,
    processedFiles: Set<string>,
  ): void {
    if (processedFiles.has(filePath)) {
      return;
    }
    processedFiles.add(filePath);
    const node = this.ensureNode(nodes, filePath, NodeType.DYNAMIC);

    const sourceText = this.moduleResolutionHost.readFile?.(filePath);
    if (sourceText === undefined) {
      return; // Unreadable file: keep it as a leaf node.
    }
    const sourceFile = ts.createSourceFile(filePath, sourceText, ts.ScriptTarget.Latest, /* setParentNodes */ false);

    for (const specifier of collectModuleSpecifiers(sourceFile)) {
      const resolvedFileName = this.resolveSpecifier(specifier, filePath);
      if (resolvedFileName === undefined) {
        continue; // Unresolved (bare npm / SDK / missing): outside our scope.
      }
      const depKey = common.fileUtils.normalizePath(resolvedFileName);
      const dependency = this.classify(depKey);
      if (dependency === undefined) {
        continue; // Resolved but not part of the project: treat as external.
      }

      this.ensureNode(nodes, depKey, dependency.type, dependency.isSentinel);
      if (dependency.isSentinel) {
        sentinels.add(depKey);
      }
      if (!node.dependencies.includes(depKey)) {
        node.dependencies.push(depKey);
      }
      if (dependency.type === NodeType.DYNAMIC && !dependency.isSentinel) {
        this.resolveFile(depKey, nodes, sentinels, processedFiles);
      }
    }
  }

  /** Classify a resolved file as an in-language dependency or a cross-language sentinel. */
  private classify(key: string): { type: NodeType; isSentinel: boolean } | undefined {
    if (this.context.fileManager.dynamicFiles.has(key)) {
      return { type: NodeType.DYNAMIC, isSentinel: false };
    }
    if (this.context.fileManager.staticFiles.has(key) || this.context.fileManager.isStaticInteropSdkFile(key)) {
      return { type: NodeType.STATIC, isSentinel: true };
    }
    return undefined;
  }

  /** Resolve a module specifier to an absolute file path, or undefined if it cannot be resolved. */
  private resolveSpecifier(specifier: string, containingFile: string): string | undefined {
    const cacheKey = JSON.stringify([containingFile, specifier]);
    if (this.specifierResolutionCache.has(cacheKey)) {
      return this.specifierResolutionCache.get(cacheKey);
    }
    const resolved = this.resolveModuleName(specifier, containingFile);
    const resolvedFileName = resolved.resolvedModule?.resolvedFileName;
    this.specifierResolutionCache.set(cacheKey, resolvedFileName);
    return resolvedFileName;
  }

  private resolveModuleName(specifier: string, containingFile: string): ts.ResolvedModuleWithFailedLookupLocations {
    let resolved = this.resolveModuleNameFromDefault(specifier, containingFile);
    if (resolved !== undefined) {
      return { resolvedModule: resolved };
    }
    resolved = this.resolveModuleNameFromSdk(specifier);
    if (resolved !== undefined) {
      return { resolvedModule: resolved };
    }
    resolved = this.resolveModuleNameFromInteropSdk(specifier);
    if (resolved !== undefined) {
      return { resolvedModule: resolved };
    }
    resolved = this.resolveModuleNameFromSpecifierWithExtension(specifier, containingFile);
    if (resolved !== undefined) {
      return { resolvedModule: resolved };
    }
    if (this.sdkAliasMap) {
      resolved = this.resolveModuleNameFromSdkAlias(specifier, containingFile);
      if (resolved !== undefined) {
        return { resolvedModule: resolved };
      }
    }
    return { resolvedModule: undefined };
  }

  private resolveModuleNameFromDefault(specifier: string, containingFile: string): ts.ResolvedModuleFull | undefined {
    const resolved = ts.resolveModuleName(
      specifier,
      containingFile,
      this.compilerOptions,
      this.moduleResolutionHost,
      this.moduleResolutionCache,
    );
    if (!resolved.resolvedModule) {
      return undefined;
    }
    const resolvedFileName = resolved.resolvedModule.resolvedFileName;
    if (path.extname(resolvedFileName) === common.fileUtils.Extension.JS) {
      const resultDETSPath = resolvedFileName.replace(common.fileUtils.Extension.JS, common.fileUtils.Extension.DETS);
      if (this.moduleResolutionHost.fileExists(resultDETSPath)) {
        return getResolveModule(resultDETSPath, common.fileUtils.Extension.DETS);
      }
    }
    return resolved.resolvedModule;
  }

  private resolveModuleNameFromSdk(specifier: string): ts.ResolvedModuleFull | undefined {
    const prefixRegex = new RegExp(`^@(${SDK_PREFIX})\\.`, 'i');
    if (!prefixRegex.test(specifier.trim())) {
      return undefined;
    }
    const resolvedPath = this.context.fileManager.queryDynamicSdkPath(specifier);
    if (resolvedPath === undefined) {
      return undefined;
    }
    const sdkExtension = getSdkExtension(resolvedPath);
    if (sdkExtension === undefined) {
      return undefined;
    }
    return getResolveModule(resolvedPath, sdkExtension);
  }

  private resolveModuleNameFromInteropSdk(specifier: string): ts.ResolvedModuleFull | undefined {
    const prefixRegex = new RegExp(`^static@(${SDK_PREFIX})\\.`, 'i');
    if (!prefixRegex.test(specifier.trim())) {
      return undefined;
    }
    const actualModuleName = specifier.replace(/^static@/i, '@');
    const interopSdkPath = this.context.fileManager.queryStaticInteropSdkPath(actualModuleName);
    if (interopSdkPath === undefined) {
      return undefined;
    }
    // The interop SDK only mirrors static SDK content for dynamic consumption;
    // relocate to the same-named static SDK declaration so the dependency edge
    // points at a file the static resolver can produce a real node for (the
    // graph records it as a static sentinel). The interop SDK file remains the
    // fallback when the static SDK has no counterpart.
    const resolvedPath = this.context.fileManager.queryStaticSdkPath(actualModuleName) ?? interopSdkPath;
    const sdkExtension = getSdkExtension(resolvedPath);
    if (sdkExtension === undefined) {
      return undefined;
    }
    return getResolveModule(resolvedPath, sdkExtension);
  }

  private resolveModuleNameFromSpecifierWithExtension(
    specifier: string,
    containingFile: string,
  ): ts.ResolvedModuleFull | undefined {
    if (/\.ts$/.test(specifier)) {
      const modulePath = path.resolve(path.dirname(containingFile), specifier);
      return this.moduleResolutionHost.fileExists(modulePath)
        ? getResolveModule(modulePath, common.fileUtils.Extension.TS)
        : undefined;
    }
    if (/\.ets$/.test(specifier) || /\.d\.ets$/.test(specifier)) {
      const modulePath = path.resolve(path.dirname(containingFile), specifier);
      return this.moduleResolutionHost.fileExists(modulePath)
        ? getResolveModule(modulePath, common.fileUtils.Extension.ETS)
        : undefined;
    }

    return undefined;
  }

  private resolveModuleNameFromSdkAlias(specifier: string, containingFile: string): ts.ResolvedModuleFull | undefined {
    if (!this.sdkAliasMap) {
      return undefined;
    }
    const aliasMap = this.sdkAliasMap!;
    const fileMeta = this.context.fileManager.queryFileMeta(containingFile);
    if (!fileMeta) {
      return undefined;
    }
    const packageName = fileMeta.module?.packageName;
    if (!packageName) {
      return undefined;
    }
    const packageAliasMap = aliasMap.get(packageName);
    if (!packageAliasMap) {
      return undefined;
    }
    const aliasItem = packageAliasMap.get(specifier);
    if (!aliasItem || !aliasItem.isStatic) {
      return undefined;
    }
    return this.resolveModuleNameFromInteropSdk(aliasItem.originalAPIName);
  }

  private ensureNode(
    nodes: Map<string, DependencyNode>,
    key: string,
    type: NodeType,
    isSentinel = false,
  ): DependencyNode {
    let node = nodes.get(key);
    if (node === undefined) {
      node = { fileName: key, type, isSentinel, isResolved: !isSentinel, dependencies: [], dependants: [] };
      nodes.set(key, node);
    } else {
      node.isSentinel ||= isSentinel;
      node.isResolved ||= !isSentinel;
    }
    return node;
  }
}

/**
 * Collect the module specifiers referenced by a source file: static imports,
 * re-exports, `import =` external references and dynamic `import()` calls.
 */
function collectModuleSpecifiers(sourceFile: ts.SourceFile): string[] {
  const specifiers: string[] = [];

  const addLiteral = (expression: ts.Expression | undefined): void => {
    if (expression !== undefined && ts.isStringLiteralLike(expression)) {
      specifiers.push(expression.text);
    }
  };

  const visit = (node: ts.Node): void => {
    if (ts.isImportDeclaration(node)) {
      addLiteral(node.moduleSpecifier);
    } else if (ts.isExportDeclaration(node)) {
      addLiteral(node.moduleSpecifier);
    } else if (ts.isImportEqualsDeclaration(node) && ts.isExternalModuleReference(node.moduleReference)) {
      addLiteral(node.moduleReference.expression);
    } else if (ts.isCallExpression(node) && node.expression.kind === ts.SyntaxKind.ImportKeyword) {
      addLiteral(node.arguments[0]);
    }
    ts.forEachChild(node, visit);
  };

  visit(sourceFile);
  return specifiers;
}

function getResolveModule(modulePath: string, type: common.fileUtils.Extension): ts.ResolvedModuleFull {
  return {
    resolvedFileName: modulePath,
    isExternalLibraryImport: false,
    extension: type as string as ts.Extension,
  };
}

function getSdkExtension(modulePath: string): common.fileUtils.Extension | undefined {
  if (modulePath.endsWith(common.fileUtils.Extension.DETS)) {
    return common.fileUtils.Extension.DETS;
  }
  if (modulePath.endsWith(common.fileUtils.Extension.DTS)) {
    return common.fileUtils.Extension.DTS;
  }
  return undefined;
}

function validateSdkAliasMap(sdkAliasMap: SdkAliasMap): void {
  if (!isRecord(sdkAliasMap)) {
    throw new InvalidSdkAliasError({
      description: 'Invalid SDK alias map: expected an object keyed by package name.',
    });
  }
  for (const [packageName, aliases] of Object.entries(sdkAliasMap)) {
    if (!isRecord(aliases)) {
      throw new InvalidSdkAliasError({
        description: `Invalid SDK alias map entry for package "${packageName}": expected an object.`,
      });
    }
    for (const [alias, item] of Object.entries(aliases)) {
      if (!isRecord(item) || typeof item.originalAPIName !== 'string' || typeof item.isStatic !== 'boolean') {
        throw new InvalidSdkAliasError({
          description: `Invalid SDK alias map entry "${packageName}.${alias}": expected { originalAPIName: string, isStatic: boolean }.`,
        });
      }
    }
  }
}

function isRecord(value: unknown): value is Record<string, unknown> {
  return typeof value === 'object' && value !== null && !Array.isArray(value);
}

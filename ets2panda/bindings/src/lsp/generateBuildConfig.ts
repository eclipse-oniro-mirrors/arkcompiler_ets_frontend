/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

import * as fs from 'fs';
import * as path from 'path';
import * as JSON5 from 'json5';
import { BuildConfig, PathConfig } from '../common/types';
import {
  DEFAULT_CACHE_DIR,
  EXTERNAL_API_PATH_FROM_SDK,
  INTEROP_DYNAMIC_API_PATH_FROM_SDK,
  LANGUAGE_VERSION
} from '../common/preDefine';
import { getFileLanguageVersion } from '../common/utils';
import { logger } from './logger';

export interface ModuleDescriptor {
  name: string;
  moduleType: string;
  srcPath: string;
  arktsversion?: string;
  entryFile?: string;
  aceModuleJsonPath?: string;
  sdkAliasConfigPath?: string;
  permissions?: string[];
  deviceTypes?: string[];
  compatibleVersion?: string;
}

interface Json5Object {
  module?: {
    type?: string;
  };
  modules?: Array<{
    name: string;
    srcPath: string;
    arktsversion?: string;
  }>;
  dependencies?: {
    [packageName: string]: string;
  };
}

function parseJson5(filePath: string): Json5Object {
  try {
    const rawContent = fs.readFileSync(filePath, 'utf8');
    return JSON5.parse(rawContent) as Json5Object;
  } catch (error) {
    logger.error(`Error parsing ${filePath}:`, error);
    return {} as Json5Object;
  }
}

function isDirectory(targetPath: string): boolean {
  try {
    return fs.statSync(targetPath).isDirectory();
  } catch (error) {
    logger.error(`Error while checking the path represents a directory: ${targetPath}`, error);
    return false;
  }
}

function getEtsFiles(modulePath: string): string[] {
  const files: string[] = [];

  const shouldSkipDirectory = (relativePath: string): boolean => {
    const filterList = [`src${path.sep}test`, `src${path.sep}ohosTest`, `build${path.sep}`, `oh_modules${path.sep}`];
    return filterList.some((directoryPrefix: string) => relativePath.startsWith(directoryPrefix));
  };

  const processEntry = (dir: string, entry: fs.Dirent): void => {
    const fullPath = path.join(dir, entry.name);
    const relativePath = path.relative(modulePath, fullPath);

    if (entry.isDirectory()) {
      if (shouldSkipDirectory(relativePath)) {
        return;
      }
      traverseDir(fullPath);
      return;
    }

    if (entry.isFile() && entry.name.endsWith('.ets')) {
      files.push(fullPath);
    }
  };

  const traverseDir = (dir: string): void => {
    if (!isDirectory(dir)) {
      return;
    }

    const entries = fs.readdirSync(dir, { withFileTypes: true });
    entries.forEach((entry) => processEntry(dir, entry));
  };

  traverseDir(modulePath);
  return files;
}

function getModuleDependencies(modulePath: string, visited = new Set<string>()): string[] {
  if (visited.has(modulePath)) {
    return [];
  }
  visited.add(modulePath);

  const extractDependencies = (): string[] => {
    const packageFilePath = path.join(modulePath, 'oh-package.json5');
    if (!fs.existsSync(packageFilePath)) {
      return [];
    }

    try {
      const packageData = parseJson5(packageFilePath);
      // Temp solution only support 'file:' until third-party using in 1.2
      return Object.entries(packageData.dependencies || {})
        .filter(([_, depPath]) => depPath.startsWith('file:'))
        .map(([_, depPath]) => path.resolve(modulePath, depPath.replace('file:', '')));
    } catch (error) {
      logger.error(`Error parsing ${packageFilePath}:`, error);
      return [];
    }
  };

  const dependencies = extractDependencies();
  // Transitive closure, mirroring the build system's flattened dependency graph: every
  // dependency module contributes its own oh-package 'file:' dependencies as well.
  const transitive = dependencies.flatMap((depPath) => getModuleDependencies(depPath, visited));
  return Array.from(new Set([...dependencies, ...transitive]));
}

function createMapEntryForPlugin(buildSdkPath: string, pluginName: string): string {
  return path.join(buildSdkPath, 'build-tools', 'ui-plugins', 'lib', pluginName, 'index');
}

// The hvigor declgen task writes the declaration files while the separated interop-context
// task writes the manifest: an IDE that notices the freshly generated declarations may trigger
// the config regeneration (update / Sync Now) before the manifest catches up, and a manifest
// from an older generation leaves the arktsconfig dependencies incomplete - the interop
// scenario stops resolving the new entries. The manifest is therefore validated against the
// declaration directory itself (the ground truth of what is importable right now, no clock
// involved): a manifest is current when its key set equals the scanned declaration set, and a
// stale or missing manifest is repaired by synthesizing the entries from the directory scan,
// enriched with the per-file metadata of the most complete manifest available. The repaired
// copy lands in the LSP's own declgen cache (the first candidate below) and is marked with
// 'generatedBy', so the authoritative hvigor manifest takes over again once it lands.
const DECL_FILE_INFO_NAME = 'decl-fileInfo.json';
const DECL_MANIFEST_REPAIRED_BY = 'lsp-repair';

interface DeclFileEntry {
  declPath: string;
  filePath?: string;
  ohmUrl?: string;
}

interface DeclManifest {
  packageName?: string;
  generatedBy?: string;
  files: Record<string, DeclFileEntry>;
}

function readDeclManifest(manifestPath: string): DeclManifest | undefined {
  let manifestContent: string;
  try {
    manifestContent = fs.readFileSync(manifestPath, 'utf-8');
  } catch (error) {
    logger.warn(`failed to read decl manifest ${manifestPath}: ${String(error)}`);
    return undefined;
  }
  const asManifest = (parsed: unknown): DeclManifest | undefined =>
    parsed !== undefined && parsed !== null && typeof parsed === 'object' &&
    (parsed as DeclManifest).files !== undefined && typeof (parsed as DeclManifest).files === 'object'
      ? (parsed as DeclManifest)
      : undefined;
  // The manifests are machine written (hvigor / the repair below) and therefore plain
  // JSON; JSON5 is only the fallback for hand authored files, it parses an order of
  // magnitude slower and this read sits on the update() path of every dynamic module.
  try {
    return asManifest(JSON.parse(manifestContent));
  } catch {
    // fall through to the JSON5 retry below
  }
  try {
    return asManifest(JSON5.parse(manifestContent));
  } catch (error) {
    logger.warn(`failed to parse decl manifest ${manifestPath}: ${String(error)}`);
  }
  return undefined;
}

// Whether a target path lies inside (or equals) a directory.
function isUnderDirectory(directory: string, target: string): boolean {
  const relative = path.relative(directory, target);
  return relative === '' || (!relative.startsWith('..') && !path.isAbsolute(relative));
}

// The declaration root of a manifest: the known output layouts are probed first (the
// 'decls' subdirectory next to the manifest, the module-level output roots, the manifest
// directory itself) and the first layout that contains every recorded declaration path
// wins. A layout root spans the whole output tree, so a directory scan under it notices
// the files a fresh declgen run added anywhere in the tree and derives their keys in the
// recorded form. The entries' longest common directory stays the fallback for declaration
// sets that live under none of the probed layouts: the fold stops at the deepest shared
// directory and would miss siblings of a single leaf-clustered entry set.
function declManifestRoot(manifest: DeclManifest, manifestPath: string, moduleRootPath?: string): string | undefined {
  const declPaths = Object.values(manifest.files)
    .map((entry) => entry?.declPath)
    .filter((declPath): declPath is string => typeof declPath === 'string' && declPath.length > 0);
  if (declPaths.length === 0) {
    return undefined;
  }
  const manifestDir = path.dirname(manifestPath);
  const resolvedDeclPaths = declPaths.map((declPath) => path.resolve(manifestDir, declPath));
  const layoutRoots = [
    path.join(manifestDir, 'decls'),
    ...(moduleRootPath !== undefined
      ? [
          path.join(moduleRootPath, 'interop-declaration', 'default', 'from-dynamic'),
          path.join(moduleRootPath, 'build', 'declgen'),
          path.join(moduleRootPath, 'build', 'default', 'declgen')
        ]
      : []),
    manifestDir
  ];
  const layoutRoot = layoutRoots.find((candidate) =>
    resolvedDeclPaths.every((declPath) => isUnderDirectory(candidate, declPath))
  );
  if (layoutRoot !== undefined) {
    return layoutRoot;
  }
  let root = path.dirname(resolvedDeclPaths[0]);
  for (const declPath of resolvedDeclPaths.slice(1)) {
    while (root !== path.parse(root).root) {
      if (isUnderDirectory(root, path.dirname(declPath))) {
        break;
      }
      root = path.dirname(root);
    }
  }
  if (root === path.parse(root).root) {
    return undefined;
  }
  return root;
}

// Matches the declaration file suffix of a directory entry: the legacy interop '.d.ets'
// or the '.ats' static naming ('.d.ats').
function matchDeclSuffix(fileName: string): string | undefined {
  const declSuffixes = ['.d.ets', '.d.ats'];
  for (const suffix of declSuffixes) {
    if (fileName.endsWith(suffix)) {
      return suffix;
    }
  }
  return undefined;
}

// Scans a declaration directory: declaration file path by module relative key (extension
// stripped), mirroring the manifest entry shape. The declaration files carry the legacy
// interop '.d.ets' suffix or the '.ats' static naming ('.d.ats'); both are collected.
function scanDeclDir(declRoot: string): Map<string, string> {
  const scanned = new Map<string, string>();
  const walk = (currentDir: string): void => {
    const items = fs.readdirSync(currentDir);
    for (const item of items) {
      const itemPath = path.join(currentDir, item);
      let stat: fs.Stats;
      try {
        stat = fs.statSync(itemPath);
      } catch (error) {
        continue;
      }
      if (stat.isDirectory()) {
        walk(itemPath);
        continue;
      }
      const declSuffix = matchDeclSuffix(item);
      if (declSuffix === undefined) {
        continue;
      }
      const relative = path.relative(declRoot, itemPath).replace(/\\/g, '/');
      scanned.set(relative.slice(0, -declSuffix.length), itemPath);
    }
  };
  walk(declRoot);
  return scanned;
}

// A manifest is current when the declaration files it records are exactly the declaration
// files that exist under its root: a declgen run ahead of the interop-context task adds files
// the manifest does not know, a removed source leaves ghost entries behind, and both make the
// recorded set stale. The comparison is by resolved declaration path so it stays independent
// of the per-module key naming ('src/main/ets/Deep' vs a flat directory layout).
function manifestCoversDecls(
  manifest: DeclManifest | undefined,
  manifestPath: string,
  moduleRootPath?: string
): boolean {
  if (manifest === undefined) {
    return false;
  }
  const declRoot = declManifestRoot(manifest, manifestPath, moduleRootPath);
  if (declRoot === undefined || !fs.existsSync(declRoot)) {
    return false;
  }
  const manifestDir = path.dirname(manifestPath);
  const manifestPaths = new Set(
    Object.values(manifest.files)
      .map((entry) => (entry?.declPath !== undefined ? path.resolve(manifestDir, entry.declPath) : undefined))
      .filter((declPath): declPath is string => declPath !== undefined)
  );
  const scannedPaths = new Set(scanDeclDir(declRoot).values());
  if (manifestPaths.size !== scannedPaths.size) {
    return false;
  }
  for (const declPath of manifestPaths) {
    if (!scannedPaths.has(declPath)) {
      return false;
    }
  }
  return true;
}

// The metadata source: the most complete readable manifest, regardless of its freshness. The
// candidate path travels along: the entry metadata is matched by resolved declaration path,
// so the recorded relative paths of a manifest only resolve against its own directory.
interface MetaSourceEntry {
  candidate: string;
  manifest: DeclManifest;
}

function selectMetaSource(
  manifests: Array<{ candidate: string; manifest: DeclManifest | undefined }>
): MetaSourceEntry | undefined {
  return manifests
    .filter((entry): entry is MetaSourceEntry => entry.manifest !== undefined)
    .sort((lhs, rhs) => Object.keys(rhs.manifest.files).length - Object.keys(lhs.manifest.files).length)[0];
}

// The declaration roots the readable manifests themselves record: a root derived from a real
// manifest is a located output directory, as opposed to the probed layouts of
// collectRepairDeclRoots, which only guess where the declarations might live.
function collectManifestDeclRoots(
  manifests: Array<{ candidate: string; manifest: DeclManifest | undefined }>,
  moduleRootPath?: string
): Set<string> {
  const declRoots = new Set<string>();
  manifests.forEach((entry) => {
    if (entry.manifest === undefined) {
      return;
    }
    const declRoot = declManifestRoot(entry.manifest, entry.candidate, moduleRootPath);
    if (declRoot !== undefined) {
      declRoots.add(declRoot);
    }
  });
  return declRoots;
}

// The candidate declaration roots of a repair: the roots the readable manifests themselves
// record, plus the probed layouts for the first generation before the interop-context task
// ever ran - the 'decls' subdirectory next to where the manifest will be (probed first: the
// scan of the manifest directory itself would otherwise swallow that subdirectory into the
// keys), the manifest directory itself, the interop-declaration from-dynamic directory and
// the legacy module build roots.
function collectRepairDeclRoots(
  manifests: Array<{ candidate: string; manifest: DeclManifest | undefined }>,
  candidates: string[],
  moduleRootPath: string
): Set<string> {
  const declRoots = collectManifestDeclRoots(manifests, moduleRootPath);
  candidates.forEach((candidate) => {
    const manifestDir = path.dirname(candidate);
    declRoots.add(path.join(manifestDir, 'decls'));
    declRoots.add(manifestDir);
  });
  // The interop-declaration layout: the from-dynamic declarations are project-shared inputs
  // rather than build intermediates, so they live next to the module sources (outside
  // 'build/'). A manifest that exists points at them through its entries (the roots above),
  // but the first initialization runs before any manifest is materialized - without this
  // probe the repair would find no declaration directory and the arktsconfig dependencies
  // would stay empty until the interop-context task lands.
  declRoots.add(path.join(moduleRootPath, 'interop-declaration', 'default', 'from-dynamic'));
  declRoots.add(path.join(moduleRootPath, 'build', 'declgen'));
  declRoots.add(path.join(moduleRootPath, 'build', 'default', 'declgen'));
  return declRoots;
}

// Resolves the source file behind a manifest key: the '.ats' suffix is probed first, the
// legacy '.ets' one stays the fallback (also when neither exists), so a key with an '.ats'
// source never records a nonexistent '.ets' path that downstream would trust blindly.
function resolveKeySourceFile(key: string, moduleRootPath: string): string {
  const candidates = ['.ats', '.ets'].map((suffix) => path.resolve(moduleRootPath, `${key}${suffix}`));
  return candidates.find((candidate) => fs.existsSync(candidate)) ?? candidates[1];
}

// Synthesizes the repaired manifest from a declaration directory scan: a scanned file that
// the metadata source already records is matched by its resolved declaration path and keeps
// the original key and per-file metadata (source file, ohmUrl) - the scan derives keys from
// the directory layout, and a layout that differs from the recorded key form ('src/main/ets/Deep'
// recorded, a flat scan derives 'Deep') would rename every entry and break the import keys
// the previous generation served. Unknown files take the scan-derived key, their source path
// resolves from the key ('.ats' probed before the legacy '.ets'). An empty scan is a valid
// repair outcome for a located output root (every declaration was deleted); which roots may
// publish an empty result is decided by repairDeclManifest.
function writeRepairedManifest(
  scanned: Map<string, string>,
  metaSource: MetaSourceEntry | undefined,
  repairBaseDir: string,
  moduleName: string,
  moduleRootPath: string
): string {
  const metaByPath = new Map<string, { key: string; entry: DeclFileEntry }>();
  if (metaSource !== undefined) {
    const metaDir = path.dirname(metaSource.candidate);
    Object.entries(metaSource.manifest.files).forEach(([key, entry]) => {
      if (entry?.declPath !== undefined && entry.declPath.length > 0) {
        metaByPath.set(path.resolve(metaDir, entry.declPath), { key, entry });
      }
    });
  }
  const files: Record<string, DeclFileEntry> = {};
  scanned.forEach((declPath, key) => {
    const known = metaByPath.get(declPath);
    if (known !== undefined) {
      const knownEntry = known.entry;
      const knownFilePath =
        knownEntry.filePath !== undefined
          ? path.isAbsolute(knownEntry.filePath)
            ? knownEntry.filePath
            : path.resolve(moduleRootPath, knownEntry.filePath)
          : undefined;
      files[known.key] = {
        declPath,
        filePath: knownFilePath ?? resolveKeySourceFile(known.key, moduleRootPath),
        ohmUrl: knownEntry.ohmUrl ?? ''
      };
      return;
    }
    files[key] = {
      declPath,
      filePath: resolveKeySourceFile(key, moduleRootPath),
      ohmUrl: ''
    };
  });
  const repairedPath = path.join(repairBaseDir, moduleName, 'declgen', 'dynamic', DECL_FILE_INFO_NAME);
  fs.mkdirSync(path.dirname(repairedPath), { recursive: true });
  fs.writeFileSync(
    repairedPath,
    JSON.stringify({ packageName: moduleName, generatedBy: DECL_MANIFEST_REPAIRED_BY, files }, null, 2),
    'utf-8'
  );
  return repairedPath;
}

function repairDeclManifest(
  manifests: Array<{ candidate: string; manifest: DeclManifest | undefined }>,
  candidates: string[],
  repairBaseDir: string,
  moduleName: string,
  moduleRootPath: string
): string | undefined {
  if (!repairBaseDir) {
    // No LSP cache location to place the repaired copy at.
    return undefined;
  }
  const metaSource = selectMetaSource(manifests);
  // An existing root recorded by a readable manifest that scans empty means every
  // declaration was deleted: the correct repair is an EMPTY manifest - falling through to
  // the caller's 'existing[0]' would keep registering dependencies that point at the
  // deleted files. Probed roots stay non-authoritative: an empty scan there may simply
  // mean the declarations live elsewhere, and a root that still holds declarations always
  // wins over the empty result.
  const locatedRoots = collectManifestDeclRoots(manifests, moduleRootPath);
  let locatedEmptyScan: Map<string, string> | undefined;
  for (const declRoot of collectRepairDeclRoots(manifests, candidates, moduleRootPath)) {
    if (!fs.existsSync(declRoot)) {
      continue;
    }
    const scanned = scanDeclDir(declRoot);
    if (scanned.size > 0) {
      return writeRepairedManifest(scanned, metaSource, repairBaseDir, moduleName, moduleRootPath);
    }
    if (locatedEmptyScan === undefined && locatedRoots.has(declRoot)) {
      locatedEmptyScan = scanned;
    }
  }
  if (locatedEmptyScan !== undefined) {
    return writeRepairedManifest(locatedEmptyScan, metaSource, repairBaseDir, moduleName, moduleRootPath);
  }
  return undefined;
}

// The decl file info of a dynamic module is looked up in the LSP declgen cache first, then in
// the module build outputs produced by hvigor (declgen default layout, the separated interop
// context task layout, and the etsFortgz loader output layout). A candidate is only used when
// it covers the declaration directory it describes; see the repair note above. 'repairBaseDir'
// is where a repaired copy lands: the declgen cache, or the LSP cache directory when the
// caller did not configure a declgen output (the copy must never land inside the hvigor
// build outputs).
function resolveDeclFilesPath(
  declgenOutDir: string,
  cacheDir: string,
  moduleName: string,
  moduleRootPath: string
): string {
  const candidates = [
    path.join(declgenOutDir, moduleName, 'declgen', 'dynamic', DECL_FILE_INFO_NAME),
    path.join(moduleRootPath, 'build', 'default', 'intermediates', 'declgen', 'default', DECL_FILE_INFO_NAME),
    path.join(moduleRootPath, 'build', 'default', 'intermediates', 'interop_context', 'default', DECL_FILE_INFO_NAME),
    path.join(
      moduleRootPath,
      'build',
      'default',
      'intermediates',
      'loader_out',
      'default',
      'etsFortgz',
      DECL_FILE_INFO_NAME
    )
  ];
  const existing = candidates.filter((candidate) => fs.existsSync(candidate));
  const manifests = existing.map((candidate) => ({ candidate, manifest: readDeclManifest(candidate) }));
  const repaired = (entry: { manifest: DeclManifest | undefined }): boolean =>
    entry.manifest?.generatedBy === DECL_MANIFEST_REPAIRED_BY;
  // An authoritative (hvigor / IDE declgen produced) manifest that covers the current
  // declaration set wins; a previously repaired copy only fills in while nothing better
  // exists, so the authoritative metadata replaces it once the interop-context task lands.
  const current =
    manifests.find(
      (entry) => manifestCoversDecls(entry.manifest, entry.candidate, moduleRootPath) && !repaired(entry)
    ) ?? manifests.find((entry) => manifestCoversDecls(entry.manifest, entry.candidate, moduleRootPath));
  const repairBaseDir = declgenOutDir || cacheDir;
  return (
    current?.candidate ?? repairDeclManifest(manifests, candidates, repairBaseDir, moduleName, moduleRootPath) ??
    existing[0] ??
    candidates[0]
  );
}

const DEFAULT_PLUGIN_LIST = ['ui-syntax-plugins', 'ui-plugins', 'memo-plugins'];

function createPluginMap(buildSdkPath: string, pluginList: string[] = DEFAULT_PLUGIN_LIST): Record<string, string> {
  const pluginMap: Record<string, string> = {};

  for (const plugin of pluginList) {
    pluginMap[plugin] = createMapEntryForPlugin(buildSdkPath, plugin);
  }
  return pluginMap;
}

function addPluginPathConfigs(buildConfig: BuildConfig, module: ModuleDescriptor): void {
  buildConfig.aceModuleJsonPath = module.aceModuleJsonPath;
}

function getModuleLanguageVersion(compileFiles: Set<string>): string {
  let found1_1 = false;
  let found1_2 = false;

  for (const file of compileFiles) {
    const sourceFile = fs.readFileSync(file, 'utf8');
    const languageVersion = getFileLanguageVersion(sourceFile);

    if (languageVersion === LANGUAGE_VERSION.ARKTS_1_2) {
      found1_2 = true;
    } else if (languageVersion === LANGUAGE_VERSION.ARKTS_1_1) {
      found1_1 = true;
    }

    if (found1_1 && found1_2) {
      return LANGUAGE_VERSION.ARKTS_HYBRID;
    }
  }

  return found1_2 ? LANGUAGE_VERSION.ARKTS_1_2 : found1_1 ? LANGUAGE_VERSION.ARKTS_1_1 : '';
}

// Mirrors the build system: the module language is an input provided by the caller
// (hvigor passes it per dependency module); the per-file 'use static' scan is only a
// fallback for callers that do not declare it.
function resolveModuleLanguageVersion(module: ModuleDescriptor, compileFiles: Set<string>): string {
  if (module.arktsversion === LANGUAGE_VERSION.ARKTS_1_2 || module.arktsversion === LANGUAGE_VERSION.ARKTS_1_1) {
    return module.arktsversion;
  }
  return getModuleLanguageVersion(compileFiles);
}

// The recursive dependency sources of a module: the compile files of every module its
// oh-package dependencies resolve to.
function collectDepModuleCompileFiles(dependencies: string[]): Set<string> {
  const depModuleCompileFiles = new Set<string>();
  for (const depPath of dependencies) {
    getEtsFiles(depPath).forEach((file) => depModuleCompileFiles.add(file));
  }
  return depModuleCompileFiles;
}

// The build configuration of one module: its compiled files, the recursive dependency
// sources and the SDK/declgen paths the config generation and the LSP queries read.
function createModuleBuildConfig(
  module: ModuleDescriptor,
  pathConfig: PathConfig,
  pluginMap: Record<string, string>,
  definedModules: ModuleDescriptor[]
): BuildConfig {
  const modulePath = module.srcPath;
  const compileFiles = new Set(getEtsFiles(modulePath));
  const dependencies = getModuleDependencies(modulePath);
  const depModuleCompileFiles = collectDepModuleCompileFiles(dependencies);
  const languageVersion = resolveModuleLanguageVersion(module, compileFiles);
  const buildConfig: BuildConfig = {
    plugins: pluginMap,
    compileFiles: Array.from(compileFiles),
    depModuleCompileFiles: Array.from(depModuleCompileFiles),
    packageName: module.name,
    moduleType: module.moduleType,
    moduleRootPath: modulePath,
    language: languageVersion,
    buildSdkPath: pathConfig.buildSdkPath,
    projectPath: pathConfig.projectPath,
    declgenOutDir: pathConfig.declgenOutDir,
    externalApiPath: pathConfig.externalApiPath
      ? pathConfig.externalApiPath
      : path.resolve(pathConfig.buildSdkPath, EXTERNAL_API_PATH_FROM_SDK),
    // Dynamic SDK declarations live under the dynamic SDK root ('<sdk>/../dynamic/build-tools/interop',
    // no 'declaration' level) - the same base the build system uses (process_build_config.ts
    // initInteropSDKInfo); pointing at the static root's interop/declaration directory would
    // load condensed interop stubs instead of the dynamic API declarations.
    interopApiPath: pathConfig.interopApiPath
      ? pathConfig.interopApiPath
      : path.resolve(pathConfig.buildSdkPath, INTEROP_DYNAMIC_API_PATH_FROM_SDK),
    cacheDir:
      pathConfig.cacheDir !== undefined ? pathConfig.cacheDir : path.join(pathConfig.projectPath, DEFAULT_CACHE_DIR),
    declFilesPath: resolveDeclFilesPath(
      pathConfig.declgenOutDir,
      pathConfig.cacheDir ?? path.join(pathConfig.projectPath, DEFAULT_CACHE_DIR),
      module.name,
      modulePath
    ),
    declgenV1OutPath:
      languageVersion !== LANGUAGE_VERSION.ARKTS_1_1
        ? path.join(pathConfig.declgenOutDir, module.name, 'declgen', 'static')
        : undefined,
    dependencies: processDependencies(module.name, dependencies, definedModules),
    sdkAliasConfigPath: module.sdkAliasConfigPath ? module.sdkAliasConfigPath : undefined,
    entryFile: module.entryFile,
    permissions: module.permissions,
    deviceTypes: module.deviceTypes,
    compatibleVersion: module.compatibleVersion
  };
  addPluginPathConfigs(buildConfig, module);
  return buildConfig;
}

export function generateBuildConfigs(
  pathConfig: PathConfig,
  modules: ModuleDescriptor[],
  plugins?: string[]
): Record<string, BuildConfig> {
  const allBuildConfigs: Record<string, BuildConfig> = {};
  const definedModules = modules;
  const pluginMap = createPluginMap(pathConfig.buildSdkPath, plugins);

  for (const module of definedModules) {
    allBuildConfigs[module.name] = createModuleBuildConfig(module, pathConfig, pluginMap, definedModules);
  }
  return allBuildConfigs;
}

function processDependencies(
  currentModule: string,
  dependencies: string[],
  definedModules: ModuleDescriptor[]
): string[] {
  const moduleMap = new Map<string, ModuleDescriptor>();
  definedModules.forEach((module) => {
    moduleMap.set(module.srcPath, module);
  });

  const result: string[] = [];
  dependencies.forEach((dep) => {
    const depModule = moduleMap.get(dep);
    if (depModule && depModule.name !== currentModule) {
      result.push(depModule.name);
    }
  });
  return result;
}

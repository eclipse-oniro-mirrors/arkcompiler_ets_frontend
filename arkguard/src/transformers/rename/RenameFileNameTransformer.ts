/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

import {
  factory,
  isStringLiteral,
  isExportDeclaration,
  isImportDeclaration,
  isSourceFile,
  setParentRecursive,
  visitEachChild,
  isStructDeclaration,
  SyntaxKind,
  isConstructorDeclaration,
} from 'typescript';

import type {
  CallExpression,
  Expression,
  ImportDeclaration,
  ExportDeclaration,
  Node,
  StringLiteral,
  TransformationContext,
  Transformer,
  StructDeclaration,
  SourceFile,
  ClassElement,
  ImportCall,
  TransformerFactory,
} from 'typescript';

import fs from 'fs';
import path from 'path';

import type { IOptions } from '../../configs/IOptions';
import type { TransformPlugin } from '../TransformPlugin';
import { TransformerOrder } from '../TransformPlugin';
import type { IFileNameObfuscationOption } from '../../configs/INameObfuscationOption';
import { OhmUrlStatus } from '../../configs/INameObfuscationOption';
import { NameGeneratorType, getNameGenerator } from '../../generator/NameFactory';
import type { INameGenerator, NameGeneratorOptions } from '../../generator/INameGenerator';
import { FileUtils, BUNDLE, NORMALIZE } from '../../utils/FileUtils';
import { NodeUtils } from '../../utils/NodeUtils';
import { orignalFilePathForSearching, performancePrinter, ArkObfuscator } from '../../ArkObfuscator';
import type { PathAndExtension, ProjectInfo } from '../../common/type';
import { endFilesEvent, endSingleFileEvent, startFilesEvent, startSingleFileEvent } from '../../utils/PrinterUtils';
import { EventList, endSingleFileForMoreTimeEvent, startSingleFileForMoreTimeEvent } from '../../utils/PrinterTimeAndMemUtils';
import { needToBeReserved } from '../../utils/TransformUtil';
import { MemoryDottingDefine } from '../../utils/MemoryDottingDefine';
namespace secharmony {

  // global mangled file name table used by all files in a project
  export let globalFileNameMangledTable: Map<string, string> = new Map<string, string>();

  // used for file name cache
  export let historyFileNameMangledTable: Map<string, string> = undefined;

  // file name whiteList
  export let reservedFileNames: Set<string> = new Set<string>();

  // When the module is compiled, call this function to clear global collections related to file name.
  export function clearCaches(): void {
    globalFileNameMangledTable.clear();
    historyFileNameMangledTable?.clear();
  }

  let profile: IFileNameObfuscationOption | undefined;
  let generator: INameGenerator | undefined;
  let localPackageSet: Set<string> | undefined;
  let useNormalized: boolean = false;
  let universalReservedFileNames: RegExp[] | undefined;

  /**
   * Rename Properties Transformer
   *
   * @param option obfuscation options
   */
  const createRenameFileNameFactory = function (options: IOptions): TransformerFactory<Node> | null {
    startFilesEvent(EventList.FILENAME_OBFUSCATION_INITIALIZATION);
    profile = options?.mRenameFileName;
    if (!profile || !profile.mEnable) {
      endFilesEvent(EventList.FILENAME_OBFUSCATION_INITIALIZATION);
      return null;
    }

    let nameGeneratorOption: NameGeneratorOptions = {};

    generator = getNameGenerator(profile.mNameGeneratorType, nameGeneratorOption);
    let configReservedFileNameOrPath: string[] = profile?.mReservedFileNames ?? [];
    reservedFileNames = new Set<string>(generateReservedList(configReservedFileNameOrPath));
    universalReservedFileNames = profile?.mUniversalReservedFileNames ?? [];
    endFilesEvent(EventList.FILENAME_OBFUSCATION_INITIALIZATION);
    return renameFileNameFactory;

    function renameFileNameFactory(context: TransformationContext): Transformer<Node> {
      let projectInfo: ProjectInfo = ArkObfuscator.mProjectInfo;
      if (projectInfo && projectInfo.localPackageSet) {
        localPackageSet = projectInfo.localPackageSet;
        useNormalized = projectInfo.useNormalized;
      }

      return renameFileNameTransformer;

      function renameFileNameTransformer(node: Node): Node {
        if (globalFileNameMangledTable === undefined) {
          globalFileNameMangledTable = new Map<string, string>();
        }

        const recordInfo = ArkObfuscator.recordStage(MemoryDottingDefine.FILENAME_OBFUSCATION);
        startSingleFileEvent(EventList.FILENAME_OBFUSCATION, performancePrinter.timeSumPrinter);
        let ret: Node = updateNodeInfo(node);
        if (!isInOhModules(projectInfo, orignalFilePathForSearching) && isSourceFile(ret)) {
          const orignalAbsPath = ret.fileName;
          const mangledAbsPath: string = getMangleCompletePath(orignalAbsPath);
          ret.fileName = mangledAbsPath;
        }
        endSingleFileEvent(EventList.FILENAME_OBFUSCATION, performancePrinter.timeSumPrinter);
        startSingleFileEvent(EventList.UPDATE_PARENT_NODE);
        let parentNodes = setParentRecursive(ret, true);
        endSingleFileEvent(EventList.UPDATE_PARENT_NODE);
        ArkObfuscator.stopRecordStage(recordInfo);
        return parentNodes;
      }

      function updateNodeInfo(node: Node): Node {
        if (isImportDeclaration(node) || isExportDeclaration(node)) {
          startSingleFileForMoreTimeEvent(EventList.UPDATE_IMPORT_OR_EXPORT);
          const updatedNode = updateImportOrExportDeclaration(node);
          endSingleFileForMoreTimeEvent(EventList.UPDATE_IMPORT_OR_EXPORT);
          return updatedNode;
        }
        if (isImportCall(node)) {
          startSingleFileForMoreTimeEvent(EventList.UPDATE_DYNAMIC_IMPORT);
          const tryUpdatedNode = tryUpdateDynamicImport(node);
          endSingleFileForMoreTimeEvent(EventList.UPDATE_DYNAMIC_IMPORT);
          return tryUpdatedNode;
        }

        return visitEachChild(node, updateNodeInfo, context);
      }
    }
  };

  export function isInOhModules(proInfo: ProjectInfo, originalPath: string): boolean {
    let ohPackagePath: string = '';
    if (proInfo && proInfo.projectRootPath && proInfo.packageDir) {
      ohPackagePath = FileUtils.toUnixPath(path.resolve(proInfo.projectRootPath, proInfo.packageDir));
    }
    return ohPackagePath && FileUtils.toUnixPath(originalPath).indexOf(ohPackagePath) !== -1;
  }

  function updateImportOrExportDeclaration(node: ImportDeclaration | ExportDeclaration): ImportDeclaration | ExportDeclaration {
    if (!node.moduleSpecifier) {
      return node;
    }
    const mangledModuleSpecifier = renameStringLiteral(node.moduleSpecifier as StringLiteral);
    if (isImportDeclaration(node)) {
      return factory.updateImportDeclaration(node, node.modifiers, node.importClause, mangledModuleSpecifier as Expression, node.assertClause);
    } else {
      return factory.updateExportDeclaration(node, node.modifiers, node.isTypeOnly, node.exportClause, mangledModuleSpecifier as Expression,
        node.assertClause);
    }
  }

  export function updateImportOrExportDeclarationForTest(node: ImportDeclaration | ExportDeclaration): ImportDeclaration | ExportDeclaration {
    return updateImportOrExportDeclaration(node);
  }

  function isImportCall(n: Node): n is ImportCall {
    return n.kind === SyntaxKind.CallExpression && (<CallExpression>n).expression.kind === SyntaxKind.ImportKeyword;
  }

  function canBeObfuscatedFilePath(filePath: string): boolean {
    return path.isAbsolute(filePath) || FileUtils.isRelativePath(filePath) || isLocalDependencyOhmUrl(filePath);
  }

  function isLocalDependencyOhmUrl(filePath: string): boolean {
    // mOhmUrlStatus: for unit test in Arkguard
    if (profile?.mOhmUrlStatus === OhmUrlStatus.AT_BUNDLE ||
        profile?.mOhmUrlStatus === OhmUrlStatus.NORMALIZED) {
      return true;
    }

    let packageName: string;
    // Only hap and local har need be mangled.
    if (useNormalized) {
      if (!filePath.startsWith(NORMALIZE)) {
        return false;
      }
      packageName = handleNormalizedOhmUrl(filePath, true);
    } else {
      if (!filePath.startsWith(BUNDLE)) {
        return false;
      }
      packageName = getAtBundlePgkName(filePath);
    }
    return localPackageSet && localPackageSet.has(packageName);
  }

  export function isLocalDependencyOhmUrlForTest(filePath: string): boolean {
    return isLocalDependencyOhmUrl(filePath);
  }

  function getAtBundlePgkName(ohmUrl: string): string {
    /* Unnormalized OhmUrl Format:
    * hap/hsp: @bundle:${bundleName}/${moduleName}/
    * har: @bundle:${bundleName}/${moduleName}@${harName}/
    * package name is {moduleName} in hap/hsp or {harName} in har.
    */
    let moduleName: string = ohmUrl.split('/')[1]; // 1: the index of moduleName in array.
    const indexOfSign: number = moduleName.indexOf('@');
    if (indexOfSign !== -1) {
      moduleName = moduleName.slice(indexOfSign + 1); // 1: the index start from indexOfSign + 1.
    }
    return moduleName;
  }

  // dynamic import example: let module = import('./a')
  function tryUpdateDynamicImport(node: CallExpression): CallExpression {
    if (node.expression && node.arguments.length === 1 && isStringLiteral(node.arguments[0])) {
      const obfuscatedArgument = [renameStringLiteral(node.arguments[0] as StringLiteral)];
      if (obfuscatedArgument[0] !== node.arguments[0]) {
        return factory.updateCallExpression(node, node.expression, node.typeArguments, obfuscatedArgument);
      }
    }
    return node;
  }

  function renameStringLiteral(node: StringLiteral): Expression {
    let expr: StringLiteral = renameFileName(node) as StringLiteral;
    if (expr !== node) {
      return factory.createStringLiteral(expr.text);
    }
    return node;
  }

  function renameFileName(node: StringLiteral): Node {
    let original: string = '';
    original = node.text;
    original = original.replace(/\\/g, '/');

    if (!canBeObfuscatedFilePath(original)) {
      return node;
    }

    let mangledFileName: string = getMangleIncompletePath(original);
    if (mangledFileName === original) {
      return node;
    }

    return factory.createStringLiteral(mangledFileName);
  }

  export function getMangleCompletePath(originalCompletePath: string): string {
    originalCompletePath = FileUtils.toUnixPath(originalCompletePath);
    const { path: filePathWithoutSuffix, ext: extension } = FileUtils.getFileSuffix(originalCompletePath);
    const mangleFilePath = mangleFileName(filePathWithoutSuffix);
    return mangleFilePath + extension;
  }

  function getMangleIncompletePath(orignalPath: string): string {
    // The ohmUrl format does not have file extension
    if (isLocalDependencyOhmUrl(orignalPath)) {
      const mangledOhmUrl = mangleOhmUrl(orignalPath);
      return mangledOhmUrl;
    }

    // Try to concat the extension for orignalPath.
    const pathAndExtension : PathAndExtension | undefined = tryValidateFileExisting(orignalPath);
    if (!pathAndExtension) {
      return orignalPath;
    }

    if (pathAndExtension.ext) {
      const mangleFilePath = mangleFileName(pathAndExtension.path);
      return mangleFilePath;
    }
    /**
     * import * from './filename1.js'. We just need to obfuscate 'filename1' and then concat the extension 'js'.
     * import * from './direcotry'. For the grammar of importing directory, TSC will look for index.ets/index.ts when parsing.
     * We obfuscate directory name and do not need to concat extension.
     */
    const { path: filePathWithoutSuffix, ext: extension } = FileUtils.getFileSuffix(pathAndExtension.path);
    const mangleFilePath = mangleFileName(filePathWithoutSuffix);
    return mangleFilePath + extension;
  }

  export function getMangleIncompletePathForTest(orignalPath: string): string {
    return getMangleIncompletePath(orignalPath);
  };

  export function mangleOhmUrl(ohmUrl: string): string {
    let mangledOhmUrl: string;
    // mOhmUrlStatus: for unit test in Arkguard
    if (useNormalized || profile?.mOhmUrlStatus === OhmUrlStatus.NORMALIZED) {
      mangledOhmUrl = handleNormalizedOhmUrl(ohmUrl);
    } else {
      /**
       * OhmUrl Format:
       * fixed parts in hap/hsp: @bundle:${bundleName}/${moduleName}/
       * fixed parts in har: @bundle:${bundleName}/${moduleName}@${harName}/
       * hsp example: @bundle:com.example.myapplication/entry/index
       * har example: @bundle:com.example.myapplication/entry@library_test/index
       * we do not mangle fixed parts.
       */
      const originalOhmUrlSegments: string[] = FileUtils.splitFilePath(ohmUrl);
      const prefixSegments: string[] = originalOhmUrlSegments.slice(0, 2); // 2: length of fixed parts in array
      const urlSegments: string[] = originalOhmUrlSegments.slice(2); // 2: index of mangled parts in array
      const mangledOhmUrlSegments: string[] = urlSegments.map(originalSegment => mangleFileNamePart(originalSegment));
      mangledOhmUrl = prefixSegments.join('/') + '/' + mangledOhmUrlSegments.join('/');
    }
    return mangledOhmUrl;
  }

  /**
   * Normalized OhmUrl Format:
   * hap/hsp: @normalized:N&<module name>&<bundle name>&<standard import path>&
   * har: @normalized:N&&<bundle name>&<standard import path>&<version>
   * we only mangle <standard import path>.
   */
  export function handleNormalizedOhmUrl(ohmUrl: string, needPkgName?: boolean): string {
    let originalOhmUrlSegments: string[] = ohmUrl.split('&');
    const standardImportPath = originalOhmUrlSegments[3]; // 3: index of standard import path in array.
    let index = standardImportPath.indexOf('/');
    // The format of <module name>: @group/packagename or packagename,
    // and there should only be one '@' symbol and one path separator '/' if and only if the 'group' exists.
    if (standardImportPath.startsWith('@')) {
      index = standardImportPath.indexOf('/', index + 1);
    }

    const pakName = standardImportPath.substring(0, index);
    if (needPkgName) {
      return pakName;
    }
    const realImportPath = standardImportPath.substring(index + 1); // 1: index of real import path in array.
    const originalImportPathSegments: string[] = FileUtils.splitFilePath(realImportPath);
    const mangledImportPathSegments: string[] = originalImportPathSegments.map(originalSegment => mangleFileNamePart(originalSegment));
    const mangledImportPath: string = pakName + '/' + mangledImportPathSegments.join('/');
    originalOhmUrlSegments[3] = mangledImportPath; // 3: index of standard import path in array.
    return originalOhmUrlSegments.join('&');
  }

  function mangleFileName(orignalPath: string): string {
    const originalFileNameSegments: string[] = FileUtils.splitFilePath(orignalPath);
    const mangledSegments: string[] = originalFileNameSegments.map(originalSegment => mangleFileNamePart(originalSegment));
    let mangledFileName: string = mangledSegments.join('/');
    return mangledFileName;
  }

  function mangleFileNamePart(original: string): string {
    if (needToBeReserved(reservedFileNames, universalReservedFileNames, original)) {
      return original;
    }

    const historyName: string = historyFileNameMangledTable?.get(original);
    let mangledName: string = historyName ? historyName : globalFileNameMangledTable.get(original);

    while (!mangledName) {
      mangledName = generator.getName();
      if (mangledName === original || needToBeReserved(reservedFileNames, universalReservedFileNames, mangledName)) {
        mangledName = null;
        continue;
      }

      let reserved: string[] = [...globalFileNameMangledTable.values()];
      if (reserved.includes(mangledName)) {
        mangledName = null;
        continue;
      }

      if (historyFileNameMangledTable && [...historyFileNameMangledTable.values()].includes(mangledName)) {
        mangledName = null;
        continue;
      }
    }
    globalFileNameMangledTable.set(original, mangledName);
    return mangledName;
  }

  function generateReservedList(fileNameOrPathList: string[]): string[] {
    const reservedNames: string[] = ['.', '..', ''];
    fileNameOrPathList.map(fileNameOrPath => {
      if (!fileNameOrPath || fileNameOrPath.length === 0) {
        return;
      }
      const directories = FileUtils.splitFilePath(fileNameOrPath);
      directories.forEach(directory => {
        reservedNames.push(directory);
        const pathOrExtension: PathAndExtension = FileUtils.getFileSuffix(directory);
        if (pathOrExtension.ext) {
          reservedNames.push(pathOrExtension.ext);
          reservedNames.push(pathOrExtension.path);
        }
      });
    });
    return reservedNames;
  }

  export let transformerPlugin: TransformPlugin = {
    'name': 'renamePropertiesPlugin',
    'order': TransformerOrder.RENAME_FILE_NAME_TRANSFORMER,
    'createTransformerFactory': createRenameFileNameFactory
  };

  /**
   * add whiteList of reserved file names
   * @param fileNameOrPathList file paths that need to reserve 
   */
  export function addReservedFileNames(fileNameOrPathList: string[]): void {
    const tempReservedNames: string[] = generateReservedList(fileNameOrPathList);
    reservedFileNames = new Set<string>([...reservedFileNames, ...tempReservedNames]);
  }
}

export = secharmony;

// typescript doesn't add the json extension.
const extensionOrder: string[] = ['.ets', '.ts', '.d.ets', '.d.ts', '.js'];

function tryValidateFileExisting(importPath: string): PathAndExtension | undefined {
  let fileAbsPath: string = '';
  if (path.isAbsolute(importPath)) {
    fileAbsPath = importPath;
  } else {
    fileAbsPath = path.join(path.dirname(orignalFilePathForSearching), importPath);
  }

  const filePathExtensionLess: string = path.normalize(fileAbsPath);
  for (let ext of extensionOrder) {
    const targetPath = filePathExtensionLess + ext;
    if (fs.existsSync(targetPath)) {
      return {path: importPath, ext: ext};
    }
  }

  // all suffixes are not matched, search this file directly.
  if (fs.existsSync(filePathExtensionLess)) {
    return { path: importPath, ext: undefined };
  }
  return undefined;
}
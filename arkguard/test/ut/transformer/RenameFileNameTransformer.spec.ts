/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

import { describe, it } from 'mocha';
import { TransformerManager } from '../../../src/transformers/TransformerManager';
import { assert, expect } from 'chai';
import { FileUtils } from '../../../src/utils/FileUtils';
import path from 'path';
import { IOptions } from '../../../src/configs/IOptions';
import fs from 'fs';
import ts, {
  TransformerFactory,
  Node,
  TransformationContext,
  factory,
  SyntaxKind,
  CallExpression,
  Expression,
  CompilerOptions,
  EmitHelper,
  EmitHint,
  FunctionDeclaration,
  Identifier,
  Statement,
  SourceFile,
  VariableDeclaration,
} from 'typescript';
import {
  getMangleCompletePath,
  handleNormalizedOhmUrl,
  globalFileNameMangledTable,
  clearCaches,
  isInOhModules,
  isLocalDependencyOhmUrlForTest,
  mangleOhmUrl,
  getMangleIncompletePathForTest,
  addReservedFileNames,
} from '../../../src/transformers/rename/RenameFileNameTransformer';
import type { ProjectInfo } from '../../../src/common/type';
import { OhmUrlStatus } from '../../../src/configs/INameObfuscationOption';
import secharmony from '../../../src/transformers/rename/RenameFileNameTransformer';
import { ArkObfuscator } from '../../../src/ArkObfuscator';

const renameFileNameModule = require('../../../src/transformers/rename/RenameFileNameTransformer');
const obfuscator = require('../../../src/ArkObfuscator');

describe('Tester Cases for <RenameFileNameTransformer>.', function () {
  let options;
  let context: TransformationContext = {
    requestEmitHelper: function (helper: EmitHelper): void {},
    readEmitHelpers: function (): EmitHelper[] | undefined {
      return undefined;
    },
    enableSubstitution: function (kind: SyntaxKind): void {},
    isSubstitutionEnabled: function (node: Node): boolean {
      throw new Error('Function not implemented.');
    },
    onSubstituteNode: function (hint: EmitHint, node: Node): Node {
      throw new Error('Function not implemented.');
    },
    enableEmitNotification: function (kind: SyntaxKind): void {},
    isEmitNotificationEnabled: function (node: Node): boolean {
      throw new Error('Function not implemented.');
    },
    onEmitNode: function (hint: EmitHint, node: Node, emitCallback: (hint: EmitHint, node: Node) => void): void {
      throw new Error('Function not implemented.');
    },
    factory: ts.factory,
    getCompilerOptions: function (): CompilerOptions {
      throw new Error('Function not implemented.');
    },
    startLexicalEnvironment: function (): void {},
    suspendLexicalEnvironment: function (): void {},
    resumeLexicalEnvironment: function (): void {},
    endLexicalEnvironment: function (): Statement[] | undefined {
      return undefined;
    },
    hoistFunctionDeclaration: function (node: FunctionDeclaration): void {},
    hoistVariableDeclaration: function (node: Identifier): void {}
  };
  let node: Node;

  describe('Tester Cases for <createRenameFileNameFactory>.', function () {
    let projectInfo;

    beforeEach(() => {
      options = {
        mRenameFileName: {
          mEnable: true,
          mNameGeneratorType: 1,
          mReservedFileNames: ['file1.ts', 'file2.ts'],
          mUniversalReservedFileNames: [],
          mReservedProperties: ['1'],
          mReservedToplevelNames: ['2'],
        }
      };
      projectInfo = {
        projectRootPath: '/Users/test/project',
        packageDir: 'src/ohmodules',
        localPackageSet: new Set<string>(),
        useNormalized: true,
        useTsHar: true,
      };
    });

    it('Tester: Test initialization of config in api createRenameFileNameFactory', function () {
      let mCustomProfiles: IOptions | undefined = FileUtils.readFileAsJson(path.join(__dirname, "obfuscate_filename_config.json"));
      assert.strictEqual(mCustomProfiles !== undefined, true);
      let mTransformers: TransformerFactory<Node>[] = [];
      if (mCustomProfiles) {
        mTransformers = new TransformerManager(mCustomProfiles).getTransformers();
      }
      const originalPath = 'D:/workplace/src/ets/entryability/EntryAbility.ts';
      const mangledPath = getMangleCompletePath(originalPath);
      assert.strictEqual(mangledPath === 'D:/workplace/src/ets/a/b.ts', true);
    });

    it('should return null if mEnable is false', () => {
      options.mRenameFileName.mEnable = false;
      const result = secharmony.transformerPlugin.createTransformerFactory(options);
      expect(result).to.be.null;
    });

    it('should return null if profile is null', () => {
      options = {};
      const result = secharmony.transformerPlugin.createTransformerFactory(options);
      expect(result).to.be.null;
    });

    it('should create globalFileNameMangledTable if not exist', () => {
      const transformerFactory = secharmony.transformerPlugin.createTransformerFactory(options);
      clearCaches();
      renameFileNameModule.globalFileNameMangledTable = undefined;
      const sourcefile = factory.createSourceFile([],factory.createToken(SyntaxKind.EndOfFileToken),0);
      const transformer = transformerFactory(context);
      const node = transformer(sourcefile);
      expect(globalFileNameMangledTable).to.not.be.undefined;
    })

    it('should rename the node.fileName if it is a sourcefile and not in OhModules', () => {
      ArkObfuscator.projectInfo = projectInfo;
      obfuscator.orignalFilePathForSearching = '/Users/test/project/src/othermodules/somefile.js';
      options.mRenameFileName.mReservedFileNames = ["D:", "workplace", "src", "ets"];
      const transformerFactory = secharmony.transformerPlugin.createTransformerFactory(options);
      clearCaches();
      const sourcefile = factory.createSourceFile([], factory.createToken(SyntaxKind.EndOfFileToken), 0);
      sourcefile.fileName = 'D:/workplace/src/ets/entryability/EntryAbility.ts';
      const transformer = transformerFactory(context);
      const node = transformer(sourcefile);
      expect((node as SourceFile).fileName).to.equal('D:/workplace/src/ets/a/b.ts');
    })

    it('should update declaration if node is ImportDeclaration', () => {
      const transformerFactory = secharmony.transformerPlugin.createTransformerFactory(options);
      clearCaches();
      const importdeclaration = factory.createImportDeclaration(
        undefined, undefined, factory.createStringLiteral('ModuleInfo'), undefined);
      const transformer = transformerFactory(context);
      const node = transformer(importdeclaration);
      expect(node).to.equal(importdeclaration);
    })

    it('should update declaration if node is ExportDeclaration', () => {
      const transformerFactory = secharmony.transformerPlugin.createTransformerFactory(options);
      clearCaches();
      const exportdeclaration = factory.createExportDeclaration(
        undefined, true, undefined, factory.createStringLiteral('ModuleInfo'), undefined);
      const transformer = transformerFactory(context);
      const node = transformer(exportdeclaration);
      expect(node).to.equal(exportdeclaration);
    })

    it('should try update dynamic import if node is ImportCall', () => {
      const transformerFactory = secharmony.transformerPlugin.createTransformerFactory(options);
      clearCaches();
      const sourceCode = `let module = import('./test');`;
      let sourcefile = ts.createSourceFile("getNode.ts", sourceCode, ts.ScriptTarget.ES2022,false);
      const transformer = transformerFactory(context);
      const node = transformer(sourcefile);
      expect(node).to.equal(sourcefile);
    });

    it('should update call expression if renamefilename', () => {
      options.mRenameFileName.mOhmUrlStatus = OhmUrlStatus.NORMALIZED;
      const transformerFactory = secharmony.transformerPlugin.createTransformerFactory(options);
      clearCaches();
      const sourceCode = `let module = import('@normalized:N&&&entry/src/main/ets/pages/test&');`;
      let sourcefile = ts.createSourceFile("getNode.ts", sourceCode, ts.ScriptTarget.ES2022,false);
      const transformer = transformerFactory(context);
      const node = transformer(sourcefile);
      expect(node).to.not.equal(sourcefile);
    });
  });

  describe('Tester Cases for <handleNormalizedOhmUrl>.', function () {
    beforeEach(() => {
      options = {
        mRenameFileName: {
          mEnable: true,
          mNameGeneratorType: 1,
          mReservedFileNames: ["D:", "workplace", "src", "ets"],
          mUniversalReservedFileNames: [],
          mOhmUrlStatus: OhmUrlStatus.NONE,
        }
      };
    });

    it('should return pakName if needPkgName is true', () => {
      secharmony.transformerPlugin.createTransformerFactory(options)(context);
      let ohmUrl1 = '@normalized:N&&&entry/src/main/ets/pages/test&';
      let ohmUrl2 = '@normalized:N&&&library/Index&1.0.0';
      let ohmUrl3 = '@normalized:N&&&@abc/a/src/main/ets/pages/test&';

      let pkgname1 = handleNormalizedOhmUrl(ohmUrl1, true);
      let pkgname2 = handleNormalizedOhmUrl(ohmUrl2, true);
      let pkgname3 = handleNormalizedOhmUrl(ohmUrl3, true);

      assert.strictEqual(pkgname1, 'entry');
      assert.strictEqual(pkgname2, 'library');
      assert.strictEqual(pkgname3, '@abc/a');
    });

    it('should correctly normalize OhmUrl', () => {
      secharmony.transformerPlugin.createTransformerFactory(options)(context);
      clearCaches();
      let ohmUrl1 = '@normalized:N&&&entry/src/main/ets/pages/test&';
      let ohmUrl2 = '@normalized:N&&&library/Index&1.0.0';
      let ohmUrl3 = '@normalized:N&&&@abc/a/src/main/ets/pages/test&';

      let mangledOhmurl1 = handleNormalizedOhmUrl(ohmUrl1);
      let mangledOhmurl2 = handleNormalizedOhmUrl(ohmUrl2);
      let mangledOhmurl3 = handleNormalizedOhmUrl(ohmUrl3);

      assert.strictEqual(mangledOhmurl1, '@normalized:N&&&entry/src/a/ets/b/c&');
      assert.strictEqual(mangledOhmurl2, '@normalized:N&&&library/d&1.0.0');
      assert.strictEqual(mangledOhmurl3, '@normalized:N&&&@abc/a/src/a/ets/b/c&');
    });
  });

  describe('Tester Cases for <clearCaches>.', function () {
    it('should clear globalFileNameMangledTable', function () {
      globalFileNameMangledTable.set('key1', 'value1');
      clearCaches();
      expect(globalFileNameMangledTable.size).to.equal(0);
    });

    it('should clear historyFileNameMangledTable if it exists', function () {
      renameFileNameModule.historyFileNameMangledTable = new Map<string, string>();
      renameFileNameModule.historyFileNameMangledTable.set('key1', 'value1');
      clearCaches();
      expect(renameFileNameModule.historyFileNameMangledTable.size).to.equal(0);
    });

    it('should not throw an error if historyFileNameMangledTable is undefined', () => {
      renameFileNameModule.historyFileNameMangledTable = undefined;
      expect(() => clearCaches()).not.to.throw();
    });
  });

  describe('Tester Cases for <isInOhModules>.', function () {

    let proInfo: ProjectInfo;

    beforeEach(() => {
      proInfo = {
        projectRootPath: '/Users/test/project',
        packageDir: 'src/ohmodules',
        localPackageSet: new Set<string>(),
        useNormalized: true,
        useTsHar: true,
      };
    });

    it('should return true when originalPath is in the ohPackagePath', () => {
      const originalPath = '/Users/test/project/src/ohmodules/somefile.js';
      const result = isInOhModules(proInfo, originalPath);
      expect(result).to.be.true;
    });

    it('should return false when originalPath is not in the ohPackagePath', () => {
      const originalPath = '/Users/test/project/src/othermodules/somefile.js';
      const result = isInOhModules(proInfo, originalPath);
      expect(result).to.be.false;
    });
  });

  describe('Tester Cases for <updateImportOrExportDeclaration>.', function () {

    beforeEach(() => {
      options = {
        mRenameFileName: {
          mEnable: true,
          mNameGeneratorType: 1,
          mReservedFileNames: ['file1.ts', 'file2.ts'],
          mUniversalReservedFileNames: [],
          mReservedProperties: ['1'],
          mReservedToplevelNames: ['2'],
          mOhmUrlStatus: OhmUrlStatus.NONE,
        }
      };
    });

    it('should return the same node if moduleSpecifier is missing', () => {
      node = factory.createExportDeclaration(undefined, true, undefined);
      secharmony.transformerPlugin.createTransformerFactory(options);
      const result = renameFileNameModule.updateImportOrExportDeclarationForTest(node);
      assert.deepEqual(result, node);
    });

    it('should update the moduleSpecifier of an ImportDeclaration', () => {
      node = factory.createImportDeclaration(undefined, undefined, factory.createStringLiteral('ModuleInfo'), undefined);
      secharmony.transformerPlugin.createTransformerFactory(options);
      const result = renameFileNameModule.updateImportOrExportDeclarationForTest(node);
      assert.deepEqual(result, node);
    });

    it('should update the moduleSpecifier of an ExportDeclaration', () => {
      node = factory.createExportDeclaration(undefined, true, undefined, factory.createStringLiteral('ModuleInfo'), undefined);
      secharmony.transformerPlugin.createTransformerFactory(options);
      const result = renameFileNameModule.updateImportOrExportDeclarationForTest(node);
      assert.deepEqual(result, node);
    });
  });

  describe('Tester Cases for <isLocalDependencyOhmUrl>.', function () {
    let projectInfo;

    beforeEach(() => {
      options = {
        mRenameFileName: {
          mEnable: true,
          mNameGeneratorType: 1,
          mReservedFileNames: ['file1.ts', 'file2.ts'],
          mUniversalReservedFileNames: [],
          mReservedProperties: ['1'],
          mReservedToplevelNames: ['2'],
          mOhmUrlStatus: OhmUrlStatus.NONE,
        }
      };
      projectInfo = {
        packageDir: 'path',
        projectRootPath: 'rootpath',
        localPackageSet: new Set<string>(),
        useNormalized: false,
        useTsHar: false,
      };
    });

    it('should return true if mOhmUrlStatus is AT_BUNDLE or NORMALIZED', () => {
      options.mRenameFileName.mOhmUrlStatus = OhmUrlStatus.AT_BUNDLE;
      secharmony.transformerPlugin.createTransformerFactory(options);
      expect(isLocalDependencyOhmUrlForTest('path/to/file')).to.be.true;
      options.mRenameFileName.mOhmUrlStatus = OhmUrlStatus.NORMALIZED;
      secharmony.transformerPlugin.createTransformerFactory(options);
      expect(isLocalDependencyOhmUrlForTest('path/to/file')).to.be.true;
    });

    it('should return false if filePath does not start with NORMALIZE and useNormalized is true', () => {
      projectInfo.useNormalized = true;
      ArkObfuscator.projectInfo = projectInfo;
      secharmony.transformerPlugin.createTransformerFactory(options)(context);
      expect(isLocalDependencyOhmUrlForTest('path/to/file')).to.be.false;
    });

    it('should return true if packageName is in localPackageSet when use normalized', () => {
      projectInfo.useNormalized = true;
      projectInfo.localPackageSet.add('library');
      ArkObfuscator.projectInfo = projectInfo;
      secharmony.transformerPlugin.createTransformerFactory(options)(context);
      expect(isLocalDependencyOhmUrlForTest('@normalized:N&&&library/Index&1.0.0')).to.be.true;
    });

    it('should return false if filePath does not start with BUNDLE and useNormalized is false', () => {
      projectInfo.useNormalized = false;
      ArkObfuscator.projectInfo = projectInfo;
      secharmony.transformerPlugin.createTransformerFactory(options)(context);
      expect(isLocalDependencyOhmUrlForTest('path/to/file')).to.be.false;
    });

    it('should return true if packageName is in localPackageSet when not use normalized', () => {
      projectInfo.useNormalized = false;
      projectInfo.localPackageSet.add('testName');
      ArkObfuscator.projectInfo = projectInfo;
      secharmony.transformerPlugin.createTransformerFactory(options)(context);
      expect(isLocalDependencyOhmUrlForTest('@bundle:bundleName/testName/')).to.be.true;
      expect(isLocalDependencyOhmUrlForTest('@bundle:bundleName/moduleName@testName/')).to.be.true;
    });

    it('should return false if packageName is not in localPackageSet', () => {
      projectInfo.useNormalized = false;
      ArkObfuscator.projectInfo = projectInfo;
      secharmony.transformerPlugin.createTransformerFactory(options)(context);
      expect(isLocalDependencyOhmUrlForTest('@bundle:bundleName/testName/')).to.be.false;
      expect(isLocalDependencyOhmUrlForTest('@bundle:bundleName/moduleName@testName/')).to.be.false;
    });
  });

  describe('Tester Cases for <getMangleCompletePath>.', function () {
    beforeEach(() => {
      options = {
        mRenameFileName: {
          mEnable: true,
          mNameGeneratorType: 1,
          mReservedFileNames: ["D:", "workplace", "src", "ets"],
          mUniversalReservedFileNames: [],
        }
      };
    });

    it('should return a mangled file path with the same extension', () => {
      secharmony.transformerPlugin.createTransformerFactory(options)(context);
      const originalPath = 'D:/workplace/src/ets/entryability/EntryAbility.ts'
      const mangledPath = getMangleCompletePath(originalPath);
      assert.strictEqual(mangledPath === 'D:/workplace/src/ets/a/b.ts', true);
    });

    it('should reserve file names in mReservedFileNames', () => {
      options.mRenameFileName.mReservedFileNames = ["D:", "workplace", "src", "ets", "entryability", "EntryAbility"];
      secharmony.transformerPlugin.createTransformerFactory(options)(context);
      const originalPath = 'D:/workplace/src/ets/entryability/EntryAbility.ts';
      const mangledPath = getMangleCompletePath(originalPath);
      assert.strictEqual(mangledPath === 'D:/workplace/src/ets/entryability/EntryAbility.ts', true);
    });

    it('should return history name when in historyFileNameMangledTable', () => {
      secharmony.transformerPlugin.createTransformerFactory(options)(context);
      renameFileNameModule.historyFileNameMangledTable = new Map<string, string>();
      renameFileNameModule.historyFileNameMangledTable.set('entryability', 'test1');
      const originalPath = 'D:/workplace/src/ets/entryability/EntryAbility.ts';
      const mangledPath = getMangleCompletePath(originalPath);
      assert.strictEqual(mangledPath === 'D:/workplace/src/ets/test1/b.ts', true);
    });

    it('should reuse name when in globalFileNameMangledTable', () => {
      secharmony.transformerPlugin.createTransformerFactory(options)(context);
      clearCaches();
      const originalPath = 'D:/workplace/src/ets/entryability1/EntryAbility.ts';
      const mangledPath = getMangleCompletePath(originalPath);
      assert.strictEqual(mangledPath === 'D:/workplace/src/ets/a/b.ts', true);
      const originalPath2 = 'D:/workplace/src/ets/entryability2/EntryAbility.ts';
      const mangledPath2 = getMangleCompletePath(originalPath2);
      assert.strictEqual(mangledPath2 === 'D:/workplace/src/ets/c/b.ts', true);
    });

    it('should return next available name when the generated name same as original name', () => {
      secharmony.transformerPlugin.createTransformerFactory(options)(context);
      clearCaches();
      const originalPath = 'D:/workplace/src/ets/a.ts';
      const mangledPath = getMangleCompletePath(originalPath);
      assert.strictEqual(mangledPath === 'D:/workplace/src/ets/b.ts', true);
    });

    it('should return next available name when the generated name need to be reserved', () => {
      options.mRenameFileName.mReservedFileNames = ["a", "D:", "workplace", "src", "ets"];
      secharmony.transformerPlugin.createTransformerFactory(options)(context);
      clearCaches();
      const originalPath = 'D:/workplace/src/ets/entryability/EntryAbility.ts';
      const mangledPath = getMangleCompletePath(originalPath);
      assert.strictEqual(mangledPath === 'D:/workplace/src/ets/b/c.ts', true);
    });

    it('should return next available name when the generated name exist in globalFileNameMangledTable', () => {
      secharmony.transformerPlugin.createTransformerFactory(options)(context);
      clearCaches();
      renameFileNameModule.globalFileNameMangledTable.set('testability', 'a');
      renameFileNameModule.globalFileNameMangledTable.set('Testability', 'b');
      const originalPath = 'D:/workplace/src/ets/entryability/EntryAbility.ts';
      const mangledPath = getMangleCompletePath(originalPath);
      assert.strictEqual(mangledPath === 'D:/workplace/src/ets/c/d.ts', true);
    })

    it('should return next available name when the generated name exist in historyFileNameMangledTable', () => {
      secharmony.transformerPlugin.createTransformerFactory(options)(context);
      clearCaches();
      renameFileNameModule.historyFileNameMangledTable = new Map<string, string>();
      renameFileNameModule.historyFileNameMangledTable.set('testability', 'a');
      renameFileNameModule.historyFileNameMangledTable.set('Testability', 'b');
      const originalPath = 'D:/workplace/src/ets/entryability/EntryAbility.ts';
      const mangledPath = getMangleCompletePath(originalPath);
      assert.strictEqual(mangledPath === 'D:/workplace/src/ets/c/d.ts', true);
    })
  });

  describe('Tester Cases for <mangleOhmUrl>.', function () {
    let projectInfo;

    beforeEach(() => {
      options = {
        mRenameFileName: {
          mEnable: true,
          mNameGeneratorType: 1,
          mReservedFileNames: ["D:", "workplace", "src", "ets"],
          mUniversalReservedFileNames: [],
          mOhmUrlStatus: OhmUrlStatus.NONE,
        }
      };
      projectInfo = {
        packageDir: 'path',
        projectRootPath: 'rootpath',
        localPackageSet: new Set<string>(),
        useNormalized: false,
        useTsHar: false,
      };
    });

    it('should normalize ohmUrl if useNormalized is true', () => {
      projectInfo.useNormalized = true;
      ArkObfuscator.projectInfo = projectInfo;
      secharmony.transformerPlugin.createTransformerFactory(options)(context);
      clearCaches();
      let ohmUrl1 = '@normalized:N&&&entry/src/main/ets/pages/test&';
      let ohmUrl2 = '@normalized:N&&&library/Index&1.0.0';
      let ohmUrl3 = '@normalized:N&&&@abc/a/src/main/ets/pages/test&';

      let mangledOhmurl1 = mangleOhmUrl(ohmUrl1);
      let mangledOhmurl2 = mangleOhmUrl(ohmUrl2);
      let mangledOhmurl3 = mangleOhmUrl(ohmUrl3);

      assert.strictEqual(mangledOhmurl1, '@normalized:N&&&entry/src/a/ets/b/c&');
      assert.strictEqual(mangledOhmurl2, '@normalized:N&&&library/d&1.0.0');
      assert.strictEqual(mangledOhmurl3, '@normalized:N&&&@abc/a/src/a/ets/b/c&');
    });

    it('should normalize ohmUrl if mOhmUrlStatus is OhmUrlStatus.NORMALIZED', () => {
      options.mRenameFileName.mOhmUrlStatus = OhmUrlStatus.NORMALIZED;
      secharmony.transformerPlugin.createTransformerFactory(options)(context);
      clearCaches();
      let ohmUrl1 = '@normalized:N&&&entry/src/main/ets/pages/test&';
      let ohmUrl2 = '@normalized:N&&&library/Index&1.0.0';
      let ohmUrl3 = '@normalized:N&&&@abc/a/src/main/ets/pages/test&';

      let mangledOhmurl1 = mangleOhmUrl(ohmUrl1);
      let mangledOhmurl2 = mangleOhmUrl(ohmUrl2);
      let mangledOhmurl3 = mangleOhmUrl(ohmUrl3);

      assert.strictEqual(mangledOhmurl1, '@normalized:N&&&entry/src/a/ets/b/c&');
      assert.strictEqual(mangledOhmurl2, '@normalized:N&&&library/d&1.0.0');
      assert.strictEqual(mangledOhmurl3, '@normalized:N&&&@abc/a/src/a/ets/b/c&');
    });

    it('should return mangled Ohm URL with fixed parts and mangled parts', () => {
      projectInfo.useNormalized = false;
      ArkObfuscator.projectInfo = projectInfo;
      options.mRenameFileName.mOhmUrlStatus = OhmUrlStatus.NONE;
      secharmony.transformerPlugin.createTransformerFactory(options)(context);
      clearCaches();
      let ohmUrl1 = '@normalized:N&&&entry/src/main/ets/pages/test&';
      let ohmUrl2 = '@normalized:N&&&library/Index&1.0.0';
      let ohmUrl3 = '@normalized:N&&&@abc/a/src/main/ets/pages/test&';

      let mangledOhmurl1 = mangleOhmUrl(ohmUrl1);
      let mangledOhmurl2 = mangleOhmUrl(ohmUrl2);
      let mangledOhmurl3 = mangleOhmUrl(ohmUrl3);

      assert.strictEqual(mangledOhmurl1, '@normalized:N&&&entry/src/a/ets/b/c');
      assert.strictEqual(mangledOhmurl2, '@normalized:N&&&library/Index&1.0.0/');
      assert.strictEqual(mangledOhmurl3, '@normalized:N&&&@abc/a/src/a/ets/b/c');
    });
  });

  describe('Tester Cases for <getMangleIncompletePath>.', function () {
    let projectInfo;

    beforeEach(() => {
      options = {
        mRenameFileName: {
          mEnable: true,
          mNameGeneratorType: 1,
          mReservedFileNames: ["D:", "workplace", "src", "ets"],
          mUniversalReservedFileNames: [],
          mOhmUrlStatus: OhmUrlStatus.NONE,
        }
      };
      projectInfo = {
        packageDir: 'path',
        projectRootPath: 'rootpath',
        localPackageSet: new Set<string>(),
        useNormalized: false,
        useTsHar: false,
      };
    });

    it('should return mangled Ohm URL for local dependencies', () => {
      projectInfo.localPackageSet.add('entry');
      ArkObfuscator.projectInfo = projectInfo;
      secharmony.transformerPlugin.createTransformerFactory(options)(context);
      clearCaches();
      let ohmUrl = '@bundle:com.example.myapplication/entry/index';
      let mangledOhmurl = getMangleIncompletePathForTest(ohmUrl);
      assert.strictEqual(mangledOhmurl, '@bundle:com.example.myapplication/entry/a');
    });

    it('should return the original path if it is not a valid file path and not a local dependency', () => {
      secharmony.transformerPlugin.createTransformerFactory(options)(context);
      clearCaches();
      let path = '/workplace/src/ets/entryability/EntryAbility';
      let mangledPath = getMangleIncompletePathForTest(path);
      assert.strictEqual(mangledPath, '/workplace/src/ets/entryability/EntryAbility');
    });

    it('should return mangled directory name without extension', () => {
      secharmony.transformerPlugin.createTransformerFactory(options)(context);
      obfuscator.orignalFilePathForSearching = '';
      clearCaches();
      let path = 'test/ut/transformer/demo.ts';
      fs.writeFileSync(path, 'test');
      let path1 = 'test/ut/transformer/demo';
      let mangledPath = getMangleIncompletePathForTest(path1);
      assert.strictEqual(mangledPath, 'a/b/c/d');
      fs.unlinkSync(path);
    });

    it('should return mangled file path with extension for valid file paths', () => {
      secharmony.transformerPlugin.createTransformerFactory(options)(context);
      obfuscator.orignalFilePathForSearching = '';
      clearCaches();
      let path = 'test/ut/transformer/demo.ts';
      fs.writeFileSync(path, 'test');
      let path1 = 'test/ut/transformer/demo.ts';
      let mangledPath = getMangleIncompletePathForTest(path1);
      assert.strictEqual(mangledPath, 'a/b/c/d.ts');
      fs.unlinkSync(path);
    });
  });

  describe('Tester Cases for <addReservedFileNames>.', function () {
    let projectInfo;

    beforeEach(() => {
      options = {
        mRenameFileName: {
          mEnable: true,
          mNameGeneratorType: 1,
          mReservedFileNames: ["D:", "workplace", "src", "ets"],
          mUniversalReservedFileNames: [],
          mOhmUrlStatus: OhmUrlStatus.NONE,
        }
      };
      projectInfo = {
        packageDir: 'path',
        projectRootPath: 'rootpath',
        localPackageSet: new Set<string>(),
        useNormalized: false,
        useTsHar: false,
      };
    });

    it('should have added file names', () => {
      let filePath: string[] = [
        'D:\\OH\\project\\Application\\har1',
        'D:\\OH\\project\\Application\\har2'
      ];
      addReservedFileNames(filePath);
      assert.strictEqual(renameFileNameModule.reservedFileNames.has('har1'), true);
      assert.strictEqual(renameFileNameModule.reservedFileNames.has('har2'), true);
    });

    it('should have added and whitelisted file names', () => {
      secharmony.transformerPlugin.createTransformerFactory(options)(context);
      let filePath: string[] = [
        'D:\\OH\\project\\Application\\har1',
        'D:\\OH\\project\\Application\\har2'
      ];
      addReservedFileNames(filePath);
      assert.strictEqual(renameFileNameModule.reservedFileNames.has("workplace"), true);
      assert.strictEqual(renameFileNameModule.reservedFileNames.has("ets"), true);
      assert.strictEqual(renameFileNameModule.reservedFileNames.has("har1"), true);
      assert.strictEqual(renameFileNameModule.reservedFileNames.has("har2"), true);
    });
  });
});

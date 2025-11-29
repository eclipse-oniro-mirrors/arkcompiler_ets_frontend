/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
import { expect } from 'chai';
import {
  ArkObfuscator,
  clearGlobalCaches,
  collectReservedNameForObf,
  collectResevedFileNameInIDEConfig,
  deleteLineInfoForNameString,
  enableObfuscatedFilePathConfig,
  enableObfuscateFileName,
  generateConsumerObConfigFile,
  getRelativeSourcePath,
  handleObfuscatedFilePath,
  handleUniversalPathInObf,
  initObfuscationConfig,
  mangleFilePath,
  MemoryUtils,
  MergedConfig,
  nameCacheMap,
  ObConfigResolver,
  ObfuscationResultType,
  orignalFilePathForSearching,
  PropCollections,
  readNameCache,
  readProjectPropertiesByCollectedPaths,
  renameFileNameModule,
  ReseverdSetForArkguard,
  TimeSumPrinter,
  UnobfuscationCollections,
  writeObfuscationNameCache,
  writeUnobfuscationContent,
  unobfuscationNamesObj,
  FileUtils,
} from '../../../src/ArkObfuscator'; 

import {
  createSourceFile,
  createObfTextSingleLineWriter,
  RawSourceMap,
  SourceFile,
  ScriptTarget,
} from 'typescript';

import { IOptions } from '../../../src/configs/IOptions';
import { getSourceMapGenerator } from '../../../src/utils/SourceMapUtil';
import { 
  globalFileNameMangledTable,
  historyFileNameMangledTable,
} from '../../../src/transformers/rename/RenameFileNameTransformer';
import { LocalVariableCollections } from '../../../src/utils/CommonCollections';
import { SOURCE_FILE_PATHS, projectWhiteListManager } from '../../../src/utils/ProjectCollections';
import { FilePathObj } from '../../../src/common/type';
import { historyAllUnobfuscatedNamesMap } from '../../../src/initialization/Initializer';
import path from 'path';
import { nameCache } from '../../../src/transformers/rename/RenameIdentifierTransformer';
import { 
  decodeSourcemap,
  MappingsNameTypeForTest,
  Source,
  SourceMapLink
} from '../../../src/utils/SourceMapMergingUtil';

describe('Tester Cases for <ArkObfuscator>', function () {
  let obfuscator: ArkObfuscator;
  let defaultConfig: IOptions;

  const sourceFilePathObj: FilePathObj = {
    buildFilePath: 'demo.ts',
    relativeFilePath: 'demo',
  };
  let sourceFile: SourceFile;
  let sourceFileContent: string = `class Person {
  constructor(public name: string, public age: number) {
      this.name = name;
      this.age = age;
  }
}`;

  const jsSourceFilePathObj: FilePathObj = {
    buildFilePath: 'demo.js',
    relativeFilePath: '',
  };
  let jsSourceFile: SourceFile;
  let jsSourceFileContent: string = `//This is a comment
//This is a comment
function subtract(a, b) {
    return a - b;
}`;

  const declSourceFilePathObj: FilePathObj = {
    buildFilePath: 'demo.d.ts',
    relativeFilePath: '',
  };
  let declSourceFile: SourceFile;
  let declSourceFileContent: string = `//This is a comment
//This is a comment
export declare function add(num1: number, num2: number): number;
export declare function findElement<T>(arr: T[], callback: (item: T) => boolean): T | undefined;`;

  beforeEach(() => {
    obfuscator = new ArkObfuscator();

    // Clear the collection to ensure test isolation
    PropCollections.clearPropsCollections();
    UnobfuscationCollections.clear();
    LocalVariableCollections.clear();
  
    defaultConfig = {
      mRemoveComments: true,
      mNameObfuscation: {
        mEnable: true,
        mNameGeneratorType: 1,
        mReservedNames: [],
        mRenameProperties: true,
        mReservedProperties: [],
        mTopLevel: true,
        mReservedToplevelNames: [],
      },
      mEnableSourceMap: true,
      mEnableNameCache: true,
    };

    sourceFile = createSourceFile(sourceFilePathObj.buildFilePath, sourceFileContent, ScriptTarget.ES2015, true);
    jsSourceFile = createSourceFile(jsSourceFilePathObj.buildFilePath, jsSourceFileContent, ScriptTarget.ES2015, true);
    declSourceFile = createSourceFile(declSourceFilePathObj.buildFilePath, declSourceFileContent, ScriptTarget.ES2015, true);
  });

  describe('test for ArkObfuscator.init', () => {
    it('should return false if config is undefined', () => {
      const result = obfuscator.init(undefined);
      expect(result).to.be.false;
    });

    it('should return true if config is valid', () => {
      const result = obfuscator.init(defaultConfig);
      expect(result).to.be.true;
    });

    it('source code should be compacted into one line', () => {
      const config: IOptions = {
        mCompact: true,
        mEnableSourceMap: true,
        mRemoveComments: true
      };
      obfuscator.init(config);

      let sourceMapGenerator = getSourceMapGenerator(jsSourceFilePathObj.buildFilePath);
      const textWriter = createObfTextSingleLineWriter();
      obfuscator.createObfsPrinter(jsSourceFile.isDeclarationFile).writeFile(jsSourceFile, textWriter, sourceMapGenerator);
      const actualContent = textWriter.getText();
      const expectContent = `function subtract(a, b) {return a - b;}`;
      expect(actualContent === expectContent).to.be.true;
    });

    it('should not init incremental cache when cachePath is not passed', () => {
      const config: IOptions = {
        mNameObfuscation: {
          mEnable: true,
          mRenameProperties: true,
          mReservedProperties: [],
        },
        mCompact: true,
        mEnableSourceMap: true,
        mRemoveComments: true
      };
      obfuscator.init(config);
      expect(obfuscator.fileContentManager).to.be.undefined;
      expect(obfuscator.filePathManager).to.be.undefined;
      expect(projectWhiteListManager).to.be.undefined;
    });

    it('should init incremental cache when cachePath is passed', () => {
      const config: IOptions = {
        mNameObfuscation: {
          mEnable: true,
          mRenameProperties: true,
          mReservedProperties: [],
        },
        mCompact: true,
        mEnableSourceMap: true,
        mRemoveComments: true
      };
      const cachePath = 'test/ut/utils/obfuscation';
      obfuscator.init(config, cachePath);
      expect(obfuscator.fileContentManager).to.not.be.undefined;
      expect(obfuscator.filePathManager).to.not.be.undefined;
      expect(projectWhiteListManager).to.not.be.undefined;
    });

    it('should set is incremental flag if is incremental', () => {
      const config: IOptions = {
        mNameObfuscation: {
          mEnable: true,
          mRenameProperties: true,
          mReservedProperties: [],
        },
        mCompact: true,
        mEnableSourceMap: true,
        mRemoveComments: true
      };
      const cachePath = 'test/ut/utils/obfuscation';
      const filePathsCache = path.join(cachePath, SOURCE_FILE_PATHS);
      let content = 'hello';
      FileUtils.writeFile(filePathsCache, content);
      obfuscator.init(config, cachePath);
      expect(obfuscator.isIncremental).to.be.true;
      FileUtils.deleteFile(filePathsCache);
    });
  });

  describe('test for ArkObfuscator.obfuscate', () => {
    it('should return empty result for ignored files', async () => {
      const sourceFilePathObj: FilePathObj = {
        buildFilePath: 'ignoredFile.cpp',
        relativeFilePath: '',
      };
      const result: ObfuscationResultType = await obfuscator.obfuscate(
        'hello world',
        sourceFilePathObj,
      );
      expect(result).to.deep.equal({ content: undefined });
    });

    it('should return empty result for empty AST', async () => {
      const sourceFilePathObj: FilePathObj = {
        buildFilePath: 'emptyFile.js',
        relativeFilePath: '',
      };
      const result: ObfuscationResultType = await obfuscator.obfuscate('', sourceFilePathObj);
      expect(result).to.deep.equal({ content: undefined });
    });

    it('should be correctly obfuscated for valid AST', async () => {
      obfuscator.init(defaultConfig);
      const result: ObfuscationResultType = await obfuscator.obfuscate(
        sourceFile,
        sourceFilePathObj,
      );
      const expectResult = `class g {
    constructor(public name: string, public h: number) {
        this.name = name;
        this.h = h;
    }
}
`;
      expect(result.content === expectResult).to.be.true;
    });

    it('comments in declaration file should be removed when enable -remove-comments', async () => {
      const config: IOptions = {
        mRemoveDeclarationComments: {
          mEnable: true,
          mReservedComments: [],
          mUniversalReservedComments: [],
        },
      };

      obfuscator.init(config);
      const result: ObfuscationResultType = await obfuscator.obfuscate(declSourceFile, declSourceFilePathObj);
      const expectResult = `export declare function add(num1: number, num2: number): number;
export declare function findElement<T>(arr: T[], callback: (item: T) => boolean): T | undefined;
`;
      expect(result.content === expectResult).to.be.true;
    });

    it('comments in declaration file should not be removed when enable -remove-comments', async () => {
      const config: IOptions = {
        mRemoveDeclarationComments: {
          mEnable: false,
          mReservedComments: [],
          mUniversalReservedComments: [],
        },
      };
      obfuscator.init(config);
      const result: ObfuscationResultType = await obfuscator.obfuscate(declSourceFile, declSourceFilePathObj);
      const expectResult = `//This is a comment
//This is a comment
export declare function add(num1: number, num2: number): number;
export declare function findElement<T>(arr: T[], callback: (item: T) => boolean): T | undefined;
`;
      expect(result.content === expectResult).to.be.true;
    });

    it('unobfuscationNameMap should be Map type when enable -print-kept-names', async () => {
      const config = {
        mUnobfuscationOption: {
          mPrintKeptNames: true,
          mPrintPath: 'local.json',
        },
      };
      historyAllUnobfuscatedNamesMap.set('demo', {'key': ['value']});
      obfuscator.init(config);
      const result: ObfuscationResultType = await obfuscator.obfuscate(
        sourceFile,
        sourceFilePathObj,
      );
      expect(result.unobfuscationNameMap instanceof Map).to.be.true;
    });

    it('unobfuscationNameMap should be undefined when disable -print-kept-names, ', async () => {
      obfuscator.init(defaultConfig);
      const result: ObfuscationResultType = await obfuscator.obfuscate(
        sourceFile,
        sourceFilePathObj,
      );
      expect(result.unobfuscationNameMap === undefined).to.be.true;
    });

    it('historyNameCache should be used when provide historyNameCache', async () => {
      obfuscator.init(defaultConfig);
      const historyNameCache = new Map([['#Person', 'm'], ['Person:2:5', 'm']]);
      const result: ObfuscationResultType = await obfuscator.obfuscate(
        sourceFile,
        sourceFilePathObj,
        undefined,
        historyNameCache,
      );
      const expectResult = `
        class m {
          constructor(public name: string, public g: number) {
            this.name = name;
            this.g = g;
          }
        }
      `;
      expect(compareStringsIgnoreNewlines(result.content, expectResult)).to.be.true;
    });

    it('fileName should be obfuscated when enable -enable-filename-obfuscation', async () => {
      const config: IOptions = {
        mRenameFileName: {
          mEnable: true,
          mNameGeneratorType: 1,
          mReservedFileNames: [],
        },
      };
      obfuscator.init(config);
      const result: ObfuscationResultType = await obfuscator.obfuscate(
        sourceFile,
        sourceFilePathObj,
      );

      expect(orignalFilePathForSearching === 'demo.ts').to.be.true;
      expect(result.filePath === 'a.ts').to.be.true;
    });

    it('PropCollections shoule be cleared when only enable toplevel option', async () => {
      PropCollections.globalMangledTable.set('test', 'obfuscated');
      const config: IOptions = {
        mNameObfuscation: {
          mEnable: true,
          mNameGeneratorType: 1,
          mRenameProperties: false,
          mReservedProperties: [],
          mTopLevel: true,
        },
        mExportObfuscation: false,
      };
      obfuscator.init(config);
      await obfuscator.obfuscate(sourceFile, sourceFilePathObj);
      expect(PropCollections.globalMangledTable.size).to.equal(0);
    });

    it('PropCollections shoule not be cleared when enable toplevel、property and export option', async () => {
      PropCollections.globalMangledTable.set('test', 'obfuscated');
      const config: IOptions = {
        mNameObfuscation: {
          mEnable: true,
          mNameGeneratorType: 1,
          mRenameProperties: true,
          mReservedProperties: [],
          mTopLevel: true,
        },
        mExportObfuscation: true,
      };
      obfuscator.init(config);
      await obfuscator.obfuscate(sourceFile, sourceFilePathObj);
      expect(PropCollections.globalMangledTable.get('test') === 'obfuscated').to.be.true;
    });

    it('test for use sourcemap mapping', async () => {
      obfuscator.init(defaultConfig);
      const previousStageSourceMap = {
        "version":3,
        "file":"demo.js",
        "sourceRoot":"",
        "sources":["demo.js"],
        "names":[],
        "mappings":"AAAA,mBAAmB;AACnB,mBAAmB;AACnB,SAAS,QAAQ,CAAC,CAAC,EAAE,CAAC;IAClB,OAAO,CAAC,GAAG,CAAC,CAAC;AACjB,CAAC"
      } as RawSourceMap;
      const result: ObfuscationResultType = await obfuscator.obfuscate(
        jsSourceFile,
        jsSourceFilePathObj,
        previousStageSourceMap,
      );
      const actualSourceMap = JSON.stringify(result.sourceMap);
      const expectSourceMap = `{"version":3,"file":"demo.js","sources":["demo.js"],"names":[],
        "mappings":"AAEA,WAAkB,CAAC,EAAE,CAAC;IAClB,OAAO,KAAK,CAAC;AACjB,CAAC","sourceRoot":""}`;
      expect(compareStringsIgnoreNewlines(actualSourceMap, expectSourceMap)).to.be.true;
    });

    it('test for not use sourcemap mapping', async () => {
      obfuscator.init(defaultConfig);
      const result: ObfuscationResultType = await obfuscator.obfuscate(
        jsSourceFile,
        jsSourceFilePathObj,
      );
      const actualSourceMap = JSON.stringify(result.sourceMap);
      const expectSourceMap = `{"version":3,"file":"demo.js","sourceRoot":"","sources":["demo.js"],
        "names":[],"mappings":"AAEA,WAAkB,CAAC,EAAE,CAAC;IAClB,OAAO,KAAK,CAAC;AACjB,CAAC"}`;
      expect(compareStringsIgnoreNewlines(actualSourceMap, expectSourceMap)).to.be.true;
    });
  });

  describe('test for ArkObfuscator.setWriteOriginalFile', () => {
    it('should set writeOriginalFile to true', () => {
      obfuscator.setWriteOriginalFile(true);
      expect(obfuscator.getWriteOriginalFileForTest()).to.be.true;
    });

    it('should set writeOriginalFile to false', () => {
      obfuscator.setWriteOriginalFile(false);
      expect(obfuscator.getWriteOriginalFileForTest()).to.be.false;
    });
  });

  describe('test for ArkObfuscator.addReservedSetForPropertyObf', () => {
    let obfuscator: ArkObfuscator;

    beforeEach(() => {
      // Reset the status of UnobfuscationCollections
      UnobfuscationCollections.reservedStruct.clear();
      UnobfuscationCollections.reservedStrProp.clear();
      UnobfuscationCollections.reservedObjProp.clear();
      UnobfuscationCollections.reservedExportNameAndProp.clear();
      UnobfuscationCollections.reservedEnum.clear();
      
      obfuscator = new ArkObfuscator();
    });

    it('should add reserved sets correctly', () => {
      const properties: ReseverdSetForArkguard = {
        structPropertySet: new Set(['struct1']),
        stringPropertySet: new Set(['string1']),
        objectPropertySet: new Set(['object1']),
        exportNameAndPropSet: new Set(['export1']),
        exportNameSet: undefined,
        enumPropertySet: new Set(['enum1']),
      };

      obfuscator.addReservedSetForPropertyObf(properties);

      expect(UnobfuscationCollections.reservedStruct.has('struct1')).to.be.true;
      expect(UnobfuscationCollections.reservedStrProp.has('string1')).to.be.true;
      expect(UnobfuscationCollections.reservedObjProp.has('object1')).to.be.true;
      expect(UnobfuscationCollections.reservedExportNameAndProp.has('export1')).to.be.true;
      expect(UnobfuscationCollections.reservedEnum.has('enum1')).to.be.true;
    });

    it('should not add empty sets', () => {
      const properties: ReseverdSetForArkguard = {
        structPropertySet: new Set(),
        stringPropertySet: new Set(),
        objectPropertySet: new Set(),
        exportNameAndPropSet: new Set(),
        exportNameSet: undefined,
        enumPropertySet: new Set(),
      };

      obfuscator.addReservedSetForPropertyObf(properties);

      expect(UnobfuscationCollections.reservedStruct.size).to.equal(0);
      expect(UnobfuscationCollections.reservedStrProp.size).to.equal(0);
      expect(UnobfuscationCollections.reservedObjProp.size).to.equal(0);
      expect(UnobfuscationCollections.reservedExportNameAndProp.size).to.equal(0);
      expect(UnobfuscationCollections.reservedEnum.size).to.equal(0);
    });

    it('should add all non-empty reserved sets to corresponding collections', () => {
      const properties: ReseverdSetForArkguard = {
        structPropertySet: new Set(['struct1', 'struct2']),
        stringPropertySet: new Set(['string1', 'string2']),
        objectPropertySet: new Set(['object1', 'object2']),
        exportNameAndPropSet: new Set(['export1', 'export2']),
        exportNameSet: undefined,
        enumPropertySet: new Set(['enum1', 'enum2']),
      };

      obfuscator.addReservedSetForPropertyObf(properties);
  
      // Verify structPropertySet (using the add method)
      expect(UnobfuscationCollections.reservedStruct.has('struct1')).to.be.true;
      expect(UnobfuscationCollections.reservedStruct.has('struct2')).to.be.true;
      expect(UnobfuscationCollections.reservedStruct.size).to.equal(2);

      // Verify stringPropertySet (using assignment)
      expect(UnobfuscationCollections.reservedStrProp.has('string1')).to.be.true;
      expect(UnobfuscationCollections.reservedStrProp.has('string2')).to.be.true;
      expect(UnobfuscationCollections.reservedStrProp.size).to.equal(2);

      // Verify objectPropertySet (using assignment)
      expect(UnobfuscationCollections.reservedObjProp.has('object1')).to.be.true;
      expect(UnobfuscationCollections.reservedObjProp.has('object2')).to.be.true;
      expect(UnobfuscationCollections.reservedObjProp.size).to.equal(2);

      // Verify exportNameAndPropSet (using assignment)
      expect(UnobfuscationCollections.reservedExportNameAndProp.has('export1')).to.be.true;
      expect(UnobfuscationCollections.reservedExportNameAndProp.has('export2')).to.be.true;
      expect(UnobfuscationCollections.reservedExportNameAndProp.size).to.equal(2);

      // Verify enumPropertySet (using the add method)
      expect(UnobfuscationCollections.reservedEnum.has('enum1')).to.be.true;
      expect(UnobfuscationCollections.reservedEnum.has('enum2')).to.be.true;
      expect(UnobfuscationCollections.reservedEnum.size).to.equal(2);
    });

    it('should handle adding to existing collections correctly', () => {
      // Set initial value
      UnobfuscationCollections.reservedStruct.add('existingStruct');
      UnobfuscationCollections.reservedEnum.add('existingEnum');

      const properties: ReseverdSetForArkguard = {
        structPropertySet: new Set(['newStruct1', 'newStruct2']),
        stringPropertySet: new Set(['newString']),
        objectPropertySet: new Set(['newObject']),
        exportNameAndPropSet: new Set(['newExport']),
        exportNameSet: undefined,
        enumPropertySet: new Set(['newEnum1', 'newEnum2']),
      };

      obfuscator.addReservedSetForPropertyObf(properties);

      // Verify that structPropertySet is merged
      expect(UnobfuscationCollections.reservedStruct.has('existingStruct')).to.be.true;
      expect(UnobfuscationCollections.reservedStruct.has('newStruct1')).to.be.true;
      expect(UnobfuscationCollections.reservedStruct.has('newStruct2')).to.be.true;
      expect(UnobfuscationCollections.reservedStruct.size).to.equal(3);

      // Verify that enumPropertySet is merged
      expect(UnobfuscationCollections.reservedEnum.has('existingEnum')).to.be.true;
      expect(UnobfuscationCollections.reservedEnum.has('newEnum1')).to.be.true;
      expect(UnobfuscationCollections.reservedEnum.has('newEnum2')).to.be.true;
      expect(UnobfuscationCollections.reservedEnum.size).to.equal(3);

      // Verify that the other set is substitutable
      expect(UnobfuscationCollections.reservedStrProp.size).to.equal(1);
      expect(UnobfuscationCollections.reservedObjProp.size).to.equal(1);
      expect(UnobfuscationCollections.reservedExportNameAndProp.size).to.equal(1);
    });

    it('should not modify collections when all sets are empty', () => {
      const initialStructSize = UnobfuscationCollections.reservedStruct.size;
      const initialStrPropSize = UnobfuscationCollections.reservedStrProp.size;
      const initialObjPropSize = UnobfuscationCollections.reservedObjProp.size;
      const initialExportSize = UnobfuscationCollections.reservedExportNameAndProp.size;
      const initialEnumSize = UnobfuscationCollections.reservedEnum.size;

      const properties: ReseverdSetForArkguard = {
        structPropertySet: new Set(),
        stringPropertySet: new Set(),
        objectPropertySet: new Set(),
        exportNameAndPropSet: new Set(),
        exportNameSet: undefined,
        enumPropertySet: new Set(),
      };

      obfuscator.addReservedSetForPropertyObf(properties);
  
      expect(UnobfuscationCollections.reservedStruct.size).to.equal(initialStructSize);
      expect(UnobfuscationCollections.reservedStrProp.size).to.equal(initialStrPropSize);
      expect(UnobfuscationCollections.reservedObjProp.size).to.equal(initialObjPropSize);
      expect(UnobfuscationCollections.reservedExportNameAndProp.size).to.equal(initialExportSize);
      expect(UnobfuscationCollections.reservedEnum.size).to.equal(initialEnumSize);
    });

    it('should handle undefined sets correctly', () => {
      const properties: ReseverdSetForArkguard = {
        structPropertySet: undefined,
        stringPropertySet: undefined,
        objectPropertySet: undefined,
        exportNameAndPropSet: undefined,
        exportNameSet: undefined,
        enumPropertySet: undefined,
      };

      // This shouldn't throw an error.
      expect(() => {
        obfuscator.addReservedSetForPropertyObf(properties);
      }).not.to.throw();

      expect(UnobfuscationCollections.reservedStruct.size).to.equal(0);
      expect(UnobfuscationCollections.reservedStrProp.size).to.equal(0);
      expect(UnobfuscationCollections.reservedObjProp.size).to.equal(0);
      expect(UnobfuscationCollections.reservedExportNameAndProp.size).to.equal(0);
      expect(UnobfuscationCollections.reservedEnum.size).to.equal(0);
    });

    it('should handle mixed empty and non-empty sets', () => {
      const properties: ReseverdSetForArkguard = {
        structPropertySet: new Set(['struct1']),
        stringPropertySet: new Set(), // empty set
        objectPropertySet: new Set(['object1']),
        exportNameAndPropSet: new Set(), // empty set
        exportNameSet: undefined,
        enumPropertySet: new Set(['enum1']),
      };

      obfuscator.addReservedSetForPropertyObf(properties);

      // Verify that a non-empty set is added
      expect(UnobfuscationCollections.reservedStruct.has('struct1')).to.be.true;
      expect(UnobfuscationCollections.reservedObjProp.has('object1')).to.be.true;
      expect(UnobfuscationCollections.reservedEnum.has('enum1')).to.be.true;

      // Verify that the empty set has not been added
      expect(UnobfuscationCollections.reservedStrProp.size).to.equal(0);
      expect(UnobfuscationCollections.reservedExportNameAndProp.size).to.equal(0);
    });

    it('should handle duplicate values in struct and enum sets', () => {
      // Set initial value
      UnobfuscationCollections.reservedStruct.add('duplicateValue');
      UnobfuscationCollections.reservedEnum.add('duplicateValue');

      const properties: ReseverdSetForArkguard = {
        structPropertySet: new Set(['duplicateValue', 'newValue']),
        stringPropertySet: new Set(['string1']),
        objectPropertySet: new Set(['object1']),
        exportNameAndPropSet: new Set(['export1']),
        exportNameSet: undefined,
        enumPropertySet: new Set(['duplicateValue', 'newEnum']),
      };

      obfuscator.addReservedSetForPropertyObf(properties);

      // Verify that duplicate values are not added repeatedly (a characteristic of Set)
      expect(UnobfuscationCollections.reservedStruct.has('duplicateValue')).to.be.true;
      expect(UnobfuscationCollections.reservedStruct.has('newValue')).to.be.true;
      expect(UnobfuscationCollections.reservedStruct.size).to.equal(2);

      expect(UnobfuscationCollections.reservedEnum.has('duplicateValue')).to.be.true;
      expect(UnobfuscationCollections.reservedEnum.has('newEnum')).to.be.true;
      expect(UnobfuscationCollections.reservedEnum.size).to.equal(2);
    });
  });

  describe('test for ArkObfuscator.addReservedSetForDefaultObf', () => {
    it('should add reserved export name set correctly', () => {
      const properties: ReseverdSetForArkguard = {
        structPropertySet: undefined,
        stringPropertySet: undefined,
        objectPropertySet: undefined,
        exportNameAndPropSet: undefined,
        exportNameSet: new Set(['exportName1']),
        enumPropertySet: undefined,
      };

      obfuscator.addReservedSetForDefaultObf(properties);
      expect(UnobfuscationCollections.reservedExportName.has('exportName1')).to.be.true;
    });
  });

  describe('test for ArkObfuscator.setKeepSourceOfPaths', () => {
    it('should set the keep source of paths correctly', () => {
      const config: IOptions = {
        mKeepFileSourceCode: {
          mKeepSourceOfPaths: new Set(),
          mkeepFilesAndDependencies: new Set(),
        },
      };
      obfuscator.init(config);

      const paths = new Set(['path1', 'path2']);
      obfuscator.setKeepSourceOfPaths(paths);
      expect(obfuscator.customProfiles.mKeepFileSourceCode?.mKeepSourceOfPaths).to.equal(paths);
    });
  });

  describe('test for ArkObfuscator.isCurrentFileInKeepPaths', () => {
    it('should return false if mKeepSourceOfPaths is empty', () => {
      const customProfiles: IOptions = {
        mKeepFileSourceCode: {
          mKeepSourceOfPaths: new Set(),
          mkeepFilesAndDependencies: new Set(),
        },
      };
      const result = obfuscator.isCurrentFileInKeepPathsForTest(customProfiles, 'some/file/path.js');
      expect(result).to.be.false;
    });

    it('should return true if originalFilePath is in mKeepSourceOfPaths', () => {
      const keepPaths = new Set(['some/file/path.js']);
      const customProfiles: IOptions = {
        mKeepFileSourceCode: {
          mKeepSourceOfPaths: keepPaths,
          mkeepFilesAndDependencies: new Set(),
        }
      };
      const result = obfuscator.isCurrentFileInKeepPathsForTest(customProfiles, 'some/file/path.js');
      expect(result).to.be.true;
    });
  });

  describe('test for ArkObfuscator.convertLineBasedOnSourceMap', () => {
    it('there is no location but the value is obfuscated, it should be recorded in nameacahe', () => {
      const identifierCache = new Map<string, string>([  
        ['#Person', 'Person'],
        ['obfsucationValue:31:5:35:6', 'h'],
        ['getName:75:5:77:6', 'getName']
      ]);
      nameCache.set('IdentifierCache', identifierCache);
      const source: Source = new Source('utils.ets', null);
      const previousStageSourceMap: RawSourceMap = {
        version: 3,
        file: 'utils.ets',
        sourceRoot: '',
        sources: [ 'entry/src/main/ets/pages/utils/utils.ets'],
        names: [],
        mappings: '',
        sourcesContent: undefined,
      }
      const decodedSourceMap = decodeSourcemap(previousStageSourceMap as RawSourceMap);
      const sourceMapLink: SourceMapLink = new SourceMapLink(decodedSourceMap as MappingsNameTypeForTest, [source]);

      const result = obfuscator.convertLineBasedOnSourceMapForTest('IdentifierCache', sourceMapLink);
      const expectedResult= {
        '#Person': 'Person',
        'obfsucationValue': 'h' 
      };
      const compareResult = JSON.stringify(result) === JSON.stringify(expectedResult);
      expect(compareResult).to.be.true;
    });
    
    it('there is start and end location, it should be recorded in nameacahe', () => {
      const identifierCache = new Map<string, string>([  
        ['#Person', 'Person'],
        ['#foo:1:1:3:2', 'foo'],
        ['getName:75:5:77:6', 'getName']
      ]);
      nameCache.set('IdentifierCache', identifierCache);
      const source: Source = new Source('file.ts', null);
      const previousStageSourceMap: RawSourceMap = {
        "version": 3,
        "file": "file.ts",
        "sources": [
          "entry/src/main/ets/pages/file.ts"
        ],
        "names": [],
        "mappings": "AAAA,MAAM,UAAU,GAAG;IACjB,OAAO,CAAC,CAAC;AACX,CAAC",
        "sourceRoot": ""
      }
      const decodedSourceMap = decodeSourcemap(previousStageSourceMap as RawSourceMap);
      const sourceMapLink: SourceMapLink = new SourceMapLink(decodedSourceMap as MappingsNameTypeForTest, [source]);

      const result = obfuscator.convertLineBasedOnSourceMapForTest('IdentifierCache', sourceMapLink);
      const expectedResult= {
        '#Person': 'Person',
        '#foo:1:3': 'foo' 
      };
      const compareResult = JSON.stringify(result) === JSON.stringify(expectedResult);
      expect(compareResult).to.be.true;
    });
  });

  describe('test for clearGlobalCaches', () => {
    beforeEach(() => {
      PropCollections.globalMangledTable.set('test1', 'obfuscated1');
      PropCollections.historyMangledTable = new Map([['test2', 'obfuscated2']]);
      PropCollections.reservedProperties.add('reserved1');
      PropCollections.universalReservedProperties.push(/universal\d+/);
      globalFileNameMangledTable.set('key1', 'value1');
      renameFileNameModule.historyFileNameMangledTable = new Map([['keyA', 'valueA']]);
      UnobfuscationCollections.reservedSdkApiForProp.add('api1');
      UnobfuscationCollections.reservedSdkApiForGlobal.add('globalApi1');
      UnobfuscationCollections.reservedSdkApiForLocal.add('localApi1');
      UnobfuscationCollections.reservedStruct.add('struct1');
      UnobfuscationCollections.reservedLangForProperty.add('lang1');
      UnobfuscationCollections.reservedExportName.add('exportName1');
      UnobfuscationCollections.reservedExportNameAndProp.add('exportNameAndProp1');
      UnobfuscationCollections.reservedStrProp.add('stringProp1');
      UnobfuscationCollections.reservedObjProp.add('objectProp1');
      UnobfuscationCollections.reservedEnum.add('enum1');  
      UnobfuscationCollections.unobfuscatedPropMap.set('age', new Set(['key', 'value']));
      UnobfuscationCollections.unobfuscatedNamesMap.set('name1', new Set(['key1', 'value2']));  
      LocalVariableCollections.reservedConfig.add('localConfig1');
    });
  
    it('should clear all global caches', () => {
      clearGlobalCaches();
  
      expect(PropCollections.globalMangledTable.size).to.equal(0);
      expect(PropCollections.historyMangledTable.size).to.equal(0);
      expect(PropCollections.reservedProperties.size).to.equal(0);
      expect(PropCollections.universalReservedProperties.length).to.equal(0);
      expect(globalFileNameMangledTable.size).to.equal(0);
      expect(historyFileNameMangledTable.size).to.equal(0);
      expect(UnobfuscationCollections.reservedSdkApiForProp.size).to.equal(0);
      expect(UnobfuscationCollections.reservedStruct.size).to.equal(0);
      expect(UnobfuscationCollections.reservedExportName.size).to.equal(0);
      expect(LocalVariableCollections.reservedConfig.size).to.equal(0);
    });
  });
  
  describe('test whether the methods exported from the ArkObfuscator file exist', () => {
    it('The ArkObfuscator export the collectReservedNameForObf method', () => {
      expect(collectReservedNameForObf).to.exist;
    });

    it('The ArkObfuscator export the collectResevedFileNameInIDEConfig method', () => {
      expect(collectResevedFileNameInIDEConfig).to.exist;
    });

    it('The ArkObfuscator export the deleteLineInfoForNameString method', () => {
      expect(deleteLineInfoForNameString).to.exist;
    });

    it('The ArkObfuscator export the enableObfuscatedFilePathConfig method', () => {
      expect(enableObfuscatedFilePathConfig).to.exist;
    });

    it('The ArkObfuscator export the enableObfuscateFileName method', () => {
      expect(enableObfuscateFileName).to.exist;
    });

    it('The ArkObfuscator export the generateConsumerObConfigFile method', () => {
      expect(generateConsumerObConfigFile).to.exist;
    });

    it('The ArkObfuscator export the getRelativeSourcePath method', () => {
      expect(getRelativeSourcePath).to.exist;
    });

    it('The ArkObfuscator export the handleObfuscatedFilePath method', () => {
      expect(handleObfuscatedFilePath).to.exist;
    });

    it('The ArkObfuscator export the handleUniversalPathInObf method', () => {
      expect(handleUniversalPathInObf).to.exist;
    });

    it('The ArkObfuscator export the initObfuscationConfig method', () => {
      expect(initObfuscationConfig).to.exist;
    });

    it('The ArkObfuscator export the mangleFilePath method', () => {
      expect(mangleFilePath).to.exist;
    });

    it('The ArkObfuscator export the MemoryUtils method', () => {
      expect(MemoryUtils).to.exist;
    });

    it('The ArkObfuscator export the MergedConfig method', () => {
      expect(MergedConfig).to.exist;
    });

    it('The ArkObfuscator export the nameCacheMap method', () => {
      expect(nameCacheMap).to.exist;
    });

    it('The ArkObfuscator export the ObConfigResolver method', () => {
      expect(ObConfigResolver).to.exist;
    });

    it('The ArkObfuscator export the readNameCache method', () => {
      expect(readNameCache).to.exist;
    });

    it('The ArkObfuscator export the readProjectPropertiesByCollectedPaths method', () => {
      expect(readProjectPropertiesByCollectedPaths).to.exist;
    });

    it('The ArkObfuscator export the TimeSumPrinter method', () => {
      expect(TimeSumPrinter).to.exist;
    });

    it('The ArkObfuscator export the writeObfuscationNameCache method', () => {
      expect(writeObfuscationNameCache).to.exist;
    });

    it('The ArkObfuscator export the writeUnobfuscationContent method', () => {
      expect(writeUnobfuscationContent).to.exist;
    });

    it('The ArkObfuscator export the unobfuscationNamesObj method', () => {
      expect(unobfuscationNamesObj).to.exist;
    });
  });
});

function compareStringsIgnoreNewlines(str1: string, str2: string): boolean {
  const normalize = (str: string) => str.replace(/[\n\r\s]+/g, '');
  return normalize(str1) === normalize(str2);
}

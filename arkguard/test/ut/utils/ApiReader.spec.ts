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

import {
  readProjectPropertiesByCollectedPaths,
  ReseverdSetForArkguard,
  scanProjectConfig,
  initScanProjectConfig
} from '../../../src/common/ApiReader';
import { assert, expect } from 'chai';
import { NameGeneratorType } from '../../../src/generator/NameFactory';
import {
  FileWhiteList,
  ProjectWhiteList,
  projectWhiteListManager,
  initProjectWhiteListManager
} from '../../../src/utils/ProjectCollections';
import { IOptions } from '../../../src/configs/IOptions';

describe('test for ApiReader', function () {
  describe('test for readProjectPropertiesByCollectedPaths', function () {
    const path1: string = 'test/ut/utils/apiTest_readProjectPropertiesByCollectedPaths/block_enum_test.ts';
    const path2: string = 'test/ut/utils/apiTest_readProjectPropertiesByCollectedPaths/enum_test.ts';
    const path3: string = 'test/ut/utils/apiTest_readProjectPropertiesByCollectedPaths/export_enum_test.ts';
    const path4: string = 'test/ut/utils/apiTest_readProjectPropertiesByCollectedPaths/namespace_enum_test.ts';
    const fileList: Set<string> = new Set([path1, path2, path3, path4]);

    it('-enable-export-obfuscation + -enable-property-obfuscation', function () {
      let config: IOptions = {
        mNameObfuscation : {
          mEnable: true,
          mReservedProperties: [],
          mRenameProperties: true,
          mKeepStringProperty: false,
          mNameGeneratorType: NameGeneratorType.ORDERED,
          mReservedNames: [],
          mReservedToplevelNames: []
        }
      };
      let cachePath = 'test/ut/utils/obfuscation';
      initProjectWhiteListManager(cachePath, false, false, false);
      let projectAndLibs: ReseverdSetForArkguard =
        readProjectPropertiesByCollectedPaths(fileList,
        {
          mNameObfuscation: config.mNameObfuscation,
          mExportObfuscation: true,
          mKeepFileSourceCode: {
            mKeepSourceOfPaths: new Set(),
            mkeepFilesAndDependencies: new Set(),
          },
          
        }, true);
      let enumPropertySet = projectAndLibs.enumPropertySet;
      assert.strictEqual(enumPropertySet.has('BLOCK_PARAM1'), true);
      assert.strictEqual(enumPropertySet.has('BLOCK_PARAM2'), true);
      assert.strictEqual(enumPropertySet.has('ENUM_PARAM1'), true);
      assert.strictEqual(enumPropertySet.has('ENUM_PARAM2'), true);
      assert.strictEqual(enumPropertySet.has('NS_PARAM1'), true);
      assert.strictEqual(enumPropertySet.has('NS_PARAM2'), true);
      assert.strictEqual(enumPropertySet.has('EXPORT_PARAM1'), true);
      assert.strictEqual(enumPropertySet.has('EXPORT_PARAM2'), true);
      const fileWhiteList1: FileWhiteList = projectWhiteListManager!.getFileWhiteListMap().get(path1)!;
      expect(fileWhiteList1.fileKeepInfo.enumProperties.has('BLOCK_PARAM1')).to.be.true;
      expect(fileWhiteList1.fileKeepInfo.enumProperties.has('BLOCK_PARAM2')).to.be.true;
      const fileWhiteList2: FileWhiteList = projectWhiteListManager!.getFileWhiteListMap().get(path2)!;
      expect(fileWhiteList2.fileKeepInfo.enumProperties.has('ENUM_PARAM1')).to.be.true;
      expect(fileWhiteList2.fileKeepInfo.enumProperties.has('ENUM_PARAM2')).to.be.true;
      const fileWhiteList3: FileWhiteList = projectWhiteListManager!.getFileWhiteListMap().get(path3)!;
      expect(fileWhiteList3.fileKeepInfo.enumProperties.has('EXPORT_PARAM1')).to.be.true;
      expect(fileWhiteList3.fileKeepInfo.enumProperties.has('EXPORT_PARAM2')).to.be.true;
      const fileWhiteList4: FileWhiteList = projectWhiteListManager!.getFileWhiteListMap().get(path4)!;
      expect(fileWhiteList4.fileKeepInfo.enumProperties.has('NS_PARAM1')).to.be.true;
      expect(fileWhiteList4.fileKeepInfo.enumProperties.has('NS_PARAM2')).to.be.true;
    });

    it('-enable-property-obfuscation', function () {
      let config: IOptions = {
        mNameObfuscation : {
          mEnable: true,
          mReservedProperties: [],
          mRenameProperties: true,
          mKeepStringProperty: false,
          mNameGeneratorType: NameGeneratorType.ORDERED,
          mReservedNames: [],
          mReservedToplevelNames: []
        }
      };
      let cachePath = 'test/ut/utils/obfuscation';
      initProjectWhiteListManager(cachePath, false, false, false);
      let projectAndLibs: ReseverdSetForArkguard =
        readProjectPropertiesByCollectedPaths(fileList,
        {
          mNameObfuscation: config.mNameObfuscation,
          mExportObfuscation: false,
          mKeepFileSourceCode: {
            mKeepSourceOfPaths: new Set(),
            mkeepFilesAndDependencies: new Set(),
          }
        }, true);
      let enumPropertySet = projectAndLibs.enumPropertySet;
      let exportNameAndPropSet = projectAndLibs.exportNameAndPropSet;
      assert.strictEqual(enumPropertySet.has('BLOCK_PARAM1'), true);
      assert.strictEqual(enumPropertySet.has('BLOCK_PARAM2'), true);
      assert.strictEqual(enumPropertySet.has('ENUM_PARAM1'), true);
      assert.strictEqual(enumPropertySet.has('ENUM_PARAM2'), true);
      assert.strictEqual(enumPropertySet.has('NS_PARAM1'), true);
      assert.strictEqual(enumPropertySet.has('NS_PARAM2'), true);
      assert.strictEqual(enumPropertySet.has('EXPORT_PARAM1'), true);
      assert.strictEqual(enumPropertySet.has('EXPORT_PARAM2'), true);
      assert.strictEqual(exportNameAndPropSet.has('ExportEnum'), true);
      assert.strictEqual(exportNameAndPropSet.has('EXPORT_PARAM1'), true);
      assert.strictEqual(exportNameAndPropSet.has('EXPORT_PARAM2'), true);
      const fileWhiteList1: FileWhiteList = projectWhiteListManager!.getFileWhiteListMap().get(path1)!;
      expect(fileWhiteList1.fileKeepInfo.enumProperties.has('BLOCK_PARAM1')).to.be.true;
      expect(fileWhiteList1.fileKeepInfo.enumProperties.has('BLOCK_PARAM2')).to.be.true;
      const fileWhiteList2: FileWhiteList = projectWhiteListManager!.getFileWhiteListMap().get(path2)!;
      expect(fileWhiteList2.fileKeepInfo.enumProperties.has('ENUM_PARAM1')).to.be.true;
      expect(fileWhiteList2.fileKeepInfo.enumProperties.has('ENUM_PARAM2')).to.be.true;
      const fileWhiteList3: FileWhiteList = projectWhiteListManager!.getFileWhiteListMap().get(path3)!;
      expect(fileWhiteList3.fileKeepInfo.enumProperties.has('EXPORT_PARAM1')).to.be.true;
      expect(fileWhiteList3.fileKeepInfo.enumProperties.has('EXPORT_PARAM2')).to.be.true;
      expect(fileWhiteList3.fileKeepInfo.exported.propertyNames.has('ExportEnum')).to.be.true;
      expect(fileWhiteList3.fileKeepInfo.exported.propertyNames.has('EXPORT_PARAM1')).to.be.true;
      expect(fileWhiteList3.fileKeepInfo.exported.propertyNames.has('EXPORT_PARAM2')).to.be.true;
      const fileWhiteList4: FileWhiteList = projectWhiteListManager!.getFileWhiteListMap().get(path4)!;
      expect(fileWhiteList4.fileKeepInfo.enumProperties.has('NS_PARAM1')).to.be.true;
      expect(fileWhiteList4.fileKeepInfo.enumProperties.has('NS_PARAM2')).to.be.true;
    });

    it('-enable-export-obfuscation', function () {
      let config: IOptions = {
        mNameObfuscation : {
          mEnable: true,
          mReservedProperties: [],
          mRenameProperties: false,
          mKeepStringProperty: false,
          mNameGeneratorType: NameGeneratorType.ORDERED,
          mReservedNames: [],
          mReservedToplevelNames: []
        }
      };
      let cachePath = 'test/ut/utils/obfuscation';
      initProjectWhiteListManager(cachePath, false, false, false);
      let projectAndLibs: ReseverdSetForArkguard =
        readProjectPropertiesByCollectedPaths(fileList,
        {
          mNameObfuscation: config.mNameObfuscation,
          mExportObfuscation: true,
          mKeepFileSourceCode: {
            mKeepSourceOfPaths: new Set(),
            mkeepFilesAndDependencies: new Set(),
          }
        }, true);
      let exportNameAndPropSet = projectAndLibs.exportNameAndPropSet;
      let exportNameSet = projectAndLibs.exportNameSet;
      assert.strictEqual(exportNameAndPropSet, undefined);
      assert.strictEqual(exportNameSet.has('ExportEnum'), false);
      const fileWhiteLists: Map<string, FileWhiteList> = projectWhiteListManager!.getFileWhiteListMap();
      const projectWhiteList: ProjectWhiteList = projectWhiteListManager!.createProjectWhiteList(fileWhiteLists);
      expect(projectWhiteList.projectKeepInfo.propertyNames.size).to.be.equal(0);
      expect(projectWhiteList.projectKeepInfo.globalNames.size).to.be.equal(0);
    });
  });

  describe('test for -keep and export obfuscation', function () {
    const filePath = 'test/ut/utils/keep_export/exportFile1.ts';
    const fileList: Set<string> = new Set([filePath]);

    it('-enable-export-obfuscation', function () {
      let config: IOptions = {
        mNameObfuscation : {
          mEnable: true,
          mReservedProperties: [],
          mRenameProperties: false,
          mKeepStringProperty: false,
          mNameGeneratorType: NameGeneratorType.ORDERED,
          mReservedNames: [],
          mReservedToplevelNames: []
        }
      };
      let cachePath = 'test/ut/utils/obfuscation';
      initProjectWhiteListManager(cachePath, false, false, false);
      let projectAndLibs: ReseverdSetForArkguard =
        readProjectPropertiesByCollectedPaths(fileList,
        {
          mNameObfuscation: config.mNameObfuscation,
          mExportObfuscation: true,
          mKeepFileSourceCode: {
            mKeepSourceOfPaths: new Set(),
            mkeepFilesAndDependencies: new Set([
              "test/ut/utils/keep_export/exportFile1.ts"
            ]),
          }
        }, true);
      let exportNameSet = projectAndLibs.exportNameSet;
      let exportNameAndPropSet = projectAndLibs.exportNameAndPropSet;
      assert.strictEqual(exportNameSet.has('TestClass'), true);
      assert.strictEqual(exportNameSet.has('prop1'), false);
      assert.strictEqual(exportNameSet.has('prop2'), false);
      assert.strictEqual(exportNameSet.has('objProp'), false);
      assert.strictEqual(exportNameSet.has('innerProp2'), false);
      assert.strictEqual(exportNameSet.has('var1'), true);
      assert.strictEqual(exportNameSet.has('var2'), false);
      assert.strictEqual(exportNameSet.has('foo'), true);
      assert.strictEqual(exportNameSet.has('ns'), false);
      assert.strictEqual(exportNameSet.has('var3'), true);
      assert.strictEqual(exportNameSet.has('nsFunction'), true);
      assert.strictEqual(exportNameSet.has('TestInterface'), true);
      assert.strictEqual(exportNameSet.has('feature1'), false);
      assert.strictEqual(exportNameSet.has('feature2'), false);
      assert.strictEqual(exportNameSet.has('TestClass2'), false);
      assert.strictEqual(exportNameSet.has('prop4'), false);
      assert.strictEqual(exportNameSet.has('propObj'), false);
      assert.strictEqual(exportNameSet.has('innerProp'), false);
      assert.strictEqual(exportNameSet.has('TestClass3'), false);
      assert.strictEqual(exportNameSet.has('exportProp1'), false);
      assert.strictEqual(exportNameSet.has('exportPropObj'), false);
      assert.strictEqual(exportNameSet.has('exportInnerProp'), false);
      assert.strictEqual(exportNameSet.has('v2'), true);
      assert.strictEqual(exportNameSet.has('default'), true);
      assert.strictEqual(exportNameSet.has('t3'), true);
      assert.strictEqual(exportNameSet.has('outterElement1'), true);
      assert.strictEqual(exportNameSet.has('outterElement2'), true);
      assert.strictEqual(exportNameSet.has('o2'), true);
      assert.strictEqual(exportNameAndPropSet, undefined);
      const fileWhiteList: FileWhiteList = projectWhiteListManager!.getFileWhiteListMap().get(filePath)!;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('TestClass')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('prop1')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('prop2')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('objProp')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('innerProp2')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('var1')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('var2')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('foo')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('ns')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('var3')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('nsFunction')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('TestInterface')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('feature1')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('feature2')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('TestClass2')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('prop4')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('propObj')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('innerProp')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('TestClass3')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('exportProp1')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('exportPropObj')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('exportInnerProp')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('v2')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('default')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('t3')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('outterElement1')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('outterElement2')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('o2')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.size).to.be.equal(0);
    });

    it('-enable-property-obfuscation', function () {
      let config: IOptions = {
        mNameObfuscation : {
          mEnable: true,
          mReservedProperties: [],
          mRenameProperties: true,
          mKeepStringProperty: false,
          mNameGeneratorType: NameGeneratorType.ORDERED,
          mReservedNames: [],
          mReservedToplevelNames: []
        }
      };
      let cachePath = 'test/ut/utils/obfuscation';
      initProjectWhiteListManager(cachePath, false, false, false);
      let projectAndLibs: ReseverdSetForArkguard =
        readProjectPropertiesByCollectedPaths(fileList,
        {
          mNameObfuscation: config.mNameObfuscation,
          mExportObfuscation: false,
          mKeepFileSourceCode: {
            mKeepSourceOfPaths: new Set(),
            mkeepFilesAndDependencies: new Set([
              "test/ut/utils/keep_export/exportFile1.ts"
            ]),
          }
        }, true);
      let exportNameAndPropSet = projectAndLibs.exportNameAndPropSet;
      let exportNameSet = projectAndLibs.exportNameSet;
      assert.strictEqual(exportNameAndPropSet.has('TestClass'), true);
      assert.strictEqual(exportNameAndPropSet.has('prop1'), true);
      assert.strictEqual(exportNameAndPropSet.has('prop2'), true);
      assert.strictEqual(exportNameAndPropSet.has('objProp'), true);
      assert.strictEqual(exportNameAndPropSet.has('innerProp2'), true);
      assert.strictEqual(exportNameAndPropSet.has('var1'), true);
      assert.strictEqual(exportNameAndPropSet.has('var2'), false);
      assert.strictEqual(exportNameAndPropSet.has('foo'), true);
      assert.strictEqual(exportNameAndPropSet.has('ns'), false);
      assert.strictEqual(exportNameAndPropSet.has('var3'), true);
      assert.strictEqual(exportNameAndPropSet.has('nsFunction'), true);
      assert.strictEqual(exportNameAndPropSet.has('TestInterface'), true);
      assert.strictEqual(exportNameAndPropSet.has('feature1'), true);
      assert.strictEqual(exportNameAndPropSet.has('feature2'), true);
      assert.strictEqual(exportNameAndPropSet.has('TestClass2'), false);
      assert.strictEqual(exportNameAndPropSet.has('prop4'), false);
      assert.strictEqual(exportNameAndPropSet.has('propObj'), false);
      assert.strictEqual(exportNameAndPropSet.has('innerProp'), false);
      assert.strictEqual(exportNameAndPropSet.has('TestClass3'), false);
      assert.strictEqual(exportNameAndPropSet.has('exportProp1'), true);
      assert.strictEqual(exportNameAndPropSet.has('exportPropObj'), true);
      assert.strictEqual(exportNameAndPropSet.has('exportInnerProp'), true);
      assert.strictEqual(exportNameAndPropSet.has('v2'), true);
      assert.strictEqual(exportNameAndPropSet.has('default'), true);
      assert.strictEqual(exportNameAndPropSet.has('t3'), true);
      assert.strictEqual(exportNameAndPropSet.has('outterElement1'), true);
      assert.strictEqual(exportNameAndPropSet.has('outterElement2'), false);
      assert.strictEqual(exportNameAndPropSet.has('o2'), true);
      assert.strictEqual(exportNameSet, undefined);
      const fileWhiteList: FileWhiteList = projectWhiteListManager!.getFileWhiteListMap().get(filePath)!;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('TestClass')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('prop1')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('prop2')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('objProp')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('innerProp2')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('var1')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('var2')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('foo')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('ns')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('var3')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('nsFunction')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('TestInterface')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('feature1')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('feature2')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('TestClass2')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('prop4')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('propObj')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('innerProp')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('TestClass3')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('exportProp1')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('exportPropObj')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('exportInnerProp')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('v2')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('default')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('t3')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('outterElement1')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('outterElement2')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('o2')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.size).to.be.equal(0);
    });

    it('-enable-toplevel-obfuscation', function () {
      let config: IOptions = {
        mNameObfuscation : {
          mEnable: true,
          mReservedProperties: [],
          mRenameProperties: false,
          mKeepStringProperty: false,
          mTopLevel: true,
          mNameGeneratorType: NameGeneratorType.ORDERED,
          mReservedNames: [],
          mReservedToplevelNames: []
        }
      };
      let cachePath = 'test/ut/utils/obfuscation';
      initProjectWhiteListManager(cachePath, false, false, false);
      let projectAndLibs: ReseverdSetForArkguard =
        readProjectPropertiesByCollectedPaths(fileList,
        {
          mNameObfuscation: config.mNameObfuscation,
          mExportObfuscation: false,
          mKeepFileSourceCode: {
            mKeepSourceOfPaths: new Set(),
            mkeepFilesAndDependencies: new Set([
              "test/ut/utils/keep_export/exportFile1.ts"
            ]),
          }
        }, true);
      let exportNameAndPropSet = projectAndLibs.exportNameAndPropSet;
      let exportNameSet = projectAndLibs.exportNameSet;
      assert.strictEqual(exportNameAndPropSet, undefined);
      assert.strictEqual(exportNameSet, undefined);
      const fileWhiteList: FileWhiteList = projectWhiteListManager!.getFileWhiteListMap().get(filePath)!;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.size).to.be.equal(0);
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.size).to.be.equal(0);
    });

    it('-enable-toplevel-obfuscation -enable-export-obfuscation', function () {
      let config: IOptions = {
        mNameObfuscation : {
          mEnable: true,
          mReservedProperties: [],
          mRenameProperties: false,
          mKeepStringProperty: false,
          mTopLevel: true,
          mNameGeneratorType: NameGeneratorType.ORDERED,
          mReservedNames: [],
          mReservedToplevelNames: []
        }
      };
      let cachePath = 'test/ut/utils/obfuscation';
      initProjectWhiteListManager(cachePath, false, false, false);
      let projectAndLibs: ReseverdSetForArkguard =
        readProjectPropertiesByCollectedPaths(fileList,
        {
          mNameObfuscation: config.mNameObfuscation,
          mExportObfuscation: true,
          mKeepFileSourceCode: {
            mKeepSourceOfPaths: new Set(),
            mkeepFilesAndDependencies: new Set([
              "test/ut/utils/keep_export/exportFile1.ts"
            ]),
          }
        }, true);
      let exportNameAndPropSet = projectAndLibs.exportNameAndPropSet;
      let exportNameSet = projectAndLibs.exportNameSet;
      assert.strictEqual(exportNameSet.has('TestClass'), true);
      assert.strictEqual(exportNameSet.has('prop1'), false);
      assert.strictEqual(exportNameSet.has('prop2'), false);
      assert.strictEqual(exportNameSet.has('objProp'), false);
      assert.strictEqual(exportNameSet.has('innerProp2'), false);
      assert.strictEqual(exportNameSet.has('var1'), true);
      assert.strictEqual(exportNameSet.has('var2'), false);
      assert.strictEqual(exportNameSet.has('foo'), true);
      assert.strictEqual(exportNameSet.has('ns'), false);
      assert.strictEqual(exportNameSet.has('var3'), true);
      assert.strictEqual(exportNameSet.has('nsFunction'), true);
      assert.strictEqual(exportNameSet.has('TestInterface'), true);
      assert.strictEqual(exportNameSet.has('feature1'), false);
      assert.strictEqual(exportNameSet.has('feature2'), false);
      assert.strictEqual(exportNameSet.has('TestClass2'), false);
      assert.strictEqual(exportNameSet.has('prop4'), false);
      assert.strictEqual(exportNameSet.has('propObj'), false);
      assert.strictEqual(exportNameSet.has('innerProp'), false);
      assert.strictEqual(exportNameSet.has('TestClass3'), false);
      assert.strictEqual(exportNameSet.has('exportProp1'), false);
      assert.strictEqual(exportNameSet.has('exportPropObj'), false);
      assert.strictEqual(exportNameSet.has('exportInnerProp'), false);
      assert.strictEqual(exportNameSet.has('v2'), true);
      assert.strictEqual(exportNameSet.has('default'), true);
      assert.strictEqual(exportNameSet.has('t3'), true);
      assert.strictEqual(exportNameSet.has('outterElement1'), true);
      assert.strictEqual(exportNameSet.has('outterElement2'), true);
      assert.strictEqual(exportNameSet.has('o2'), true);
      assert.strictEqual(exportNameAndPropSet, undefined);
      const fileWhiteList: FileWhiteList = projectWhiteListManager!.getFileWhiteListMap().get(filePath)!;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('TestClass')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('prop1')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('prop2')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('objProp')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('innerProp2')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('var1')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('var2')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('foo')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('ns')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('var3')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('nsFunction')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('TestInterface')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('feature1')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('feature2')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('TestClass2')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('prop4')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('propObj')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('innerProp')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('TestClass3')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('exportProp1')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('exportPropObj')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('exportInnerProp')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('v2')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('default')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('t3')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('outterElement1')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('outterElement2')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('o2')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.size).to.be.equal(0);
    });

    it('-enable-property-obfuscation -enable-export-obfuscation', function () {
      let config: IOptions = {
        mNameObfuscation : {
          mEnable: true,
          mReservedProperties: [],
          mRenameProperties: true,
          mKeepStringProperty: false,
          mTopLevel: false,
          mNameGeneratorType: NameGeneratorType.ORDERED,
          mReservedNames: [],
          mReservedToplevelNames: []
        }
      };
      let cachePath = 'test/ut/utils/obfuscation';
      initProjectWhiteListManager(cachePath, false, false, false);
      let projectAndLibs: ReseverdSetForArkguard =
        readProjectPropertiesByCollectedPaths(fileList,
        {
          mNameObfuscation: config.mNameObfuscation,
          mExportObfuscation: true,
          mKeepFileSourceCode: {
            mKeepSourceOfPaths: new Set(),
            mkeepFilesAndDependencies: new Set([
              "test/ut/utils/keep_export/exportFile1.ts"
            ]),
          }
        }, true);
      let exportNameAndPropSet = projectAndLibs.exportNameAndPropSet;
      let exportNameSet = projectAndLibs.exportNameSet;
      assert.strictEqual(exportNameSet.has('TestClass'), true);
      assert.strictEqual(exportNameSet.has('prop1'), false);
      assert.strictEqual(exportNameSet.has('prop2'), false);
      assert.strictEqual(exportNameSet.has('objProp'), false);
      assert.strictEqual(exportNameSet.has('innerProp2'), false);
      assert.strictEqual(exportNameSet.has('var1'), true);
      assert.strictEqual(exportNameSet.has('var2'), false);
      assert.strictEqual(exportNameSet.has('foo'), true);
      assert.strictEqual(exportNameSet.has('ns'), false);
      assert.strictEqual(exportNameSet.has('var3'), true);
      assert.strictEqual(exportNameSet.has('nsFunction'), true);
      assert.strictEqual(exportNameSet.has('TestInterface'), true);
      assert.strictEqual(exportNameSet.has('feature1'), false);
      assert.strictEqual(exportNameSet.has('feature2'), false);
      assert.strictEqual(exportNameSet.has('TestClass2'), false);
      assert.strictEqual(exportNameSet.has('prop4'), false);
      assert.strictEqual(exportNameSet.has('propObj'), false);
      assert.strictEqual(exportNameSet.has('innerProp'), false);
      assert.strictEqual(exportNameSet.has('TestClass3'), false);
      assert.strictEqual(exportNameSet.has('exportProp1'), false);
      assert.strictEqual(exportNameSet.has('exportPropObj'), false);
      assert.strictEqual(exportNameSet.has('exportInnerProp'), false);
      assert.strictEqual(exportNameSet.has('v2'), true);
      assert.strictEqual(exportNameSet.has('default'), true);
      assert.strictEqual(exportNameSet.has('t3'), true);
      assert.strictEqual(exportNameSet.has('outterElement1'), true);
      assert.strictEqual(exportNameSet.has('outterElement2'), true);
      assert.strictEqual(exportNameSet.has('o2'), true);
      assert.strictEqual(exportNameAndPropSet.has('TestClass'), true);
      assert.strictEqual(exportNameAndPropSet.has('prop1'), true);
      assert.strictEqual(exportNameAndPropSet.has('prop2'), true);
      assert.strictEqual(exportNameAndPropSet.has('objProp'), true);
      assert.strictEqual(exportNameAndPropSet.has('innerProp2'), true);
      assert.strictEqual(exportNameAndPropSet.has('var1'), true);
      assert.strictEqual(exportNameAndPropSet.has('var2'), false);
      assert.strictEqual(exportNameAndPropSet.has('foo'), true);
      assert.strictEqual(exportNameAndPropSet.has('ns'), false);
      assert.strictEqual(exportNameAndPropSet.has('var3'), true);
      assert.strictEqual(exportNameAndPropSet.has('nsFunction'), true);
      assert.strictEqual(exportNameAndPropSet.has('TestInterface'), true);
      assert.strictEqual(exportNameAndPropSet.has('feature1'), true);
      assert.strictEqual(exportNameAndPropSet.has('feature2'), true);
      assert.strictEqual(exportNameAndPropSet.has('TestClass2'), false);
      assert.strictEqual(exportNameAndPropSet.has('prop4'), false);
      assert.strictEqual(exportNameAndPropSet.has('propObj'), false);
      assert.strictEqual(exportNameAndPropSet.has('innerProp'), false);
      assert.strictEqual(exportNameAndPropSet.has('TestClass3'), false);
      assert.strictEqual(exportNameAndPropSet.has('exportProp1'), true);
      assert.strictEqual(exportNameAndPropSet.has('exportPropObj'), true);
      assert.strictEqual(exportNameAndPropSet.has('exportInnerProp'), true);
      assert.strictEqual(exportNameAndPropSet.has('v2'), true);
      assert.strictEqual(exportNameAndPropSet.has('default'), true);
      assert.strictEqual(exportNameAndPropSet.has('t3'), true);
      assert.strictEqual(exportNameAndPropSet.has('outterElement1'), true);
      assert.strictEqual(exportNameAndPropSet.has('outterElement2'), false);
      assert.strictEqual(exportNameAndPropSet.has('o2'), true);
      const fileWhiteList: FileWhiteList = projectWhiteListManager!.getFileWhiteListMap().get(filePath)!;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('TestClass')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('prop1')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('prop2')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('objProp')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('innerProp2')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('var1')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('var2')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('foo')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('ns')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('var3')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('nsFunction')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('TestInterface')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('feature1')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('feature2')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('TestClass2')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('prop4')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('propObj')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('innerProp')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('TestClass3')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('exportProp1')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('exportPropObj')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('exportInnerProp')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('v2')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('default')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('t3')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('outterElement1')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('outterElement2')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('o2')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('TestClass')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('prop1')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('prop2')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('objProp')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('innerProp2')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('var1')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('var2')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('foo')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('ns')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('var3')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('nsFunction')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('TestInterface')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('feature1')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('feature2')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('TestClass2')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('prop4')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('propObj')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('innerProp')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('TestClass3')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('exportProp1')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('exportPropObj')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('exportInnerProp')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('v2')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('default')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('t3')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('outterElement1')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('outterElement2')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('o2')).to.be.true;
    });

    it('-enable-property-obfuscation -enable-export-obfuscation -enable-toplevel-obfuscation', function () {
      let config: IOptions = {
        mNameObfuscation : {
          mEnable: true,
          mReservedProperties: [],
          mRenameProperties: true,
          mKeepStringProperty: false,
          mTopLevel: true,
          mNameGeneratorType: NameGeneratorType.ORDERED,
          mReservedNames: [],
          mReservedToplevelNames: []
        }
      };
      let cachePath = 'test/ut/utils/obfuscation';
      initProjectWhiteListManager(cachePath, false, false, false);
      let projectAndLibs: ReseverdSetForArkguard =
        readProjectPropertiesByCollectedPaths(fileList,
        {
          mNameObfuscation: config.mNameObfuscation,
          mExportObfuscation: true,
          mKeepFileSourceCode: {
            mKeepSourceOfPaths: new Set(),
            mkeepFilesAndDependencies: new Set([
              "test/ut/utils/keep_export/exportFile1.ts"
            ]),
          }
        }, true);
      let exportNameAndPropSet = projectAndLibs.exportNameAndPropSet;
      let exportNameSet = projectAndLibs.exportNameSet;
      assert.strictEqual(exportNameSet.has('TestClass'), true);
      assert.strictEqual(exportNameSet.has('prop1'), false);
      assert.strictEqual(exportNameSet.has('prop2'), false);
      assert.strictEqual(exportNameSet.has('objProp'), false);
      assert.strictEqual(exportNameSet.has('innerProp2'), false);
      assert.strictEqual(exportNameSet.has('var1'), true);
      assert.strictEqual(exportNameSet.has('var2'), false);
      assert.strictEqual(exportNameSet.has('foo'), true);
      assert.strictEqual(exportNameSet.has('ns'), false);
      assert.strictEqual(exportNameSet.has('var3'), true);
      assert.strictEqual(exportNameSet.has('nsFunction'), true);
      assert.strictEqual(exportNameSet.has('TestInterface'), true);
      assert.strictEqual(exportNameSet.has('feature1'), false);
      assert.strictEqual(exportNameSet.has('feature2'), false);
      assert.strictEqual(exportNameSet.has('TestClass2'), false);
      assert.strictEqual(exportNameSet.has('prop4'), false);
      assert.strictEqual(exportNameSet.has('propObj'), false);
      assert.strictEqual(exportNameSet.has('innerProp'), false);
      assert.strictEqual(exportNameSet.has('TestClass3'), false);
      assert.strictEqual(exportNameSet.has('exportProp1'), false);
      assert.strictEqual(exportNameSet.has('exportPropObj'), false);
      assert.strictEqual(exportNameSet.has('exportInnerProp'), false);
      assert.strictEqual(exportNameSet.has('v2'), true);
      assert.strictEqual(exportNameSet.has('default'), true);
      assert.strictEqual(exportNameSet.has('t3'), true);
      assert.strictEqual(exportNameSet.has('outterElement1'), true);
      assert.strictEqual(exportNameSet.has('outterElement2'), true);
      assert.strictEqual(exportNameSet.has('o2'), true);
      assert.strictEqual(exportNameAndPropSet.has('TestClass'), true);
      assert.strictEqual(exportNameAndPropSet.has('prop1'), true);
      assert.strictEqual(exportNameAndPropSet.has('prop2'), true);
      assert.strictEqual(exportNameAndPropSet.has('objProp'), true);
      assert.strictEqual(exportNameAndPropSet.has('innerProp2'), true);
      assert.strictEqual(exportNameAndPropSet.has('var1'), true);
      assert.strictEqual(exportNameAndPropSet.has('var2'), false);
      assert.strictEqual(exportNameAndPropSet.has('foo'), true);
      assert.strictEqual(exportNameAndPropSet.has('ns'), false);
      assert.strictEqual(exportNameAndPropSet.has('var3'), true);
      assert.strictEqual(exportNameAndPropSet.has('nsFunction'), true);
      assert.strictEqual(exportNameAndPropSet.has('TestInterface'), true);
      assert.strictEqual(exportNameAndPropSet.has('feature1'), true);
      assert.strictEqual(exportNameAndPropSet.has('feature2'), true);
      assert.strictEqual(exportNameAndPropSet.has('TestClass2'), false);
      assert.strictEqual(exportNameAndPropSet.has('prop4'), false);
      assert.strictEqual(exportNameAndPropSet.has('propObj'), false);
      assert.strictEqual(exportNameAndPropSet.has('innerProp'), false);
      assert.strictEqual(exportNameAndPropSet.has('TestClass3'), false);
      assert.strictEqual(exportNameAndPropSet.has('exportProp1'), true);
      assert.strictEqual(exportNameAndPropSet.has('exportPropObj'), true);
      assert.strictEqual(exportNameAndPropSet.has('exportInnerProp'), true);
      assert.strictEqual(exportNameAndPropSet.has('v2'), true);
      assert.strictEqual(exportNameAndPropSet.has('default'), true);
      assert.strictEqual(exportNameAndPropSet.has('t3'), true);
      assert.strictEqual(exportNameAndPropSet.has('outterElement1'), true);
      assert.strictEqual(exportNameAndPropSet.has('outterElement2'), false);
      assert.strictEqual(exportNameAndPropSet.has('o2'), true);
      const fileWhiteList: FileWhiteList = projectWhiteListManager!.getFileWhiteListMap().get(filePath)!;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('TestClass')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('prop1')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('prop2')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('objProp')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('innerProp2')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('var1')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('var2')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('foo')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('ns')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('var3')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('nsFunction')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('TestInterface')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('feature1')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('feature2')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('TestClass2')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('prop4')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('propObj')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('innerProp')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('TestClass3')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('exportProp1')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('exportPropObj')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('exportInnerProp')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('v2')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('default')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('t3')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('outterElement1')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('outterElement2')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('o2')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('TestClass')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('prop1')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('prop2')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('objProp')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('innerProp2')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('var1')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('var2')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('foo')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('ns')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('var3')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('nsFunction')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('TestInterface')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('feature1')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('feature2')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('TestClass2')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('prop4')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('propObj')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('innerProp')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('TestClass3')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('exportProp1')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('exportPropObj')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('exportInnerProp')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('v2')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('default')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('t3')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('outterElement1')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('outterElement2')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('o2')).to.be.true;
    });

    it('oh_modules test', function () {
      const filePath = 'test/ut/utils/oh_modules/exportFile1.ts'
      const ohModulesFileList: Set<string> = new Set([filePath]);
      let config: IOptions = {
        mNameObfuscation : {
          mEnable: true,
          mReservedProperties: [],
          mRenameProperties: true,
          mKeepStringProperty: false,
          mTopLevel: true,
          mNameGeneratorType: NameGeneratorType.ORDERED,
          mReservedNames: [],
          mReservedToplevelNames: []
        }
      };
      let cachePath = 'test/ut/utils/obfuscation';
      initProjectWhiteListManager(cachePath, false, false, false);
      let projectAndLibs: ReseverdSetForArkguard =
        readProjectPropertiesByCollectedPaths(ohModulesFileList,
        {
          mNameObfuscation: config.mNameObfuscation,
          mExportObfuscation: true,
          mKeepFileSourceCode: {
            mKeepSourceOfPaths: new Set(),
            mkeepFilesAndDependencies: new Set(),
          }
        }, true);
      let exportNameAndPropSet = projectAndLibs.exportNameAndPropSet;
      let exportNameSet = projectAndLibs.exportNameSet;
      assert.strictEqual(exportNameSet.has('ModuleNs'), true);
      assert.strictEqual(exportNameSet.has('nsProp1'), true);
      assert.strictEqual(exportNameSet.has('nsFunc'), true);
      assert.strictEqual(exportNameSet.has('ModuleClass'), true);
      assert.strictEqual(exportNameSet.has('classProp1'), false);
      assert.strictEqual(exportNameSet.has('objProp'), false);
      assert.strictEqual(exportNameSet.has('innerProp'), false);
      assert.strictEqual(exportNameSet.has('TestClass'), false);
      assert.strictEqual(exportNameSet.has('prop4'), false);
      assert.strictEqual(exportNameSet.has('propObj'), false);
      assert.strictEqual(exportNameSet.has('innerProp1'), false);
      assert.strictEqual(exportNameSet.has('TestClass2'), true);
      assert.strictEqual(exportNameSet.has('prop1'), false);
      assert.strictEqual(exportNameSet.has('objProp1'), false);
      assert.strictEqual(exportNameSet.has('innerProp2'), false);
      assert.strictEqual(exportNameSet.has('default'), true);
      assert.strictEqual(exportNameSet.has('mc'), true);
      assert.strictEqual(exportNameSet.has('otherElement1'), true);
      assert.strictEqual(exportNameSet.has('otherElement2'), true);
      assert.strictEqual(exportNameSet.has('o2'), true);
      assert.strictEqual(exportNameAndPropSet.has('ModuleNs'), false);
      assert.strictEqual(exportNameAndPropSet.has('nsProp1'), true);
      assert.strictEqual(exportNameAndPropSet.has('nsFunc'), true);
      assert.strictEqual(exportNameAndPropSet.has('ModuleClass'), false);
      assert.strictEqual(exportNameAndPropSet.has('classProp1'), true);
      assert.strictEqual(exportNameAndPropSet.has('objProp'), true);
      assert.strictEqual(exportNameAndPropSet.has('innerProp'), true);
      assert.strictEqual(exportNameAndPropSet.has('TestClass'), false);
      assert.strictEqual(exportNameAndPropSet.has('prop4'), false);
      assert.strictEqual(exportNameAndPropSet.has('propObj'), false);
      assert.strictEqual(exportNameAndPropSet.has('innerProp1'), false);
      assert.strictEqual(exportNameAndPropSet.has('TestClass2'), true);
      assert.strictEqual(exportNameAndPropSet.has('prop1'), true);
      assert.strictEqual(exportNameAndPropSet.has('objProp1'), true);
      assert.strictEqual(exportNameAndPropSet.has('innerProp2'), true);
      assert.strictEqual(exportNameAndPropSet.has('default'), true);
      assert.strictEqual(exportNameAndPropSet.has('mc'), true);
      assert.strictEqual(exportNameAndPropSet.has('otherElement1'), true);
      assert.strictEqual(exportNameAndPropSet.has('otherElement2'), false);
      assert.strictEqual(exportNameAndPropSet.has('o2'), true);
      const fileWhiteList: FileWhiteList = projectWhiteListManager!.getFileWhiteListMap().get(filePath)!;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('ModuleNs')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('nsProp1')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('nsFunc')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('ModuleClass')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('classProp1')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('objProp')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('innerProp')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('TestClass')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('prop4')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('propObj')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('innerProp1')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('TestClass2')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('prop1')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('objProp1')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('innerProp2')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('default')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('mc')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('otherElement1')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('otherElement2')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.globalNames.has('o2')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('ModuleNs')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('nsProp1')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('nsFunc')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('ModuleClass')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('classProp1')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('objProp')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('innerProp')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('TestClass')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('prop4')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('propObj')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('innerProp1')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('TestClass2')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('prop1')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('objProp1')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('innerProp2')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('default')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('mc')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('otherElement1')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('otherElement2')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('o2')).to.be.true;
    });
    it('decorate collect test', function () {
      const filePath = 'test/ut/utils/apiTest_collectFileWhiteList/collectFileWhiteList02.ets'
      const fileList: Set<string> = new Set([filePath]);
      let config: IOptions = {
        mNameObfuscation: {
          mEnable: true,
          mReservedProperties: [],
          mRenameProperties: true,
          mKeepStringProperty: false,
          mTopLevel: false,
          mNameGeneratorType: NameGeneratorType.ORDERED,
          mReservedNames: [],
          mReservedToplevelNames: []
        }
      };
      let cachePath = 'test/ut/utils/obfuscation';
      initProjectWhiteListManager(cachePath, false, false, false);
      let projectAndLibs: ReseverdSetForArkguard =
        readProjectPropertiesByCollectedPaths(fileList,
          {
            mNameObfuscation: config.mNameObfuscation,
            mExportObfuscation: true,
            mKeepFileSourceCode: {
              mKeepSourceOfPaths: new Set(),
              mkeepFilesAndDependencies: new Set(),
            }
          }, true, true);
      const fileWhiteList: FileWhiteList = projectWhiteListManager!.getFileWhiteListMap().get(filePath)!;
      if (fileWhiteList?.bytecodeObfuscateKeepInfo?.decoratorMap) {
        const decoratorKeys = Object.keys(fileWhiteList.bytecodeObfuscateKeepInfo.decoratorMap);
        expect(decoratorKeys.length).to.be.greaterThan(0);
      }
    });
  });

  describe('test initScanProjectConfig', function () {
    it('scanDecorator is false', function () {
      let customProfiles = {}
      initScanProjectConfig(customProfiles, false, false)
      expect(scanProjectConfig.scanDecorator).to.be.false;
    });

    it('scanDecorator is default', function () {
      let customProfiles = {}
      initScanProjectConfig(customProfiles, false)
      expect(scanProjectConfig.scanDecorator).to.be.false;
    });

    it('scanDecorator is true', function () {
      let customProfiles = {}
      initScanProjectConfig(customProfiles, false, true)
      expect(scanProjectConfig.scanDecorator).to.be.true;
    });
  });
});
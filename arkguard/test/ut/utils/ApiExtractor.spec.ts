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

import { ApiExtractor } from '../../../src/common/ApiExtractor';
import { assert, expect } from 'chai';
import { initScanProjectConfigByMergeConfig, readProjectPropertiesByCollectedPaths, scanProjectConfig } from '../../../src/common/ApiReader';
import { NameGeneratorType } from '../../../src/generator/NameFactory';
import { MergedConfig, ProjectCollections } from '../../../src/ArkObfuscator';
import { clearProjectWhiteListManager, FileWhiteList, projectWhiteListManager, initProjectWhiteListManager  } from '../../../src/utils/ProjectCollections';
import { AtKeepCollections } from '../../../src/utils/CommonCollections';
import { IOptions } from '../../../src/configs/IOptions';

function collectApi(apiPath: string, apiType: ApiExtractor.ApiType): void {
  clearAll();
  ApiExtractor.traverseApiFiles(apiPath, apiType);
}

function clearAll(): void {
  ApiExtractor.mPropertySet.clear();
  ApiExtractor.mSystemExportSet.clear();
}

describe('test for ApiExtractor', function () {
  describe('test for visitExport', function () {
    it('export {ExportDeclarationClass1, ExportDeclarationClass2}', function () {
      let exportDeclarationAst: string = 'test/ut/utils/apiTest_visitExport/exportDeclarationAst.d.ts';
      collectApi(exportDeclarationAst, ApiExtractor.ApiType.API);
      assert.strictEqual(ApiExtractor.mSystemExportSet.has('ExportDeclarationClass1'), true);
      assert.strictEqual(ApiExtractor.mSystemExportSet.has('ExportDeclarationClass2'), true);
      clearAll();
    });

    it('export Annotation', function () {
      let exportDeclarationAst: string = 'test/ut/utils/apiTest_visitExport/exportAnnotation.d.ets';
      collectApi(exportDeclarationAst, ApiExtractor.ApiType.API);
      assert.strictEqual(ApiExtractor.mSystemExportSet.has('Available1'), true);
      assert.strictEqual(ApiExtractor.mSystemExportSet.has('Available2'), true);
      clearAll();
    });

    it('export {ExportDeclarationClass1 as class1, ExportDeclarationClass2} from `./exportDeclarationFrom`',
     function () {
      let exportDeclarationFromAst: string = 'test/ut/utils/apiTest_visitExport/exportDeclarationFrom.d.ts';
      collectApi(exportDeclarationFromAst, ApiExtractor.ApiType.API);
      assert.strictEqual(ApiExtractor.mSystemExportSet.has('ExportDeclarationClass1'), false);
      assert.strictEqual(ApiExtractor.mSystemExportSet.has('class1'), true);
      assert.strictEqual(ApiExtractor.mSystemExportSet.has('ExportDeclarationClass2'), true);
      clearAll();
    });

    it('export {default as name1, ExportDeclarationClass2, exportName} from `./exportDefaultDeclarationAst`',
     function () {
      let exportDeclarationDefault: string = 'test/ut/utils/apiTest_visitExport/exportDeclarationDefault.d.ts';
      collectApi(exportDeclarationDefault, ApiExtractor.ApiType.API);
      assert.strictEqual(ApiExtractor.mSystemExportSet.has('default'), false);
      assert.strictEqual(ApiExtractor.mSystemExportSet.has('name1'), true);
      assert.strictEqual(ApiExtractor.mSystemExportSet.has('ExportDeclarationClass2'), true);
      assert.strictEqual(ApiExtractor.mSystemExportSet.has('exportName'), true);
      clearAll();
    });

    it('export {ExportDeclarationClass1 as exportName, ExportDeclarationClass2, ExportDeclarationClass3 as default}',
     function () {
      let exportDefaultDeclarationAst: string = 'test/ut/utils/apiTest_visitExport/exportDefaultDeclarationAst.d.ts';
      collectApi(exportDefaultDeclarationAst, ApiExtractor.ApiType.API);
      assert.strictEqual(ApiExtractor.mSystemExportSet.has('ExportDeclarationClass1'), false);
      assert.strictEqual(ApiExtractor.mSystemExportSet.has('exportName'), true);
      assert.strictEqual(ApiExtractor.mSystemExportSet.has('ExportDeclarationClass2'), true);
      assert.strictEqual(ApiExtractor.mSystemExportSet.has('ExportDeclarationClass3'), false);
      assert.strictEqual(ApiExtractor.mSystemExportSet.has('default'), true);
      clearAll();
    });

    it('export * as name1', function () {
      let exportAllAst: string = 'test/ut/utils/apiTest_visitExport/exportAll.d.ts';
      collectApi(exportAllAst, ApiExtractor.ApiType.API);
      assert.strictEqual(ApiExtractor.mSystemExportSet.has('name1'), true);
      clearAll();
    });

    it('export * from `export*.ts`', function () {
      let exportFile: string = 'test/ut/utils/apiTest_visitExport/export.d.ts';
      collectApi(exportFile, ApiExtractor.ApiType.API);
      assert.strictEqual(ApiExtractor.mSystemExportSet.size === 0, true);
      assert.strictEqual(ApiExtractor.mPropertySet.size === 0, true);
      clearAll();
    });

    it('doubleUnderscoreTest', function () {
      let doubleUnderscoreTestAst: string = 'test/ut/utils/apiTest_visitExport/doubleUnderscoreTest.d.ts';
      collectApi(doubleUnderscoreTestAst, ApiExtractor.ApiType.API);
      assert.strictEqual(ApiExtractor.mPropertySet.has('__Admin'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('___Admin'), false);
      assert.strictEqual(ApiExtractor.mPropertySet.has('__Moderator'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('___Moderator'), false);
      assert.strictEqual(ApiExtractor.mPropertySet.has('__User'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('___User'), false);
      assert.strictEqual(ApiExtractor.mPropertySet.has('__name'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('___name'), false);
      assert.strictEqual(ApiExtractor.mPropertySet.has('__Admin'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('___Admin'), false);
      assert.strictEqual(ApiExtractor.mPropertySet.has('__age'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('___age'), false);
      assert.strictEqual(ApiExtractor.mPropertySet.has('__greet'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('___greet'), false);
      assert.strictEqual(ApiExtractor.mPropertySet.has('__typeProp1'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('___typeProp1'), false);
      assert.strictEqual(ApiExtractor.mPropertySet.has('__typeProp2'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('___typeProp2'), false);
      assert.strictEqual(ApiExtractor.mPropertySet.has('__speak'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('___speak'), false);
      assert.strictEqual(ApiExtractor.mPropertySet.has('__appName'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('___appName'), false);
      assert.strictEqual(ApiExtractor.mPropertySet.has('__version'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('___version'), false);
      assert.strictEqual(ApiExtractor.mPropertySet.has('__logDetails'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('___logDetails'), false);
      clearAll();
    });
  });

  describe('test for visitPropertyAndName', function () {
    it('Class Test', function () {
      let filePath: string = 'test/ut/utils/apiTest_visitPropertyAndName/classTest.d.ts';
      let config: MergedConfig = new MergedConfig();
      config.options.stripSystemApiArgs = false;
      initScanProjectConfigByMergeConfig(config);

      collectApi(filePath, ApiExtractor.ApiType.API);
      assert.strictEqual(ApiExtractor.mSystemExportSet.has('TestClass1'), true);
      assert.strictEqual(ApiExtractor.mSystemExportSet.has('TestClass2'), false);
      assert.strictEqual(ApiExtractor.mSystemExportSet.has('AbstractClass'), false);
      assert.strictEqual(ApiExtractor.mPropertySet.has('prop1'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('prop2'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('abstractProp'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('foo1'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('foo2'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('param1'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('param2'), false);
      clearAll();
    });

    it('Interface Test', function () {
      let filePath: string = 'test/ut/utils/apiTest_visitPropertyAndName/interfaceTest.d.ts';
      let config: MergedConfig = new MergedConfig();
      config.options.stripSystemApiArgs = false;
      initScanProjectConfigByMergeConfig(config);

      collectApi(filePath, ApiExtractor.ApiType.API);
      assert.strictEqual(ApiExtractor.mSystemExportSet.has('TestInterface1'), true);
      assert.strictEqual(ApiExtractor.mSystemExportSet.has('TestInterface2'), false);
      assert.strictEqual(ApiExtractor.mPropertySet.has('prop1'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('prop2'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('foo1'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('foo2'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('param1'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('param2'), false);
      clearAll();
    });

    it('TypeLiteral Test', function () {
      let filePath: string = 'test/ut/utils/apiTest_visitPropertyAndName/typeLiteralTest.d.ts';
      let config: MergedConfig = new MergedConfig();
      config.options.stripSystemApiArgs = false;
      initScanProjectConfigByMergeConfig(config);

      collectApi(filePath, ApiExtractor.ApiType.API);
      assert.strictEqual(ApiExtractor.mSystemExportSet.has('TestType1'), true);
      assert.strictEqual(ApiExtractor.mSystemExportSet.has('TestType2'), false);
      assert.strictEqual(ApiExtractor.mPropertySet.has('collectProp1'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('collectProp2'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('testFunc1'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('testFunc2'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('message1'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('message2'), false);
      clearAll();
    });

    it('Enum Test', function () {
      let filePath: string = 'test/ut/utils/apiTest_visitPropertyAndName/enumTest.d.ts';
      let config: MergedConfig = new MergedConfig();
      config.options.stripSystemApiArgs = false;
      initScanProjectConfigByMergeConfig(config);

      collectApi(filePath, ApiExtractor.ApiType.API);
      assert.strictEqual(ApiExtractor.mSystemExportSet.has('TestEnum1'), true);
      assert.strictEqual(ApiExtractor.mSystemExportSet.has('TestEnum2'), false);
      assert.strictEqual(ApiExtractor.mPropertySet.has('PARAM1'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('PARAM2'), true);
      clearAll();
    });

    it('ObjectLiteral Test', function () {
      let filePath: string = 'test/ut/utils/apiTest_visitPropertyAndName/objectLiteral.d.ts';
      let config: MergedConfig = new MergedConfig();
      config.options.stripSystemApiArgs = false;
      initScanProjectConfigByMergeConfig(config);

      collectApi(filePath, ApiExtractor.ApiType.API);
      assert.strictEqual(ApiExtractor.mSystemExportSet.has('obj1'), true);
      assert.strictEqual(ApiExtractor.mSystemExportSet.has('obj2'), false);
      assert.strictEqual(ApiExtractor.mPropertySet.has('prop1'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('prop2'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('prop3'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('prop4'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('innerProp1'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('innerProp2'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('innerProp3'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('innerProp4'), true);
      clearAll();
    });

    it('Module Test', function () {
      let filePath: string = 'test/ut/utils/apiTest_visitPropertyAndName/moduleTest.d.ts';
      let config: MergedConfig = new MergedConfig();
      config.options.stripSystemApiArgs = false;
      initScanProjectConfigByMergeConfig(config);

      collectApi(filePath, ApiExtractor.ApiType.API);
      assert.strictEqual(ApiExtractor.mSystemExportSet.has('ns1'), true);
      assert.strictEqual(ApiExtractor.mSystemExportSet.has('ns2'), false);
      assert.strictEqual(ApiExtractor.mPropertySet.has('TestClass1'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('TestInterface1'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('prop1'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('prop2'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('prop3'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('TestClass2'), false);
      assert.strictEqual(ApiExtractor.mPropertySet.has('TestInterface2'), false);
      assert.strictEqual(ApiExtractor.mPropertySet.has('prop4'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('prop5'), true);
      clearAll();
    });

    it('When "-extra-options strip-system-api-args" option is enabled, no function parameters are collected', function () {
      let filePath: string = 'test/ut/utils/apiTest_visitPropertyAndName/systemApiArgsTest.d.ts';
      let config: MergedConfig = new MergedConfig();
      config.options.stripSystemApiArgs = true;
      initScanProjectConfigByMergeConfig(config);

      collectApi(filePath, ApiExtractor.ApiType.API);
      assert.strictEqual(ApiExtractor.mSystemExportSet.has('TestClass1'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('prop1'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('param1'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('param2'), false);
      assert.strictEqual(ApiExtractor.mPropertySet.has('foo1'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('param5'), false);
      assert.strictEqual(ApiExtractor.mSystemExportSet.has('TestClass2'), false);
      assert.strictEqual(ApiExtractor.mPropertySet.has('prop2'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('param3'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('param4'), false);
      assert.strictEqual(ApiExtractor.mPropertySet.has('foo2'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('param6'), false);
      clearAll();
    });

    it('When "-extra-options strip-system-api-args" option is not enabled, function parameters are collected', function () {
      let filePath: string = 'test/ut/utils/apiTest_visitPropertyAndName/systemApiArgsTest.d.ts';
      let config: MergedConfig = new MergedConfig();
      config.options.stripSystemApiArgs = false;
      initScanProjectConfigByMergeConfig(config);

      collectApi(filePath, ApiExtractor.ApiType.API);
      assert.strictEqual(ApiExtractor.mSystemExportSet.has('TestClass1'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('prop1'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('param1'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('param2'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('foo1'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('param5'), true);
      assert.strictEqual(ApiExtractor.mSystemExportSet.has('TestClass2'), false);
      assert.strictEqual(ApiExtractor.mPropertySet.has('prop2'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('param3'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('param4'), false);
      assert.strictEqual(ApiExtractor.mPropertySet.has('foo2'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('param6'), false);
      clearAll();
    });

    it('When property names with wrong recordkey tag, it will not be added in whitelist', function () {
      let filePath: string = 'test/ut/utils/apiTest_visitPropertyAndName/recordkeyTest.d.ts';
      let config: MergedConfig = new MergedConfig();
      config.options.stripSystemApiArgs = false;
      initScanProjectConfigByMergeConfig(config);

      collectApi(filePath, ApiExtractor.ApiType.API);
      assert.strictEqual(ApiExtractor.mPropertySet.has('testForRecordkey1'), false);
      assert.strictEqual(ApiExtractor.mPropertySet.has('testForRecordkey2'), false);
      clearAll();
    });

    it('When property names with recordkey tag for MethodSignature, it should be added in whitelist', function () {
      let filePath: string = 'test/ut/utils/apiTest_visitPropertyAndName/recordkeyTest.d.ts';
      let config: MergedConfig = new MergedConfig();
      config.options.stripSystemApiArgs = false;
      initScanProjectConfigByMergeConfig(config);

      collectApi(filePath, ApiExtractor.ApiType.API);
      assert.strictEqual(ApiExtractor.mPropertySet.has('objectTest1'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('objectTest2'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('objectTest3'), false);
      assert.strictEqual(ApiExtractor.mPropertySet.has('indexTest1'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('indexTest2'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('indexTest3'), false);
      assert.strictEqual(ApiExtractor.mPropertySet.has('recordTest1'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('recordTest2'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('recordTest3'), false);
      assert.strictEqual(ApiExtractor.mPropertySet.has('anyTest1'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('anyTest2'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('anyTest11'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('anyTest22'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('anyTest3'), false);
      clearAll();
    });

    it('When property names with recordkey tag for PropertySignature, it should be added in whitelist', function () {
      let filePath: string = 'test/ut/utils/apiTest_visitPropertyAndName/recordkeyTest.d.ts';
      let config: MergedConfig = new MergedConfig();
      config.options.stripSystemApiArgs = false;
      initScanProjectConfigByMergeConfig(config);

      collectApi(filePath, ApiExtractor.ApiType.API);
      assert.strictEqual(ApiExtractor.mPropertySet.has('propertySignatureTest1'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('propertySignatureTest2'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('propertySignatureTest3'), false);
      clearAll();
    });

    it('When property names with recordkey tag for Constructor, it should be added in whitelist', function () {
      let filePath: string = 'test/ut/utils/apiTest_visitPropertyAndName/recordkeyTest.d.ts';
      let config: MergedConfig = new MergedConfig();
      config.options.stripSystemApiArgs = false;
      initScanProjectConfigByMergeConfig(config);

      collectApi(filePath, ApiExtractor.ApiType.API);
      assert.strictEqual(ApiExtractor.mPropertySet.has('constructorTest1'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('constructorTest2'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('constructorTest3'), false);
      clearAll();
    });

    it('When property names with recordkey tag for PropertyDeclaration, it should be added in whitelist', function () {
      let filePath: string = 'test/ut/utils/apiTest_visitPropertyAndName/recordkeyTest.d.ts';
      let config: MergedConfig = new MergedConfig();
      config.options.stripSystemApiArgs = false;
      initScanProjectConfigByMergeConfig(config);

      collectApi(filePath, ApiExtractor.ApiType.API);
      assert.strictEqual(ApiExtractor.mPropertySet.has('propertyTest1'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('propertyTest2'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('propertyTest3'), false);
      clearAll();
    });

    it('When property names with recordkey tag for MethodDeclaration, it should be added in whitelist', function () {
      let filePath: string = 'test/ut/utils/apiTest_visitPropertyAndName/recordkeyTest.d.ts';
      let config: MergedConfig = new MergedConfig();
      config.options.stripSystemApiArgs = false;
      initScanProjectConfigByMergeConfig(config);

      collectApi(filePath, ApiExtractor.ApiType.API);
      assert.strictEqual(ApiExtractor.mPropertySet.has('methodTest1'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('methodTest2'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('methodTest3'), false);
      clearAll();
    });

    it('When property names with recordkey tag for SetAccessor, it should be added in whitelist', function () {
      let filePath: string = 'test/ut/utils/apiTest_visitPropertyAndName/recordkeyTest.d.ts';
      let config: MergedConfig = new MergedConfig();
      config.options.stripSystemApiArgs = false;
      initScanProjectConfigByMergeConfig(config);

      collectApi(filePath, ApiExtractor.ApiType.API);
      assert.strictEqual(ApiExtractor.mPropertySet.has('setterTest1'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('setterTest2'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('setterTest3'), false);
      clearAll();
    });

    it('When property names with recordkey tag for FunctionDeclaration, it should be added in whitelist', function () {
      let filePath: string = 'test/ut/utils/apiTest_visitPropertyAndName/recordkeyTest.d.ts';
      let config: MergedConfig = new MergedConfig();
      config.options.stripSystemApiArgs = false;
      initScanProjectConfigByMergeConfig(config);

      collectApi(filePath, ApiExtractor.ApiType.API);
      assert.strictEqual(ApiExtractor.mPropertySet.has('functionTest1'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('functionTest2'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('functionTest3'), false);
      clearAll();
    });

    it('When property names with recordkey tag for VariableStatement, it should be added in whitelist', function () {
      let filePath: string = 'test/ut/utils/apiTest_visitPropertyAndName/recordkeyTest.d.ts';
      let config: MergedConfig = new MergedConfig();
      config.options.stripSystemApiArgs = false;
      initScanProjectConfigByMergeConfig(config);

      collectApi(filePath, ApiExtractor.ApiType.API);
      assert.strictEqual(ApiExtractor.mPropertySet.has('variableStatementTest1'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('variableStatementTest2'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('variableStatementTest3'), false);
      clearAll();
    });

    it('When property names with recordkey tag for TypeAliasDeclaration, it should be added in whitelist', function () {
      let filePath: string = 'test/ut/utils/apiTest_visitPropertyAndName/recordkeyTest.d.ts';
      let config: MergedConfig = new MergedConfig();
      config.options.stripSystemApiArgs = false;
      initScanProjectConfigByMergeConfig(config);

      collectApi(filePath, ApiExtractor.ApiType.API);
      assert.strictEqual(ApiExtractor.mPropertySet.has('variableStatementTest1'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('variableStatementTest2'), true);
      assert.strictEqual(ApiExtractor.mPropertySet.has('variableStatementTest3'), false);
      clearAll();
    });
  });

  describe('test for visitProjectExport', function () {
    const fileList: Set<string> = new Set([
      "test/ut/utils/module_exports_test/exportFile1.js",
      "test/ut/utils/oh_modules/exportFile.js"
    ]);
    it('test for module.exports(property)', function () {
      let projectAndLibs = readProjectPropertiesByCollectedPaths(fileList,
        {
          mNameObfuscation: {
            mEnable: true,
            mReservedProperties: [],
            mRenameProperties: true,
            mKeepStringProperty: false,
            mNameGeneratorType: NameGeneratorType.ORDERED,
            mReservedNames: [],
            mReservedToplevelNames: []
          },
          mExportObfuscation: false,
          mKeepFileSourceCode: {
            mKeepSourceOfPaths: new Set(),
            mkeepFilesAndDependencies: new Set(),
          }
        }, true);
      let reservedProperties = projectAndLibs.exportNameAndPropSet == undefined? new Set<string> : projectAndLibs.exportNameAndPropSet;
      let reservedExportNames = projectAndLibs.exportNameSet == undefined? new Set<string> : projectAndLibs.exportNameSet;
      assert.strictEqual(reservedProperties.has('projectPropertyAssignment1'), true);
      assert.strictEqual(reservedProperties.has('projectPropertyAssignment2'), true);
      assert.strictEqual(reservedProperties.has('projectPropertyAssignment3'), true);
      assert.strictEqual(reservedProperties.has('projectPropertyAssignment4'), true);
      assert.strictEqual(reservedProperties.has('projectIndirectObj'), false);
      assert.strictEqual(reservedProperties.has('indirectProp1'), true);
      assert.strictEqual(reservedProperties.has('projectShorthand'), true);
      assert.strictEqual(reservedProperties.has('projectShorthandProp'), true);
      assert.strictEqual(reservedProperties.has('projectMethod1'), true);
      assert.strictEqual(reservedProperties.has('projectMethod2'), true);
      assert.strictEqual(reservedProperties.has('projectMethod3'), true);
      assert.strictEqual(reservedProperties.has('projectGetProp1'), true);
      assert.strictEqual(reservedProperties.has('projectGetProp2'), true);
      assert.strictEqual(reservedProperties.has('projectGetProp3'), true);
      assert.strictEqual(reservedProperties.has('projectSetProp1'), true);
      assert.strictEqual(reservedProperties.has('projectSetProp2'), true);
      assert.strictEqual(reservedProperties.has('projectSetProp3'), true);
      assert.strictEqual(reservedProperties.has('projectExportElement1'), true);
      assert.strictEqual(reservedProperties.has('projectExportElement2'), true);
      assert.strictEqual(reservedProperties.has('indirectClass1'), false);
      assert.strictEqual(reservedProperties.has('indirectProp2'), true);
      assert.strictEqual(reservedProperties.has('projectExportElement3'), true);
      assert.strictEqual(reservedProperties.has('indirectClass2'), false);
      assert.strictEqual(reservedProperties.has('indirectProp3'), true);
      assert.strictEqual(reservedProperties.has('projectExportElement4'), true);
      assert.strictEqual(reservedProperties.has('indirectProp4'), true);
      assert.strictEqual(reservedProperties.has('projectExportElement5'), true);
      assert.strictEqual(reservedProperties.has('projectExportElement6'), true);
      assert.strictEqual(reservedProperties.has('projectExportElement7'), true);
      assert.strictEqual(reservedProperties.has('indirectClass3'), false);
      assert.strictEqual(reservedProperties.has('indirectProp5'), true);
      assert.strictEqual(reservedProperties.has('projectExportElement8'), true);
      assert.strictEqual(reservedProperties.has('indirectClass4'), false);
      assert.strictEqual(reservedProperties.has('indirectProp6'), true);
      assert.strictEqual(reservedProperties.has('projectExportElement9'), true);
      assert.strictEqual(reservedProperties.has('indirectProp7'), true);
      assert.strictEqual(reservedProperties.has('ohPropertyAssignment1'), true);
      assert.strictEqual(reservedProperties.has('ohPropertyAssignment2'), true);
      assert.strictEqual(reservedProperties.has('ohPropertyAssignment3'), true);
      assert.strictEqual(reservedProperties.has('ohPropertyAssignment4'), true);
      assert.strictEqual(reservedProperties.has('ohIndirectObj'), false);
      assert.strictEqual(reservedProperties.has('ohIndirectProp1'), true);
      assert.strictEqual(reservedProperties.has('ohShorthand'), true);
      assert.strictEqual(reservedProperties.has('ohShorthandProp'), true);
      assert.strictEqual(reservedProperties.has('ohMethod1'), true);
      assert.strictEqual(reservedProperties.has('ohMethod2'), true);
      assert.strictEqual(reservedProperties.has('ohMethod3'), true);
      assert.strictEqual(reservedProperties.has('ohGetProp1'), true);
      assert.strictEqual(reservedProperties.has('ohGetProp2'), true);
      assert.strictEqual(reservedProperties.has('ohGetProp3'), true);
      assert.strictEqual(reservedProperties.has('ohSetProp1'), true);
      assert.strictEqual(reservedProperties.has('ohSetProp2'), true);
      assert.strictEqual(reservedProperties.has('ohSetProp3'), true);
      assert.strictEqual(reservedProperties.has('ohExportElement1'), true);
      assert.strictEqual(reservedProperties.has('ohExportElement2'), true);
      assert.strictEqual(reservedProperties.has('ohIndirectClass1'), false);
      assert.strictEqual(reservedProperties.has('ohIndirectProp2'), true);
      assert.strictEqual(reservedProperties.has('ohExportElement3'), true);
      assert.strictEqual(reservedProperties.has('ohIndirectClass2'), false);
      assert.strictEqual(reservedProperties.has('ohIndirectProp3'), true);
      assert.strictEqual(reservedProperties.has('ohExportElement4'), true);
      assert.strictEqual(reservedProperties.has('ohIndirectProp4'), true);
      assert.strictEqual(reservedProperties.has('ohExportElement5'), true);
      assert.strictEqual(reservedProperties.has('ohExportElement6'), true);
      assert.strictEqual(reservedProperties.has('ohExportElement7'), true);
      assert.strictEqual(reservedProperties.has('ohIndirectClass3'), false);
      assert.strictEqual(reservedProperties.has('ohIndirectProp5'), true);
      assert.strictEqual(reservedProperties.has('ohExportElement8'), true);
      assert.strictEqual(reservedProperties.has('ohIndirectClass4'), false);
      assert.strictEqual(reservedProperties.has('ohIndirectProp6'), true);
      assert.strictEqual(reservedExportNames.size === 0, true);
    });

    it('test for module.exports(export)', function () {
      let projectAndLibs = readProjectPropertiesByCollectedPaths(fileList,
        {
          mNameObfuscation: {
            mEnable: true,
            mReservedProperties: [],
            mRenameProperties: false,
            mKeepStringProperty: false,
            mNameGeneratorType: NameGeneratorType.ORDERED,
            mReservedNames: [],
            mReservedToplevelNames: []
          },
          mExportObfuscation: true,
          mKeepFileSourceCode: {
            mKeepSourceOfPaths: new Set(),
            mkeepFilesAndDependencies: new Set(["test/ut/utils/module_exports_test/exportFile3.js"]),
          }
        }, true);
      let reservedProperties = projectAndLibs.exportNameAndPropSet == undefined? new Set<string> : projectAndLibs.exportNameAndPropSet;
      let reservedExportNames = projectAndLibs.exportNameSet == undefined? new Set<string> : projectAndLibs.exportNameSet;
      assert.strictEqual(reservedExportNames.has('projectPropertyAssignment1'), false);
      assert.strictEqual(reservedExportNames.has('projectPropertyAssignment2'), false);
      assert.strictEqual(reservedExportNames.has('projectPropertyAssignment3'), false);
      assert.strictEqual(reservedExportNames.has('projectPropertyAssignment4'), false);
      assert.strictEqual(reservedExportNames.has('projectIndirectObj'), false);
      assert.strictEqual(reservedExportNames.has('indirectProp1'), false);
      assert.strictEqual(reservedExportNames.has('projectShorthand'), false);
      assert.strictEqual(reservedExportNames.has('projectShorthandProp'), false);
      assert.strictEqual(reservedExportNames.has('projectMethod1'), false);
      assert.strictEqual(reservedExportNames.has('projectMethod2'), false);
      assert.strictEqual(reservedExportNames.has('projectMethod3'), false);
      assert.strictEqual(reservedExportNames.has('projectGetProp1'), false);
      assert.strictEqual(reservedExportNames.has('projectGetProp2'), false);
      assert.strictEqual(reservedExportNames.has('projectGetProp3'), false);
      assert.strictEqual(reservedExportNames.has('projectSetProp1'), false);
      assert.strictEqual(reservedExportNames.has('projectSetProp2'), false);
      assert.strictEqual(reservedExportNames.has('projectSetProp3'), false);
      assert.strictEqual(reservedExportNames.has('projectExportElement1'), false);
      assert.strictEqual(reservedExportNames.has('projectExportElement2'), false);
      assert.strictEqual(reservedExportNames.has('indirectClass1'), false);
      assert.strictEqual(reservedExportNames.has('indirectProp2'), false);
      assert.strictEqual(reservedExportNames.has('projectExportElement3'), false);
      assert.strictEqual(reservedExportNames.has('indirectClass2'), false);
      assert.strictEqual(reservedExportNames.has('indirectProp3'), false);
      assert.strictEqual(reservedExportNames.has('projectExportElement4'), false);
      assert.strictEqual(reservedExportNames.has('indirectProp4'), false);
      assert.strictEqual(reservedExportNames.has('projectExportElement5'), false);
      assert.strictEqual(reservedExportNames.has('projectExportElement6'), false);
      assert.strictEqual(reservedExportNames.has('projectExportElement7'), false);
      assert.strictEqual(reservedExportNames.has('indirectClass3'), false);
      assert.strictEqual(reservedExportNames.has('indirectProp5'), false);
      assert.strictEqual(reservedExportNames.has('projectExportElement8'), false);
      assert.strictEqual(reservedExportNames.has('indirectClass4'), false);
      assert.strictEqual(reservedExportNames.has('indirectProp6'), false);
      assert.strictEqual(reservedExportNames.has('projectExportElement9'), false);
      assert.strictEqual(reservedExportNames.has('indirectProp7'), false);
      assert.strictEqual(reservedExportNames.has('ohPropertyAssignment1'), true);
      assert.strictEqual(reservedExportNames.has('ohPropertyAssignment2'), true);
      assert.strictEqual(reservedExportNames.has('ohPropertyAssignment3'), true);
      assert.strictEqual(reservedExportNames.has('ohPropertyAssignment4'), true);
      assert.strictEqual(reservedExportNames.has('ohIndirectObj'), false);
      assert.strictEqual(reservedExportNames.has('ohIndirectProp1'), false);
      assert.strictEqual(reservedExportNames.has('ohShorthand'), true);
      assert.strictEqual(reservedExportNames.has('ohShorthandProp'), false);
      assert.strictEqual(reservedExportNames.has('ohMethod1'), true);
      assert.strictEqual(reservedExportNames.has('ohMethod2'), true);
      assert.strictEqual(reservedExportNames.has('ohMethod3'), true);
      assert.strictEqual(reservedExportNames.has('ohGetProp1'), true);
      assert.strictEqual(reservedExportNames.has('ohGetProp2'), true);
      assert.strictEqual(reservedExportNames.has('ohGetProp3'), true);
      assert.strictEqual(reservedExportNames.has('ohSetProp1'), true);
      assert.strictEqual(reservedExportNames.has('ohSetProp2'), true);
      assert.strictEqual(reservedExportNames.has('ohSetProp3'), true);
      assert.strictEqual(reservedExportNames.has('ohExportElement1'), true);
      assert.strictEqual(reservedExportNames.has('ohExportElement2'), true);
      assert.strictEqual(reservedExportNames.has('ohIndirectClass1'), true);
      assert.strictEqual(reservedExportNames.has('ohIndirectProp2'), false);
      assert.strictEqual(reservedExportNames.has('ohExportElement3'), true);
      assert.strictEqual(reservedExportNames.has('ohIndirectClass2'), false);
      assert.strictEqual(reservedExportNames.has('ohIndirectProp3'), false);
      assert.strictEqual(reservedExportNames.has('ohExportElement4'), true);
      assert.strictEqual(reservedExportNames.has('ohIndirectProp4'), false);
      assert.strictEqual(reservedExportNames.has('ohExportElement5'), true);
      assert.strictEqual(reservedExportNames.has('ohExportElement6'), true);
      assert.strictEqual(reservedExportNames.has('ohExportElement7'), true);
      assert.strictEqual(reservedExportNames.has('ohIndirectClass3'), true);
      assert.strictEqual(reservedExportNames.has('ohIndirectProp5'), false);
      assert.strictEqual(reservedExportNames.has('ohExportElement8'), true);
      assert.strictEqual(reservedExportNames.has('ohIndirectClass4'), false);
      assert.strictEqual(reservedExportNames.has('ohIndirectProp6'), false);
      assert.strictEqual(reservedExportNames.has('ohExportElement9'), true);
      assert.strictEqual(reservedExportNames.has('ohIndirectProp7'), false);
      assert.strictEqual(reservedProperties.size === 0, true);
    });

    it('test for NamespaceExport(export)', function () {
      const fileList: Set<string> = new Set([
      'test/ut/utils/oh_modules/NamespaceExport.d.ts'
      ]);
      let projectAndLibs = readProjectPropertiesByCollectedPaths(fileList,
        {
          mNameObfuscation: {
            mEnable: true,
            mReservedProperties: [],
            mRenameProperties: false,
            mKeepStringProperty: false,
            mNameGeneratorType: NameGeneratorType.ORDERED,
            mReservedNames: [],
            mReservedToplevelNames: []
          },
          mExportObfuscation: true,
        }, true);
      let reservedProperties = projectAndLibs.exportNameAndPropSet == undefined? new Set<string> : projectAndLibs.exportNameAndPropSet;
      let reservedExportNames = projectAndLibs.exportNameSet == undefined? new Set<string> : projectAndLibs.exportNameSet;
      assert.strictEqual(reservedExportNames.has('Utils'), true);
      assert.strictEqual(reservedExportNames.size === 1, true);
      assert.strictEqual(reservedProperties.size === 0, true);
    });

    it('test for module.exports(property + export)', function () {
      let projectAndLibs = readProjectPropertiesByCollectedPaths(fileList,
        {
          mNameObfuscation: {
            mEnable: true,
            mReservedProperties: [],
            mRenameProperties: true,
            mKeepStringProperty: false,
            mNameGeneratorType: NameGeneratorType.ORDERED,
            mReservedNames: [],
            mReservedToplevelNames: []
          },
          mExportObfuscation: true,
          mKeepFileSourceCode: {
            mKeepSourceOfPaths: new Set(),
            mkeepFilesAndDependencies: new Set(["test/ut/utils/module_exports_test/exportFile3.js"]),
          }
        }, true);
      let reservedProperties = projectAndLibs.exportNameAndPropSet == undefined? new Set<string> : projectAndLibs.exportNameAndPropSet;
      let reservedExportNames = projectAndLibs.exportNameSet == undefined? new Set<string> : projectAndLibs.exportNameSet;
      assert.strictEqual(reservedProperties.has('projectPropertyAssignment1'), false);
      assert.strictEqual(reservedProperties.has('projectPropertyAssignment2'), false);
      assert.strictEqual(reservedProperties.has('projectPropertyAssignment3'), false);
      assert.strictEqual(reservedProperties.has('projectPropertyAssignment4'), false);
      assert.strictEqual(reservedProperties.has('projectIndirectObj'), false);
      assert.strictEqual(reservedProperties.has('indirectProp1'), false);
      assert.strictEqual(reservedProperties.has('projectShorthand'), false);
      assert.strictEqual(reservedProperties.has('projectShorthandProp'), false);
      assert.strictEqual(reservedProperties.has('projectMethod1'), false);
      assert.strictEqual(reservedProperties.has('projectMethod2'), false);
      assert.strictEqual(reservedProperties.has('projectMethod3'), false);
      assert.strictEqual(reservedProperties.has('projectGetProp1'), false);
      assert.strictEqual(reservedProperties.has('projectGetProp2'), false);
      assert.strictEqual(reservedProperties.has('projectGetProp3'), false);
      assert.strictEqual(reservedProperties.has('projectSetProp1'), false);
      assert.strictEqual(reservedProperties.has('projectSetProp2'), false);
      assert.strictEqual(reservedProperties.has('projectSetProp3'), false);
      assert.strictEqual(reservedProperties.has('projectExportElement1'), false);
      assert.strictEqual(reservedProperties.has('projectExportElement2'), false);
      assert.strictEqual(reservedProperties.has('indirectClass1'), false);
      assert.strictEqual(reservedProperties.has('indirectProp2'), false);
      assert.strictEqual(reservedProperties.has('projectExportElement3'), false);
      assert.strictEqual(reservedProperties.has('indirectClass2'), false);
      assert.strictEqual(reservedProperties.has('indirectProp3'), false);
      assert.strictEqual(reservedProperties.has('projectExportElement4'), false);
      assert.strictEqual(reservedProperties.has('indirectProp4'), false);
      assert.strictEqual(reservedProperties.has('projectExportElement5'), false);
      assert.strictEqual(reservedProperties.has('projectExportElement6'), false);
      assert.strictEqual(reservedProperties.has('projectExportElement7'), false);
      assert.strictEqual(reservedProperties.has('indirectClass3'), false);
      assert.strictEqual(reservedProperties.has('indirectProp5'), false);
      assert.strictEqual(reservedProperties.has('projectExportElement8'), false);
      assert.strictEqual(reservedProperties.has('indirectClass4'), false);
      assert.strictEqual(reservedProperties.has('indirectProp6'), false);
      assert.strictEqual(reservedProperties.has('projectExportElement9'), false);
      assert.strictEqual(reservedProperties.has('indirectProp7'), false);
      assert.strictEqual(reservedProperties.has('ohPropertyAssignment1'), true);
      assert.strictEqual(reservedProperties.has('ohPropertyAssignment2'), true);
      assert.strictEqual(reservedProperties.has('ohPropertyAssignment3'), true);
      assert.strictEqual(reservedProperties.has('ohPropertyAssignment4'), true);
      assert.strictEqual(reservedProperties.has('ohIndirectObj'), false);
      assert.strictEqual(reservedProperties.has('ohIndirectProp1'), true);
      assert.strictEqual(reservedProperties.has('ohShorthand'), true);
      assert.strictEqual(reservedProperties.has('ohShorthandProp'), true);
      assert.strictEqual(reservedProperties.has('ohMethod1'), true);
      assert.strictEqual(reservedProperties.has('ohMethod2'), true);
      assert.strictEqual(reservedProperties.has('ohMethod3'), true);
      assert.strictEqual(reservedProperties.has('ohGetProp1'), true);
      assert.strictEqual(reservedProperties.has('ohGetProp2'), true);
      assert.strictEqual(reservedProperties.has('ohGetProp3'), true);
      assert.strictEqual(reservedProperties.has('ohSetProp1'), true);
      assert.strictEqual(reservedProperties.has('ohSetProp2'), true);
      assert.strictEqual(reservedProperties.has('ohSetProp3'), true);
      assert.strictEqual(reservedProperties.has('ohExportElement1'), true);
      assert.strictEqual(reservedProperties.has('ohExportElement2'), true);
      assert.strictEqual(reservedProperties.has('ohIndirectClass1'), false);
      assert.strictEqual(reservedProperties.has('ohIndirectProp2'), true);
      assert.strictEqual(reservedProperties.has('ohExportElement3'), true);
      assert.strictEqual(reservedProperties.has('ohIndirectClass2'), false);
      assert.strictEqual(reservedProperties.has('ohIndirectProp3'), true);
      assert.strictEqual(reservedProperties.has('ohExportElement4'), true);
      assert.strictEqual(reservedProperties.has('ohIndirectProp4'), true);
      assert.strictEqual(reservedProperties.has('ohExportElement5'), true);
      assert.strictEqual(reservedProperties.has('ohExportElement6'), true);
      assert.strictEqual(reservedProperties.has('ohExportElement7'), true);
      assert.strictEqual(reservedProperties.has('ohIndirectClass3'), false);
      assert.strictEqual(reservedProperties.has('ohIndirectProp5'), true);
      assert.strictEqual(reservedProperties.has('ohExportElement8'), true);
      assert.strictEqual(reservedProperties.has('ohIndirectClass4'), false);
      assert.strictEqual(reservedProperties.has('ohIndirectProp6'), true);
      assert.strictEqual(reservedExportNames.has('projectPropertyAssignment1'), false);
      assert.strictEqual(reservedExportNames.has('projectPropertyAssignment2'), false);
      assert.strictEqual(reservedExportNames.has('projectPropertyAssignment3'), false);
      assert.strictEqual(reservedExportNames.has('projectPropertyAssignment4'), false);
      assert.strictEqual(reservedExportNames.has('projectIndirectObj'), false);
      assert.strictEqual(reservedExportNames.has('indirectProp1'), false);
      assert.strictEqual(reservedExportNames.has('projectShorthand'), false);
      assert.strictEqual(reservedExportNames.has('projectShorthandProp'), false);
      assert.strictEqual(reservedExportNames.has('projectMethod1'), false);
      assert.strictEqual(reservedExportNames.has('projectMethod2'), false);
      assert.strictEqual(reservedExportNames.has('projectMethod3'), false);
      assert.strictEqual(reservedExportNames.has('projectGetProp1'), false);
      assert.strictEqual(reservedExportNames.has('projectGetProp2'), false);
      assert.strictEqual(reservedExportNames.has('projectGetProp3'), false);
      assert.strictEqual(reservedExportNames.has('projectSetProp1'), false);
      assert.strictEqual(reservedExportNames.has('projectSetProp2'), false);
      assert.strictEqual(reservedExportNames.has('projectSetProp3'), false);
      assert.strictEqual(reservedExportNames.has('projectExportElement1'), false);
      assert.strictEqual(reservedExportNames.has('projectExportElement2'), false);
      assert.strictEqual(reservedExportNames.has('indirectClass1'), false);
      assert.strictEqual(reservedExportNames.has('indirectProp2'), false);
      assert.strictEqual(reservedExportNames.has('projectExportElement3'), false);
      assert.strictEqual(reservedExportNames.has('indirectClass2'), false);
      assert.strictEqual(reservedExportNames.has('indirectProp3'), false);
      assert.strictEqual(reservedExportNames.has('projectExportElement4'), false);
      assert.strictEqual(reservedExportNames.has('indirectProp4'), false);
      assert.strictEqual(reservedExportNames.has('projectExportElement5'), false);
      assert.strictEqual(reservedExportNames.has('projectExportElement6'), false);
      assert.strictEqual(reservedExportNames.has('projectExportElement7'), false);
      assert.strictEqual(reservedExportNames.has('indirectClass3'), false);
      assert.strictEqual(reservedExportNames.has('indirectProp5'), false);
      assert.strictEqual(reservedExportNames.has('projectExportElement8'), false);
      assert.strictEqual(reservedExportNames.has('indirectClass4'), false);
      assert.strictEqual(reservedExportNames.has('indirectProp6'), false);
      assert.strictEqual(reservedExportNames.has('projectExportElement9'), false);
      assert.strictEqual(reservedExportNames.has('indirectProp7'), false);
      assert.strictEqual(reservedExportNames.has('ohPropertyAssignment1'), true);
      assert.strictEqual(reservedExportNames.has('ohPropertyAssignment2'), true);
      assert.strictEqual(reservedExportNames.has('ohPropertyAssignment3'), true);
      assert.strictEqual(reservedExportNames.has('ohPropertyAssignment4'), true);
      assert.strictEqual(reservedExportNames.has('ohIndirectObj'), false);
      assert.strictEqual(reservedExportNames.has('ohIndirectProp1'), false);
      assert.strictEqual(reservedExportNames.has('ohShorthand'), true);
      assert.strictEqual(reservedExportNames.has('ohShorthandProp'), false);
      assert.strictEqual(reservedExportNames.has('ohMethod1'), true);
      assert.strictEqual(reservedExportNames.has('ohMethod2'), true);
      assert.strictEqual(reservedExportNames.has('ohMethod3'), true);
      assert.strictEqual(reservedExportNames.has('ohGetProp1'), true);
      assert.strictEqual(reservedExportNames.has('ohGetProp2'), true);
      assert.strictEqual(reservedExportNames.has('ohGetProp3'), true);
      assert.strictEqual(reservedExportNames.has('ohSetProp1'), true);
      assert.strictEqual(reservedExportNames.has('ohSetProp2'), true);
      assert.strictEqual(reservedExportNames.has('ohSetProp3'), true);
      assert.strictEqual(reservedExportNames.has('ohExportElement1'), true);
      assert.strictEqual(reservedExportNames.has('ohExportElement2'), true);
      assert.strictEqual(reservedExportNames.has('ohIndirectClass1'), true);
      assert.strictEqual(reservedExportNames.has('ohIndirectProp2'), false);
      assert.strictEqual(reservedExportNames.has('ohExportElement3'), true);
      assert.strictEqual(reservedExportNames.has('ohIndirectClass2'), false);
      assert.strictEqual(reservedExportNames.has('ohIndirectProp3'), false);
      assert.strictEqual(reservedExportNames.has('ohExportElement4'), true);
      assert.strictEqual(reservedExportNames.has('ohIndirectProp4'), false);
      assert.strictEqual(reservedExportNames.has('ohExportElement5'), true);
      assert.strictEqual(reservedExportNames.has('ohExportElement6'), true);
      assert.strictEqual(reservedExportNames.has('ohExportElement7'), true);
      assert.strictEqual(reservedExportNames.has('ohIndirectClass3'), true);
      assert.strictEqual(reservedExportNames.has('ohIndirectProp5'), false);
      assert.strictEqual(reservedExportNames.has('ohExportElement8'), true);
      assert.strictEqual(reservedExportNames.has('ohIndirectClass4'), false);
      assert.strictEqual(reservedExportNames.has('ohIndirectProp6'), false);
      assert.strictEqual(reservedExportNames.has('ohExportElement9'), true);
      assert.strictEqual(reservedExportNames.has('ohIndirectProp7'), false);
    });
  });

  describe('test for visitNodeForConstructorProperty', function () {
    it('should collect constructor properties', function () {
      ApiExtractor.mConstructorPropertySet = new Set();
      let constructorPropertyAst: string = 'test/ut/utils/apiTest_visitConstructorProperty/constructorProperty.ts';
      let cachePath = 'test/ut/utils/obfuscation';
      initProjectWhiteListManager(cachePath, false, false);
      collectApi(constructorPropertyAst, ApiExtractor.ApiType.CONSTRUCTOR_PROPERTY);
      const fileWhiteList: FileWhiteList | undefined = projectWhiteListManager?.getFileWhiteListMap().get(constructorPropertyAst);
      projectWhiteListManager?.createOrUpdateWhiteListCaches();
      expect(fileWhiteList!.fileReservedInfo.propertyParams.has('para1')).to.be.true;
      expect(fileWhiteList!.fileReservedInfo.propertyParams.has('para2')).to.be.true;
      expect(fileWhiteList!.fileReservedInfo.propertyParams.has('para3')).to.be.true;
      expect(fileWhiteList!.fileReservedInfo.propertyParams.has('para4')).to.be.true;
      expect(ApiExtractor.mConstructorPropertySet.has('para1')).to.be.true;
      expect(ApiExtractor.mConstructorPropertySet.has('para2')).to.be.true;
      expect(ApiExtractor.mConstructorPropertySet.has('para3')).to.be.true;
      expect(ApiExtractor.mConstructorPropertySet.has('para4')).to.be.true;
      clearAll();
      ApiExtractor.mConstructorPropertySet.clear();
    })
  })

  describe('test for visitEnumMembers', function () {
    it('should collect enum members', function () {
      let enumMembersAst: string = 'test/ut/utils/apiTest_visitEnumMembers/enumMembers.ts';
      let cachePath = 'test/ut/utils/obfuscation';
      initProjectWhiteListManager(cachePath, false, false);
      collectApi(enumMembersAst, ApiExtractor.ApiType.PROJECT);
      const fileWhiteList: FileWhiteList | undefined = projectWhiteListManager?.getFileWhiteListMap().get(enumMembersAst);
      projectWhiteListManager?.createOrUpdateWhiteListCaches();
      expect(fileWhiteList!.fileReservedInfo.enumProperties.has('A1')).to.be.false;
      expect(fileWhiteList!.fileReservedInfo.enumProperties.has('A2')).to.be.false;
      expect(fileWhiteList!.fileReservedInfo.enumProperties.has('A3')).to.be.false;
      expect(fileWhiteList!.fileReservedInfo.enumProperties.has('B1')).to.be.true;
      expect(fileWhiteList!.fileReservedInfo.enumProperties.has('B2')).to.be.true;
      expect(fileWhiteList!.fileReservedInfo.enumProperties.has('B3')).to.be.true;
      expect(fileWhiteList!.fileReservedInfo.enumProperties.has('C1')).to.be.true;
      expect(fileWhiteList!.fileReservedInfo.enumProperties.has('C2')).to.be.true;
      expect(fileWhiteList!.fileReservedInfo.enumProperties.has('C3')).to.be.true;
      expect(ApiExtractor.mEnumMemberSet.has('A1')).to.be.false;
      expect(ApiExtractor.mEnumMemberSet.has('A2')).to.be.false;
      expect(ApiExtractor.mEnumMemberSet.has('A3')).to.be.false;
      expect(ApiExtractor.mEnumMemberSet.has('B1')).to.be.true;
      expect(ApiExtractor.mEnumMemberSet.has('B2')).to.be.true;
      expect(ApiExtractor.mEnumMemberSet.has('B3')).to.be.true;
      expect(ApiExtractor.mEnumMemberSet.has('C1')).to.be.true;
      expect(ApiExtractor.mEnumMemberSet.has('C2')).to.be.true;
      expect(ApiExtractor.mEnumMemberSet.has('C3')).to.be.true;
      clearAll();
      ApiExtractor.mEnumMemberSet.clear();
    })
    it('should not collect enum members of js file', function () {
      let enumMembersAst: string = 'test/ut/utils/apiTest_visitEnumMembers/enumMembers.js';
      collectApi(enumMembersAst, ApiExtractor.ApiType.PROJECT);
      expect(ApiExtractor.mEnumMemberSet.size === 0).to.be.true;
    })
  })

  describe('test for collectNamesWithAtKeep', function () {
    beforeEach(() => {
      let cachePath = 'test/ut/utils/obfuscation';
      initProjectWhiteListManager(cachePath, false, true);
    })
    afterEach(() => {
      clearProjectWhiteListManager();
    })

    it('should not collect atKeepNames if not enabled', function () {
      let filePath: string = 'test/ut/utils/apiTest_visitAtKeepNames/atKeepClass01.ts';
      AtKeepCollections.clear();
      scanProjectConfig.mEnableAtKeep = false;
      collectApi(filePath, ApiExtractor.ApiType.PROJECT);
      expect(AtKeepCollections.keepSymbol.globalNames.size).to.be.equal(0);
      expect(AtKeepCollections.keepSymbol.propertyNames.size).to.be.equal(0);
      expect(AtKeepCollections.keepAsConsumer.globalNames.size).to.be.equal(0);
      expect(AtKeepCollections.keepAsConsumer.propertyNames.size).to.be.equal(0);
    })
    it('should collect keepSymbol names from class if enabled', function () {
      let filePath: string = 'test/ut/utils/apiTest_visitAtKeepNames/atKeepClass01.ts';
      AtKeepCollections.clear();
      scanProjectConfig.mEnableAtKeep = true;
      collectApi(filePath, ApiExtractor.ApiType.PROJECT);
      expect(AtKeepCollections.keepSymbol.globalNames.size).to.be.equal(7);
      expect(AtKeepCollections.keepSymbol.globalNames.has('MyClass01')).to.be.true;
      expect(AtKeepCollections.keepSymbol.globalNames.has('MyClass02')).to.be.true;
      expect(AtKeepCollections.keepSymbol.globalNames.has('MyClass03')).to.be.true;
      expect(AtKeepCollections.keepSymbol.globalNames.has('MyClass04')).to.be.true;
      expect(AtKeepCollections.keepSymbol.globalNames.has('MyClass05')).to.be.true;
      expect(AtKeepCollections.keepSymbol.globalNames.has('MyClass06')).to.be.true;
      expect(AtKeepCollections.keepSymbol.globalNames.has('MyClass07')).to.be.true;
      expect(AtKeepCollections.keepSymbol.globalNames.has('MyClass08')).to.be.false;
      expect(AtKeepCollections.keepSymbol.propertyNames.size).to.be.equal(22);
      expect(AtKeepCollections.keepSymbol.propertyNames.has('staticProperty01')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('property02')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('property03')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('myMethod01')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('myStaticMethod')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('myGetter')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('mySetter')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('MyClass04')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('staticProperty05_01')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('property05_02')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('myMethod05_01')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('myStaticMethod05')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('myGetter05')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('mySetter05')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('MyClass06')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('property06')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('MyClass07')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('property07_01')).to.be.false;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('property07_02')).to.be.false;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('property07_03')).to.be.false;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('property07_04')).to.be.false;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('property07_05')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('property07_06')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('property07_07')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('property07_08')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('aa')).to.be.false;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('11')).to.be.false;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('myMethod')).to.be.false;
      expect(AtKeepCollections.keepAsConsumer.globalNames.size).to.be.equal(0);
      expect(AtKeepCollections.keepAsConsumer.propertyNames.size).to.be.equal(0);
    })
    it('should collect keepAsConsumer names from class if enabled', function () {
      let filePath: string = 'test/ut/utils/apiTest_visitAtKeepNames/atKeepClass02.ts';
      AtKeepCollections.clear();
      scanProjectConfig.mEnableAtKeep = true;
      collectApi(filePath, ApiExtractor.ApiType.PROJECT);
      expect(AtKeepCollections.keepAsConsumer.globalNames.size).to.be.equal(6);
      expect(AtKeepCollections.keepAsConsumer.globalNames.has('MyClass01')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.globalNames.has('MyClass02')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.globalNames.has('MyClass03')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.globalNames.has('MyClass04')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.globalNames.has('MyClass05')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.globalNames.has('MyClass06')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.size).to.be.equal(17);
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('staticProperty01')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('property02')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('property03')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('myMethod01')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('myStaticMethod')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('myGetter')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('mySetter')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('MyClass04')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('staticProperty05_01')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('property05_02')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('myMethod05_01')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('myStaticMethod05')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('myGetter05')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('mySetter05')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('MyClass06')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('property06')).to.be.true;
      expect(AtKeepCollections.keepSymbol.globalNames.size).to.be.equal(0);
      expect(AtKeepCollections.keepSymbol.propertyNames.size).to.be.equal(0);
    })
    it('should collect keepSymbol names from interface if enabled', function () {
      let filePath: string = 'test/ut/utils/apiTest_visitAtKeepNames/atKeepInterface01.ts';
      AtKeepCollections.clear();
      scanProjectConfig.mEnableAtKeep = true;
      collectApi(filePath, ApiExtractor.ApiType.PROJECT);
      expect(AtKeepCollections.keepSymbol.globalNames.size).to.be.equal(4);
      expect(AtKeepCollections.keepSymbol.globalNames.has('MyInterface01')).to.be.true;
      expect(AtKeepCollections.keepSymbol.globalNames.has('MyInterface02')).to.be.true;
      expect(AtKeepCollections.keepSymbol.globalNames.has('MyInterface03')).to.be.true;
      expect(AtKeepCollections.keepSymbol.globalNames.has('MyInterface04')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.size).to.be.equal(10);
      expect(AtKeepCollections.keepSymbol.propertyNames.has('interfaceProperty01')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('interfaceMethod01')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('interfaceProperty02')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('interfaceMethod02')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('MyInterface02')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('interfaceProperty03')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('interfaceMethod03')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('interfaceProperty04')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('interfaceMethod04')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('MyInterface04')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.globalNames.size).to.be.equal(0);
      expect(AtKeepCollections.keepAsConsumer.propertyNames.size).to.be.equal(0);
    })
    it('should collect keepAsConsumer names from interface if enabled', function () {
      let filePath: string = 'test/ut/utils/apiTest_visitAtKeepNames/atKeepInterface02.ts';
      AtKeepCollections.clear();
      scanProjectConfig.mEnableAtKeep = true;
      collectApi(filePath, ApiExtractor.ApiType.PROJECT);
      expect(AtKeepCollections.keepAsConsumer.globalNames.size).to.be.equal(4);
      expect(AtKeepCollections.keepAsConsumer.globalNames.has('MyInterface01')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.globalNames.has('MyInterface02')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.globalNames.has('MyInterface03')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.globalNames.has('MyInterface04')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.size).to.be.equal(10);
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('interfaceProperty01')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('interfaceMethod01')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('interfaceProperty02')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('interfaceMethod02')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('MyInterface02')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('interfaceProperty03')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('interfaceMethod03')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('interfaceProperty04')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('interfaceMethod04')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('MyInterface04')).to.be.true;
      expect(AtKeepCollections.keepSymbol.globalNames.size).to.be.equal(0);
      expect(AtKeepCollections.keepSymbol.propertyNames.size).to.be.equal(0);
    })
    it('should collect keepSymbol names from enum if enabled', function () {
      let filePath: string = 'test/ut/utils/apiTest_visitAtKeepNames/atKeepEnum01.ts';
      AtKeepCollections.clear();
      scanProjectConfig.mEnableAtKeep = true;
      collectApi(filePath, ApiExtractor.ApiType.PROJECT);
      expect(AtKeepCollections.keepSymbol.globalNames.size).to.be.equal(4);
      expect(AtKeepCollections.keepSymbol.globalNames.has('Color01')).to.be.true;
      expect(AtKeepCollections.keepSymbol.globalNames.has('Color02')).to.be.true;
      expect(AtKeepCollections.keepSymbol.globalNames.has('Color03')).to.be.true;
      expect(AtKeepCollections.keepSymbol.globalNames.has('Color04')).to.be.true;
      expect(AtKeepCollections.keepSymbol.globalNames.has('Color05')).to.be.false;
      expect(AtKeepCollections.keepSymbol.propertyNames.size).to.be.equal(6);
      expect(AtKeepCollections.keepSymbol.propertyNames.has('RED01')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('RED02')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('Color02')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('RED03')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('BLUE04')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('Color04')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('RED05')).to.be.false;
      expect(AtKeepCollections.keepAsConsumer.globalNames.size).to.be.equal(0);
      expect(AtKeepCollections.keepAsConsumer.propertyNames.size).to.be.equal(0);
    })
    it('should collect keepAsConsumer names from enum if enabled', function () {
      let filePath: string = 'test/ut/utils/apiTest_visitAtKeepNames/atKeepEnum02.ts';
      AtKeepCollections.clear();
      scanProjectConfig.mEnableAtKeep = true;
      collectApi(filePath, ApiExtractor.ApiType.PROJECT);
      expect(AtKeepCollections.keepAsConsumer.globalNames.size).to.be.equal(4);
      expect(AtKeepCollections.keepAsConsumer.globalNames.has('Color01')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.globalNames.has('Color02')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.globalNames.has('Color03')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.globalNames.has('Color04')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.size).to.be.equal(6);
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('RED01')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('RED02')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('Color02')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('RED03')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('BLUE04')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('Color04')).to.be.true;
      expect(AtKeepCollections.keepSymbol.globalNames.size).to.be.equal(0);
      expect(AtKeepCollections.keepSymbol.propertyNames.size).to.be.equal(0);
    })
    it('should collect atKeep names from function if enabled', function () {
      let filePath: string = 'test/ut/utils/apiTest_visitAtKeepNames/atKeepFunction.ts';
      AtKeepCollections.clear();
      scanProjectConfig.mEnableAtKeep = true;
      collectApi(filePath, ApiExtractor.ApiType.PROJECT);
      expect(AtKeepCollections.keepSymbol.globalNames.size).to.be.equal(2);
      expect(AtKeepCollections.keepSymbol.globalNames.has('addNumbers01')).to.be.true;
      expect(AtKeepCollections.keepSymbol.globalNames.has('addNumbers02')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.size).to.be.equal(1);
      expect(AtKeepCollections.keepSymbol.propertyNames.has('addNumbers02')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.globalNames.size).to.be.equal(2);
      expect(AtKeepCollections.keepAsConsumer.globalNames.has('addNumbers03')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.globalNames.has('addNumbers04')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.size).to.be.equal(1);
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('addNumbers04')).to.be.true;
    })
    it('should collect keepSymbol names from namespace if enabled', function () {
      let filePath: string = 'test/ut/utils/apiTest_visitAtKeepNames/atKeepNamespace01.ts';
      AtKeepCollections.clear();
      scanProjectConfig.mEnableAtKeep = true;
      collectApi(filePath, ApiExtractor.ApiType.PROJECT);
      // globalNames:
      expect(AtKeepCollections.keepSymbol.globalNames.size).to.be.equal(26);
      // from MyNamespace01
      expect(AtKeepCollections.keepSymbol.globalNames.has('MyNamespace01')).to.be.true;
      expect(AtKeepCollections.keepSymbol.globalNames.has('nsConstValue01')).to.be.true;
      expect(AtKeepCollections.keepSymbol.globalNames.has('nsFunction01')).to.be.true;
      expect(AtKeepCollections.keepSymbol.globalNames.has('nsClass01')).to.be.true;
      expect(AtKeepCollections.keepSymbol.globalNames.has('nsInterface01')).to.be.true;
      expect(AtKeepCollections.keepSymbol.globalNames.has('nsEnum01')).to.be.true;
      expect(AtKeepCollections.keepSymbol.globalNames.has('InnerNamespace01')).to.be.true;
      expect(AtKeepCollections.keepSymbol.globalNames.has('innerConstValue01')).to.be.true;
      expect(AtKeepCollections.keepSymbol.globalNames.has('nsType01')).to.be.true;
      // from MyNamespace02
      expect(AtKeepCollections.keepSymbol.globalNames.has('nsConstValue02')).to.be.true;
      expect(AtKeepCollections.keepSymbol.globalNames.has('nsFunction02')).to.be.true;
      expect(AtKeepCollections.keepSymbol.globalNames.has('nsClass02')).to.be.true;
      expect(AtKeepCollections.keepSymbol.globalNames.has('nsClass02_01')).to.be.true;
      expect(AtKeepCollections.keepSymbol.globalNames.has('nsClass02_02')).to.be.true;
      expect(AtKeepCollections.keepSymbol.globalNames.has('nsInterface02')).to.be.true;
      expect(AtKeepCollections.keepSymbol.globalNames.has('nsInterface02_01')).to.be.true;
      expect(AtKeepCollections.keepSymbol.globalNames.has('nsEnum02')).to.be.true;
      expect(AtKeepCollections.keepSymbol.globalNames.has('nsEnum02_01')).to.be.true;
      expect(AtKeepCollections.keepSymbol.globalNames.has('InnerNamespace02')).to.be.true;
      expect(AtKeepCollections.keepSymbol.globalNames.has('innerConstValue02')).to.be.true;
      // from MyNamespace04
      expect(AtKeepCollections.keepSymbol.globalNames.has('MyNamespace04')).to.be.true;
      expect(AtKeepCollections.keepSymbol.globalNames.has('MyNamespace05')).to.be.true;
      expect(AtKeepCollections.keepSymbol.globalNames.has('MyNamespace06')).to.be.true;
      expect(AtKeepCollections.keepSymbol.globalNames.has('MyNamespace08Class')).to.be.true;
      expect(AtKeepCollections.keepSymbol.globalNames.has('MyNamespace09')).to.be.true;
      expect(AtKeepCollections.keepSymbol.globalNames.has('MyNamespace09Class')).to.be.true;
      // propertyNames:
      expect(AtKeepCollections.keepSymbol.propertyNames.size).to.be.equal(37);
      // from MyNamespace01
      expect(AtKeepCollections.keepSymbol.propertyNames.has('nsConstValue01')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('nsFunction01')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('nsClass01')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('nsInterface01')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('nsEnum01')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('InnerNamespace01')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('innerConstValue01')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('nsType01')).to.be.true;
      // from MyNamespace02
      expect(AtKeepCollections.keepSymbol.propertyNames.has('nsConstValue02')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('nsFunction02')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('nsClass02')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('classProp02')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('nsClass02_01')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('nsClass02_02')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('classProp02_02')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('nsInterface02')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('interfaceProp02')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('nsInterface02_01')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('interfaceProp02_01')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('nsEnum02')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('nsEnum02_01')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('RED02')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('RED02_01')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('InnerNamespace02')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('innerConstValue02')).to.be.true;
      // from MyNamespace03
      expect(AtKeepCollections.keepSymbol.propertyNames.has('classProp03')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('classProp03_02')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('interfaceProp03')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('interfaceProp03_01')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('RED03')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('RED03_01')).to.be.true;
      // from MyNamespace04
      expect(AtKeepCollections.keepSymbol.propertyNames.has('MyNamespace04')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('MyNamespace05')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('MyNamespace06')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('MyNamespace08Class')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('MyNamespace09')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('MyNamespace09Class')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.globalNames.size).to.be.equal(0);
      expect(AtKeepCollections.keepAsConsumer.propertyNames.size).to.be.equal(0);
    })
    it('should collect keepAsConsumer names from namespace if enabled', function () {
      let filePath: string = 'test/ut/utils/apiTest_visitAtKeepNames/atKeepNamespace02.ts';
      AtKeepCollections.clear();
      scanProjectConfig.mEnableAtKeep = true;
      collectApi(filePath, ApiExtractor.ApiType.PROJECT);
      // globalNames:
      expect(AtKeepCollections.keepAsConsumer.globalNames.size).to.be.equal(26);
      // from MyNamespace01
      expect(AtKeepCollections.keepAsConsumer.globalNames.has('MyNamespace01')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.globalNames.has('nsConstValue01')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.globalNames.has('nsFunction01')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.globalNames.has('nsClass01')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.globalNames.has('nsInterface01')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.globalNames.has('nsEnum01')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.globalNames.has('InnerNamespace01')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.globalNames.has('innerConstValue01')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.globalNames.has('nsType01')).to.be.true;
      // from MyNamespace02
      expect(AtKeepCollections.keepAsConsumer.globalNames.has('nsConstValue02')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.globalNames.has('nsFunction02')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.globalNames.has('nsClass02')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.globalNames.has('nsClass02_01')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.globalNames.has('nsClass02_02')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.globalNames.has('nsInterface02')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.globalNames.has('nsInterface02_01')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.globalNames.has('nsEnum02')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.globalNames.has('nsEnum02_01')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.globalNames.has('InnerNamespace02')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.globalNames.has('innerConstValue02')).to.be.true;
      // from MyNamespace04
      expect(AtKeepCollections.keepAsConsumer.globalNames.has('MyNamespace04')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.globalNames.has('MyNamespace05')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.globalNames.has('MyNamespace06')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.globalNames.has('MyNamespace08Class')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.globalNames.has('MyNamespace09')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.globalNames.has('MyNamespace09Class')).to.be.true;
      // propertyNames:
      expect(AtKeepCollections.keepAsConsumer.propertyNames.size).to.be.equal(37);
      // from MyNamespace01
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('nsConstValue01')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('nsFunction01')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('nsClass01')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('nsInterface01')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('nsEnum01')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('InnerNamespace01')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('innerConstValue01')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('nsType01')).to.be.true;
      // from MyNamespace02
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('nsConstValue02')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('nsFunction02')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('nsClass02')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('classProp02')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('nsClass02_01')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('nsClass02_02')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('classProp02_02')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('nsInterface02')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('interfaceProp02')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('nsInterface02_01')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('interfaceProp02_01')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('nsEnum02')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('nsEnum02_01')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('RED02')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('RED02_01')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('InnerNamespace02')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('innerConstValue02')).to.be.true;
      // from MyNamespace03
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('classProp03')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('classProp03_02')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('interfaceProp03')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('interfaceProp03_01')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('RED03')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('RED03_01')).to.be.true;
      // from MyNamespace04
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('MyNamespace04')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('MyNamespace05')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('MyNamespace06')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('MyNamespace08Class')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('MyNamespace09')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('MyNamespace09Class')).to.be.true;
      expect(AtKeepCollections.keepSymbol.globalNames.size).to.be.equal(0);
      expect(AtKeepCollections.keepSymbol.propertyNames.size).to.be.equal(0);
    })
    it('should collect atKeep names from global variable if enabled', function () {
      let filePath: string = 'test/ut/utils/apiTest_visitAtKeepNames/atKeepGlobalVar.ts';
      AtKeepCollections.clear();
      scanProjectConfig.mEnableAtKeep = true;
      collectApi(filePath, ApiExtractor.ApiType.PROJECT);
      expect(AtKeepCollections.keepSymbol.globalNames.size).to.be.equal(6);
      expect(AtKeepCollections.keepSymbol.globalNames.has('globalVar01')).to.be.true;
      expect(AtKeepCollections.keepSymbol.globalNames.has('globalFunc01')).to.be.true;
      expect(AtKeepCollections.keepSymbol.globalNames.has('globalMyClass01')).to.be.true;
      expect(AtKeepCollections.keepSymbol.globalNames.has('globalVar02')).to.be.true;
      expect(AtKeepCollections.keepSymbol.globalNames.has('globalFunc02')).to.be.true;
      expect(AtKeepCollections.keepSymbol.globalNames.has('globalMyClass02')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.size).to.be.equal(3);
      expect(AtKeepCollections.keepSymbol.propertyNames.has('globalVar02')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('globalFunc02')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('globalMyClass02')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.globalNames.size).to.be.equal(6);
      expect(AtKeepCollections.keepAsConsumer.globalNames.has('globalVar03')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.globalNames.has('globalFunc03')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.globalNames.has('globalMyClass03')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.globalNames.has('globalVar04')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.globalNames.has('globalFunc04')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.globalNames.has('globalMyClass04')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.size).to.be.equal(3);
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('globalVar04')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('globalFunc04')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.has('globalMyClass04')).to.be.true;
    })
    it('should collect atKeep names from annotation declarations if enabled', function () {
      let filePath: string = 'test/ut/utils/apiTest_visitAtKeepNames/atKeepAnnotation.ets';
      AtKeepCollections.clear();
      scanProjectConfig.mEnableAtKeep = true;
      collectApi(filePath, ApiExtractor.ApiType.PROJECT);
      expect(AtKeepCollections.keepSymbol.globalNames.size).to.be.equal(2);
      expect(AtKeepCollections.keepSymbol.globalNames.has('MyAnnotation01')).to.be.true;
      expect(AtKeepCollections.keepSymbol.globalNames.has('MyAnnotation02')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.size).to.be.equal(1);
      expect(AtKeepCollections.keepSymbol.propertyNames.has('MyAnnotation02')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.globalNames.size).to.be.equal(2);
      expect(AtKeepCollections.keepAsConsumer.globalNames.has('MyAnnotation04')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.globalNames.has('MyAnnotation05')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.size).to.be.equal(1);
      expect(AtKeepCollections.keepAsConsumer.globalNames.has('MyAnnotation05')).to.be.true;
    })
    it('should collect atKeep names from .ets', function () {
      let filePath: string = 'test/ut/utils/apiTest_visitAtKeepNames/atKeepTest01.ets';
      AtKeepCollections.clear();
      scanProjectConfig.mEnableAtKeep = true;
      collectApi(filePath, ApiExtractor.ApiType.PROJECT);
      expect(AtKeepCollections.keepSymbol.globalNames.size).to.be.equal(1);
      expect(AtKeepCollections.keepSymbol.globalNames.has('MyClass01')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.size).to.be.equal(2);
      expect(AtKeepCollections.keepSymbol.propertyNames.has('property01')).to.be.true;
      expect(AtKeepCollections.keepSymbol.propertyNames.has('MyClass01')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.globalNames.size).to.be.equal(2);
      expect(AtKeepCollections.keepAsConsumer.globalNames.has('MyVar01')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.globalNames.has('MyVar02')).to.be.true;
      expect(AtKeepCollections.keepAsConsumer.propertyNames.size).to.be.equal(0);
    })
    it('should not collect atKeep names from .js', function () {
      let filePath: string = 'test/ut/utils/apiTest_visitAtKeepNames/atKeepTest02.js';
      AtKeepCollections.clear();
      scanProjectConfig.mEnableAtKeep = true;
      collectApi(filePath, ApiExtractor.ApiType.PROJECT);
      expect(AtKeepCollections.keepSymbol.globalNames.size).to.be.equal(0);
      expect(AtKeepCollections.keepSymbol.propertyNames.size).to.be.equal(0);
      expect(AtKeepCollections.keepAsConsumer.globalNames.size).to.be.equal(0);
      expect(AtKeepCollections.keepAsConsumer.propertyNames.size).to.be.equal(0);
    })
    it('should not collect atKeep names from .d.ts', function () {
      let filePath: string = 'test/ut/utils/apiTest_visitAtKeepNames/atKeepTest03.d.ts';
      AtKeepCollections.clear();
      scanProjectConfig.mEnableAtKeep = true;
      collectApi(filePath, ApiExtractor.ApiType.PROJECT);
      expect(AtKeepCollections.keepSymbol.globalNames.size).to.be.equal(0);
      expect(AtKeepCollections.keepSymbol.propertyNames.size).to.be.equal(0);
      expect(AtKeepCollections.keepAsConsumer.globalNames.size).to.be.equal(0);
      expect(AtKeepCollections.keepAsConsumer.propertyNames.size).to.be.equal(0);
    })
    it('should not collect atKeep names from .d.ets', function () {
      let filePath: string = 'test/ut/utils/apiTest_visitAtKeepNames/atKeepTest04.d.ets';
      AtKeepCollections.clear();
      scanProjectConfig.mEnableAtKeep = true;
      collectApi(filePath, ApiExtractor.ApiType.PROJECT);
      expect(AtKeepCollections.keepSymbol.globalNames.size).to.be.equal(0);
      expect(AtKeepCollections.keepSymbol.propertyNames.size).to.be.equal(0);
      expect(AtKeepCollections.keepAsConsumer.globalNames.size).to.be.equal(0);
      expect(AtKeepCollections.keepAsConsumer.propertyNames.size).to.be.equal(0);
    })
  })

  describe('test for collect fileWhiteList', function () {
    beforeEach(() => {
      let cachePath = 'test/ut/utils/obfuscation';
      initProjectWhiteListManager(cachePath, false, true);
    })

    afterEach(() => {
      clearProjectWhiteListManager();
    })

    it('should collect structProperties, stringProperties, enumProperties if propertyObf is enabled', function () {
      let filePath: string = 'test/ut/utils/apiTest_collectFileWhiteList/collectFileWhiteList01.ets';
      scanProjectConfig.mPropertyObfuscation = true;
      scanProjectConfig.mKeepStringProperty = true;
      scanProjectConfig.isHarCompiled = true;
      collectApi(filePath, ApiExtractor.ApiType.PROJECT);
      const fileWhiteList: FileWhiteList = projectWhiteListManager?.getFileWhiteListMap().get(filePath)!;
      expect(fileWhiteList.fileKeepInfo.enumProperties.has('RED01')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.enumProperties.has('RED02')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.enumProperties.has('BLUE02')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('MyEnum')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('MyClass')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('obj01')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('RED01')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('myProp01')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('objProp')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.stringProperties.has('name')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.stringProperties.has('age')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.structProperties.has('myStructProp')).to.be.true;
    })
    it('should not collect stringProperties if mKeepStringProperty is not enabled', function () {
      let filePath: string = 'test/ut/utils/apiTest_collectFileWhiteList/collectFileWhiteList01.ets';
      scanProjectConfig.mPropertyObfuscation = true;
      scanProjectConfig.mKeepStringProperty = false;
      scanProjectConfig.isHarCompiled = true;
      collectApi(filePath, ApiExtractor.ApiType.PROJECT);
      const fileWhiteList: FileWhiteList = projectWhiteListManager?.getFileWhiteListMap().get(filePath)!;
      expect(fileWhiteList.fileKeepInfo.enumProperties.has('RED01')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.enumProperties.has('RED02')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.enumProperties.has('BLUE02')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('MyEnum')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('MyClass')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('obj01')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('RED01')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('myProp01')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.has('objProp')).to.be.true;
      expect(fileWhiteList.fileKeepInfo.stringProperties.has('name')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.stringProperties.has('age')).to.be.false;
      expect(fileWhiteList.fileKeepInfo.structProperties.has('myStructProp')).to.be.true;
    })
    it('should not collect structProperties, stringProperties, enumProperties if propertyObf is not enabled', function () {
      let filePath: string = 'test/ut/utils/apiTest_collectFileWhiteList/collectFileWhiteList01.ets';
      scanProjectConfig.mPropertyObfuscation = false;
      scanProjectConfig.mKeepStringProperty = true;
      scanProjectConfig.isHarCompiled = true;
      collectApi(filePath, ApiExtractor.ApiType.PROJECT);
      const fileWhiteList: FileWhiteList = projectWhiteListManager?.getFileWhiteListMap().get(filePath)!;
      expect(fileWhiteList.fileKeepInfo.enumProperties.size).to.be.equal(0);
      expect(fileWhiteList.fileKeepInfo.exported.propertyNames.size).to.be.equal(0);
      expect(fileWhiteList.fileKeepInfo.stringProperties.size).to.be.equal(0);
      expect(fileWhiteList.fileKeepInfo.structProperties.size).to.be.equal(0);
    })
    it('should collect decoratorMap if need scanDecorator', function () {
      let filePath: string = 'test/ut/utils/apiTest_collectFileWhiteList/collectFileWhiteList02.ets';
      scanProjectConfig.scanDecorator = true;
      collectApi(filePath, ApiExtractor.ApiType.PROJECT);
      const fileWhiteList: FileWhiteList = projectWhiteListManager?.getFileWhiteListMap().get(filePath)!;
      if (fileWhiteList?.bytecodeObfuscateKeepInfo?.decoratorMap) {
        const decoratorKeys = Object.keys(fileWhiteList.bytecodeObfuscateKeepInfo.decoratorMap);
        expect(decoratorKeys.length).to.be.greaterThan(0);
      }
    })
  })
});
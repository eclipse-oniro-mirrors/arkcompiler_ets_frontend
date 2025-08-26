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
import { assert, expect } from 'chai';
import {
  ObOptionsForTest,
  ObConfigResolver,
  MergedConfig,
  collectResevedFileNameInIDEConfig,
  readNameCache,
  handleUniversalPathInObf,
  getArkguardNameCache,
  fillNameCache,
  writeObfuscationNameCache,
  generateConsumerObConfigFile,
  mangleFilePath,
  enableObfuscatedFilePathConfig,
  handleObfuscatedFilePath,
  enableObfuscateFileName,
  getRelativeSourcePath,
  OptionTypeForTest,
  SourceObConfig,
  Obfuscation,
  printUnobfuscationReasons,
  clearNameCache,
  generateobfLoggerPrinter
} from '../../../src/initialization/ConfigResolver';
import { HvigorErrorInfo, PropCollections, renameFileNameModule } from '../../../src/ArkObfuscator';
import {
  clearUnobfuscationNamesObj,
  nameCacheMap,
  unobfuscationNamesObj
} from '../../../src/initialization/CommonObject';
import path from 'path';
import fs from 'fs';
import { FileUtils } from '../../../src/utils/FileUtils';
import sinon from 'sinon';
import { AtKeepCollections, UnobfuscationCollections } from '../../../src/utils/CommonCollections';
import {
  clearHistoryUnobfuscatedMap,
  historyAllUnobfuscatedNamesMap,
  historyUnobfuscatedPropMap
} from '../../../src/initialization/Initializer';

const OBFUSCATE_TESTDATA_DIR = path.resolve(__dirname, '../../testData/obfuscation/system_api_obfuscation');
const projectConfig = {
  obfuscationOptions: { option1: 'value1' },
  compileHar: true
};

function printObfLogger(errorInfo: string, errorCodeInfo: HvigorErrorInfo | string, level: string): void {
  switch (level) {
    case 'warn':
      console.warn(errorInfo);
      break;
    case 'error':
      console.error(errorInfo);
      break;
    default:
      break;
  }
}

function printObfHvigorLogger(errorInfo: string, errorCodeInfo: HvigorErrorInfo | string, level: string): void {
  switch (level) {
    case 'warn':
      console.warn(errorCodeInfo);
      break;
    case 'error':
      console.error(errorCodeInfo);
      break;
    default:
      break;
  }
}

const logger = console;
const isTerser = false;
let newObConfigResolver = new ObConfigResolver(projectConfig, printObfLogger, isTerser);

describe('test for ConfigResolve', function() {
  describe('ObOptions', () => {
    it('should merge options correctly', () => {
      const ob1 = new ObOptionsForTest();
      ob1.disableObfuscation = true;
      ob1.enablePropertyObfuscation = true;

      const ob2 = new ObOptionsForTest();
      ob2.enableToplevelObfuscation = true;
      ob2.printNameCache = 'test';
      ob2.printKeptNamesPath = './test/ut/initialization/printKeptNamesPath.txt';
      ob2.applyNameCache = 'test case';

      ob1.mergeObOptions(ob2);

      expect(ob1.disableObfuscation).to.be.true;
      expect(ob1.enablePropertyObfuscation).to.be.true;
      expect(ob1.enableToplevelObfuscation).to.be.true;
      expect(ob1.printNameCache).to.equal('test');
      expect(ob1.printKeptNamesPath).to.equal('./test/ut/initialization/printKeptNamesPath.txt');
      expect(ob1.applyNameCache).to.equal('test case');
    });
  });

  describe('MergedConfig', () => {
    describe('should merge two MergedConfig instances correctly', () => {
      it('merge arrays and sets in MergeConfig', () => {
        const config1 = new MergedConfig();
        config1.reservedPropertyNames = ['prop1'];
        config1.reservedGlobalNames = ['global1'];
        config1.keepComments = ['comment1'];
        config1.excludePathSet.add('path1');

        const config2 = new MergedConfig();
        config2.reservedPropertyNames = ['prop2'];
        config2.reservedGlobalNames = ['global2'];
        config2.keepComments = ['comment2'];
        config2.excludePathSet.add('path2');

        config1.mergeAllRules(config2);

        expect(config1.reservedPropertyNames).to.deep.equal(['prop1', 'prop2']);
        expect(config1.reservedGlobalNames).to.deep.equal(['global1', 'global2']);
        expect(config1.keepComments).to.deep.equal(['comment1', 'comment2']);
        expect(config1.excludePathSet).to.deep.equal(new Set(['path1', 'path2']));
      });

      it('merge options in MergeConfig', () => {
        const config1 = new MergedConfig();
        config1.options.enableExportObfuscation = false;
        config1.options.stripLanguageDefault = true;
        config1.options.stripSystemApiArgs = true;

        const config2 = new MergedConfig();
        config2.options.enableExportObfuscation = true;
        config2.options.stripLanguageDefault = false;
        config2.options.stripSystemApiArgs = true;
        config1.mergeAllRules(config2);

        expect(config1.options.disableObfuscation).to.be.false;
        expect(config1.options.enablePropertyObfuscation).to.be.false;
        expect(config1.options.enableToplevelObfuscation).to.be.false;
        expect(config1.options.enableStringPropertyObfuscation).to.be.false;
        expect(config1.options.removeLog).to.be.false;
        expect(config1.options.compact).to.be.false;
        expect(config1.options.removeComments).to.be.false;
        expect(config1.options.enableFileNameObfuscation).to.be.false;
        expect(config1.options.enableExportObfuscation).to.be.true;
        expect(config1.options.stripLanguageDefault).to.be.true;
        expect(config1.options.stripSystemApiArgs).to.be.true;
      });
    });

    it('should sort and deduplicate arrays correctly', () => {
      const config = new MergedConfig();
      config.reservedPropertyNames = ['prop2', 'prop1', 'prop1'];
      config.reservedGlobalNames = ['global2', 'global1', 'global1'];
      config.keepComments = ['comment2', 'comment1', 'comment1'];

      config.sortAndDeduplicate();

      expect(config.reservedPropertyNames).to.deep.equal(['prop1', 'prop2']);
      expect(config.reservedGlobalNames).to.deep.equal(['global1', 'global2']);
      expect(config.keepComments).to.deep.equal(['comment1', 'comment2']);
    });

    it('should serialize merged config correctly', () => {
      let resultStr: string = '';
      const config = new MergedConfig();
      config.options = new ObOptionsForTest();
      config.options['option1'] = true;
      config.options['option2'] = false;
      config.options['disableObfuscation'] = true;
      config.options['enableStringPropertyObfuscation'] = true;
      config.reservedGlobalNames = ['global1'];
      config.reservedPropertyNames = ['prop1'];

      const serialized = config.serializeMergedConfig();
      const serializedExportSwitchMap = new Map([
        ['disableObfuscation', ObConfigResolver.KEEP_DTS],
        ['enablePropertyObfuscation', ObConfigResolver.ENABLE_PROPERTY_OBFUSCATION],
        ['enableStringPropertyObfuscation', ObConfigResolver.ENABLE_STRING_PROPERTY_OBFUSCATION],
        ['enableToplevelObfuscation', ObConfigResolver.ENABLE_TOPLEVEL_OBFUSCATION],
        ['compact', ObConfigResolver.COMPACT],
        ['removeLog', ObConfigResolver.REMOVE_LOG],
      ]);

      expect(serialized).to.include(ObConfigResolver.KEEP_GLOBAL_NAME);
      expect(serialized).to.include('global1');
      expect(serialized).to.include(ObConfigResolver.KEEP_PROPERTY_NAME);
      expect(serialized).to.include('prop1');
      expect(serialized).to.not.include('option2');

      expect(ObConfigResolver.exportedSwitchMap.has(String('disableObfuscation'))).to.be.false;
      expect(ObConfigResolver.exportedSwitchMap.has(String('enableStringPropertyObfuscation'))).to.be.true;
      expect(resultStr).to.equal('');
    });
  });

  describe('ObConfigResolver', () => {
    describe('constructor', () => {
      it('should create an instance with the correct properties', () => {
        const projectConfig = {
          obfuscationOptions: { option1: 'value1' },
          compileHar: true
        };
        const isTerser = false;
        const myInstance = new ObConfigResolver(projectConfig, printObfLogger, isTerser);

        expect(myInstance).to.be.an('object');
        expect(myInstance.sourceObConfig).to.deep.equal({ option1: 'value1' });
        expect(myInstance.isHarCompiled).to.be.true;
        expect(myInstance.isTerser).to.be.false;
      });

      it('should create an instance with Terser enabled', () => {
        const projectConfig = {
          obfuscationOptions: { option1: 'value1' },
          compileHar: true
        };
        const myInstance = new ObConfigResolver(projectConfig, printObfLogger, true);

        expect(myInstance).to.be.an('object');
        expect(myInstance.sourceObConfig).to.deep.equal({ option1: 'value1' });
        expect(myInstance.isHarCompiled).to.be.true;
        expect(myInstance.isTerser).to.be.true;
      });
    });

    describe('resolveObfuscationConfigs', () => {
      const projectConfig = {
        obfuscationOptions: { option1: 'value1' },
        compileHar: true
      };
      const logger = console;
      const isTerser = false;
      let newObConfigResolver = new ObConfigResolver(projectConfig, printObfLogger, isTerser);
      let testClass: ObConfigResolver & {
        performancePrinter?: any;
      };
      testClass = newObConfigResolver;

      it('should return a MergedConfig object when sourceObConfig is undefined', () => {
        testClass.sourceObConfig = undefined;
        testClass.isHarCompiled = true;
        testClass.performancePrinter = {};
        const result = testClass.resolveObfuscationConfigs();
        generateConsumerObConfigFile(testClass.sourceObConfig, undefined);

        expect(testClass.sourceObConfig).to.be.an.undefined;
        expect(result).to.deep.equal(new MergedConfig());
        expect(result).to.be.an('object');
        expect(result).to.have.property('options');
      });

      it('should return a MergedConfig object when sourceObConfig is null', () => {
        testClass.sourceObConfig = null;
        testClass.isHarCompiled = true;
        testClass.performancePrinter = {};
        const result = testClass.resolveObfuscationConfigs();
        generateConsumerObConfigFile(null, undefined);

        expect(testClass.sourceObConfig).to.be.a.null;
        expect(result).to.deep.equal(new MergedConfig());
        expect(result).to.be.an('object');
        expect(result).to.have.property('options');
      });

      it('should handle the case when sourceObConfig.selfConfig.ruleOptions.enable is false', function () {
        testClass.sourceObConfig = {
          selfConfig: {
            ruleOptions: { enable: false },
          },
          dependencies: { libraries: [], hars: [] },
        };
        generateConsumerObConfigFile(testClass.sourceObConfig, undefined);
        const result = testClass.resolveObfuscationConfigs();

        expect(result).to.be.an('object');
        expect(result).to.have.property('options');
        expect(result.options.disableObfuscation).to.be.true;
      });

      it('should handle the case when sourceObConfig.selfConfig.ruleOptions.enable is true', function () {
        testClass.sourceObConfig = {
          selfConfig: {
            ruleOptions: { enable: true },
          },
          dependencies: { libraries: [], hars: [] },
        };
        generateConsumerObConfigFile(testClass.sourceObConfig, undefined);
        const result = testClass.resolveObfuscationConfigs();

        expect(result).to.be.an('object');
        expect(result).to.have.property('options');
        expect(result.options.disableObfuscation).to.be.false;
      });

      it('should handle the case when enableObfuscation is false', () => {
        const sourceObConfig = {
          selfConfig: {
            ruleOptions: { enable: false },
            consumerRules: ['./test/testData/obfuscation/keepDts/obfuscation-template.txt'],
          },
          dependencies: { libraries: [], hars: [] },
          obfuscationCacheDir: './test/testData/cache',
          options: {
            disableObfuscation: true
          },
          exportRulePath: '',
          sdkApis: []
        };
        const isHarCompiled = true;
        let enableObfuscation = sourceObConfig.selfConfig.ruleOptions.enable;
        let selfConfig = new MergedConfig();
        enableObfuscation = !selfConfig.options.disableObfuscation;
        let needConsumerConfigs = isHarCompiled && sourceObConfig.selfConfig.consumerRules &&
          sourceObConfig.selfConfig.consumerRules.length > 0;
        let needDependencyConfigs = enableObfuscation || needConsumerConfigs;
        let dependencyConfigs = new MergedConfig();
        const dependencyMaxLength = Math.max(
          sourceObConfig.dependencies.libraries.length,
          sourceObConfig.dependencies.hars.length,
        );
        newObConfigResolver.getDependencyConfigsForTest(sourceObConfig, dependencyConfigs);

        expect(needConsumerConfigs).to.be.true;
        expect(!dependencyMaxLength).to.be.true;
        expect(needDependencyConfigs).to.be.true;
        expect(dependencyConfigs).to.deep.equal(new MergedConfig());
        expect(enableObfuscation).to.be.true;
      });

      it('should handle the case when enableObfuscation is false', () => {
        const localHarConfig = {
          selfConfig: {
            ruleOptions: {
              enable: true,
              rules: ['./test/testData/obfuscation/Configs/localHar/obfuscation-rules.txt']
            },
            consumerRules: [],
            libDir: "",
            consumerFiles: ""
          },
          dependencies: { libraries: [], hars: [] },
          sdkApis: [],
          obfuscationCacheDir: '',
          exportRulePath: ''
        };

        const sourceObConfig = {
          selfConfig: {
            options: {
              disableObfuscation: false,
              enablePropertyObfuscation: true,
              enableExportObfuscation: true,
              enableToplevelObfuscation: true
            },
            ruleOptions: { enable: true },
            consumerRules: ['./test/testData/obfuscation/keepDts/obfuscation-template.txt'],
            libDir: ''
          },
          dependencies: { libraries: [localHarConfig.selfConfig], hars: [] },
          obfuscationCacheDir: './test/testData/cache',
          sdkApis: [],
          exportRulePath: ''
        };
        const isHarCompiled = true;
        let enableObfuscation = sourceObConfig.selfConfig.ruleOptions.enable;
        let selfConfig = new MergedConfig();
        enableObfuscation = !selfConfig.options.disableObfuscation;
        let needConsumerConfigs = isHarCompiled && sourceObConfig.selfConfig.consumerRules &&
          sourceObConfig.selfConfig.consumerRules.length > 0;
        let needDependencyConfigs = enableObfuscation || needConsumerConfigs;
        let dependencyConfigs = new MergedConfig();
        const dependencyMaxLength = Math.max(
          sourceObConfig.dependencies.libraries.length,
          sourceObConfig.dependencies.hars.length,
        );
        const mergedConfigs = newObConfigResolver.getMergedConfigsForTest(selfConfig, dependencyConfigs);
        newObConfigResolver.handleReservedArrayForTest(mergedConfigs);
        let needKeepSystemApi = enableObfuscation &&
          (mergedConfigs.options.enablePropertyObfuscation ||
            (mergedConfigs.options.enableExportObfuscation && mergedConfigs.options.enableToplevelObfuscation));

        newObConfigResolver.getDependencyConfigsForTest(sourceObConfig, dependencyConfigs);

        expect(needConsumerConfigs).to.be.true;
        expect(!dependencyMaxLength).to.be.false;
        expect(needDependencyConfigs).to.be.true;
        expect(dependencyConfigs).to.deep.equal(new MergedConfig());
        expect(enableObfuscation).to.be.true;
        expect(needKeepSystemApi).to.be.false;
        expect(needConsumerConfigs).to.be.true;
      });

      it('should handle the case when enableObfuscation is true', () => {
        testClass.sourceObConfig = {
          selfConfig: {
            ruleOptions: { enable: true },
            consumerRules: ['./test/testData/obfuscation/keepDts/obfuscation-template.txt'],
          },
          dependencies: { libraries: [], hars: [] },
          obfuscationCacheDir: './test/testData/cache',
          options: {
            disableObfuscation: true
          }
        };
        testClass.isHarCompiled = true;
        testClass.performancePrinter = {};
        let enableObfuscation = testClass.sourceObConfig.selfConfig.ruleOptions.enable;
        let selfConfig = new MergedConfig();
        newObConfigResolver.getSelfConfigsForTest(selfConfig);
        enableObfuscation = !selfConfig.options.disableObfuscation;
        enableObfuscation = false;
        let needConsumerConfigs = testClass.isHarCompiled && testClass.sourceObConfig.selfConfig.consumerRules &&
          testClass.sourceObConfig.selfConfig.consumerRules.length > 0;
        let needDependencyConfigs = enableObfuscation || needConsumerConfigs;
        let dependencyConfigs = new MergedConfig();
        const dependencyMaxLength = Math.max(
          testClass.sourceObConfig.dependencies.libraries.length,
          testClass.sourceObConfig.dependencies.hars.length,
          testClass.sourceObConfig.dependencies.hsps?.length ?? 0,
          testClass.sourceObConfig.dependencies.hspLibraries?.length ?? 0
        );
        newObConfigResolver.getDependencyConfigsForTest(testClass.sourceObConfig, dependencyConfigs);

        expect(needConsumerConfigs).to.be.true;
        expect(!dependencyMaxLength).to.be.true;
        expect(needDependencyConfigs).to.be.true;
        expect(dependencyConfigs).to.deep.equal(new MergedConfig());
        expect(enableObfuscation).to.be.false;
      });
    });

    describe('getSelfConfigs', () => {
      it('should create an instance with the correct properties', () => {
        const projectConfig = {
          obfuscationOptions: {
            selfConfig: {
              ruleOptions: {
                rules: []
              }
            }
          },
          compileHar: true
        };
        const isTerser = false;
        const resolver = new ObConfigResolver(projectConfig, printObfLogger, isTerser);

        expect(resolver).to.be.an('object');
        expect(resolver.sourceObConfig).to.deep.equal(projectConfig.obfuscationOptions);
        expect(resolver.isHarCompiled).to.equal(projectConfig.compileHar);
        expect(resolver.isTerser).to.equal(isTerser);
      });

      it('should create an instance with the correct properties', () => {
        const projectConfig = {
          obfuscationOptions: {
            selfConfig: {
              ruleOptions: {
                rules: ['./test/ut/initialization/tempNameCache.json']
              }
            }
          },
          compileHar: true
        };
        const isTerser = false;
        const resolver = new ObConfigResolver(projectConfig, printObfLogger, isTerser);

        expect(resolver).to.be.an('object');
        expect(resolver.sourceObConfig).to.deep.equal(projectConfig.obfuscationOptions);
        expect(resolver.isHarCompiled).to.equal(projectConfig.compileHar);
        expect(resolver.isTerser).to.equal(isTerser);
      });

      it('should get self configs correctly', () => {
        const projectConfig = {
          obfuscationOptions: {
            selfConfig: {
              ruleOptions: {
                rules: ['./test/ut/initialization/tempNameCache.json']
              }
            }
          },
          compileHar: true
        };
        const isTerser = false;
        const resolver = new ObConfigResolver(projectConfig, printObfLogger, isTerser);
        const selfConfigs = new MergedConfig();

        resolver.getSelfConfigsForTest(selfConfigs);

        expect(resolver).to.be.an('object');
        expect(resolver.sourceObConfig).to.deep.equal(projectConfig.obfuscationOptions);
        expect(resolver.isHarCompiled).to.equal(projectConfig.compileHar);
        expect(resolver.isTerser).to.equal(isTerser);
      });
    });

    describe('getConfigByPath', () => {
      const projectConfig = {
        obfuscationOptions: {
          selfConfig: {
            ruleOptions: {
              rules: ['./test/testData/obfuscation/filename_obf/getConfigByPath.json']
            }
          }
        },
        compileHar: true
      };
      let sandbox;
      let instance;

      beforeEach(() => {
        sandbox = sinon.createSandbox();
        instance = new ObConfigResolver(projectConfig, printObfLogger, true);
        instance.logger = { error: sandbox.stub() };
      });

      afterEach(() => {
        sandbox.restore();
      });

      it('should read file content and handle config content', () => {
        const path = './test/testData/obfuscation/filename_obf/getConfigByPath.json';
        let fileContent;
        const expectedContent = '{"key": "value"}';
        const configs = new MergedConfig();

        fileContent = fs.readFileSync(path, 'utf-8');
        instance.getConfigByPathForTest(path, configs);
        instance.handleConfigContent(fileContent, configs, path);

        expect(fileContent).to.equal(expectedContent);
        expect(configs).to.be.an.instanceOf(MergedConfig);
      });

      it('should log error and throw when failed to open file', () => {
        const path = './test/testData/obfuscation/filename_obf/non-existent-file.json';
        const configs = new MergedConfig();
        const stub = sinon.stub(console, 'error');
        const errorMessage = `Failed to open ${path}. Error message: Error: ENOENT: no such file or directory, open ${path}`;
        sandbox.stub(fs, 'readFileSync').throws(new Error(`ENOENT: no such file or directory, open ${path}`));
        try {
          instance.getConfigByPath(path, configs);
        } catch (err) {
        }
        expect(stub.calledWith(errorMessage)).to.be.true;
        stub.restore();
      });

      it('should log error and throw when failed to open file with hvigor errorCode', () => {
        const path = './test/testData/obfuscation/filename_obf/non-existent-file.json';
        const configs = new MergedConfig();
        const stub = sinon.stub(console, 'error');
        const errorMessage = {
          code: '10804001',
          description: 'ArkTS compiler Error',
          cause: `Failed to open obfuscation config file from ${path}. Error message: Error: ENOENT: no such file or directory, open ${path}`,
          position: path,
          solutions: [`Please check whether ${path} exists.`],
        };
        instance = new ObConfigResolver(projectConfig, printObfHvigorLogger, true);
        sandbox.stub(fs, 'readFileSync').throws(new Error(`ENOENT: no such file or directory, open ${path}`));
        try {
          instance.getConfigByPath(path, configs);
        } catch (err) {
        }
        expect(stub.calledWith(errorMessage)).to.be.true;
        stub.restore();
      });
    });

    it('should return a new MergedConfig instance when sourceObConfig is not defined', function() {
      const projectConfig = {
        sourceObConfig: null,
        getMergedConfig: function() {
          let sourceObConfig = this.sourceObConfig;
          if (!sourceObConfig) {
            return new MergedConfig();
          }
        }
      };

      const result = projectConfig.getMergedConfig();
      expect(result).to.be.an.instanceOf(MergedConfig);
    });

    describe('resolveKeepConfig', () => {
      it('should resolve keep config correctly, starts with "!" and contains "*", not starts with "!"', () => {
        const keepConfigs = ['!test/*/exclude.js', 'test/include.js'];
        const configs = new MergedConfig();
        const configPath = './test/testData/obfuscation/keepDts/obfuscation-template.txt';

        newObConfigResolver.resolveKeepConfig(keepConfigs, configs, configPath);

        expect(configs.excludeUniversalPaths).to.have.lengthOf(1);
        expect(configs.keepUniversalPaths).to.have.lengthOf(0);
        expect(configs.excludePathSet).to.have.lengthOf(0);
        expect(configs.keepSourceOfPaths).to.have.lengthOf(0);
      });

      it('should resolve keep config correctly, starts with "!" and contains "*", not starts with "!" and "?"', () => {
        const keepConfigs = ['!test/*/exclude.js', 'test/?/include.js'];
        const configs = new MergedConfig();
        const configPath = './test/testData/obfuscation/keepDts/obfuscation-template.txt';

        newObConfigResolver.resolveKeepConfig(keepConfigs, configs, configPath);

        expect(configs.excludeUniversalPaths).to.have.lengthOf(1);
        expect(configs.keepUniversalPaths).to.have.lengthOf(1);
        expect(configs.excludePathSet).to.have.lengthOf(0);
        expect(configs.keepSourceOfPaths).to.have.lengthOf(0);
      });

      it('should resolve keep config correctly, both start with "!", contains "*" and "?"', () => {
        const keepConfigs = ['!test/*/exclude.js', '!test/?/include.js'];
        const configs = new MergedConfig();
        const configPath = './test/testData/obfuscation/keepDts/obfuscation-template.txt';

        newObConfigResolver.resolveKeepConfig(keepConfigs, configs, configPath);

        expect(configs.excludeUniversalPaths).to.have.lengthOf(2);
        expect(configs.keepUniversalPaths).to.have.lengthOf(0);
        expect(configs.excludePathSet).to.have.lengthOf(0);
        expect(configs.keepSourceOfPaths).to.have.lengthOf(0);
      });

      it('should resolve keep config correctly, not starts with "!" and contains "*" and "?"', () => {
        const keepConfigs = ['test/*/exclude.js', 'test/?/include.js'];
        const configs = new MergedConfig();
        const configPath = './test/testData/obfuscation/keepDts/obfuscation-template.txt';

        newObConfigResolver.resolveKeepConfig(keepConfigs, configs, configPath);

        expect(configs.excludeUniversalPaths).to.have.lengthOf(0);
        expect(configs.keepUniversalPaths).to.have.lengthOf(2);
        expect(configs.excludePathSet).to.have.lengthOf(0);
        expect(configs.keepSourceOfPaths).to.have.lengthOf(0);
      });

      it('should resolve keep config correctly, starts with "!", not starts with "!" and contains "*"', () => {
        const keepConfigs = ['!test/exclude.js', 'test/*/include.js'];
        const configs = new MergedConfig();
        const configPath = './test/testData/obfuscation/keepDts/obfuscation-template.txt';

        newObConfigResolver.resolveKeepConfig(keepConfigs, configs, configPath);

        expect(configs.excludeUniversalPaths).to.have.lengthOf(0);
        expect(configs.keepUniversalPaths).to.have.lengthOf(1);
        expect(configs.excludePathSet).to.have.lengthOf(1);
        expect(configs.keepSourceOfPaths).to.have.lengthOf(0);
      });

      it('should resolve keep config correctly, starts with "!" and not starts with "!"', () => {
        const keepConfigs = ['!test/exclude.js', 'test/include.js'];
        const configs = new MergedConfig();
        const configPath = './test/testData/obfuscation/keepDts/obfuscation-template.txt';

        newObConfigResolver.resolveKeepConfig(keepConfigs, configs, configPath);

        expect(configs.excludeUniversalPaths).to.have.lengthOf(0);
        expect(configs.keepUniversalPaths).to.have.lengthOf(0);
        expect(configs.excludePathSet).to.have.lengthOf(1);
        expect(configs.keepSourceOfPaths).to.have.lengthOf(0);
      });

      it('should resolve keep config correctly, both start with "!"', () => {
        const keepConfigs = ['!test/exclude.js', '!test/include.js'];
        const configs = new MergedConfig();
        const configPath = './test/testData/obfuscation/keepDts/obfuscation-template.txt';

        newObConfigResolver.resolveKeepConfig(keepConfigs, configs, configPath);

        expect(configs.excludeUniversalPaths).to.have.lengthOf(0);
        expect(configs.keepUniversalPaths).to.have.lengthOf(0);
        expect(configs.excludePathSet).to.have.lengthOf(2);
        expect(configs.keepSourceOfPaths).to.have.lengthOf(0);
      });
    });

    describe('resolvePath', () => {
      it('should return the absolute path if token is already an absolute path', () => {
        const configPath = '/home/user/config.json';
        const token = '/home/user/data.txt';
        const result = newObConfigResolver.resolvePathForTest(configPath, token);
        expect(result).to.equal(token);
      });

      it('should resolve the relative path based on the config file directory', () => {
        const configPath = '/home/user/config.json';
        const token = 'data.txt';
        const expectedResult = '/home/user/data.txt';
        const result = newObConfigResolver.resolvePathForTest(configPath, token);
        expect(result).to.equal(expectedResult);
      });
    });

    describe('getTokenType', () => {
      it('should return the correct OptionType for each token', () => {
        const tokens = [
          ObConfigResolver.KEEP_DTS,
          ObConfigResolver.KEEP_GLOBAL_NAME,
          ObConfigResolver.KEEP_PROPERTY_NAME,
          ObConfigResolver.KEEP_FILE_NAME,
          ObConfigResolver.KEEP_COMMENTS,
          ObConfigResolver.DISABLE_OBFUSCATION,
          ObConfigResolver.ENABLE_PROPERTY_OBFUSCATION,
          ObConfigResolver.ENABLE_STRING_PROPERTY_OBFUSCATION,
          ObConfigResolver.ENABLE_TOPLEVEL_OBFUSCATION,
          ObConfigResolver.ENABLE_FILENAME_OBFUSCATION,
          ObConfigResolver.ENABLE_EXPORT_OBFUSCATION,
          ObConfigResolver.ENABLE_LIB_OBFUSCATION_OPTIONS,
          ObConfigResolver.REMOVE_COMMENTS,
          ObConfigResolver.COMPACT,
          ObConfigResolver.REMOVE_LOG,
          ObConfigResolver.PRINT_NAMECACHE,
          ObConfigResolver.PRINT_KEPT_NAMES,
          ObConfigResolver.APPLY_NAMECACHE,
          ObConfigResolver.KEEP,
          ObConfigResolver.EXTRA_OPTIONS,
          ObConfigResolver.STRIP_LANGUAGE_DEFAULT,
          ObConfigResolver.STRIP_SYSTEM_API_ARGS,
          'unknown_token'
        ];

        const expectedResults = [
          OptionTypeForTest.KEEP_DTS,
          OptionTypeForTest.KEEP_GLOBAL_NAME,
          OptionTypeForTest.KEEP_PROPERTY_NAME,
          OptionTypeForTest.KEEP_FILE_NAME,
          OptionTypeForTest.KEEP_COMMENTS,
          OptionTypeForTest.DISABLE_OBFUSCATION,
          OptionTypeForTest.ENABLE_PROPERTY_OBFUSCATION,
          OptionTypeForTest.ENABLE_STRING_PROPERTY_OBFUSCATION,
          OptionTypeForTest.ENABLE_TOPLEVEL_OBFUSCATION,
          OptionTypeForTest.ENABLE_FILENAME_OBFUSCATION,
          OptionTypeForTest.ENABLE_EXPORT_OBFUSCATION,
          OptionTypeForTest.ENABLE_LIB_OBFUSCATION_OPTIONS,
          OptionTypeForTest.REMOVE_COMMENTS,
          OptionTypeForTest.COMPACT,
          OptionTypeForTest.REMOVE_LOG,
          OptionTypeForTest.PRINT_NAMECACHE,
          OptionTypeForTest.PRINT_KEPT_NAMES,
          OptionTypeForTest.APPLY_NAMECACHE,
          OptionTypeForTest.KEEP,
          OptionTypeForTest.EXTRA_OPTIONS,
          OptionTypeForTest.STRIP_LANGUAGE_DEFAULT,
          OptionTypeForTest.STRIP_SYSTEM_API_ARGS,
          OptionTypeForTest.NONE
        ];

        for (let i = 0; i < tokens.length; i++) {
          const result = newObConfigResolver.getTokenTypeForTest(tokens[i]);
          expect(result).to.equal(expectedResults[i]);
        }
      });
    });

    describe('handleConfigContent', () => {
      it('should handle config content correctly', () => {
        const configs: MergedConfig = new MergedConfig();
        configs.options = new ObOptionsForTest();

        const configPath = './test/testData/obfuscation/keepDts/obfuscation-template.txt';
        const data = `
          #This is a comment
          -none,
          -keep obfuscation-template.txt,
          -keep-dts,
          -keep-global-name,
          -keep-property-name,
          -keep-file-name,
          -keep-comments,
          -disable-obfuscation,
          -enable-property-obfuscation,
          -enable-string-property-obfuscation,
          -enable-toplevel-obfuscation,
          -enable-filename-obfuscation,
          -enable-export-obfuscation,
          -enable-lib-obfuscation-options,
          -use-keep-in-source,
          -extra-options strip-language-default,
          -extra-options strip-system-api-args,
          -keep-parameter-names
          -remove-comments,
          -compact,
          -remove-log,
          -print-namecache obfuscation-template.txt,
          -print-kept-names,
          -apply-namecache obfuscation-template.txt
        `;

        newObConfigResolver.handleConfigContentForTest(data, configs, configPath);

        expect(configs.options.disableObfuscation).to.be.true;
        expect(configs.options.enablePropertyObfuscation).to.be.true;
        expect(configs.options.enableStringPropertyObfuscation).to.be.true;
        expect(configs.options.enableToplevelObfuscation).to.be.true;
        expect(configs.options.removeComments).to.be.true;
        expect(configs.options.enableFileNameObfuscation).to.be.true;
        expect(configs.options.enableExportObfuscation).to.be.true;
        expect(configs.options.enableLibObfuscationOptions).to.be.true;
        expect(configs.options.enableAtKeep).to.be.true;
        expect(configs.options.compact).to.be.true;
        expect(configs.options.removeLog).to.be.true;
        expect(configs.options.stripLanguageDefault).to.be.true;
        expect(configs.options.stripSystemApiArgs).to.be.true;
        expect(configs.options.keepParameterNames).to.be.true;
      });

      it('should handle config content correctly when diffenent combinations use line break splitting', () => {
        const configs: MergedConfig = new MergedConfig();
        configs.options = new ObOptionsForTest();

        const configPath = './test/testData/obfuscation/keepDts/obfuscation-template.txt';
        const data = `
          #This is a comment
          -extra-options
          strip-language-default
          -extra-options
          strip-system-api-args
        `;

        newObConfigResolver.handleConfigContentForTest(data, configs, configPath);

        expect(configs.options.stripLanguageDefault).to.be.true;
        expect(configs.options.stripSystemApiArgs).to.be.true;
      });

      it('should handle config content correctly when diffenent combinations use line break splitting', () => {
        const configs: MergedConfig = new MergedConfig();
        configs.options = new ObOptionsForTest();

        const configPath = './test/testData/obfuscation/keepDts/obfuscation-template.txt';
        const data = `
          #This is a comment
          -extra-options strip-language-default
          -extra-options strip-system-api-args
        `;

        newObConfigResolver.handleConfigContentForTest(data, configs, configPath);

        expect(configs.options.stripLanguageDefault).to.be.true;
        expect(configs.options.stripSystemApiArgs).to.be.true;
      });

      it('should handle config content correctly when diffenent combinations use space separation', () => {
        const configs: MergedConfig = new MergedConfig();
        configs.options = new ObOptionsForTest();

        const configPath = './test/testData/obfuscation/keepDts/obfuscation-template.txt';
        const data = `
          #This is a comment
          -extra-options strip-language-default -extra-options strip-system-api-args
        `;

        newObConfigResolver.handleConfigContentForTest(data, configs, configPath);

        expect(configs.options.stripLanguageDefault).to.be.true;
        expect(configs.options.stripSystemApiArgs).to.be.true;
      });

      it('should handle config content correctly when diffenent combinations only use one "-extra-options"', () => {
        const configs: MergedConfig = new MergedConfig();
        configs.options = new ObOptionsForTest();

        const configPath = './test/testData/obfuscation/keepDts/obfuscation-template.txt';
        const data = `
          #This is a comment
          -extra-options strip-language-default, strip-system-api-args
        `;

        newObConfigResolver.handleConfigContentForTest(data, configs, configPath);

        expect(configs.options.stripLanguageDefault).to.be.true;
        expect(configs.options.stripSystemApiArgs).to.be.true;
      });

      it('should handle config content correctly when no "-extra-options"', () => {
        const configs: MergedConfig = new MergedConfig();
        configs.options = new ObOptionsForTest();

        const configPath = './test/testData/obfuscation/keepDts/obfuscation-template.txt';
        const data = `
          #This is a comment
          strip-language-default, strip-system-api-args
        `;

        newObConfigResolver.handleConfigContentForTest(data, configs, configPath);

        expect(configs.options.stripLanguageDefault).to.be.false;
        expect(configs.options.stripSystemApiArgs).to.be.false;
      });

      it('should handle config content correctly when have invalid token between "-extra-options" and "strip-language-default"', () => {
        const configs: MergedConfig = new MergedConfig();
        configs.options = new ObOptionsForTest();
        
        const configPath = './test/testData/obfuscation/keepDts/obfuscation-template.txt';
        const data = `
          #This is a comment
          -extra-options
          invalid-token
          strip-system-api-args
          strip-language-default
        `;

        newObConfigResolver.handleConfigContentForTest(data, configs, configPath);

        expect(configs.options.stripLanguageDefault).to.be.false;
        expect(configs.options.stripSystemApiArgs).to.be.false;
      });

      it('should handle config content correctly when have valid token between "-extra-options" and "strip-language-default"', () => {
        const configs: MergedConfig = new MergedConfig();
        configs.options = new ObOptionsForTest();

        const configPath = './test/testData/obfuscation/keepDts/obfuscation-template.txt';
        const data = `
          #This is a comment
          -extra-options
          -enable-property-obfuscation
          strip-language-default
          strip-system-api-args
        `;

        newObConfigResolver.handleConfigContentForTest(data, configs, configPath);

        expect(configs.options.enablePropertyObfuscation).to.be.true;
        expect(configs.options.stripLanguageDefault).to.be.false;
        expect(configs.options.stripSystemApiArgs).to.be.false;
      });

      it('should handle config content correctly when enable lib obfuscation options', () => {
        const configs: MergedConfig = new MergedConfig();
        configs.options = new ObOptionsForTest();

        const configPath = './test/testData/obfuscation/enable_lib_obfuscation_options/obfuscation-rule.txt';
        const data = fs.readFileSync(configPath, 'utf-8');

        newObConfigResolver.handleConfigContentForTest(data, configs, configPath);

        expect(configs.options.enableLibObfuscationOptions).to.be.true;
      });
      
      it('should handle config correctly when enable bytecode obfuscation options', () => {
        const configs: MergedConfig = new MergedConfig();
        configs.options = new ObOptionsForTest();

        const configPath = './test/testData/obfuscation/enable_lib_obfuscation_options/obfuscation-rule.txt';
        const data = `
        -enable-bytecode-obfuscation
        -enable-bytecode-obfuscation-debugging
        -enable-bytecode-obfuscation-enhanced
        -enable-bytecode-obfuscation-arkui
      `;
        newObConfigResolver.handleConfigContentForTest(data, configs, configPath);
        expect(configs.options.bytecodeObf.enable).to.be.true;
        expect(configs.options.bytecodeObf.debugging).to.be.true;
        expect(configs.options.bytecodeObf.enhanced).to.be.true;
        expect(configs.options.bytecodeObf.obfArkUI).to.be.true;
      });
    });

    describe('1: test Api collectSystemApiWhitelist', function() {
      it('1-1: test collectSystemApiWhitelist: -enable-property-obfuscation', function () {
        let obfuscationCacheDir = path.join(OBFUSCATE_TESTDATA_DIR, 'property');
        let obfuscationOptions = {
          'selfConfig': {
            'ruleOptions': {
              'enable': true,
              'rules': [
                path.join(OBFUSCATE_TESTDATA_DIR, 'property/property.txt')
              ]
            },
            'consumerRules': [],
          },
          'dependencies': {
            'libraries': [],
            'hars': []
          },
          'obfuscationCacheDir': obfuscationCacheDir,
          'sdkApis': [
            path.join(OBFUSCATE_TESTDATA_DIR, 'system_api.d.ts')
          ]
        };
        let projectConfig = {
          obfuscationOptions,
          compileHar: false
        };
        const obConfig: ObConfigResolver =  new ObConfigResolver(projectConfig, undefined);
        obConfig.resolveObfuscationConfigs();
        const reservedSdkApiForProp = UnobfuscationCollections.reservedSdkApiForProp;
        const reservedSdkApiForGlobal = UnobfuscationCollections.reservedSdkApiForGlobal;
        const reservedSdkApiForLocal = UnobfuscationCollections.reservedSdkApiForLocal;

        expect(reservedSdkApiForProp.size == 12).to.be.true;
        expect(reservedSdkApiForProp.has('TestClass')).to.be.true;
        expect(reservedSdkApiForProp.has('para1')).to.be.true;
        expect(reservedSdkApiForProp.has('para2')).to.be.true;
        expect(reservedSdkApiForProp.has('foo')).to.be.true;
        expect(reservedSdkApiForProp.has('TestFunction')).to.be.true;
        expect(reservedSdkApiForProp.has('funcPara1')).to.be.true;
        expect(reservedSdkApiForProp.has('funcPara2')).to.be.true;
        expect(reservedSdkApiForProp.has('ns')).to.be.true;
        expect(reservedSdkApiForProp.has('NavigationBuilderRegister')).to.be.true;
        expect(reservedSdkApiForProp.has('ViewV2')).to.be.true;
        expect(reservedSdkApiForProp.has('initParam')).to.be.true;
        expect(reservedSdkApiForProp.has('updateParam')).to.be.true;
        expect(reservedSdkApiForProp.has('NavigationBuilderRegister')).to.be.true;
        expect(reservedSdkApiForProp.has('__Recycle__')).to.be.false;
        expect(reservedSdkApiForGlobal.size === 0).to.be.true;
        expect(reservedSdkApiForLocal.size === 8).to.be.true;
        UnobfuscationCollections.clear();

        let systemApiPath = obfuscationCacheDir + '/systemApiCache.json';
        const data = fs.readFileSync(systemApiPath, 'utf8');
        const systemApiContent = JSON.parse(data);

        expect(systemApiContent.ReservedPropertyNames.length == 12).to.be.true;
        expect(systemApiContent.ReservedPropertyNames.includes('TestClass')).to.be.true;
        expect(systemApiContent.ReservedPropertyNames.includes('para1')).to.be.true;
        expect(systemApiContent.ReservedPropertyNames.includes('para2')).to.be.true;
        expect(systemApiContent.ReservedPropertyNames.includes('foo')).to.be.true;
        expect(systemApiContent.ReservedPropertyNames.includes('TestFunction')).to.be.true;
        expect(systemApiContent.ReservedPropertyNames.includes('funcPara1')).to.be.true;
        expect(systemApiContent.ReservedPropertyNames.includes('funcPara2')).to.be.true;
        expect(systemApiContent.ReservedPropertyNames.includes('ns')).to.be.true;
        expect(systemApiContent.ReservedPropertyNames.includes('NavigationBuilderRegister')).to.be.true;
        expect(systemApiContent.ReservedPropertyNames.includes('ViewV2')).to.be.true;
        expect(systemApiContent.ReservedPropertyNames.includes('initParam')).to.be.true;
        expect(systemApiContent.ReservedPropertyNames.includes('updateParam')).to.be.true;
        expect(systemApiContent.ReservedPropertyNames.includes('NavigationBuilderRegister')).to.be.true;
        expect(systemApiContent.ReservedPropertyNames.includes('__Recycle__')).to.be.false;
        expect(systemApiContent.ReservedGlobalNames === undefined).to.be.true;
        expect(systemApiContent.ReservedLocalNames.size === 8).to.be.false;

        fs.unlinkSync(systemApiPath);
      });

      it('1-2: test collectSystemApiWhitelist: -enable-export-obfuscation', function () {
        let obfuscationCacheDir = path.join(OBFUSCATE_TESTDATA_DIR, 'export');
        let obfuscationOptions = {
          'selfConfig': {
            'ruleOptions': {
              'enable': true,
              'rules': [
                path.join(OBFUSCATE_TESTDATA_DIR, 'export/export.txt')
              ]
            },
            'consumerRules': [],
          },
          'dependencies': {
            'libraries': [],
            'hars': []
          },
          'obfuscationCacheDir': obfuscationCacheDir,
          'sdkApis': [
            path.join(OBFUSCATE_TESTDATA_DIR, 'system_api.d.ts')
          ]
        };
        let projectConfig = {
          obfuscationOptions,
          compileHar: false
        };
        const obConfig: ObConfigResolver =  new ObConfigResolver(projectConfig, undefined);
        obConfig.resolveObfuscationConfigs();
        const reservedSdkApiForProp = UnobfuscationCollections.reservedSdkApiForProp;
        const reservedSdkApiForGlobal = UnobfuscationCollections.reservedSdkApiForGlobal;

        expect(reservedSdkApiForProp.size == 0).to.be.true;
        expect(reservedSdkApiForGlobal.size == 0).to.be.true;
        UnobfuscationCollections.clear();

        let systemApiPath = obfuscationCacheDir + '/systemApiCache.json';
        const noSystemApi = fs.existsSync(systemApiPath);

        expect(noSystemApi).to.be.false;
      });

      it('1-3: test collectSystemApiWhitelist: -enable-export-obfuscation -enable-toplevel-obfuscation', function () {
        let obfuscationCacheDir = path.join(OBFUSCATE_TESTDATA_DIR, 'export_toplevel');
        let obfuscationOptions = {
          'selfConfig': {
            'ruleOptions': {
              'enable': true,
              'rules': [
                path.join(OBFUSCATE_TESTDATA_DIR, 'export_toplevel/export_toplevel.txt')
              ]
            },
            'consumerRules': [],
          },
          'dependencies': {
            'libraries': [],
            'hars': []
          },
          'obfuscationCacheDir': obfuscationCacheDir,
          'sdkApis': [
            path.join(OBFUSCATE_TESTDATA_DIR, 'system_api.d.ts')
          ]
        };
        let projectConfig = {
          obfuscationOptions,
          compileHar: false
        };
        const obConfig: ObConfigResolver =  new ObConfigResolver(projectConfig, undefined);
        obConfig.resolveObfuscationConfigs();
        const reservedSdkApiForProp = UnobfuscationCollections.reservedSdkApiForProp;
        const reservedSdkApiForGlobal = UnobfuscationCollections.reservedSdkApiForGlobal;

        expect(reservedSdkApiForProp.size == 0).to.be.true;
        expect(reservedSdkApiForGlobal.size == 3).to.be.true;
        expect(reservedSdkApiForGlobal.has('TestClass')).to.be.true;
        expect(reservedSdkApiForGlobal.has('TestFunction')).to.be.true;
        expect(reservedSdkApiForGlobal.has('ns')).to.be.true;
        UnobfuscationCollections.clear();

        let systemApiPath = obfuscationCacheDir + '/systemApiCache.json';
        const data = fs.readFileSync(systemApiPath, 'utf8');
        const systemApiContent = JSON.parse(data);

        expect(systemApiContent.ReservedPropertyNames == undefined).to.be.true;
        expect(systemApiContent.ReservedGlobalNames.length == 3).to.be.true;
        expect(systemApiContent.ReservedGlobalNames.includes('TestClass')).to.be.true;
        expect(systemApiContent.ReservedGlobalNames.includes('TestFunction')).to.be.true;
        expect(systemApiContent.ReservedGlobalNames.includes('ns')).to.be.true;

        fs.unlinkSync(systemApiPath);
      });

      it('1-4: test collectSystemApiWhitelist: system api when -enable-property-obfuscation and -extra-options strip-system-api-args', function () {
        let obfuscationCacheDir = path.join(OBFUSCATE_TESTDATA_DIR, 'system_api_optimize');
        let obfuscationOptions = {
          'selfConfig': {
            'ruleOptions': {
              'enable': true,
              'rules': [
                path.join(OBFUSCATE_TESTDATA_DIR, 'system_api_optimize/system_api_optimize.txt')
              ]
            },
            'consumerRules': [],
          },
          'dependencies': {
            'libraries': [],
            'hars': []
          },
          'obfuscationCacheDir': obfuscationCacheDir,
          'sdkApis': [
            path.join(OBFUSCATE_TESTDATA_DIR, 'system_api.d.ts')
          ]
        };
        let projectConfig = {
          obfuscationOptions,
          compileHar: false
        };
        const obConfig: ObConfigResolver =  new ObConfigResolver(projectConfig, undefined);
        obConfig.resolveObfuscationConfigs();
        const reservedSdkApiForProp = UnobfuscationCollections.reservedSdkApiForProp;
        const reservedSdkApiForGlobal = UnobfuscationCollections.reservedSdkApiForGlobal;
        const reservedSdkApiForLocal = UnobfuscationCollections.reservedSdkApiForLocal;

        expect(reservedSdkApiForProp.has('TestClass')).to.be.true;
        expect(reservedSdkApiForProp.has('para1')).to.be.true;
        expect(reservedSdkApiForProp.has('para2')).to.be.true;
        expect(reservedSdkApiForProp.has('foo')).to.be.true;
        expect(reservedSdkApiForProp.has('TestFunction')).to.be.true;
        expect(reservedSdkApiForProp.has('funcPara1')).to.be.false;
        expect(reservedSdkApiForProp.has('funcPara2')).to.be.false;
        expect(reservedSdkApiForProp.has('ns')).to.be.true;
        expect(reservedSdkApiForProp.has('NavigationBuilderRegister')).to.be.true;
        expect(reservedSdkApiForProp.has('__Recycle__')).to.be.true;
        expect(reservedSdkApiForGlobal.size == 0).to.be.true;
        expect(reservedSdkApiForLocal.size == 0).to.be.true;
        UnobfuscationCollections.clear();

        let systemApiPath = obfuscationCacheDir + '/systemApiCache.json';
        const data = fs.readFileSync(systemApiPath, 'utf8');
        const systemApiContent = JSON.parse(data);

        expect(systemApiContent.ReservedPropertyNames.includes('TestClass')).to.be.true;
        expect(systemApiContent.ReservedPropertyNames.includes('para1')).to.be.true;
        expect(systemApiContent.ReservedPropertyNames.includes('para2')).to.be.true;
        expect(systemApiContent.ReservedPropertyNames.includes('foo')).to.be.true;
        expect(systemApiContent.ReservedPropertyNames.includes('TestFunction')).to.be.true;
        expect(systemApiContent.ReservedPropertyNames.includes('funcPara1')).to.be.false;
        expect(systemApiContent.ReservedPropertyNames.includes('funcPara2')).to.be.false;
        expect(systemApiContent.ReservedPropertyNames.includes('ns')).to.be.true;
        expect(systemApiContent.ReservedPropertyNames.includes('NavigationBuilderRegister')).to.be.true;
        expect(systemApiContent.ReservedPropertyNames.includes('__Recycle__')).to.be.true;
        expect(systemApiContent.ReservedGlobalNames === undefined).to.be.true;
        expect(systemApiContent.ReservedLocalNames === undefined).to.be.true;

        fs.unlinkSync(systemApiPath);
      });

      it('1-5: test collectSystemApiWhitelist: -enable-bytecode-obfuscation', function () {
        let obfuscationCacheDir = path.join(OBFUSCATE_TESTDATA_DIR, 'bytecode_obfuscation');
        let obfuscationOptions = {
          'selfConfig': {
            'ruleOptions': {
              'enable': true,
              'rules': [
                path.join(OBFUSCATE_TESTDATA_DIR, 'bytecode_obfuscation/bytecode_obfuscation.txt')
              ]
            },
            'consumerRules': [],
          },
          'dependencies': {
            'libraries': [],
            'hars': []
          },
          'obfuscationCacheDir': obfuscationCacheDir,
          'sdkApis': [
            path.join(OBFUSCATE_TESTDATA_DIR, 'system_api.d.ts')
          ]
        };
        let projectConfig = {
          obfuscationOptions,
          compileHar: false
        };
        const obConfig: ObConfigResolver =  new ObConfigResolver(projectConfig, undefined);
        obConfig.resolveObfuscationConfigs();
        const reservedSdkApiForProp = UnobfuscationCollections.reservedSdkApiForProp;
        const reservedSdkApiForGlobal = UnobfuscationCollections.reservedSdkApiForGlobal;
        const reservedSdkApiForLocal = UnobfuscationCollections.reservedSdkApiForLocal;

        expect(reservedSdkApiForProp.size == 12).to.be.true;
        expect(reservedSdkApiForGlobal.size == 3).to.be.true;
        expect(reservedSdkApiForLocal.size == 8).to.be.true;
        UnobfuscationCollections.clear();

        let systemApiPath = obfuscationCacheDir + '/systemApiCache.json';
        const data = fs.readFileSync(systemApiPath, 'utf8');
        const systemApiContent = JSON.parse(data);

        expect(systemApiContent.ReservedLocalNames.length == 8).to.be.true;
        expect(systemApiContent.ReservedPropertyNames.length == 12).to.be.true;
        expect(systemApiContent.ReservedGlobalNames.length == 3).to.be.true;
        fs.unlinkSync(systemApiPath);
      });
    });
  });

  describe('getMergedConfigs', function() {
    it('should merge all configs', () => {
      const config1 = new MergedConfig();
      config1.options.enableLibObfuscationOptions = true;
  
      const config2 = new MergedConfig();
      config2.options.enablePropertyObfuscation = true;
      config2.options.enableStringPropertyObfuscation = true;
      config2.options.enableToplevelObfuscation = true;
      config2.options.compact = true;
      config2.options.removeLog = true;
      config2.reservedPropertyNames = ['prop2'];
      config2.reservedGlobalNames = ['global2'];
      config2.keepComments = ['comment2'];
      config2.excludePathSet.add('path2');
  
      const projectConfig = {
        obfuscationOptions: { option1: 'value1' },
        compileHar: false
      };
      const logger = console;
      const isTerser = false;
      let newObConfigResolver = new ObConfigResolver(projectConfig, printObfLogger, isTerser);
      const res: MergedConfig = newObConfigResolver.getMergedConfigsForTest(config1, config2);
  
      expect(res.options.enablePropertyObfuscation).to.be.true;
      expect(res.options.enableStringPropertyObfuscation).to.be.true;
      expect(res.options.enableToplevelObfuscation).to.be.true;
      expect(res.options.compact).to.be.true;
      expect(res.options.removeLog).to.be.true;
      expect(res.reservedPropertyNames).to.deep.equal(['prop2']);
      expect(res.reservedGlobalNames).to.deep.equal(['global2']);
      expect(res.keepComments).to.deep.equal(['comment2']);
      expect(res.excludePathSet).to.deep.equal(new Set(['path2']));
    });

    it('should merge only keep configs', () => {
      const config1 = new MergedConfig();
      config1.options.enableLibObfuscationOptions = false;
  
      const config2 = new MergedConfig();
      config2.options.enablePropertyObfuscation = true;
      config2.options.enableStringPropertyObfuscation = true;
      config2.options.enableToplevelObfuscation = true;
      config2.options.compact = true;
      config2.options.removeLog = true;
      config2.reservedPropertyNames = ['prop2'];
      config2.reservedGlobalNames = ['global2'];
      config2.keepComments = ['comment2'];
      config2.excludePathSet.add('path2');
  
      const projectConfig = {
        obfuscationOptions: { option1: 'value1' },
        compileHar: false
      };
      const logger = console;
      const isTerser = false;
      let newObConfigResolver = new ObConfigResolver(projectConfig, printObfLogger, isTerser);
      const res: MergedConfig = newObConfigResolver.getMergedConfigsForTest(config1, config2);
  
      expect(res.options.enablePropertyObfuscation).to.be.false;
      expect(res.options.enableStringPropertyObfuscation).to.be.false;
      expect(res.options.enableToplevelObfuscation).to.be.false;
      expect(res.options.compact).to.be.false;
      expect(res.options.removeLog).to.be.false;
      expect(res.reservedPropertyNames).to.deep.equal(['prop2']);
      expect(res.reservedGlobalNames).to.deep.equal(['global2']);
      expect(res.keepComments).to.deep.equal(['comment2']);
      expect(res.excludePathSet).to.deep.equal(new Set(['path2']));
    });
  });

  describe('genConsumerConfigFilesForTest', function (){
    it('should merge all configs: compileHar is true', () => {
      const config1 = new MergedConfig();
      config1.options.enableLibObfuscationOptions = true;
  
      const config2 = new MergedConfig();
      config2.options.enablePropertyObfuscation = true;
      config2.options.enableStringPropertyObfuscation = true;
      config2.options.enableToplevelObfuscation = true;
      config2.options.compact = true;
      config2.options.removeLog = true;
      config2.reservedPropertyNames = ['prop2'];
      config2.reservedGlobalNames = ['global2'];
      config2.keepComments = ['comment2'];
      config2.excludePathSet.add('path2');
  
      const projectConfig = {
        obfuscationOptions: { option1: 'value1' },
        compileHar: true
      };
      const logger = console;
      const isTerser = false;
      let newObConfigResolver = new ObConfigResolver(projectConfig, printObfLogger, isTerser);
      const sourceObConfig: SourceObConfig = {
        selfConfig: {} as Obfuscation,
        sdkApis: [],
        obfuscationCacheDir: '',
        exportRulePath: './test/ut/initialization/obfuscation.txt',
        dependencies: {
          libraries: [],
          hars: [],
          hsps: [],
          hspLibraries: [],
        },
      };
      newObConfigResolver.genConsumerConfigFilesForTest(sourceObConfig, config1, config2);

      let res: string = fs.readFileSync(sourceObConfig.exportRulePath, 'utf-8');
      expect(res.indexOf('-enable-lib-obfuscation-options') === -1).to.be.true;
      expect(res.indexOf('-enable-property-obfuscation') !== -1).to.be.true;
      expect(res.indexOf('-enable-string-property-obfuscation') !== -1).to.be.true;
      expect(res.indexOf('-enable-toplevel-obfuscation') !== -1).to.be.true;
      expect(res.indexOf('-compact') !== -1).to.be.true;
      expect(res.indexOf('-remove-log') !== -1).to.be.true;
      expect(res.indexOf('-keep-global-name') !== -1).to.be.true;
      expect(res.indexOf('global2') !== -1).to.be.true;
      expect(res.indexOf('-keep-property-name') !== -1).to.be.true;
      expect(res.indexOf('prop2') !== -1).to.be.true;
      
      fs.unlinkSync(sourceObConfig.exportRulePath);
    });

    it('should merge all configs: compileHar is false', () => {
      const config1 = new MergedConfig();
      config1.options.enableLibObfuscationOptions = false;
  
      const config2 = new MergedConfig();
      config2.options.enablePropertyObfuscation = true;
      config2.options.enableStringPropertyObfuscation = true;
      config2.options.enableToplevelObfuscation = true;
      config2.options.compact = true;
      config2.options.removeLog = true;
      config2.reservedPropertyNames = ['prop2'];
      config2.reservedGlobalNames = ['global2'];
      config2.keepComments = ['comment2'];
      config2.excludePathSet.add('path2');
  
      const projectConfig = {
        obfuscationOptions: { option1: 'value1' },
        compileHar: false
      };
      const logger = console;
      const isTerser = false;
      let newObConfigResolver = new ObConfigResolver(projectConfig, printObfLogger, isTerser);
      const sourceObConfig: SourceObConfig = {
        selfConfig: {} as Obfuscation,
        sdkApis: [],
        obfuscationCacheDir: '',
        exportRulePath: './test/ut/initialization/obfuscation.txt',
        dependencies: {
          libraries: [],
          hars: [],
          hsps: [],
          hspLibraries: [],
        },
      };
      newObConfigResolver.genConsumerConfigFilesForTest(sourceObConfig, config1, config2);

      let res: string = fs.readFileSync(sourceObConfig.exportRulePath, 'utf-8');

      expect(res.indexOf('-enable-lib-obfuscation-options') === -1).to.be.true;
      expect(res.indexOf('-enable-property-obfuscation') === -1).to.be.true;
      expect(res.indexOf('-enable-string-property-obfuscation') === -1).to.be.true;
      expect(res.indexOf('-enable-toplevel-obfuscation') === -1).to.be.true;
      expect(res.indexOf('-compact') === -1).to.be.true;
      expect(res.indexOf('-remove-log') === -1).to.be.true;
      expect(res.indexOf('-keep-global-name') === -1).to.be.true;
      expect(res.indexOf('global2') === -1).to.be.true;
      expect(res.indexOf('-keep-property-name') === -1).to.be.true;
      expect(res.indexOf('prop2') === -1).to.be.true;
      
      fs.unlinkSync(sourceObConfig.exportRulePath);
    });
  });

  describe('collectResevedFileNameInIDEConfig', () => {
    let collectPathReservedStringStub;

    beforeEach(function() {
      collectPathReservedStringStub = sinon.stub(FileUtils, 'collectPathReservedString');
      collectPathReservedStringStub.callsFake((filePath, reservedArray) => reservedArray.push(filePath));
    });

    afterEach(function() {
      collectPathReservedStringStub.restore();
      projectConfig.compileShared = false;
      projectConfig.byteCodeHar = false;
      projectConfig.aceModuleJsonPath = '';
    });

    const ohPackagePath = './test/ut/initialization/testOhPackagePath.json';
    let projectConfig = {
      aceModuleJsonPath: '',
      projectPath: 'path/to/project',
      cachePath: 'path/to/cache',
      compileShared: false,
      byteCodeHar: false,
      aceModuleBuild: 'path/to/build',
    }
    const entryArray = ['path/to/entry1', 'path/to/entry2'];
    const modulePathMap = {
      module1: 'path/to/module1',
      module2: 'path/to/module2',
    };

    it('should collect reserved file names from entryArray and projectConfig', () => {
      const result = collectResevedFileNameInIDEConfig('', projectConfig, undefined, entryArray);
      expect(result).to.deep.equal(['path/to/entry1', 'path/to/entry2','path/to/project','path/to/cache']);
    });
    it('should collect reserved file names from modulePathMap and projectConfig', () => {
      projectConfig.compileShared = true;
      const result = collectResevedFileNameInIDEConfig('', projectConfig, modulePathMap, []);
      expect(result).to.deep.equal(['path/to/module1', 'path/to/module2','module1','module2','path/to/build','etsFortgz','path/to/project','path/to/cache']);
    });
    it('should collect reserved file names from ohPackagePath and projectConfig', () => {
      projectConfig.byteCodeHar = true;
      const result = collectResevedFileNameInIDEConfig(ohPackagePath, projectConfig, undefined, []);
      expect(result).to.deep.equal(['path/to/main', 'path/to/types','path/to/build','etsFortgz','path/to/project','path/to/cache']);
    });
    it('should collect reserved file names from moduleJsonPath and projectConfig', () => {
      projectConfig.aceModuleJsonPath = "./test/ut/initialization/testModuleJsonPath_0.json";
      const hasSrcEntry = collectResevedFileNameInIDEConfig('', projectConfig, undefined, []);
      expect(hasSrcEntry).to.deep.equal(['path/to/srcEntry','path/to/project','path/to/cache']);

      projectConfig.aceModuleJsonPath = "./test/ut/initialization/testModuleJsonPath_1.json";
      const noSrcEntry = collectResevedFileNameInIDEConfig('', projectConfig, undefined, []);
      expect(noSrcEntry).to.deep.equal(['path/to/project','path/to/cache']);
    });
  });

  describe('readNameCache', () => {
    let tempFilePath;
    let testData;
    let fsWriteFileSyncStub;
    let fsUnlinkSyncStub;
    let PropCollections;
    let renameFileNameModule;

    beforeEach(() => {
      PropCollections = {
        historyMangledTable: {},
      };
      renameFileNameModule = {
        historyFileNameMangledTable: {},
      };

      tempFilePath = './test/ut/initialization/tempNameCache.json';
      testData = {
        compileSdkVersion: '1.0.0',
        PropertyCache: { key1: 'value1', key2: 'value2' },
        FileNameCache: { key3: 'value3', key4: 'value4' },
        extraKey: '',
      };

      fsWriteFileSyncStub = sinon.stub(fs, 'writeFileSync').callsFake((path, data) => {});
      fsUnlinkSyncStub = sinon.stub(fs, 'unlinkSync').callsFake((path) => {});
    });

    afterEach(() => {
      fsWriteFileSyncStub.restore();
      fsUnlinkSyncStub.restore();
    });

    it('should read and parse the name cache file correctly', () => {
      readNameCache(tempFilePath, printObfLogger);

      expect(nameCacheMap.get('extraKey')).to.equal('');
    });
  });

  describe('readNameCache', () => {
    let fsReadFileSyncStub;
    let logger;
    const testPath = './test/ut/initialization/tempNameCache.json';

    beforeEach(() => {
      fsReadFileSyncStub = sinon.stub(fs, 'readFileSync').returns('{"compileSdkVersion":"1.0","PropertyCache":{},"FileNameCache":{}}');
      logger = { error: sinon.spy(), printError: (msg)=>{}};
    });

    afterEach(() => {
      fsReadFileSyncStub.restore();
    });

    it('should read the name cache file and parse its content', () => {
      readNameCache(testPath, printObfLogger);
      expect(PropCollections.historyMangledTable).to.deep.equal(new Map());
      expect(renameFileNameModule.historyFileNameMangledTable).to.deep.equal(new Map());
      expect(nameCacheMap.get('compileSdkVersion')).to.be.undefined;
      expect(logger.error.called).to.be.false;
    });

    it('should handle errors when reading the file', () => {
      const mockLogger = {
        error: (message: string) => console.error(message),
      };

      const nonExistentFilePath = './test/ut/initialization/nonExistentFile.json';
      try {
        readNameCache(nonExistentFilePath, printObfLogger);
      } catch (err) {
      }
      expect(logger.error.calledWith(`Failed to open ${nonExistentFilePath}. Error message: Test error`)).to.be.false;
    });

    it('should handle errors when reading the file with hvigor errorCode', () => {
      const mockLogger = {
        error: (message: string) => console.error(message),
      };

      const nonExistentFilePath = './test/ut/initialization/nonExistentFile.json';
      try {
        readNameCache(nonExistentFilePath, printObfHvigorLogger);
      } catch (err) {
      }
      const errorMessage = {
        code: '10804002',
        description: 'ArkTS compiler Error',
        cause: `Failed to open namecache file from ${nonExistentFilePath}, Error message: Test error`,
        position: nonExistentFilePath,
        solutions: [`Please check ${nonExistentFilePath} as error message suggested.`],
      };
      expect(logger.error.calledWith(errorMessage)).to.be.false;
    });
  });

  describe('handleUniversalPathInObf', () => {
    it('should handle universal paths correctly', () => {
      const mergedObConfig: MergedConfig = {
        options: new ObOptionsForTest(),
        reservedPropertyNames: [],
        reservedGlobalNames: [],
        reservedNames: [],
        reservedFileNames: [],
        keepComments: [],
        keepSourceOfPaths: [],
        universalReservedPropertyNames: [],
        universalReservedGlobalNames: [],
        keepUniversalPaths: [/test\.js$/],
        excludeUniversalPaths: [/exclude\.js$/],
        excludePathSet: new Set(),
        mergeKeepOptions: () => {},
        mergeAllRules: () => {},
        sortAndDeduplicate: () => {},
        serializeMergedConfig: () => {
          return JSON.stringify(this);
        }
      };
      const allSourceFilePaths = new Set([
        'test.js',
        'exclude.js',
        'other.js',
      ]);

      handleUniversalPathInObf(mergedObConfig, allSourceFilePaths);
      expect(mergedObConfig.keepSourceOfPaths).to.deep.equal(['test.js']);
      expect(mergedObConfig.excludePathSet).to.deep.equal(new Set(['exclude.js']));
    });

    it('should return early if mergedObConfig is not provided or both keepUniversalPaths and excludeUniversalPaths are empty', () => {
      const mergedObConfig: MergedConfig = {
        options: new ObOptionsForTest(),
        reservedPropertyNames: [],
        reservedGlobalNames: [],
        reservedNames: [],
        reservedFileNames: [],
        keepComments: [],
        keepSourceOfPaths: [],
        universalReservedPropertyNames: [],
        universalReservedGlobalNames: [],
        keepUniversalPaths: [],
        excludeUniversalPaths: [],
        excludePathSet: new Set(),
        mergeKeepOptions: () => {},
        mergeAllRules: () => {},
        sortAndDeduplicate: () => {},
        serializeMergedConfig: () => {
          return JSON.stringify(this);
        }
      };
      const allSourceFilePaths = new Set([]);
      const result = handleUniversalPathInObf(mergedObConfig, allSourceFilePaths);

      expect(result).to.be.undefined;
    });
  });

  describe('getArkguardNameCache', () => {
    it('should return a JSON string with the correct structure', () => {
      const enablePropertyObfuscation = true;
      const enableFileNameObfuscation = true;
      const sdkVersion = '1.0.0';
      const entryPackageInfo = 'packageInfo';

      const result = getArkguardNameCache(enablePropertyObfuscation, enableFileNameObfuscation, false, sdkVersion, entryPackageInfo);

      try {
        JSON.parse(result);
        // If no error is thrown, the result is a valid JSON string
        console.log('Test passed: getArkguardNameCache returns a valid JSON string');
      } catch (error) {
        console.error('Test failed: getArkguardNameCache does not return a valid JSON string');
      }
    });

    it('should include the correct compileSdkVersion and entryPackageInfo', () => {
      const enablePropertyObfuscation = false;
      const enableFileNameObfuscation = false;
      const sdkVersion = '2.0.0';
      const entryPackageInfo = 'anotherPackageInfo';

      const result = getArkguardNameCache(enablePropertyObfuscation, enableFileNameObfuscation, false, sdkVersion, entryPackageInfo);
      const parsedResult = JSON.parse(result);

      expect(parsedResult.compileSdkVersion).to.equal(sdkVersion);
      expect(parsedResult.entryPackageInfo).to.equal(entryPackageInfo);
    });

    it('PropertyCache exists when enable export obfuscation', () => {
      const enablePropertyObfuscation = false;
      const enableFileNameObfuscation = false;
      const enableExportObfuscation = true;
      const sdkVersion = '2.0.0';
      const entryPackageInfo = 'anotherPackageInfo';

      PropCollections.historyMangledTable.set("key1", "value1");
      PropCollections.historyMangledTable.set("key2", "value2");
      PropCollections.globalMangledTable.set("key3", "value3");
      PropCollections.globalMangledTable.set("key4", "value4");
      const result = getArkguardNameCache(enablePropertyObfuscation, enableFileNameObfuscation, enableExportObfuscation, sdkVersion, entryPackageInfo);
      const parsedResult = JSON.parse(result);
      PropCollections.historyMangledTable.clear();
      PropCollections.globalMangledTable.clear();
      expect('PropertyCache' in parsedResult).to.be.true;
      expect(parsedResult.PropertyCache.key1 === "value1").to.be.true;
      expect(parsedResult.PropertyCache.key2 === "value2").to.be.true;
      expect(parsedResult.PropertyCache.key3 === "value3").to.be.true;
      expect(parsedResult.PropertyCache.key4 === "value4").to.be.true;
    });
  });

  describe('fillNameCache', function() {
    it('should correctly fill the name cache with the given table entries', function() {
      const table = new Map([
        ['key1', 'value1'],
        ['key2', 'value2'],
        ['key3', 'value3']
      ]);
      const nameCache = new Map();

      fillNameCache(table, nameCache);

      assert.deepEqual(nameCache, table);
    });

    it('should handle empty tables gracefully', function() {
      const table = new Map();
      const nameCache = new Map();

      fillNameCache(table, nameCache);

      assert.deepEqual(nameCache, table);
    });
  });

  describe('writeObfuscationNameCache', () => {
    let existsSyncSpy;
    let mkdirSyncSpy;
    let writeFileSyncSpy;

    beforeEach(function() {
      existsSyncSpy = sinon.spy(fs, 'existsSync');
      mkdirSyncSpy = sinon.spy(fs, 'mkdirSync');
      writeFileSyncSpy = sinon.spy(fs, 'writeFileSync');
    });

    afterEach(function() {
      existsSyncSpy.restore();
      mkdirSyncSpy.restore();
      writeFileSyncSpy.restore();
    });

    it('should not write cache if projectConfig.arkObfuscator is false', () => {
      const projectConfig = {
        arkObfuscator: false,
        obfuscationMergedObConfig: {
          options: {
            enablePropertyObfuscation: true,
            enableFileNameObfuscation: true,
          },
        },
        etsLoaderVersion: '1.0.0',
      };
      const entryPackageInfo = 'testEntryPackageInfo';
      const obfuscationCacheDir = 'testCacheDir';
      const printNameCache = 'testPrintNameCache';

      writeObfuscationNameCache(projectConfig, entryPackageInfo, obfuscationCacheDir, printNameCache);

      expect(existsSyncSpy.called).to.be.false;
      expect(mkdirSyncSpy.called).to.be.false;
      expect(writeFileSyncSpy.called).to.be.false;
    });
    it('should write cache to obfuscationCacheDir if provided', () => {
      const projectConfig = {
        arkObfuscator: true,
        obfuscationMergedObConfig: {
          options: {
            enablePropertyObfuscation: true,
            enableFileNameObfuscation: true,
            enableExportObfusaction: true
          },
        },
        etsLoaderVersion: '1.0.0',
      };
      const entryPackageInfo = 'testEntryPackageInfo';
      const obfuscationCacheDir = './test/ut/initialization/testObfuscationCacheDir';
      const printNameCache = '';
      writeObfuscationNameCache(projectConfig, entryPackageInfo, obfuscationCacheDir, printNameCache);
      const defaultNameCachePath: string = path.join(obfuscationCacheDir, 'nameCache.json');
      const expectedContent = {
        "extraKey": "",
        "compileSdkVersion": "1.0.0",
        "entryPackageInfo": "testEntryPackageInfo",
        "PropertyCache": {},
        "FileNameCache": {}
      }
      const jsonData = fs.readFileSync(defaultNameCachePath,'utf-8');
      const result = JSON.parse(jsonData);
      expect(result).to.deep.equal(expectedContent);
      fs.unlinkSync(defaultNameCachePath);
      fs.rmdirSync(path.dirname(defaultNameCachePath));
    });
    it('should write cache to printNameCache if provided', () => {
      const projectConfig = {
        arkObfuscator: true,
        obfuscationMergedObConfig: {
          options: {
            enablePropertyObfuscation: true,
            enableFileNameObfuscation: true,
          },
        },
        etsLoaderVersion: '1.0.0',
      };
      const entryPackageInfo = 'testEntryPackageInfo';
      const obfuscationCacheDir = '';
      const printNameCache = './test/ut/initialization/printNameCache.json';
      writeObfuscationNameCache(projectConfig, entryPackageInfo, obfuscationCacheDir, printNameCache);
      const expectedContent = {
        "extraKey": "",
        "compileSdkVersion": "1.0.0",
        "entryPackageInfo": "testEntryPackageInfo",
        "PropertyCache": {},
        "FileNameCache": {}
      }
      const jsonData = fs.readFileSync(printNameCache,'utf-8');
      const result = JSON.parse(jsonData);
      expect(result).to.deep.equal(expectedContent);
      expect(existsSyncSpy.called).to.be.false;
      expect(mkdirSyncSpy.called).to.be.false;
      fs.unlinkSync(printNameCache);
    });
  });

  describe('enableObfuscatedFilePathConfig', () => {
    it('should return false if in debug mode or no obfuscation config', () => {
      const projectConfig = {
        obfuscationMergedObConfig: null,
        buildMode: 'debug'
      };
      const result = enableObfuscatedFilePathConfig(true, projectConfig);
      expect(result).to.be.false;
    });

    it('should return false if obfuscation is disabled or file name obfuscation is not enabled', () => {
      const projectConfig = {
        obfuscationMergedObConfig: {
          options: {
            disableObfuscation: true,
            enableFileNameObfuscation: false,
          },
        },
        buildMode: 'debug'
      };
      const result = enableObfuscatedFilePathConfig(false, projectConfig);
      expect(result).to.be.false;
    });

    it('should return true if all conditions are met', () => {
      const projectConfig = {
        obfuscationMergedObConfig: {
          options: {
            disableObfuscation: false,
            enableFileNameObfuscation: true,
          },
        },
        buildMode: 'not debug'
      };
      const result = enableObfuscatedFilePathConfig(false, projectConfig);
      expect(result).to.be.true;
    });
  });

  describe('handleObfuscatedFilePath', () => {
    it('should return the original file path if obfuscation is not enabled', () => {
      const filePath = '/path/to/file.js';
      const isPackageModules = false;
      const projectConfig = {
        enableFileNameObfuscation: false,
        buildMode: 'debug'
      };

      const result = handleObfuscatedFilePath(filePath, isPackageModules, projectConfig);

      expect(result).to.equal(filePath);
    });

    it('should return the original file path if obfuscation is not enabled', () => {
      const filePath = '/path/to/file.txt';
      const isPackageModules = false;
      const projectConfig = {
        obfuscationMergedObConfig: null,
        buildMode: 'debug'
      };
      const result = handleObfuscatedFilePath(filePath, isPackageModules, projectConfig);
      expect(result).to.equal(filePath);
    });

    it('should return the original file path if obfuscation is not enabled', () => {
      const filePath = '/path/to/file.txt';
      const isPackageModules = false;
      const projectConfig = {
        obfuscationMergedObConfig: null,
        buildMode: 'not debug'
      };
      const result = handleObfuscatedFilePath(filePath, isPackageModules, projectConfig);
      expect(result).to.equal(filePath);
    });

    it('should return the unix formatted file path if obfuscation is enabled and is a package module', () => {
      const filePath = '/path/to/file.js';
      const isPackageModules = true;
      const projectConfig = {
        obfuscationMergedObConfig: {
          options: {
            disableObfuscation: false,
            enableFileNameObfuscation: true,
          },
        },
        buildMode: "not Debug"
      };

      const result = handleObfuscatedFilePath(filePath, isPackageModules, projectConfig);

      expect(result).to.equal(FileUtils.toUnixPath(filePath));
    });

    it('should return the unix formatted file path if obfuscation is enabled and is a package module', () => {
      const filePath = '/path/to/file.js';
      const isPackageModules = true;
      const projectConfig = {
        obfuscationMergedObConfig: {
          options: {
            disableObfuscation: false,
            enableFileNameObfuscation: true,
          },
        },
        buildMode: ""
      };

      const result = handleObfuscatedFilePath(filePath, isPackageModules, projectConfig);

      expect(result).to.equal(FileUtils.toUnixPath(filePath));
    });
  });

  describe('enableObfuscateFileName', () => {
    it('should return false if obfuscation is not enabled', () => {
      const isPackageModules = false;
      const projectConfig = {
        obfuscationMergedObConfig: null,
        buildMode: "not Debug"
      };
      const result = enableObfuscateFileName(isPackageModules, projectConfig);
      expect(result).to.be.false;
    });

    it('should return true if obfuscation is enabled and not a package module', () => {
      const isPackageModules = false;
      const projectConfig = {
        obfuscationMergedObConfig: {
          options: {
            disableObfuscation: false,
            enableFileNameObfuscation: true,
          },
        },
        buildMode: "not Debug"
      };
      const result = enableObfuscateFileName(isPackageModules, projectConfig);

      expect(result).to.be.true;
    });

    it('should return false if obfuscation is enabled and is a package module', () => {
      const isPackageModules = true;
      const projectConfig = {
        obfuscationMergedObConfig: {
          options: {
            disableObfuscation: false,
            enableFileNameObfuscation: true,
          },
        },
        buildMode: "Debug"
      };
      const result = enableObfuscateFileName(isPackageModules, projectConfig);
      expect(result).to.be.false;
    });

    it('should return false if obfuscation is enabled and is a package module', () => {
      const isPackageModules = true;
      const projectConfig = {
        obfuscationMergedObConfig: {
          options: {
            disableObfuscation: false,
            enableFileNameObfuscation: false,
          },
        },
        buildMode: "Debug"
      };
      const result = enableObfuscateFileName(isPackageModules, projectConfig);
      expect(result).to.be.false;
    });

    it('should return false if obfuscation is enabled and is a package module', () => {
      const isPackageModules = true;
      const projectConfig = {
        obfuscationMergedObConfig: {},
        buildMode: "Debug"
      };
      const result = enableObfuscateFileName(isPackageModules, projectConfig);
      expect(result).to.be.false;
    });

    it('should return false if obfuscation is enabled and is a package module', () => {
      const isPackageModules = true;
      const projectConfig = {
        obfuscationMergedObConfig: null,
        buildMode: "Debug"
      };
      const result = enableObfuscateFileName(isPackageModules, projectConfig);
      expect(result).to.be.false;
    });

    it('should return false if obfuscation is enabled and is a package module', () => {
      const isPackageModules = true;
      const projectConfig = {
        obfuscationMergedObConfig: undefined,
        buildMode: "Debug"
      };
      const result = enableObfuscateFileName(isPackageModules, projectConfig);
      expect(result).to.be.false;
    });

    it('should return false if obfuscation is enabled and is a package module', () => {
      const isPackageModules = true;
      const projectConfig = {
        obfuscationMergedObConfig: {
          options: {
            disableObfuscation: false,
            enableFileNameObfuscation: true,
          },
        },
        buildMode: "not Debug"
      };
      const result = enableObfuscateFileName(isPackageModules, projectConfig);
      expect(result).to.be.false;
    });

    it('should return false if obfuscation is enabled and is a package module', () => {
      const isPackageModules = true;
      const projectConfig = {
        obfuscationMergedObConfig: {
          options: {
            disableObfuscation: false,
            enableFileNameObfuscation: false,
          },
        },
        buildMode: "not Debug"
      };
      const result = enableObfuscateFileName(isPackageModules, projectConfig);
      expect(result).to.be.false;
    });

    it('should return false if obfuscation is enabled and is a package module', () => {
      const isPackageModules = true;
      const projectConfig = {
        obfuscationMergedObConfig: {
          options: {
            disableObfuscation: false,
            enableFileNameObfuscation: false,
          },
        },
        buildMode: "not Debug"
      };
      const result = enableObfuscateFileName(isPackageModules, projectConfig);
      expect(result).to.be.false;
    });

    it('should return false if obfuscation is enabled and is a package module', () => {
      const isPackageModules = true;
      const projectConfig = {
        obfuscationMergedObConfig: {
          options: {
            disableObfuscation: false,
            enableFileNameObfuscation: true,
          },
        },
        buildMode: "not Debug"
      };
      const result = enableObfuscateFileName(isPackageModules, projectConfig);
      expect(result).to.be.false;
    });

    it('should return false if obfuscation is enabled and bytecodeObf is enable', () => {
      const isPackageModules = false;
      const projectConfig = {
        obfuscationMergedObConfig: {
          options: {
            disableObfuscation: false,
            enableFileNameObfuscation: true,
            bytecodeObf: {
              enable: true
            },
          },
        },
        buildMode: "not Debug"
      };
      const result = enableObfuscatedFilePathConfig(isPackageModules, projectConfig);
      expect(result).to.be.false;
    });
  });

  describe('getRelativeSourcePath', () => {
    it('should return the relative path of a file within the project root', () => {
      const filePath = '/Users/user/project/src/components/Button.js';
      const projectRootPath = '/Users/user/project';
      const expectedRelativePath = 'src/components/Button.js';

      const result = getRelativeSourcePath(filePath, projectRootPath, '');
      expect(result).to.equal(expectedRelativePath);
    });

    it('should return the relative path of a file within a specified project path', () => {
      const filePath = '/Users/user/project/src/components/Button.js';
      const belongProjectPath = '/Users/user/project/src';
      const expectedRelativePath = 'components/Button.js';

      const result = getRelativeSourcePath(filePath, '', belongProjectPath);
      expect(result).to.equal(expectedRelativePath);
    });

    it('should return the original path if no project root or belong project path is provided', () => {
      const filePath = '/Users/user/project/src/components/Button.js';
      const expectedRelativePath = filePath;

      const result = getRelativeSourcePath(filePath, '', '');
      expect(result).to.equal(expectedRelativePath);
    });
  });

  describe('printUnobfuscationReasons', () => {
    it('historyAllUnobfuscatedNamesMap is empty', () => {
      unobfuscationNamesObj['Index.ets'] = { 'abc': ['sdk'] };
      unobfuscationNamesObj['Test.ets'] = { 'abc_test': ['conf'] };
      printUnobfuscationReasons('', './keptNames.json');
      let keptNamesObj = {
        keptReasons: {},
        keptNames: {}
      };
      let keptNameString = fs.readFileSync('./keptNames.json', 'utf-8');
      const parsedObj = JSON.parse(keptNameString);
      keptNamesObj.keptReasons = parsedObj.keptReasons;
      keptNamesObj.keptNames = parsedObj.keptNames;
      expect(keptNamesObj.keptNames['Index.ets']['abc'][0]).to.equal('sdk');
      expect(keptNamesObj.keptNames['Test.ets']['abc_test'][0]).to.equal('conf');
      clearHistoryUnobfuscatedMap();
      clearUnobfuscationNamesObj();
    });

    it('historyAllUnobfuscatedNamesMap is not empty', () => {
      historyAllUnobfuscatedNamesMap.set('Index.ets', { 'abc': ['sdk'] });
      historyAllUnobfuscatedNamesMap.set('Test.ets', { 'abc_test': ['conf'] });
      unobfuscationNamesObj['Index.ets'] = { 'abc': ['conf'] };
      unobfuscationNamesObj['Test2.ets'] = { 'abc_test2': ['lang'] };
      printUnobfuscationReasons('', './keptNames.json');
      let keptNamesObj = {
        keptReasons: {},
        keptNames: {}
      };
      let keptNameString = fs.readFileSync('./keptNames.json', 'utf-8');
      const parsedObj = JSON.parse(keptNameString);
      keptNamesObj.keptReasons = parsedObj.keptReasons;
      keptNamesObj.keptNames = parsedObj.keptNames;
      expect(keptNamesObj.keptNames['Index.ets']['abc'][0]).to.equal('conf');
      expect(keptNamesObj.keptNames['Test.ets']['abc_test'][0]).to.equal('conf');
      expect(keptNamesObj.keptNames['Test2.ets']['abc_test2'][0]).to.equal('lang');
      clearHistoryUnobfuscatedMap();
      clearUnobfuscationNamesObj();
    });
  });

  describe('clearNameCache', () => {
    it('should clear historyMangledTable and nameCacheMap', () => {
      PropCollections.historyMangledTable.set('name1','name2');
      nameCacheMap.set('name1','name2');
      historyAllUnobfuscatedNamesMap.set('key', {'prop1': 'aaa'});
      historyUnobfuscatedPropMap.set('key', ['value']);
      clearNameCache();
      expect(PropCollections.historyMangledTable.size).to.be.equal(0);
      expect(nameCacheMap.size).to.be.equal(0);
      expect(historyAllUnobfuscatedNamesMap.size).to.equal(0);
      expect(historyUnobfuscatedPropMap.size).to.equal(0);
    });
  });

  describe('emitConsumerConfigFiles', () => {
    it('should also collect atKeepNames when atKeep is enabled', () => {
      const atKeepExportedPath = path.join(__dirname, '../../testData/output/atKeep_obfuscation01.txt');
      const sourceObConfig = {
        selfConfig: {
          ruleOptions: {
            enable: true,
            rules: ['./test/testData/obfuscation/testAtKeep/obfuscation-rules-atKeep.txt']
          },
          consumerRules: ['./test/testData/obfuscation/testAtKeep/consumer-rules.txt'],
        },
        dependencies: { libraries: [], hars: [] },
        obfuscationCacheDir: './test/testData/cache',
        options: {
          disableObfuscation: false
        },
        exportRulePath: atKeepExportedPath,
        sdkApis: []
      };
      const projectConfig = {
        obfuscationOptions: sourceObConfig,
        compileHar: false,
        compileShared: true
      }
      const obConfigResolver = new ObConfigResolver(projectConfig, printObfLogger);
      obConfigResolver.resolveObfuscationConfigs();
      AtKeepCollections.clear();
      AtKeepCollections.keepAsConsumer.globalNames.add('globalName1');
      AtKeepCollections.keepAsConsumer.propertyNames.add('propertyName1');
      AtKeepCollections.keepSymbol.globalNames.add('globalName2');
      AtKeepCollections.keepSymbol.propertyNames.add('propertyName2');
      obConfigResolver.emitConsumerConfigFiles();
      const atKeepContent = fs.readFileSync(sourceObConfig.exportRulePath, 'utf-8');  
      expect(atKeepContent).to.include('-enable-property-obfuscation');
      expect(atKeepContent).to.include('-keep-global-name');
      expect(atKeepContent).to.include('globalName00');
      expect(atKeepContent).to.include('globalName1');
      expect(atKeepContent).to.include('-keep-property-name');
      expect(atKeepContent).to.include('propertyName00');
      expect(atKeepContent).to.include('propertyName1');
      expect(atKeepContent).not.to.include('globalName2');
      expect(atKeepContent).not.to.include('propertyName2');
      FileUtils.deleteFile(atKeepExportedPath);
    });

    it('should not collect atKeepNames when atKeep is not enabled', () => {
      const atKeepExportedPath = path.join(__dirname, '../../testData/output/atKeep_obfuscation02.txt');
      const sourceObConfig = {
        selfConfig: {
          ruleOptions: {
            enable: true,
            rules: ['./test/testData/obfuscation/testAtKeep/obfuscation-rules.txt']
          },
          consumerRules: ['./test/testData/obfuscation/testAtKeep/consumer-rules.txt'],
        },
        dependencies: { libraries: [], hars: [] },
        obfuscationCacheDir: './test/testData/cache',
        options: {
          disableObfuscation: false
        },
        exportRulePath: atKeepExportedPath,
        sdkApis: []
      };
      const projectConfig = {
        obfuscationOptions: sourceObConfig,
        compileHar: false,
        compileShared: true
      }
      AtKeepCollections.clear();
      const obConfigResolver = new ObConfigResolver(projectConfig, printObfLogger);
      obConfigResolver.resolveObfuscationConfigs();
      obConfigResolver.emitConsumerConfigFiles();
      const atKeepContent = fs.readFileSync(sourceObConfig.exportRulePath, 'utf-8');  
      expect(atKeepContent).to.include('-enable-property-obfuscation');
      expect(atKeepContent).to.include('-keep-global-name');
      expect(atKeepContent).to.include('globalName00');
      expect(atKeepContent).not.to.include('globalName1');
      expect(atKeepContent).to.include('-keep-property-name');
      expect(atKeepContent).to.include('propertyName00');
      expect(atKeepContent).not.to.include('propertyName1');
      expect(atKeepContent).not.to.include('globalName2');
      expect(atKeepContent).not.to.include('propertyName2');
      FileUtils.deleteFile(atKeepExportedPath);
    });
  });

  describe('generateobfLoggerPrinter', () => {
    it('should print info if use function', () => {
      let obfLoggerPrinter = generateobfLoggerPrinter(printObfLogger);
      const stub = sinon.stub(console, 'warn');
      const errorMessage = 'test log print1';
      obfLoggerPrinter(errorMessage, errorMessage, 'warn');
      expect(stub.calledWith(errorMessage)).to.be.true;
      stub.restore();
    });
    
    it('should print info if use object', () => {
      const logger = {
        warn: (color: string, info: string) => console.warn(info),
        error: (color: string, info: string) => console.error(info)
      };
      let obfLoggerPrinter = generateobfLoggerPrinter(logger);
      const stub = sinon.stub(console, 'warn');
      const errorMessage = 'test log print2';
      obfLoggerPrinter(errorMessage, errorMessage, 'warn');
      expect(stub.calledWith(errorMessage)).to.be.true;
      stub.restore();
    });
  });
});

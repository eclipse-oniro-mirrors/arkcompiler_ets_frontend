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
  addToSet,
  arrayToSet,
  setToArray,
  areSetsEqual,
  FilePathManager,
  FileContentManager,
  ProjectWhiteListManager,
  FileWhiteList,
  FileContent
} from '../../../src/utils/ProjectCollections';
import { AtIntentCollections, AtKeepCollections, UnobfuscationCollections } from '../../../src/utils/CommonCollections';
import { ApiExtractor } from '../../../src/common/ApiExtractor'
import { FileUtils } from '../../../src/utils/FileUtils';
import * as fs from 'fs';
import path from 'path';

function compareFiles(filePath1: string, filePath2: string): boolean {
  const content1 = fs.readFileSync(filePath1, 'utf8');
  const content2 = fs.readFileSync(filePath2, 'utf8');
  return content1 === content2;
}

describe('test for CommonCollections', function () {
  describe('test for addToSet', function () {
    it('should add all elements from sourceSet to targetSet', ()=> {
      const targetSet = new Set<number>([1, 2]);
      const sourceSet = new Set<number>([2, 3, 4]);
      addToSet(targetSet, sourceSet);
      expect(targetSet.size).to.be.equal(4);
      expect(targetSet.has(1)).to.be.true;
      expect(targetSet.has(2)).to.be.true;
      expect(targetSet.has(3)).to.be.true;
      expect(targetSet.has(4)).to.be.true;
    });
  });
  describe('test for arrayToSet', function () {
    it('should convert an array to a set', ()=> {
      const array = [1, 2, 3];
      const set = arrayToSet(array);
      expect(set.size).to.be.equal(3);
      expect(set.has(1)).to.be.true;
      expect(set.has(2)).to.be.true;
      expect(set.has(3)).to.be.true;
    });
  });
  describe('test for setToArray', function () {
    it('should convert a set to an array', () => {
      const set = new Set<number>([1, 2, 3]);
      const array = setToArray(set);
      expect(array).to.deep.equal([1, 2, 3])
    });
  });
  describe('test for areSetsEqual', function () {
    it('should return true for equal sets', () => {
      const set1 = new Set<number>([1, 2, 3]);
      const set2 = new Set<number>([1, 2, 3]);
      expect(areSetsEqual(set1, set2)).to.be.true;
    });

    it('should return false for unequal sets', () => {
      const set1 = new Set<number>([1, 2, 3]);
      const set2 = new Set<number>([1, 2, 4]);
      expect(areSetsEqual(set1, set2)).to.be.false;
    });

    it('should return false for sets of different sizes', () => {
      const set1 = new Set<number>([1, 2]);
      const set2 = new Set<number>([1, 2, 3]);
      expect(areSetsEqual(set1, set2)).to.be.false;
    });
  });

  describe('test for ProjectWhiteListManager', function () {
    let cachePath = 'test/ut/utils/obfuscation';
    describe('test for constructor', function () {
      it('the value of all the options (isIncremental/enableAtKeep/keepObjectProperty) is false', () => {
        let projectWhiteListManager = new ProjectWhiteListManager(cachePath, false, false, false);
        const fileWhiteLists = projectWhiteListManager.fileWhiteListInfo;
        expect(fileWhiteLists).to.be.undefined;
        expect(projectWhiteListManager.getEnableAtKeep()).to.be.false;
        expect(projectWhiteListManager.getKeepObjectProperty()).to.be.false;
        expect(projectWhiteListManager.getFileWhiteListsCachePath()).to.be.equal('test/ut/utils/obfuscation/fileWhiteLists.json');
        expect(projectWhiteListManager.getProjectWhiteListCachePath()).to.be.equal('test/ut/utils/obfuscation/projectWhiteList.json');
        expect(projectWhiteListManager.getIsIncremental()).to.be.false;
        expect(projectWhiteListManager.getFileWhiteListMap().size).to.be.equal(0);
      });

      it('the value of all the options (isIncremental/enableAtKeep/keepObjectProperty) is true', () => {
        let projectWhiteListManager = new ProjectWhiteListManager(cachePath, true, true, true);
        const fileWhiteLists = projectWhiteListManager.fileWhiteListInfo;
        expect(fileWhiteLists).to.be.undefined;
        expect(projectWhiteListManager.getEnableAtKeep()).to.be.true;
        expect(projectWhiteListManager.getKeepObjectProperty()).to.be.true;
        expect(projectWhiteListManager.getFileWhiteListsCachePath()).to.be.equal('test/ut/utils/obfuscation/fileWhiteLists.json');
        expect(projectWhiteListManager.getProjectWhiteListCachePath()).to.be.equal('test/ut/utils/obfuscation/projectWhiteList.json');
        expect(projectWhiteListManager.getIsIncremental()).to.be.true;
        expect(projectWhiteListManager.getFileWhiteListMap().size).to.be.equal(0);
      });
    });

    describe('test for createFileWhiteList', function () {
      it('the value of all the options (isIncremental/enableAtKeep/keepObjectProperty) is false', () => {
        let projectWhiteListManager = new ProjectWhiteListManager(cachePath, false, false, false);
        const fileWhiteLists = projectWhiteListManager.createFileWhiteList();
        expect(fileWhiteLists).to.not.be.undefined;
        expect(fileWhiteLists.fileKeepInfo.keepSymbol?.globalNames).to.be.undefined;
        expect(fileWhiteLists.fileKeepInfo.keepSymbol?.propertyNames).to.be.undefined;
        expect(fileWhiteLists.fileKeepInfo.keepAsConsumer?.globalNames).to.be.undefined;
        expect(fileWhiteLists.fileKeepInfo.keepAsConsumer?.propertyNames).to.be.undefined;
        expect(fileWhiteLists.fileKeepInfo.objectProperties).to.be.undefined;
        expect(fileWhiteLists.fileKeepInfo.structProperties.size).to.be.equal(0);
        expect(fileWhiteLists.fileKeepInfo.exported?.propertyNames.size).to.be.equal(0);
        expect(fileWhiteLists.fileKeepInfo.exported?.globalNames.size).to.be.equal(0);
        expect(fileWhiteLists.fileKeepInfo.enumProperties.size).to.be.equal(0);
        expect(fileWhiteLists.fileKeepInfo.stringProperties.size).to.be.equal(0);
        expect(fileWhiteLists.fileKeepInfo.arkUIKeepInfo?.propertyNames.size).to.be.equal(0);
        expect(fileWhiteLists.fileKeepInfo.arkUIKeepInfo?.globalNames.size).to.be.equal(0);
        expect(fileWhiteLists.fileReservedInfo.enumProperties.size).to.be.equal(0);
        expect(fileWhiteLists.fileReservedInfo.propertyParams.size).to.be.equal(0);
      });

      it('the value of all the options (isIncremental/enableAtKeep/keepObjectProperty) is true', () => {
        let projectWhiteListManager = new ProjectWhiteListManager(cachePath, true, true, true);
        const fileWhiteLists = projectWhiteListManager.createFileWhiteList();
        expect(fileWhiteLists).to.not.be.undefined;
        expect(fileWhiteLists.fileKeepInfo.keepSymbol?.globalNames.size).to.be.equal(0);
        expect(fileWhiteLists.fileKeepInfo.keepSymbol?.propertyNames.size).to.be.equal(0);
        expect(fileWhiteLists.fileKeepInfo.keepAsConsumer?.globalNames.size).to.be.equal(0);
        expect(fileWhiteLists.fileKeepInfo.keepAsConsumer?.propertyNames.size).to.be.equal(0);
        expect(fileWhiteLists.fileKeepInfo.structProperties.size).to.be.equal(0);
        expect(fileWhiteLists.fileKeepInfo.exported?.propertyNames.size).to.be.equal(0);
        expect(fileWhiteLists.fileKeepInfo.exported?.globalNames.size).to.be.equal(0);
        expect(fileWhiteLists.fileKeepInfo.enumProperties.size).to.be.equal(0);
        expect(fileWhiteLists.fileKeepInfo.stringProperties.size).to.be.equal(0);
        expect(fileWhiteLists.fileKeepInfo.objectProperties?.size).to.be.equal(0);
        expect(fileWhiteLists.fileKeepInfo.arkUIKeepInfo?.propertyNames.size).to.be.equal(0);
        expect(fileWhiteLists.fileKeepInfo.arkUIKeepInfo?.globalNames.size).to.be.equal(0);
        expect(fileWhiteLists.fileReservedInfo.enumProperties.size).to.be.equal(0);
        expect(fileWhiteLists.fileReservedInfo.propertyParams.size).to.be.equal(0);
      });
    });

    describe('test for collect FileWhiteLists', function () {
      // Define all possible parameter combinations
      const parameterCombinations = [
        // [isIncremental, enableAtKeep, keepObjectProperty]
        [false, false, false],
        [false, false, true],
        [false, true, false],
        [false, true, true],
        [true, false, false],
        [true, false, true],
        [true, true, false],
        [true, true, true]
      ];

      // Testing the update and creation behavior of the file whitelist
      describe('file white list management', () => {
        parameterCombinations.forEach(([isIncremental, enableAtKeep, keepObjectProperty]) => {
          it(`should update fileWhiteList if already exists [incremental:${isIncremental}, atKeep:${enableAtKeep}, keepObj:${keepObjectProperty}]`, () => {
            let projectWhiteListManagerForTest = new ProjectWhiteListManager(cachePath, isIncremental, enableAtKeep, keepObjectProperty);
            projectWhiteListManagerForTest.setCurrentCollector('testPath1');

            // Add test data including object properties
            projectWhiteListManagerForTest.fileWhiteListInfo!.fileKeepInfo.structProperties.add('test1');
            projectWhiteListManagerForTest.fileWhiteListInfo!.fileKeepInfo.enumProperties.add('test2');
            projectWhiteListManagerForTest.fileWhiteListInfo!.fileKeepInfo.objectProperties?.add('objTest1');

            const fileWhilteList: FileWhiteList | undefined = projectWhiteListManagerForTest.getFileWhiteListMap().get('testPath1');

            // Verify basic properties
            expect(fileWhilteList?.fileKeepInfo.structProperties.has('test1')).to.be.true;
            expect(fileWhilteList?.fileKeepInfo.enumProperties.has('test2')).to.be.true;
            expect(projectWhiteListManagerForTest.fileWhiteListInfo!.fileKeepInfo.structProperties.size).to.be.equal(1);
            expect(projectWhiteListManagerForTest.fileWhiteListInfo!.fileKeepInfo.enumProperties.size).to.be.equal(1);        

            // Verify object properties based on keepObjectProperty flag
            if (keepObjectProperty) {
              expect(fileWhilteList?.fileKeepInfo.objectProperties?.has('objTest1')).to.be.true;
              expect(projectWhiteListManagerForTest.fileWhiteListInfo!.fileKeepInfo.objectProperties?.size).to.be.equal(1);
            } else {
              expect(fileWhilteList?.fileKeepInfo.objectProperties).to.be.undefined;
            }

            // Add more test data
            projectWhiteListManagerForTest.fileWhiteListInfo!.fileKeepInfo.structProperties.add('test3');
            projectWhiteListManagerForTest.fileWhiteListInfo!.fileKeepInfo.enumProperties.add('test4');
            projectWhiteListManagerForTest.fileWhiteListInfo!.fileKeepInfo.objectProperties?.add('objTest2');

            // Verify updated data
            expect(fileWhilteList?.fileKeepInfo.structProperties.has('test1')).to.be.true;
            expect(fileWhilteList?.fileKeepInfo.enumProperties.has('test2')).to.be.true;
            expect(fileWhilteList?.fileKeepInfo.structProperties.has('test3')).to.be.true;
            expect(fileWhilteList?.fileKeepInfo.enumProperties.has('test4')).to.be.true;

            // Verify updated object properties based on keepObjectProperty flag
            if (keepObjectProperty) {
              expect(fileWhilteList?.fileKeepInfo.objectProperties?.has('objTest1')).to.be.true;
              expect(fileWhilteList?.fileKeepInfo.objectProperties?.has('objTest2')).to.be.true;
              expect(projectWhiteListManagerForTest.fileWhiteListInfo!.fileKeepInfo.objectProperties?.size).to.be.equal(2);
            } else {
              expect(fileWhilteList?.fileKeepInfo.objectProperties).to.be.undefined;
            }
          });

          it(`should create new fileWhiteList if not exists [incremental:${isIncremental}, atKeep:${enableAtKeep}, keepObj:${keepObjectProperty}]`, () => {
            let projectWhiteListManagerForTest = new ProjectWhiteListManager(cachePath, isIncremental, enableAtKeep, keepObjectProperty);
            projectWhiteListManagerForTest.setCurrentCollector('testPath2');

            // Add test data including object properties
            projectWhiteListManagerForTest.fileWhiteListInfo?.fileKeepInfo.structProperties.add("test3");
            projectWhiteListManagerForTest.fileWhiteListInfo?.fileKeepInfo.enumProperties.add("test4");
            projectWhiteListManagerForTest.fileWhiteListInfo?.fileKeepInfo.objectProperties?.add("objTest3");

            const fileWhilteList: FileWhiteList | undefined = projectWhiteListManagerForTest.getFileWhiteListMap().get('testPath2');

            // Verify basic properties
            expect(fileWhilteList?.fileKeepInfo.structProperties.has('test1')).to.be.false;
            expect(fileWhilteList?.fileKeepInfo.enumProperties.has('test2')).to.be.false;
            expect(fileWhilteList?.fileKeepInfo.structProperties.has('test3')).to.be.true;
            expect(fileWhilteList?.fileKeepInfo.enumProperties.has('test4')).to.be.true;

            // Verify object properties based on keepObjectProperty flag
            if (keepObjectProperty) {
              expect(fileWhilteList?.fileKeepInfo.objectProperties?.has('objTest3')).to.be.true;
              expect(projectWhiteListManagerForTest.fileWhiteListInfo?.fileKeepInfo.objectProperties?.size).to.be.equal(1);
            } else {
              expect(fileWhilteList?.fileKeepInfo.objectProperties).to.be.undefined;
            }
          });
        });
      });

      // Testing the behavior of the atKeep option
      describe('atKeep functionality', () => {
        parameterCombinations.forEach(([isIncremental, enableAtKeep, keepObjectProperty]) => {
          it(`should handle atKeep collections correctly [incremental:${isIncremental}, atKeep:${enableAtKeep}, keepObj:${keepObjectProperty}]`, () => {
            let projectWhiteListManagerForTest = new ProjectWhiteListManager(cachePath, isIncremental, enableAtKeep, keepObjectProperty);
            projectWhiteListManagerForTest.setCurrentCollector(`testPath_${isIncremental}_${enableAtKeep}_${keepObjectProperty}`);
            let fileWhilteListTemp: FileWhiteList = projectWhiteListManagerForTest.fileWhiteListInfo;

            // Add various types of test data
            fileWhilteListTemp.fileKeepInfo.keepSymbol?.globalNames.add('test1');
            fileWhilteListTemp.fileKeepInfo.keepSymbol?.propertyNames.add('test2');
            fileWhilteListTemp.fileKeepInfo.keepAsConsumer?.globalNames.add('test3');
            fileWhilteListTemp.fileKeepInfo.keepAsConsumer?.propertyNames.add('test4');
            fileWhilteListTemp.fileKeepInfo.structProperties.add('test5');
            fileWhilteListTemp.fileKeepInfo.enumProperties.add('test6');
            fileWhilteListTemp.fileKeepInfo.exported.globalNames.add('test7');
            fileWhilteListTemp.fileKeepInfo.exported.propertyNames.add('test8');
            fileWhilteListTemp.fileKeepInfo.stringProperties.add('test9');
            fileWhilteListTemp.fileKeepInfo.objectProperties?.add('testObjectProperties1');
            fileWhilteListTemp.fileKeepInfo.objectProperties?.add('testObjectProperties2');
            fileWhilteListTemp.fileKeepInfo.arkUIKeepInfo?.globalNames.add('test111');
            fileWhilteListTemp.fileKeepInfo.arkUIKeepInfo?.propertyNames.add('test112');
            fileWhilteListTemp.fileReservedInfo.enumProperties.add('test10');
            fileWhilteListTemp.fileReservedInfo.propertyParams.add('test11');

            const fileWhilteList: FileWhiteList | undefined = projectWhiteListManagerForTest.getFileWhiteListMap().get(`testPath_${isIncremental}_${enableAtKeep}_${keepObjectProperty}`);

            // Verify basic attributes (should always exist)
            expect(fileWhilteList?.fileKeepInfo.structProperties.has('test5')).to.be.true;
            expect(fileWhilteList?.fileKeepInfo.enumProperties.has('test6')).to.be.true;
            expect(fileWhilteList?.fileKeepInfo.exported.globalNames.has('test7')).to.be.true;
            expect(fileWhilteList?.fileKeepInfo.exported.propertyNames.has('test8')).to.be.true;
            expect(fileWhilteList?.fileKeepInfo.stringProperties.has('test9')).to.be.true;
            expect(fileWhilteList?.fileKeepInfo.arkUIKeepInfo?.globalNames.has('test111')).to.be.true;
            expect(fileWhilteList?.fileKeepInfo.arkUIKeepInfo?.propertyNames.has('test112')).to.be.true;
            expect(fileWhilteList?.fileReservedInfo.enumProperties.has('test10')).to.be.true;
            expect(fileWhilteList?.fileReservedInfo.propertyParams.has('test11')).to.be.true;

            // Verify object properties based on keepObjectProperty flag
            if (keepObjectProperty) {
              expect(fileWhilteList?.fileKeepInfo.objectProperties?.has('testObjectProperties1')).to.be.true;
              expect(fileWhilteList?.fileKeepInfo.objectProperties?.has('testObjectProperties2')).to.be.true;
              expect(fileWhilteListTemp.fileKeepInfo.objectProperties?.size).to.be.equal(2);
            } else {
              expect(fileWhilteList?.fileKeepInfo.objectProperties).to.be.undefined;
            }

            // Verify atKeep related attributes (determined by the enableAtKeep parameter)
            if (enableAtKeep) {
              // When enableAtKeep is true, the atKeep-related properties should exist.
              expect(fileWhilteList?.fileKeepInfo.keepSymbol?.globalNames.has('test1')).to.be.true;
              expect(fileWhilteList?.fileKeepInfo.keepSymbol?.propertyNames.has('test2')).to.be.true;
              expect(fileWhilteList?.fileKeepInfo.keepAsConsumer?.globalNames.has('test3')).to.be.true;
              expect(fileWhilteList?.fileKeepInfo.keepAsConsumer?.propertyNames.has('test4')).to.be.true;
            } else {
              // 当 enableAtKeep 为 false 时，atKeep 相关属性应该不存在
              expect(fileWhilteList?.fileKeepInfo.keepSymbol?.globalNames).to.be.undefined;
              expect(fileWhilteList?.fileKeepInfo.keepSymbol?.propertyNames).to.be.undefined;
              expect(fileWhilteList?.fileKeepInfo.keepAsConsumer?.globalNames).to.be.undefined;
              expect(fileWhilteList?.fileKeepInfo.keepAsConsumer?.propertyNames).to.be.undefined;
            }

            // Verify dimensions (determined by the enableAtKeep and keepObjectProperty parameters)
            expect(fileWhilteListTemp.fileKeepInfo.structProperties.size).to.be.equal(1);
            expect(fileWhilteListTemp.fileKeepInfo.enumProperties.size).to.be.equal(1);
            expect(fileWhilteListTemp.fileKeepInfo.exported.globalNames.size).to.be.equal(1);
            expect(fileWhilteListTemp.fileKeepInfo.exported.propertyNames.size).to.be.equal(1);
            expect(fileWhilteListTemp.fileKeepInfo.stringProperties.size).to.be.equal(1);
            expect(fileWhilteListTemp.fileReservedInfo.enumProperties.size).to.be.equal(1);
            expect(fileWhilteListTemp.fileReservedInfo.propertyParams.size).to.be.equal(1);
            expect(fileWhilteListTemp.fileKeepInfo.arkUIKeepInfo?.globalNames.size).to.be.equal(1);
            expect(fileWhilteListTemp.fileKeepInfo.arkUIKeepInfo?.propertyNames.size).to.be.equal(1);

            if (enableAtKeep) {
              expect(fileWhilteListTemp.fileKeepInfo.keepSymbol?.globalNames.size).to.be.equal(1);
              expect(fileWhilteListTemp.fileKeepInfo.keepSymbol?.propertyNames.size).to.be.equal(1);
              expect(fileWhilteListTemp.fileKeepInfo.keepAsConsumer?.globalNames.size).to.be.equal(1);
              expect(fileWhilteListTemp.fileKeepInfo.keepAsConsumer?.propertyNames.size).to.be.equal(1);
            }

            if (keepObjectProperty) {
              expect(fileWhilteListTemp.fileKeepInfo.objectProperties?.size).to.be.equal(2);
            }
          });
        });
      });

      // Specifically testing the effect of the keepObjectProperty parameter
      describe('keepObjectProperty functionality', () => {
        parameterCombinations.forEach(([isIncremental, enableAtKeep, keepObjectProperty]) => {
          it(`should handle object property collection based on keepObjectProperty flag [incremental:${isIncremental}, atKeep:${enableAtKeep}, keepObj:${keepObjectProperty}]`, () => {
            let projectWhiteListManagerForTest = new ProjectWhiteListManager(cachePath, isIncremental, enableAtKeep, keepObjectProperty);
            projectWhiteListManagerForTest.setCurrentCollector(`testPath_obj_${keepObjectProperty}`);
            let fileWhilteListTemp: FileWhiteList = projectWhiteListManagerForTest.fileWhiteListInfo;

            // Add test data related to object properties
            fileWhilteListTemp.fileKeepInfo.objectProperties?.add('objTest1');
            fileWhilteListTemp.fileKeepInfo.objectProperties?.add('objTest2');
            fileWhilteListTemp.fileKeepInfo.objectProperties?.add('objTest3');

            const fileWhilteList: FileWhiteList | undefined = projectWhiteListManagerForTest.getFileWhiteListMap().get(`testPath_obj_${keepObjectProperty}`);

            // Verify the behavior of object properties based on the keepObjectProperty flag
            if (keepObjectProperty) {
              // When keepObjectProperty is true, the object property should exist and contain all added values.
              expect(fileWhilteList?.fileKeepInfo.objectProperties).to.not.be.undefined;
              expect(fileWhilteList?.fileKeepInfo.objectProperties?.has('objTest1')).to.be.true;
              expect(fileWhilteList?.fileKeepInfo.objectProperties?.has('objTest2')).to.be.true;
              expect(fileWhilteList?.fileKeepInfo.objectProperties?.has('objTest3')).to.be.true;
              expect(fileWhilteListTemp.fileKeepInfo.objectProperties?.size).to.be.equal(3);

              // Validation can continue to add object properties
              fileWhilteListTemp.fileKeepInfo.objectProperties?.add('objTest4');
              expect(fileWhilteList?.fileKeepInfo.objectProperties?.has('objTest4')).to.be.true;
              expect(fileWhilteListTemp.fileKeepInfo.objectProperties?.size).to.be.equal(4);
            } else {
              // When keepObjectProperty is false, the object property should not exist
              expect(fileWhilteList?.fileKeepInfo.objectProperties).to.be.undefined;
              expect(fileWhilteListTemp.fileKeepInfo.objectProperties).to.be.undefined;

              // Verify that attempting to add object properties does not take effect
              fileWhilteListTemp.fileKeepInfo.objectProperties?.add('objTest4');
              expect(fileWhilteListTemp.fileKeepInfo.objectProperties).to.be.undefined;
            }
          });
        });
      });

      // Specifically testing the interaction between keepObjectProperty and other features
      describe('keepObjectProperty interaction with other features', () => {
        parameterCombinations.forEach(([isIncremental, enableAtKeep, keepObjectProperty]) => {
          it(`should handle object properties correctly when combined with other features [incremental:${isIncremental}, atKeep:${enableAtKeep}, keepObj:${keepObjectProperty}]`, () => {
            let projectWhiteListManagerForTest = new ProjectWhiteListManager(cachePath, isIncremental, enableAtKeep, keepObjectProperty);
            projectWhiteListManagerForTest.setCurrentCollector(`testPath_combo_${keepObjectProperty}`);
            let fileWhilteListTemp: FileWhiteList = projectWhiteListManagerForTest.fileWhiteListInfo;

            // Add test data of mixed types
            fileWhilteListTemp.fileKeepInfo.structProperties.add('struct1');
            fileWhilteListTemp.fileKeepInfo.enumProperties.add('enum1');
            fileWhilteListTemp.fileKeepInfo.stringProperties.add('string1');
            fileWhilteListTemp.fileKeepInfo.objectProperties?.add('object1');
            fileWhilteListTemp.fileKeepInfo.objectProperties?.add('object2');

            if (enableAtKeep) {
              fileWhilteListTemp.fileKeepInfo.keepSymbol?.globalNames.add('keepSymbol1');
              fileWhilteListTemp.fileKeepInfo.keepAsConsumer?.propertyNames.add('keepConsumer1');
            }

            const fileWhilteList: FileWhiteList | undefined = projectWhiteListManagerForTest.getFileWhiteListMap().get(`testPath_combo_${keepObjectProperty}`);

            // Verify that basic attributes always exist
            expect(fileWhilteList?.fileKeepInfo.structProperties.has('struct1')).to.be.true;
            expect(fileWhilteList?.fileKeepInfo.enumProperties.has('enum1')).to.be.true;
            expect(fileWhilteList?.fileKeepInfo.stringProperties.has('string1')).to.be.true;

            // Verify atKeep related properties
            if (enableAtKeep) {
              expect(fileWhilteList?.fileKeepInfo.keepSymbol?.globalNames.has('keepSymbol1')).to.be.true;
              expect(fileWhilteList?.fileKeepInfo.keepAsConsumer?.propertyNames.has('keepConsumer1')).to.be.true;
            } else {
              expect(fileWhilteList?.fileKeepInfo.keepSymbol?.globalNames).to.be.undefined;
              expect(fileWhilteList?.fileKeepInfo.keepAsConsumer?.propertyNames).to.be.undefined;
            }

            // Validate object properties based on the keepObjectProperty flag
            if (keepObjectProperty) {
              expect(fileWhilteList?.fileKeepInfo.objectProperties?.has('object1')).to.be.true;
              expect(fileWhilteList?.fileKeepInfo.objectProperties?.has('object2')).to.be.true;
              expect(fileWhilteListTemp.fileKeepInfo.objectProperties?.size).to.be.equal(2);
            } else {
              expect(fileWhilteList?.fileKeepInfo.objectProperties).to.be.undefined;
            }

            // Verify whether the length of the Set collection data is correct
            expect(fileWhilteListTemp.fileKeepInfo.structProperties.size).to.be.equal(1);
            expect(fileWhilteListTemp.fileKeepInfo.enumProperties.size).to.be.equal(1);
            expect(fileWhilteListTemp.fileKeepInfo.stringProperties.size).to.be.equal(1);

            if (keepObjectProperty) {
              expect(fileWhilteListTemp.fileKeepInfo.objectProperties?.size).to.be.equal(2);
            }
 
            if (enableAtKeep) {
              expect(fileWhilteListTemp.fileKeepInfo.keepSymbol?.globalNames.size).to.be.equal(1);
              expect(fileWhilteListTemp.fileKeepInfo.keepAsConsumer?.propertyNames.size).to.be.equal(1);
            }
          });
        });
      });

      // Specifically testing the impact of the incremental mode
      describe('incremental mode functionality', () => {
        parameterCombinations.forEach(([isIncremental, enableAtKeep, keepObjectProperty]) => {
          it(`should behave correctly in incremental mode [incremental:${isIncremental}, atKeep:${enableAtKeep}, keepObj:${keepObjectProperty}]`, () => {
            let projectWhiteListManagerForTest = new ProjectWhiteListManager(cachePath, isIncremental, enableAtKeep, keepObjectProperty);

            // Testing specific behavior in incremental mode
            projectWhiteListManagerForTest.setCurrentCollector(`testPath_inc_${isIncremental}`);

            // Add test data containing object properties
            projectWhiteListManagerForTest.fileWhiteListInfo!.fileKeepInfo.structProperties.add('incStruct1');
            projectWhiteListManagerForTest.fileWhiteListInfo!.fileKeepInfo.objectProperties?.add('incObject1');

            const fileWhilteList: FileWhiteList | undefined = projectWhiteListManagerForTest.getFileWhiteListMap().get(`testPath_inc_${isIncremental}`);

            // Test different behaviors based on the isIncremental flag
            if (isIncremental) {
              // Specific tests in incremental mode
              expect(projectWhiteListManagerForTest.getIsIncremental()).to.be.true;

              // Verify the behavior of object properties in incremental mode
              if (keepObjectProperty) {
                expect(fileWhilteList?.fileKeepInfo.objectProperties?.has('incObject1')).to.be.true;
              } else {
                expect(fileWhilteList?.fileKeepInfo.objectProperties).to.be.undefined;
              }
            } else {
              // Specific tests in full mode
              expect(projectWhiteListManagerForTest.getIsIncremental()).to.be.false;

              // Verify the behavior of object properties in full mode
              if (keepObjectProperty) {
                expect(fileWhilteList?.fileKeepInfo.objectProperties?.has('incObject1')).to.be.true;
              } else {
                expect(fileWhilteList?.fileKeepInfo.objectProperties).to.be.undefined;
              }
            }
          });
        });
      });
    });

    describe('test for createOrUpdateWhiteListCaches', function () {
      beforeEach(() => {
        UnobfuscationCollections.clear();
        ApiExtractor.mConstructorPropertySet.clear();
        ApiExtractor.mEnumMemberSet.clear();
      })
      it('should write fileWhiteLists and projectWhiteList if is not incremental', () => {
        let projectWhiteListManager = new ProjectWhiteListManager(cachePath, false, true, false);
        projectWhiteListManager.setCurrentCollector('testPath1');
        let fileWhilteListTemp: FileWhiteList = projectWhiteListManager.fileWhiteListInfo;
        fileWhilteListTemp.fileKeepInfo.structProperties.add('test01');
        fileWhilteListTemp.fileKeepInfo.enumProperties.add('test02');
        fileWhilteListTemp.fileKeepInfo.keepSymbol?.globalNames.add('test1');
        fileWhilteListTemp.fileKeepInfo.keepSymbol?.propertyNames.add('test2');
        fileWhilteListTemp.fileKeepInfo.keepAsConsumer?.globalNames.add('test3');
        fileWhilteListTemp.fileKeepInfo.keepAsConsumer?.propertyNames.add('test4');
        fileWhilteListTemp.fileKeepInfo.structProperties.add('test5');
        fileWhilteListTemp.fileKeepInfo.enumProperties.add('test6');
        fileWhilteListTemp.fileKeepInfo.exported.globalNames.add('test7');
        fileWhilteListTemp.fileKeepInfo.exported.propertyNames.add('test8');
        fileWhilteListTemp.fileKeepInfo.stringProperties.add('test9');
        fileWhilteListTemp.fileKeepInfo.objectProperties?.add('testObjectProperties1');
        fileWhilteListTemp.fileKeepInfo.arkUIKeepInfo?.globalNames.add('test111');
        fileWhilteListTemp.fileKeepInfo.arkUIKeepInfo?.propertyNames.add('test112');
        fileWhilteListTemp.fileReservedInfo.enumProperties.add('test10');
        fileWhilteListTemp.fileReservedInfo.propertyParams.add('test11');
        projectWhiteListManager.setCurrentCollector('testPath2');
        fileWhilteListTemp = projectWhiteListManager.fileWhiteListInfo;
        fileWhilteListTemp.fileKeepInfo.exported.propertyNames.add('test12');
        projectWhiteListManager.setCurrentCollector('testPath3');
        fileWhilteListTemp = projectWhiteListManager.fileWhiteListInfo;
        fileWhilteListTemp.fileKeepInfo.exported.globalNames.add('test13');
        projectWhiteListManager.createOrUpdateWhiteListCaches();
        let fileWhiteLists = path.join(__dirname,'obfuscation/fileWhiteLists.json');
        let fileWhiteLists_expected = path.join(__dirname,'obfuscation/fileWhiteLists_expected01.json');
        let projectWhiteLists = path.join(__dirname,'obfuscation/projectWhiteList.json');
        let projectWhiteLists_expected = path.join(__dirname,'obfuscation/projectWhiteList_expected01.json');
        expect(compareFiles(fileWhiteLists, fileWhiteLists_expected)).to.be.true;
        expect(compareFiles(projectWhiteLists, projectWhiteLists_expected)).to.be.true;
        expect(AtKeepCollections.keepSymbol.globalNames.size==0).to.be.true;
        expect(AtKeepCollections.keepSymbol.propertyNames.size==0).to.be.true;
        expect(AtKeepCollections.keepAsConsumer.globalNames.size==0).to.be.true;
        expect(AtKeepCollections.keepAsConsumer.propertyNames.size==0).to.be.true;
        expect(UnobfuscationCollections.reservedStruct.size==0).to.be.true;
        expect(UnobfuscationCollections.reservedEnum.size==0).to.be.true;
        expect(UnobfuscationCollections.reservedExportName.size==0).to.be.true;
        expect(UnobfuscationCollections.reservedExportNameAndProp.size==0).to.be.true;
        expect(UnobfuscationCollections.reservedStrProp.size==0).to.be.true;
        expect(ApiExtractor.mConstructorPropertySet.size==0).to.be.true;
        expect(ApiExtractor.mEnumMemberSet.size==0).to.be.true;
        expect(AtIntentCollections.globalNames.size==1).to.be.true;
        expect(AtIntentCollections.propertyNames.size==1).to.be.true;
        expect(AtIntentCollections.globalNames.has('test111')).to.be.true;
        expect(AtIntentCollections.propertyNames.has('test112')).to.be.true;
      });
      it('should update fileWhiteLists and projectWhiteList if is incremental(project white list changed)', () => {
        let projectWhiteListManager = new ProjectWhiteListManager(cachePath, true, true, false);
        projectWhiteListManager.setCurrentCollector('testPath4');
        let fileWhilteListTemp: FileWhiteList = projectWhiteListManager.fileWhiteListInfo;
        fileWhilteListTemp.fileKeepInfo.structProperties.add('test01');
        fileWhilteListTemp.fileKeepInfo.enumProperties.add('test02');
        projectWhiteListManager.setCurrentCollector('testPath4');
        fileWhilteListTemp = projectWhiteListManager.fileWhiteListInfo;
        fileWhilteListTemp.fileKeepInfo.keepSymbol?.globalNames.add('test1');
        fileWhilteListTemp.fileKeepInfo.keepSymbol?.globalNames.add('test32');
        fileWhilteListTemp.fileKeepInfo.keepSymbol?.propertyNames.add('test2');
        fileWhilteListTemp.fileKeepInfo.keepAsConsumer?.globalNames.add('test3');
        fileWhilteListTemp.fileKeepInfo.keepAsConsumer?.propertyNames.add('test4');
        fileWhilteListTemp.fileKeepInfo.structProperties.add('test5');
        fileWhilteListTemp.fileKeepInfo.enumProperties.add('test6');
        fileWhilteListTemp.fileKeepInfo.exported.globalNames.add('test7');
        fileWhilteListTemp.fileKeepInfo.exported.propertyNames.add('test8');
        fileWhilteListTemp.fileKeepInfo.stringProperties.add('test9');
        fileWhilteListTemp.fileKeepInfo.arkUIKeepInfo.globalNames.add('test122');
        fileWhilteListTemp.fileKeepInfo.arkUIKeepInfo.propertyNames.add('test123');
        fileWhilteListTemp.fileReservedInfo.enumProperties.add('test10');
        fileWhilteListTemp.fileReservedInfo.propertyParams.add('test11');
        fileWhilteListTemp.fileReservedInfo.propertyParams.add('test31');
        projectWhiteListManager.setCurrentCollector('testPath3');
        fileWhilteListTemp = projectWhiteListManager.fileWhiteListInfo;
        fileWhilteListTemp.fileKeepInfo.exported.globalNames.add('test33');
        const deletedFilesSet: Set<string> = new Set(['testPath2', 'testPathNotExists']);
        projectWhiteListManager.createOrUpdateWhiteListCaches(deletedFilesSet);
        let fileWhiteLists = path.join(__dirname,'obfuscation/fileWhiteLists.json');
        let fileWhiteLists_expected = path.join(__dirname,'obfuscation/fileWhiteLists_expected02.json');
        let projectWhiteLists = path.join(__dirname,'obfuscation/projectWhiteList.json');
        let projectWhiteLists_expected = path.join(__dirname,'obfuscation/projectWhiteList_expected02.json');
        expect(compareFiles(fileWhiteLists, fileWhiteLists_expected)).to.be.true;
        expect(compareFiles(projectWhiteLists, projectWhiteLists_expected)).to.be.true;
        expect(projectWhiteListManager.getShouldReObfuscate()).to.be.true;
        expect(AtKeepCollections.keepSymbol.globalNames.has('test1')).to.be.true;
        expect(AtKeepCollections.keepSymbol.propertyNames.has('test2')).to.be.true;
        expect(AtKeepCollections.keepAsConsumer.globalNames.has('test3')).to.be.true;
        expect(AtKeepCollections.keepAsConsumer.propertyNames.has('test4')).to.be.true;
        expect(AtKeepCollections.keepSymbol.globalNames.has('test32')).to.be.true;
        expect(UnobfuscationCollections.reservedStruct.has('test01')).to.be.true;
        expect(UnobfuscationCollections.reservedEnum.has('test02')).to.be.true;
        expect(UnobfuscationCollections.reservedStruct.has('test5')).to.be.true;
        expect(UnobfuscationCollections.reservedEnum.has('test6')).to.be.true;
        expect(UnobfuscationCollections.reservedExportName.has('test7')).to.be.true;
        expect(UnobfuscationCollections.reservedExportNameAndProp.has('test8')).to.be.true;
        expect(UnobfuscationCollections.reservedStrProp.has('test9')).to.be.true;
        expect(ApiExtractor.mEnumMemberSet.has('test10')).to.be.true;
        expect(ApiExtractor.mConstructorPropertySet.has('test11')).to.be.true;
        expect(ApiExtractor.mConstructorPropertySet.has('test31')).to.be.true;
        expect(UnobfuscationCollections.reservedExportName.has('test33')).to.be.true;
        expect(UnobfuscationCollections.reservedExportNameAndProp.has('test12')).to.be.false;
        expect(AtIntentCollections.globalNames.has('test122')).to.be.true;
        expect(AtIntentCollections.propertyNames.has('test123')).to.be.true;
      });
      it('should update fileWhiteLists and projectWhiteList if is incremental(project white list not changed)', () => {
        let projectWhiteListManager = new ProjectWhiteListManager(cachePath, true, true, false);
        projectWhiteListManager.setCurrentCollector('testPath5');
        let fileWhilteListTemp: FileWhiteList | undefined = projectWhiteListManager.fileWhiteListInfo;
        fileWhilteListTemp.fileKeepInfo.structProperties.add('test01');
        fileWhilteListTemp.fileKeepInfo.enumProperties.add('test02');
        fileWhilteListTemp.fileKeepInfo.keepSymbol?.globalNames.add('test1');
        fileWhilteListTemp.fileKeepInfo.keepSymbol?.globalNames.add('test32');
        projectWhiteListManager.createOrUpdateWhiteListCaches();
        let fileWhiteLists = path.join(__dirname,'obfuscation/fileWhiteLists.json');
        let fileWhiteLists_expected = path.join(__dirname,'obfuscation/fileWhiteLists_expected03.json');
        let projectWhiteLists = path.join(__dirname,'obfuscation/projectWhiteList.json');
        let projectWhiteLists_expected = path.join(__dirname,'obfuscation/projectWhiteList_expected02.json');
        expect(compareFiles(fileWhiteLists, fileWhiteLists_expected)).to.be.true;
        expect(compareFiles(projectWhiteLists, projectWhiteLists_expected)).to.be.true;
        expect(projectWhiteListManager.getShouldReObfuscate()).to.be.false;
      });
    });

    describe('test for createProjectWhiteList when bytecodeObfuscate enable', function () {
      it('should add property decorated to projectWhiteList when bytecodeObfuscate enable)', () => {
        let projectWhiteListManager = new ProjectWhiteListManager(cachePath, false, false, false);
        projectWhiteListManager.setCurrentCollector("testPath5")
        const fileWhiteLists = projectWhiteListManager.createFileWhiteList();
        fileWhiteLists.bytecodeObfuscateKeepInfo = {
          "decoratorMap": {
            "Track": ["prop1"]
          }
        };
        projectWhiteListManager.getFileWhiteListMap().set("testPath5", fileWhiteLists);
        projectWhiteListManager.createOrUpdateWhiteListCaches();
        const cacheContent = fs.readFileSync(projectWhiteListManager.getProjectWhiteListCachePath(), 'utf-8');
        const parsedCache = JSON.parse(cacheContent);
        expect(parsedCache.projectKeepInfo.globalNames).to.include("prop1");
      });
    })
  })

  describe('test for FilePathManager', function () {
    let cachePath = 'test/ut/utils/obfuscation';
    describe('test for constructor', function () {
      it('should be initialized correctly', () => {
        const filePathManager = new FilePathManager(cachePath);
        expect(filePathManager.getFilePathsCache()).to.be.equal('test/ut/utils/obfuscation/sourceFilePaths.cache');
        expect(filePathManager.getSourceFilePaths().size).to.be.equal(0);
        expect(filePathManager.getDeletedSourceFilePaths().size).to.be.equal(0);
        expect(filePathManager.getAddedSourceFilePaths().size).to.be.equal(0);
      });
    });

    describe('test for createOrUpdateSourceFilePaths', function () {
      let testFilePath1: string;
      let testFilePath2: string;
      let testFilePath3: string;
      let testFilePath4: string;
      let testFilePath5: string;

      before(()=>{
        FileUtils.deleteFolderRecursive('test/ut/utils/testData');
        testFilePath1 = 'test/ut/utils/testData/filePath1.ts';
        testFilePath2 = 'test/ut/utils/testData/filePath2.ts';
        testFilePath3 = 'test/ut/utils/testData/filePath3.ts';
        testFilePath4 = 'test/ut/utils/testData/filePath4.ts';
        testFilePath5 = 'test/ut/utils/testData/filePath5.json';
        FileUtils.createDirectory('test/ut/utils/testData');
        fs.writeFileSync(testFilePath1, 'test');
        fs.writeFileSync(testFilePath2, 'test');
        fs.writeFileSync(testFilePath3, 'test');
        fs.writeFileSync(testFilePath4, 'test');
  
        FileUtils.deleteFile(path.join(__dirname,'obfuscation/sourceFilePaths.cache'));
      })

      it('should writeSourceFilePaths when is not incremental', () => {
        const filePathManager = new FilePathManager(cachePath);
        const filePathsSet: Set<string> = new Set();
        filePathsSet.add(testFilePath1);
        filePathsSet.add(testFilePath2);
        filePathsSet.add(testFilePath3);
        filePathsSet.add(testFilePath5);
        filePathManager.createOrUpdateSourceFilePaths(filePathsSet);
        let sourceFilePaths = path.join(__dirname,'obfuscation/sourceFilePaths.cache');
        let sourceFilePaths_expected = path.join(__dirname,'obfuscation/sourceFilePaths_expected01.txt');
        expect(compareFiles(sourceFilePaths, sourceFilePaths_expected)).to.be.true;
      });
      it('should update SourceFilePaths correctly when is incremental', () => {
        const filePathManager = new FilePathManager(cachePath);
        const filePathsSet: Set<string> = new Set();
        filePathsSet.add(testFilePath1);
        filePathsSet.add(testFilePath3);
        filePathsSet.add(testFilePath4);
        filePathManager.createOrUpdateSourceFilePaths(filePathsSet);
        let sourceFilePaths = path.join(__dirname,'obfuscation/sourceFilePaths.cache');
        let sourceFilePaths_expected = path.join(__dirname,'obfuscation/sourceFilePaths_expected02.txt');
        expect(compareFiles(sourceFilePaths, sourceFilePaths_expected)).to.be.true;
        expect(filePathManager.getAddedSourceFilePaths().has(testFilePath4)).to.be.true;
        expect(filePathManager.getDeletedSourceFilePaths().has(testFilePath2)).to.be.true;
      });
    });
  });

  describe('test for FileContentManager', function () {
    let cachePath = 'test/ut/utils/obfuscation';
    describe('test for constructor', function () {
      it('should be initialized correctly', () => {
        FileUtils.deleteFolderRecursive('test/ut/utils/obfuscation/transformed');
        const fileContentManager = new FileContentManager(cachePath, false);
        const transformedPath = 'test/ut/utils/obfuscation/transformed';
        const fileNamesMapPath = 'test/ut/utils/obfuscation/transformed/transformedFileNamesMap.json';
        expect(fileContentManager.getTransformedFilesDir()).to.be.equal(transformedPath);
        expect(fileContentManager.getFileNamesMapPath()).to.be.equal(fileNamesMapPath);
        expect(fileContentManager.fileNamesMap.size).to.be.equal(0);
        expect(fileContentManager.getIsIncremental()).to.be.equal(false);
        expect(fs.existsSync(transformedPath)).to.be.equal(true);
      });
    });
    describe('test for updateFileContent', function () {
      const fileContent01: FileContent = {
        moduleInfo: {
          content: "let a = 1;\n",
          buildFilePath: "D:\\WORK\\demo\\libraryHar\\build\\default\\cache\\default\\default@HarCompileArkTS\\esmodule\\release\\libraryHar\\src\\main\\ets\\components\\test.ts",
          relativeSourceFilePath: "libraryHar/src/main/ets/components/test.ets",
          originSourceFilePath: "D:/WORK/demo/libraryHar/src/main/ets/components/test.ets",
          rollupModuleId: "D:\\WORK\\demo\\libraryHar\\src\\main\\ets\\components\\test.ets"
        },
        previousStageSourceMap: {
          version: 3,
          file: "test.ets",
          sourceRoot: "",
          sources: [
            "libraryHar/src/main/ets/components/test.ets"
          ],
          names: [],
          mappings: "AAAA,IAAI,CAAC,GAAG,CAAC,CAAC"
        }
      };
      const fileContent01_updated: FileContent = {
        moduleInfo: {
          content: "let a1 = \"1\";\n",
          buildFilePath: "D:\\WORK\\demo\\libraryHar\\build\\default\\cache\\default\\default@HarCompileArkTS\\esmodule\\release\\libraryHar\\src\\main\\ets\\components\\test.ts",
          relativeSourceFilePath: "libraryHar/src/main/ets/components/test.ets",
          originSourceFilePath: "D:/WORK/demo/libraryHar/src/main/ets/components/test.ets",
          rollupModuleId: "D:\\WORK\\demo\\libraryHar\\src\\main\\ets\\components\\test.ets"
        },
        previousStageSourceMap: {
          version: 3,
          file: "test.ets",
          sourceRoot: "",
          sources: [
            "libraryHar/src/main/ets/components/test.ets"
          ],
          names: [],
          mappings: "AAAA,IAAI,CAAC,GAAG,CAAC,CAAC"
        }
      };
      const fileContent02: FileContent = {
        moduleInfo: {
          content: "let a = 2;\n",
          buildFilePath: "D:\\WORK\\demo\\libraryHar\\build\\default\\cache\\default\\default@HarCompileArkTS\\esmodule\\release\\libraryHar\\src\\main\\ets\\components\\test2.ts",
          relativeSourceFilePath: "libraryHar/src/main/ets/components/test2.ets",
          originSourceFilePath: "D:/WORK/demo/libraryHar/src/main/ets/components/test2.ets",
          rollupModuleId: "D:\\WORK\\demo\\libraryHar\\src\\main\\ets\\components\\test2.ets"
        },
        previousStageSourceMap: {
          version: 3,
          file: "test2.ets",
          sourceRoot: "",
          sources: [
            "libraryHar/src/main/ets/components/test2.ets"
          ],
          names: [],
          mappings: "AAAA,IAAI,CAAC,GAAG,CAAC,CAAC"
        }
      };
      const fileContent03: FileContent = {
        moduleInfo: {
          content: "let a = 3;\n",
          buildFilePath: "D:\\WORK\\demo\\libraryHar\\build\\default\\cache\\default\\default@HarCompileArkTS\\esmodule\\release\\libraryHar\\src\\main\\ets\\components\\test3.ts",
          relativeSourceFilePath: "libraryHar/src/main/ets/components/test3.ets",
          originSourceFilePath: "D:/WORK/demo/libraryHar/src/main/ets/components/test3.ets",
          rollupModuleId: "D:\\WORK\\demo\\libraryHar\\src\\main\\ets\\components\\test3.ets"
        },
        previousStageSourceMap: {
          version: 3,
          file: "test3.ets",
          sourceRoot: "",
          sources: [
            "libraryHar/src/main/ets/components/test3.ets"
          ],
          names: [],
          mappings: "AAAA,IAAI,CAAC,GAAG,CAAC,CAAC"
        }
      };
      it('should writeFileContent when full compilation', () => {
        const fileContentManager = new FileContentManager(cachePath, false);
        fileContentManager.updateFileContent(fileContent01);
        fileContentManager.updateFileContent(fileContent02);
        const transformedFilePath01: string | undefined= fileContentManager.fileNamesMap.get(fileContent01.moduleInfo.originSourceFilePath!);
        const transformedFilePath02: string | undefined= fileContentManager.fileNamesMap.get(fileContent02.moduleInfo.originSourceFilePath!);
        const reservedFileContent01 = fileContentManager.readFileContent(transformedFilePath01!);
        const reservedFileContent02 = fileContentManager.readFileContent(transformedFilePath02!);
        fileContentManager.writeFileNamesMap();
        expect(reservedFileContent01).to.be.deep.equal(fileContent01);
        expect(reservedFileContent02).to.be.deep.equal(fileContent02);
      });
      it('should updateFileContent when incremental compilation', () => {
        const fileContentManager = new FileContentManager(cachePath, true);
        // before update
        fileContentManager.readFileNamesMap();
        const transformedFilePath01_before: string | undefined= fileContentManager.fileNamesMap.get(fileContent01.moduleInfo.originSourceFilePath!);
        const transformedFilePath02_before: string | undefined= fileContentManager.fileNamesMap.get(fileContent02.moduleInfo.originSourceFilePath!);
        const transformedFilePath03_before: string | undefined= fileContentManager.fileNamesMap.get(fileContent03.moduleInfo.originSourceFilePath!);
        const reservedFileContent01_before = fileContentManager.readFileContent(transformedFilePath01_before!);
        const reservedFileContent02_before = fileContentManager.readFileContent(transformedFilePath02_before!);
        expect(transformedFilePath03_before).to.be.undefined;
        expect(reservedFileContent01_before).to.be.deep.equal(fileContent01);
        expect(reservedFileContent02_before).to.be.deep.equal(fileContent02);

        // after update
        const deletedFilePath: Set<string> = new Set();
        deletedFilePath.add(fileContent02.moduleInfo.originSourceFilePath!);
        deletedFilePath.add('pathNotExists');
        fileContentManager.deleteFileContent(deletedFilePath);
        fileContentManager.updateFileContent(fileContent01_updated);
        fileContentManager.updateFileContent(fileContent03);
        const transformedFilePath01: string | undefined= fileContentManager.fileNamesMap.get(fileContent01.moduleInfo.originSourceFilePath!);
        const transformedFilePath02: string | undefined= fileContentManager.fileNamesMap.get(fileContent02.moduleInfo.originSourceFilePath!);
        const transformedFilePath03: string | undefined= fileContentManager.fileNamesMap.get(fileContent03.moduleInfo.originSourceFilePath!);
        expect(transformedFilePath02).to.be.undefined;
        const reservedFileContent01 = fileContentManager.readFileContent(transformedFilePath01!);
        const reservedFileContent03 = fileContentManager.readFileContent(transformedFilePath03!);
        expect(reservedFileContent01).to.be.deep.equal(fileContent01_updated);
        expect(reservedFileContent03).to.be.deep.equal(fileContent03);
      });
      it('should sort fileNamesMap correctly', () => {
        const fileContentManager = new FileContentManager(cachePath, false);
        const fileNamesMap = fileContentManager.fileNamesMap;
        fileNamesMap.set('file4','test');
        fileNamesMap.set('file3','test');
        fileNamesMap.set('file5','test');
        const sortedNames: string[] = fileContentManager.getSortedFiles();
        expect(sortedNames[0]).to.be.equal('file3');
        expect(sortedNames[1]).to.be.equal('file4');
        expect(sortedNames[2]).to.be.equal('file5');
      });
    });
  });
})
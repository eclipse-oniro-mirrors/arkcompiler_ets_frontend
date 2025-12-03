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

import { before } from 'mocha';
import { assert } from 'chai';
import ts from 'typescript';
import {
  collectExistNames,
  isInLocalWhitelist,
  isInPropertyWhitelist,
  isInTopLevelWhitelist,
  isMatchWildcard,
  recordHistoryUnobfuscatedNames,
  separateUniversalReservedItem,
  wildcardTransformer
} from '../../../src/utils/TransformUtil';
import type { ReservedNameInfo } from '../../../src/utils/TransformUtil';
import {
  LocalVariableCollections,
  PropCollections,
  UnobfuscationCollections
} from '../../../src/utils/CommonCollections';
import secharmony from '../../../src/transformers/rename/RenameIdentifierTransformer';

describe('test for TransformUtil', function () {
  let sourceFile: ts.SourceFile;

  before('init ast for source file', function () {
    const fileContent = `
    class Demo{
      constructor(public  title: string, public  content: string, public  mark: number) {
          this.title = title
          this.content = content
          this.mark = mark
      }
    }
    `;

    sourceFile = ts.createSourceFile('demo.js', fileContent, ts.ScriptTarget.ES2015, true);
  });

  describe('test for function collectExistNames', function () {
    it('test collectExistNames', function () {
      const nameSets = collectExistNames(sourceFile);
      const targetNames = ['Demo', 'title', 'content', 'mark'];

      assert.strictEqual(nameSets.size, targetNames.length);
      targetNames.forEach((value) => {
        assert.isTrue(nameSets.has(value));
      });
    });
  });

  describe('test for function wildcardTransformer', function () {
    it('test wildcardTransformer', function () {
      // special characters: '\', '^', '$', '.', '+', '|', '[', ']', '{', '}', '(', ')'
      const reserved1 = 'a\\+b*';
      const reserved2 = '{*}[*](*)';
      const reserved3 = '*^';
      const reserved4 = '?$\\..';
      const reserved5 = '?|123';

      const result1 = wildcardTransformer(reserved1);
      const result2 = wildcardTransformer(reserved2);
      const result3 = wildcardTransformer(reserved3);
      const result4 = wildcardTransformer(reserved4);
      const result5 = wildcardTransformer(reserved5);

      assert.strictEqual(result1, String.raw`a\\\+b.*`);
      assert.strictEqual(result2, String.raw`\{.*\}\[.*\]\(.*\)`);
      assert.strictEqual(result3, String.raw`.*\^`);
      assert.strictEqual(result4, String.raw`.\$\\\.\.`);
      assert.strictEqual(result5, String.raw`.\|123`);
    });
  });

  describe('test for function separateUniversalReservedItem', function () {
    it('should throw errors because originalArray is undefined', function () {
      assert.throws(() => {
        separateUniversalReservedItem(undefined, false);
      }, Error, 'Unable to handle the empty array.');
    });

    it('should return reservedInfo because originalArray is defined', function () {
      let originalArray: string[] = ['*', '?', 'originalArray'];
      let reservedInfo: ReservedNameInfo = separateUniversalReservedItem(originalArray, true);

      assert.deepStrictEqual(reservedInfo.universalReservedArray, [/^.*$/, /^.$/]);
      assert.deepStrictEqual(reservedInfo.specificReservedArray, ['originalArray']);

      UnobfuscationCollections.clear();
    });
  });

  describe('test for function isMatchWildcard', function () {
    it('The item can match wildcard characters', function () {
      let result: boolean = isMatchWildcard([/foo/, /bar/, /baz/], 'foobar');
      assert.isTrue(result);
    });

    it('The item can not match wildcard characters', function () {
      let result: boolean = isMatchWildcard([/foo/, /bar/, /baz/], 'abc');
      assert.isFalse(result);
    });
  });

  describe('test for function isInTopLevelWhitelist', function () {
    const recordMap: Map<string, Set<string>> = new Map();
    const nameWithScope:string = 'nameWithScope';

    describe('printKeptName is true', function () {
      it('isInTopLevelWhitelist returns false', function() {
        const originalName: string = 'isInTopLevelWhitelist';

        const needToRecordProperty: boolean = isInTopLevelWhitelist(originalName, recordMap, nameWithScope, true, true);
        assert.isFalse(needToRecordProperty);

        PropCollections.clearPropsCollections();
        UnobfuscationCollections.clear();
      });

      it('isInTopLevelWhitelist returns true', function() {
        const originalName1: string = '__global';
        const originalName2: string = 'abc';
        UnobfuscationCollections.reservedSdkApiForGlobal.add('abc');
        UnobfuscationCollections.reservedExportName.add('abc');
        PropCollections.reservedProperties.add('abc');

        const reservedFlag1: boolean = isInTopLevelWhitelist(originalName1, recordMap, nameWithScope, false, true);
        const reservedFlag2: boolean = isInTopLevelWhitelist(originalName2, recordMap, nameWithScope, false, true);
        assert.isTrue(reservedFlag1);
        assert.isTrue(reservedFlag2);

        PropCollections.clearPropsCollections();
        UnobfuscationCollections.clear();
      });
    });

    describe('printKeptName is false', function () {
      it('isInTopLevelWhitelist returns false', function () {
        const originalName: string = 'isInTopLevelWhitelist';
        const isReservedTopLevel: boolean = isInTopLevelWhitelist(originalName, recordMap, nameWithScope, true, false);
        assert.isFalse(isReservedTopLevel);

        PropCollections.clearPropsCollections();
        UnobfuscationCollections.clear();
      });

      it('isInTopLevelWhitelist returns true', function () {
        const originalName: string = '__global';
        const isReservedTopLevel: boolean = isInTopLevelWhitelist(originalName, recordMap, nameWithScope, false, false);
        assert.isTrue(isReservedTopLevel);

        PropCollections.clearPropsCollections();
        UnobfuscationCollections.clear();
      });
    });
  });

  describe('test for function isInPropertyWhitelist', function () {
    const originalName: string = 'isInPropertyWhitelist';
    const recordMap: Map<string, Set<string>> = new Map();

    it('printKeptName is true', function () {
      UnobfuscationCollections.reservedSdkApiForProp.add('isInPropertyWhitelist');
      UnobfuscationCollections.reservedLangForProperty.add('isInPropertyWhitelist');
      UnobfuscationCollections.reservedStruct.add('isInPropertyWhitelist');
      UnobfuscationCollections.reservedExportNameAndProp.add('isInPropertyWhitelist');
      UnobfuscationCollections.reservedStrProp.add('isInPropertyWhitelist');
      UnobfuscationCollections.reservedObjProp.add('isInPropertyWhitelist');
      UnobfuscationCollections.reservedEnum.add('isInPropertyWhitelist');
      PropCollections.reservedProperties.add('isInPropertyWhitelist');

      const needToRecordProperty: boolean = isInPropertyWhitelist(originalName, recordMap, true);
      assert.isTrue(needToRecordProperty);

      PropCollections.clearPropsCollections();
      UnobfuscationCollections.clear();
    });

    it('printKeptName is false', function () {
      const isReservedProperty: boolean = isInPropertyWhitelist(originalName, recordMap, false);
      assert.isFalse(isReservedProperty);
      UnobfuscationCollections.clear();
    });
  });

  describe('test for function isInLocalWhitelist', function () {
    it('printKeptName is true', function () {
      const originalName: string = 'isInLocalWhitelist';
      const recordMap: Map<string, Set<string>> = new Map();
      const nameWithScope: string = '246810';
      LocalVariableCollections.reservedLangForLocal.add('isInLocalWhitelist');
      UnobfuscationCollections.reservedSdkApiForLocal.add('isInLocalWhitelist');
      UnobfuscationCollections.reservedExportName.add('isInLocalWhitelist');
      LocalVariableCollections.reservedConfig.add('isInLocalWhitelist');

      const reservedFlag: boolean = isInLocalWhitelist(originalName, recordMap, nameWithScope, true);
      assert.isTrue(reservedFlag);

      LocalVariableCollections.clear();
      UnobfuscationCollections.clear();
    });

    it('printKeptName is false', function () {
      const originalName: string = 'originalName';
      const recordMap: Map<string, Set<string>> = new Map();
      const nameWithScope: string = '12345';

      const isReservedLocalVariable: boolean = isInLocalWhitelist(originalName, recordMap, nameWithScope,false);
      assert.isFalse(isReservedLocalVariable);
      UnobfuscationCollections.clear();
    });

    it('The local variables whitelist should be the same when printKeptName is enabled and disabled', function () {
      const recordMap: Map<string, Set<string>> = new Map();
      const nameWithScope: string = '246810';
      LocalVariableCollections.reservedLangForLocal.add('isInLocalWhitelist1');
      UnobfuscationCollections.reservedSdkApiForLocal.add('isInLocalWhitelist2');
      UnobfuscationCollections.reservedExportName.add('isInLocalWhitelist3');
      LocalVariableCollections.reservedConfig.add('isInLocalWhitelist4');
      UnobfuscationCollections.reservedSdkApiForProp.add('notInLocalWhitelist');

      const reservedFlag1: boolean = isInLocalWhitelist('isInLocalWhitelist1', recordMap, nameWithScope, true);
      const reservedFlag2: boolean = isInLocalWhitelist('isInLocalWhitelist2', recordMap, nameWithScope, true);
      const reservedFlag3: boolean = isInLocalWhitelist('isInLocalWhitelist3', recordMap, nameWithScope, true);
      const reservedFlag4: boolean = isInLocalWhitelist('isInLocalWhitelist4', recordMap, nameWithScope, true);
      const reservedFlag5: boolean = isInLocalWhitelist('notInLocalWhitelist', recordMap, nameWithScope, true);
      assert.isTrue(reservedFlag1);
      assert.isTrue(reservedFlag2);
      assert.isTrue(reservedFlag3);
      assert.isTrue(reservedFlag4);
      assert.isFalse(reservedFlag5);

      const isReservedLocalVariable1: boolean = isInLocalWhitelist('isInLocalWhitelist1', recordMap, nameWithScope, false);
      const isReservedLocalVariable2: boolean = isInLocalWhitelist('isInLocalWhitelist2', recordMap, nameWithScope, false);
      const isReservedLocalVariable3: boolean = isInLocalWhitelist('isInLocalWhitelist3', recordMap, nameWithScope, false);
      const isReservedLocalVariable4: boolean = isInLocalWhitelist('isInLocalWhitelist4', recordMap, nameWithScope, false);
      const isReservedLocalVariable5: boolean = isInLocalWhitelist('notInLocalWhitelist', recordMap, nameWithScope, false);
      assert.isTrue(isReservedLocalVariable1);
      assert.isTrue(isReservedLocalVariable2);
      assert.isTrue(isReservedLocalVariable3);
      assert.isTrue(isReservedLocalVariable4);
      assert.isFalse(isReservedLocalVariable5);
      LocalVariableCollections.clear();
      UnobfuscationCollections.clear();
    });
  });

  describe('test for function recordHistoryUnobfuscatedNames', function () {
    const nameWithScope: string = 'abc';

    it('historyUnobfuscatedNamesMap?.has(nameWithScope) is false', function () {
      recordHistoryUnobfuscatedNames(nameWithScope);
      assert.isFalse(UnobfuscationCollections.unobfuscatedNamesMap.has('abc'));
    });

    it('historyUnobfuscatedNamesMap?.has(nameWithScope) is true', function () {
      secharmony.historyUnobfuscatedNamesMap = new Map<string, string[]>();
      secharmony.historyUnobfuscatedNamesMap.set('abc', ['bcd', 'efg']);
      recordHistoryUnobfuscatedNames(nameWithScope);
      assert.isTrue(UnobfuscationCollections.unobfuscatedNamesMap.has('abc'));
      UnobfuscationCollections.clear();
      secharmony.clearCaches();
    });
  });
});
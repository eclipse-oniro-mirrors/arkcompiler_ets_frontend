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

import { isDebug, isFileExist, sortAndDeduplicateStringArr, mergeSet, convertSetToArray } from '../../../src/initialization/utils';
import { assert, expect } from 'chai';
import { DEBUG } from "../../../src/initialization/CommonObject";
import * as fs from 'fs';

describe('Tester Cases for <utils>.', function () {
  describe('Tester Cases for <isDebug>.', function () {
    /** test for isDebug */
    it('Tester: <Debug Mode> case for isDebug', function () {
      const projectConfig: any = {
        buildMode: DEBUG
      };
      assert.strictEqual(isDebug(projectConfig), true);
    });
  });

  describe('Tester Cases for <isFileExist>.', function () {
    /** test for isFileExist */
    it('Tester: <file not exist> case for isFileExist', function () {
      const path: string = 'test/ut/initialization/testFileNotExiet.txt';
      assert.strictEqual(isFileExist(path), false);
    });

    it('Tester: <file exist> case for isFileExist', function () {
      const path: string = 'test/ut/initialization/demo.txt';
      fs.writeFileSync(path, 'test');
      assert.strictEqual(isFileExist(path), true);
      fs.unlinkSync(path);
    });
  });

  describe('Tester Cases for <sortAndDeduplicateStringArr>.', function () {
    /** test for sortAndDeduplicateStringArr */
    it('Tester: <the length of arr is 0> case for sortAndDeduplicateStringArr', function () {
      const arr: string[] = [];
      assert.strictEqual(sortAndDeduplicateStringArr(arr).length, 0);
    });

    it('Tester: <sort and deduplicate> case for sortAndDeduplicateStringArr', function () {
      const arr0: string[] = ['test2', 'test2', 'test1', 'test0'];
      const arr2: string[] = ['test0', 'test1', 'test2'];
      let arr1 = sortAndDeduplicateStringArr(arr0);
      expect(arr1).to.deep.equal(arr2);
    });
  });

  describe('Tester Cases for <mergeSet>', () => {
    it('should merge sets and remove duplicates', () => {
      const set1 = new Set<string>(['a', 'b']);
      const set2 = new Set<string>(['b', 'c']);
      const result = mergeSet(set1, set2);

      assert.strictEqual(result.size, 3);
      assert.strictEqual(result.has('a'), true);
      assert.strictEqual(result.has('b'), true);
      assert.strictEqual(result.has('c'), true);
    });

    it('should return original set when other is empty', () => {
      const set1 = new Set<string>(['a', 'b']);
      const set2 = new Set<string>();
      const result = mergeSet(set1, set2);

      // After merging set1 and set2, they are equal to result.
      assert.strictEqual(result, set1);
    });
  });

  describe('Tester Cases for <convertSetToArray>', () => {
    it('should handle very large sets', () => {
      const largeSet = new Set<string>();
      const expectedArray = [];

      for (let i = 0; i < 1000; i++) {
        const value = `item${i}`;
        largeSet.add(value);
        expectedArray.push(value);
      }

      const result = convertSetToArray(largeSet);

      assert.strictEqual(result.length, 1000);
      assert.deepStrictEqual(result, expectedArray);
    });

    it('should maintain insertion order', () => {
      const inputSet = new Set<string>();
      inputSet.add('first');
      inputSet.add('second');
      inputSet.add('third');

      const result = convertSetToArray(inputSet);

      assert.deepStrictEqual(result, ['first', 'second', 'third']);
    });
  });
});

/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

import fs from 'fs';
import { getLsp, getRealPath } from '../utils';

describe('refactor UTF-16 offsets', () => {
  const moduleName = 'getEditsForRefactor';
  const lsp = getLsp(moduleName);

  test('getApplicableRefactors accepts UTF-16 selection offsets', () => {
    const fileName = getRealPath(moduleName, 'ExtractConstantUtf16.ets');
    const source = fs.readFileSync(fileName, 'utf8');
    const selection = '"x + y = "';
    const start = source.indexOf(selection);

    const result = lsp.getApplicableRefactors(fileName, 'refactor.extract.constant', start, start + selection.length);

    expect(start).not.toBe(-1);
    expect(result?.some((refactor) => refactor.name.valueOf() === 'ExtractSymbolRefactor')).toBe(true);
  });

  test('getEditsForRefactor returns UTF-16 text change offsets', () => {
    const fileName = getRealPath(moduleName, 'ExtractConstantUtf16.ets');
    const source = fs.readFileSync(fileName, 'utf8');
    const selection = '"x + y = " + (x + y)';
    const start = source.indexOf(selection);
    const insertionPoint = source.indexOf('\n', source.indexOf('let y'));

    const result = lsp.getEditsForRefactor(
      fileName,
      'ExtractSymbolRefactor',
      'extract_constant_scope_0',
      start,
      start + selection.length
    );
    const textChange = result?.fileTextChanges[0]?.textChanges[0];

    expect(start).not.toBe(-1);
    expect(textChange?.span.start).toBe(insertionPoint);
    expect(textChange?.span.length).toBe(0);
  });

  test('getClassConstructorInfo uses UTF-16 offsets and returns UTF-16 text changes', () => {
    const fileName = getRealPath(moduleName, 'GenerateConstructorUtf16.ets');
    const source = fs.readFileSync(fileName, 'utf8');
    const propertyPosition = source.indexOf('value: number');

    const result = lsp.getClassConstructorInfo(fileName, propertyPosition, ['value']);
    const textChange = result?.constructorInfoFileTextChanges[0]?.constructorInfoTextChanges[0];

    expect(propertyPosition).not.toBe(-1);
    expect(textChange).toBeDefined();
    expect(textChange?.span_.start).toBe(propertyPosition);
    expect(textChange?.span_.length).toBe(source.length - propertyPosition);
  });

  test('getClassConstructorInfo uses the cached CRLF source for returned offsets', () => {
    const fileName = getRealPath(moduleName, 'GenerateConstructorUtf16.ets');
    const source = fs.readFileSync(fileName, 'utf8').replace(/\n/g, '\r\n');
    const incrementalLsp = getLsp(moduleName);
    incrementalLsp.modifyFilesMap(fileName, { newDoc: source });
    const propertyPosition = source.indexOf('value: number');

    const result = incrementalLsp.getClassConstructorInfo(fileName, propertyPosition, ['value']);
    const textChange = result?.constructorInfoFileTextChanges[0]?.constructorInfoTextChanges[0];

    expect(propertyPosition).not.toBe(-1);
    expect(textChange).toBeDefined();
    expect(textChange?.span_.start).toBe(propertyPosition);
    expect(textChange?.span_.length).toBe(source.length - propertyPosition);
  });
});

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
import path from 'path';

import { ModuleDescriptor } from '../../src';
import { getMultiModuleLsp, getRealPath } from '../utils';

function getMarkerOffset(filePath: string, marker: string): number {
  const source = fs.readFileSync(filePath, 'utf8');
  const markerIndex = source.indexOf(marker);
  if (markerIndex === -1) {
    throw new Error(`Marker ${marker} not found in ${filePath}`);
  }
  return markerIndex + marker.length;
}

function getRenameText(filePath: string, start: number, end: number): string {
  const source = fs.readFileSync(filePath, 'utf8');
  return source.slice(start, end).trim();
}

function escapeRegExp(source: string): string {
  return source.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
}

function getLineNumber(source: string, offset: number): number {
  return source.slice(0, offset).split('\n').length - 1;
}

function getWordOccurrences(filePath: string, word: string) {
  const source = fs.readFileSync(filePath, 'utf8');
  const matcher = new RegExp(`\\b${escapeRegExp(word)}\\b`, 'g');
  const matches = Array.from(source.matchAll(matcher));

  return matches.map((match) => {
    const start = match.index ?? 0;
    return {
      fileName: path.resolve(filePath),
      start,
      end: start + word.length,
      line: getLineNumber(source, start)
    };
  });
}

function sortRenameLocations<T extends { fileName: string; start: number; end: number }>(locations: T[]): T[] {
  return [...locations].sort((left, right) => {
    const fileCompare = path.resolve(left.fileName).localeCompare(path.resolve(right.fileName));
    if (fileCompare !== 0) {
      return fileCompare;
    }
    if (left.start !== right.start) {
      return left.start - right.start;
    }
    return left.end - right.end;
  });
}

describe('findRenameLocationsCrossModuleTest', () => {
  const projectName = 'findRenameLocationsCrossModule';
  const modules: ModuleDescriptor[] = [
    { name: 'entry', moduleType: 'har', srcPath: 'entry' },
    { name: 'har', moduleType: 'har', srcPath: 'har' }
  ];
  const lsp = getMultiModuleLsp(projectName, modules, []);
  const entryFile1 = getRealPath(projectName, 'entry/EntryRename1.ets');
  const EXPECT_CLASS = sortRenameLocations(getWordOccurrences(entryFile1, 'Foo'));
  const EXPECT_CONST = sortRenameLocations(getWordOccurrences(entryFile1, 'answer'));

  test('findRenameLocations_imported_class_stays_in_current_file', () => {
    const offset = getMarkerOffset(entryFile1, '/*classTarget*/');
    const res = lsp.findRenameLocations(entryFile1, offset);

    expect(res).toBeDefined();
    const refs = sortRenameLocations((res ?? []).map((ref) => ({
      fileName: path.resolve(ref.fileName),
      start: ref.start,
      end: ref.end,
      line: ref.line,
      text: getRenameText(path.resolve(ref.fileName), ref.start, ref.end)
    })));

    expect(refs).toHaveLength(EXPECT_CLASS.length);
    expect(refs).toMatchObject(EXPECT_CLASS.map((ref) => ({
      ...ref,
      text: 'Foo'
    })));
  });

  test('findRenameLocations_imported_const_stays_in_current_file', () => {
    const offset = getMarkerOffset(entryFile1, '/*constTarget*/');
    const res = lsp.findRenameLocations(entryFile1, offset);

    expect(res).toBeDefined();
    const refs = sortRenameLocations((res ?? []).map((ref) => ({
      fileName: path.resolve(ref.fileName),
      start: ref.start,
      end: ref.end,
      line: ref.line,
      text: getRenameText(path.resolve(ref.fileName), ref.start, ref.end)
    })));

    expect(refs).toHaveLength(EXPECT_CONST.length);
    expect(refs).toMatchObject(EXPECT_CONST.map((ref) => ({
      ...ref,
      text: 'answer'
    })));
  });
});

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
import { getLsp, getRealPath } from '../utils';
import { AstNodeType } from '../../src';

describe('getDefinitionFromNodeTest', () => {
  const moduleName: string = 'getDefinitionFromNode';
  const lsp = getLsp(moduleName);
  const declLikePath = (fileName: string): string =>
    getRealPath(moduleName, path.join('build-tools', 'interop', 'declaration', fileName));

  test('getDefinitionFromNode_000', () => {
    const res = lsp.getDefinitionAtPosition(declLikePath('getDefinitionFromNode1.ets'), 0, [
      { name: 'Outer', kind: AstNodeType.CLASS_DECLARATION }
    ]);
    expect(res ? path.basename(res.fileName.valueOf()) : undefined).toBe('getDefinitionFromNode1.ets');
    expect(res?.start).toBe(1238);
    expect(res?.length).toBe(5);
  });
  test('getDefinitionFromNode_001', () => {
    const res = lsp.getDefinitionAtPosition(declLikePath('getDefinitionFromNode1.ets'), 0, [
      { name: 'Foo', kind: AstNodeType.CLASS_DECLARATION }
    ]);
    expect(res ? path.basename(res.fileName.valueOf()) : undefined).toBe('getDefinitionFromNode1.ets');
    expect(res?.start).toBe(889);
    expect(res?.length).toBe(3);
  });
  test('getDefinitionFromNode_002', () => {
    const res = lsp.getDefinitionAtPosition(declLikePath('getDefinitionFromNode1.ets'), 0, [
      { name: 'Foo', kind: AstNodeType.CLASS_DECLARATION },
      { name: 'bar', kind: AstNodeType.METHOD_DEFINITION }
    ]);
    expect(res ? path.basename(res.fileName.valueOf()) : undefined).toBe('getDefinitionFromNode1.ets');
    expect(res?.start).toBe(922);
    expect(res?.length).toBe(3);
  });
  test('getDefinitionFromNode_003', () => {
    const res = lsp.getDefinitionAtPosition(declLikePath('getDefinitionFromNode1.ets'), 0, [
      { name: 'count', kind: AstNodeType.CLASS_PROPERTY }
    ]);
    expect(res ? path.basename(res.fileName.valueOf()) : undefined).toBe('getDefinitionFromNode1.ets');
    expect(res?.start).toBe(899);
    expect(res?.length).toBe(5);
  });
  test('getDefinitionFromNode_004', () => {
    const res = lsp.getDefinitionAtPosition(declLikePath('getDefinitionFromNode1.ets'), 0, [
      { name: 'baz', kind: AstNodeType.METHOD_DEFINITION }
    ]);
    expect(res ? path.basename(res.fileName.valueOf()) : undefined).toBe('getDefinitionFromNode1.ets');
    expect(res?.start).toBe(961);
    expect(res?.length).toBe(3);
  });
  test('getDefinitionFromNode_005', () => {
    const res = lsp.getDefinitionAtPosition(declLikePath('getDefinitionFromNode1.ets'), 0, [
      { name: 'Color', kind: AstNodeType.CLASS_DECLARATION }
    ]);
    expect(res ? path.basename(res.fileName.valueOf()) : undefined).toBe('getDefinitionFromNode1.ets');
    expect(res?.start).toBe(738);
    expect(res?.length).toBe(5);
  });
  test('getDefinitionFromNode_006', () => {
    const res = lsp.getDefinitionAtPosition(declLikePath('getDefinitionFromNode1.ets'), 0, [
      { name: 'Red', kind: AstNodeType.CLASS_PROPERTY }
    ]);
    expect(res ? path.basename(res.fileName.valueOf()) : undefined).toBe('getDefinitionFromNode1.ets');
    expect(res?.start).toBe(750);
    expect(res?.length).toBe(3);
  });
  test('getDefinitionFromNode_007', () => {
    const res = lsp.getDefinitionAtPosition(declLikePath('getDefinitionFromNode1.ets'), 0, [
      { name: 'ID', kind: AstNodeType.TS_TYPE_ALIAS_DECLARATION }
    ]);
    expect(res ? path.basename(res.fileName.valueOf()) : undefined).toBe('getDefinitionFromNode1.ets');
    expect(res?.start).toBe(802);
    expect(res?.length).toBe(2);
  });
  test('getDefinitionFromNode_008', () => {
    const res = lsp.getDefinitionAtPosition(declLikePath('getDefinitionFromNode1.ets'), 0, [
      { name: 'Printable', kind: AstNodeType.TS_INTERFACE_DECLARATION }
    ]);
    expect(res ? path.basename(res.fileName.valueOf()) : undefined).toBe('getDefinitionFromNode1.ets');
    expect(res?.start).toBe(842);
    expect(res?.length).toBe(9);
  });
  test('getDefinitionFromNode_009', () => {
    const res = lsp.getDefinitionAtPosition(declLikePath('getDefinitionFromNode1.ets'), 0, [
      { name: 'helper', kind: AstNodeType.IMPORT_SPECIFIER }
    ]);
    expect(res ? path.basename(res.fileName.valueOf()) : undefined).toBe('getDefinitionFromNode1.ets');
    expect(res?.start).toBe(632);
    expect(res?.length).toBe(6);
  });
  test('getDefinitionFromNode_010', () => {
    const res = lsp.getDefinitionAtPosition(declLikePath('getDefinitionFromNode1.ets'), 0, [
      { name: 'helper', kind: AstNodeType.REEXPORT_STATEMENT }
    ]);
    expect(res ? path.basename(res.fileName.valueOf()) : undefined).toBe('getDefinitionFromNode1.ets');
    expect(res?.start).toBe(683);
    expect(res?.length).toBe(6);
  });
  test('getDefinitionFromNode_011', () => {
    const res = lsp.getDefinitionAtPosition(declLikePath('getDefinitionFromNode1.ets'), 0, [
      { name: 'Missing', kind: AstNodeType.CLASS_DECLARATION }
    ]);
    expect(res?.start).toBe(0);
    expect(res?.length).toBe(0);
  });
  test('getDefinitionFromNode_012', () => {
    const res = lsp.getDefinitionAtPosition(declLikePath('getDefinitionFromNode1.ets'), 0, [
      { name: 'v', kind: AstNodeType.TS_NON_NULL_EXPRESSION }
    ]);
    expect(res ? path.basename(res.fileName.valueOf()) : undefined).toBe('getDefinitionFromNode1.ets');
    expect(res?.start).toBe(1064);
    expect(res?.length).toBe(2);
  });
  test('getDefinitionFromNode_013', () => {
    const res = lsp.getDefinitionAtPosition(declLikePath('getDefinitionFromNode1.ets'), 0, [
      { name: 'x', kind: AstNodeType.SWITCH_STATEMENT }
    ]);
    expect(res ? path.basename(res.fileName.valueOf()) : undefined).toBe('getDefinitionFromNode1.ets');
    expect(res?.start).toBe(1119);
    expect(res?.length).toBe(105);
  });
  test('getDefinitionFromNode_014', () => {
    const res = lsp.getDefinitionAtPosition(declLikePath('getDefinitionFromNode1.ets'), 0, [
      { name: 'Outer', kind: AstNodeType.ETS_MODULE }
    ]);
    expect(res ? path.basename(res.fileName.valueOf()) : undefined).toBe('getDefinitionFromNode1.ets');
    expect(res?.start).toBe(1238);
    expect(res?.length).toBe(5);
  });
  test('getDefinitionFromNode_015', () => {
    const res = lsp.getDefinitionAtPosition(declLikePath('getDefinitionFromNode1.ets'), 0, [
      { name: 'baz', kind: AstNodeType.FUNCTION_DECLARATION }
    ]);
    expect(res ? path.basename(res.fileName.valueOf()) : undefined).toBe('getDefinitionFromNode1.ets');
    expect(res?.start).toBe(961);
    expect(res?.length).toBe(3);
  });
  test('getDefinitionFromNode_016', () => {
    const res = lsp.getDefinitionAtPosition(declLikePath('getDefinitionFromNode1.ets'), 0, [
      { name: 'Color', kind: AstNodeType.TS_ENUM_DECLARATION }
    ]);
    expect(res ? path.basename(res.fileName.valueOf()) : undefined).toBe('getDefinitionFromNode1.ets');
    expect(res?.start).toBe(738);
    expect(res?.length).toBe(5);
  });
  test('getDefinitionFromNode_017', () => {
    const res = lsp.getDefinitionAtPosition(declLikePath('getDefinitionFromNode1.ets'), 0, [
      { name: 'Red', kind: AstNodeType.TS_ENUM_MEMBER }
    ]);
    expect(res ? path.basename(res.fileName.valueOf()) : undefined).toBe('getDefinitionFromNode1.ets');
    expect(res?.start).toBe(750);
    expect(res?.length).toBe(3);
  });
  test('getDefinitionFromNode_018', () => {
    const res = lsp.getDefinitionAtPosition(declLikePath('getDefinitionFromNode1.ets'), 0, [
      { name: 'Foo', kind: AstNodeType.CLASS_DECLARATION },
      { name: 'junk', kind: AstNodeType.EMPTY_STATEMENT }
    ]);
    expect(res ? path.basename(res.fileName.valueOf()) : undefined).toBe('getDefinitionFromNode1.ets');
    expect(res?.start).toBe(889);
    expect(res?.length).toBe(3);
  });
  test('getDefinitionFromNode_021', () => {
    const res = lsp.getDefinitionAtPosition(declLikePath('getDefinitionFromNode1.ets'), 0, [
      { name: 'Outer', kind: AstNodeType.TS_MODULE_DECLARATION }
    ]);
    expect(res ? path.basename(res.fileName.valueOf()) : undefined).toBe('getDefinitionFromNode1.ets');
    expect(res?.start).toBe(1238);
    expect(res?.length).toBe(5);
  });
  test('getDefinitionFromNode_022', () => {
    const res = lsp.getReferencesAtPosition(declLikePath('getDefinitionFromNode1.ets'), 0, [
      { name: 'Foo', kind: AstNodeType.CLASS_DECLARATION }
    ]);
    expect(res?.length).toBe(1);
    expect(res && res[0] ? path.basename(res[0].fileName.valueOf()) : undefined).toBe('getDefinitionFromNode1.ets');
    expect(res?.[0]?.start).toBe(889);
    expect(res?.[0]?.length).toBe(3);
  });
  test('getDefinitionFromNode_023', () => {
    const res = lsp.getReferencesAtPosition(declLikePath('getDefinitionFromNode1.ets'), 0, [
      { name: 'Missing', kind: AstNodeType.CLASS_DECLARATION }
    ]);
    expect(res?.length).toBe(1);
    expect(res?.[0]?.start).toBe(0);
    expect(res?.[0]?.length).toBe(0);
  });

  // Same-name members in sibling classes: the node info chain must scope the method
  // lookup to the enclosing class instead of taking the first same-name match file-wide.
  function offsetOf(fileName: string, snippet: string): number {
    const source = fs.readFileSync(getRealPath(moduleName, fileName), 'utf8');
    const offset = source.indexOf(snippet);
    if (offset < 0) {
      throw new Error(`Snippet ${snippet} not found in ${fileName}`);
    }
    return offset;
  }

  test('getDefinitionFromNode_024_scopes_chain_to_first_class', () => {
    const file = 'getDefinitionFromNode3.ets';
    const res = lsp.getDefinitionAtPosition(declLikePath(file), 0, [
      { name: 'Foo', kind: AstNodeType.CLASS_DECLARATION },
      { name: 'real', kind: AstNodeType.METHOD_DEFINITION }
    ]);
    expect(res ? path.basename(res.fileName.valueOf()) : undefined).toBe(file);
    expect(res?.start).toBe(offsetOf(file, 'real(x: number)'));
    expect(res?.length).toBe('real'.length);
  });

  test('getDefinitionFromNode_025_scopes_chain_to_second_class', () => {
    const file = 'getDefinitionFromNode3.ets';
    const res = lsp.getDefinitionAtPosition(declLikePath(file), 0, [
      { name: 'Other', kind: AstNodeType.CLASS_DECLARATION },
      { name: 'real', kind: AstNodeType.METHOD_DEFINITION }
    ]);
    expect(res ? path.basename(res.fileName.valueOf()) : undefined).toBe(file);
    expect(res?.start).toBe(offsetOf(file, 'real(): void'));
    expect(res?.length).toBe('real'.length);
  });

  test('getDefinitionFromNode_026_rename_scopes_chain_to_enclosing_class', () => {
    const file = 'getDefinitionFromNode3.ets';
    const res = lsp.findRenameLocations(declLikePath(file), 0, [
      { name: 'Other', kind: AstNodeType.CLASS_DECLARATION },
      { name: 'work', kind: AstNodeType.METHOD_DEFINITION }
    ]);
    expect(res?.length).toBe(1);
    expect(res?.[0]?.start).toBe(offsetOf(file, 'work(): void'));
  });

  // The static language keeps no dedicated node for most export statements (a plain
  // `export { x }` becomes an empty statement that preserves the statement range, and a
  // default export collapses into the exported declaration), so node info chains with
  // export elements resolve through the export statement or the flagged declaration.
  test('getDefinitionFromNode_027_skips_export_wrapper', () => {
    const file = 'getDefinitionFromNode4.ets';
    const res = lsp.getDefinitionAtPosition(declLikePath(file), 0, [
      { name: 'Exported', kind: AstNodeType.EXPORT_SPECIFIER },
      { name: 'Exported', kind: AstNodeType.CLASS_DECLARATION }
    ]);
    expect(res ? path.basename(res.fileName.valueOf()) : undefined).toBe(file);
    expect(res?.start).toBe(offsetOf(file, 'Exported {'));
    expect(res?.length).toBe('Exported'.length);
  });

  test('getDefinitionFromNode_028_rename_skips_export_wrapper', () => {
    const file = 'getDefinitionFromNode4.ets';
    const res = lsp.findRenameLocations(declLikePath(file), 0, [
      { name: 'Exported', kind: AstNodeType.EXPORT_NAMED_DECLARATION },
      { name: 'Exported', kind: AstNodeType.CLASS_DECLARATION }
    ]);
    expect(res?.length).toBe(1);
    expect(res?.[0]?.start).toBe(offsetOf(file, 'Exported {'));
  });

  test('getDefinitionFromNode_029_locates_plain_export_statement', () => {
    const file = 'getDefinitionFromNode4.ets';
    const res = lsp.getDefinitionAtPosition(declLikePath(file), 0, [
      { name: 'Exported', kind: AstNodeType.EXPORT_SPECIFIER }
    ]);
    expect(res ? path.basename(res.fileName.valueOf()) : undefined).toBe(file);
    expect(res?.start).toBe(offsetOf(file, 'export { Exported }') + 'export { '.length);
    expect(res?.length).toBe('Exported'.length);
  });

  test('getDefinitionFromNode_030_locates_default_export_declaration', () => {
    const file = 'getDefinitionFromNode4.ets';
    const res = lsp.getDefinitionAtPosition(declLikePath(file), 0, [
      { name: 'Default', kind: AstNodeType.EXPORT_DEFAULT_DECLARATION }
    ]);
    expect(res ? path.basename(res.fileName.valueOf()) : undefined).toBe(file);
    expect(res?.start).toBe(offsetOf(file, 'class Default') + 'class '.length);
    expect(res?.length).toBe('Default'.length);
  });

  // `function A(): void {}` followed by `export { A }`: the export statement has no
  // specifier node in the checked AST (an empty statement keeps the range), so the
  // position of A inside the export statement is recovered from the statement text.
  test('getDefinitionFromNode_031_locates_function_export_statement', () => {
    const file = 'getDefinitionFromNode4.ets';
    const res = lsp.getDefinitionAtPosition(declLikePath(file), 0, [
      { name: 'fnA', kind: AstNodeType.EXPORT_SPECIFIER }
    ]);
    expect(res ? path.basename(res.fileName.valueOf()) : undefined).toBe(file);
    expect(res?.start).toBe(offsetOf(file, 'export { fnA }') + 'export { '.length);
    expect(res?.length).toBe('fnA'.length);
  });

  test('getDefinitionFromNode_032_rename_locates_function_export_statement', () => {
    const file = 'getDefinitionFromNode4.ets';
    const res = lsp.findRenameLocations(declLikePath(file), 0, [
      { name: 'fnA', kind: AstNodeType.EXPORT_NAMED_DECLARATION }
    ]);
    expect(res?.length).toBe(1);
    expect(res?.[0]?.start).toBe(offsetOf(file, 'export { fnA }') + 'export { '.length);
  });
});

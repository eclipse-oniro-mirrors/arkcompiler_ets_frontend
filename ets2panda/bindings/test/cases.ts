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

import path from 'path';
import { TextSpan } from '../src/lsp/lspNode';
import { AstNodeType, NodeInfo } from '../src/lsp';

export interface TestConfig {
  expectedFilePath: string;
  // CC-OFFNXT(no_explicit_any) project code style
  [key: string]: Array<any> | string;
}

export interface TestCases {
  [testName: string]: TestConfig;
}

const PROJECT_ROOT = path.resolve(__dirname, '../../');

function resolveTestPath(relativePath: string): string {
  return path.join(PROJECT_ROOT, relativePath);
}

export const basicCases: TestCases = {
  getDefinitionAtPosition: {
    expectedFilePath: resolveTestPath('test/expected/getDefinitionAtPosition.json'),
    '1': [resolveTestPath('test/testcases/getDefinitionAtPosition/getDefinitionAtPosition2.ets'), 655],
    '2': [resolveTestPath('test/testcases/getDefinitionAtPosition/getDefinitionAtPosition3.ets'), 662],
    '3': [resolveTestPath('test/testcases/getDefinitionAtPosition/getDefinitionAtPosition5.ets'), 664],
    '4': [resolveTestPath('test/testcases/getDefinitionAtPosition/getDefinitionAtPosition7.ets'), 683],
    '5': [resolveTestPath('test/testcases/getDefinitionAtPosition/getDefinitionAtPosition9.ets'), 666],
    '6': [resolveTestPath('test/testcases/getDefinitionAtPosition/getDefinitionAtPosition11.ets'), 675],
    '7': [resolveTestPath('test/testcases/getDefinitionAtPosition/getDefinitionAtPosition13.ets'), 664],
    '8': [resolveTestPath('test/testcases/getDefinitionAtPosition/getDefinitionAtPosition15.ets'), 617],
    '9': [resolveTestPath('test/testcases/getDefinitionAtPosition/getDefinitionAtPosition17.ets'), 677],
    '11': [resolveTestPath('test/testcases/getDefinitionAtPosition/getDefinitionAtPosition19.ets'), 634],
    '12': [resolveTestPath('test/testcases/getDefinitionAtPosition/getDefinitionAtPosition2.ets'), 637],
    '13': [
      resolveTestPath(
        'test/testcases/.idea/.deveco/getDefinitionAtPosition/declgen/static/getDefinitionAtPosition20.d.ets'
      ),
      0,
      [
        {
          kind: AstNodeType.CLASS_DEFINITION,
          name: 'Foo'
        },
        {
          kind: AstNodeType.IDENTIFIER,
          name: 'Foo'
        }
      ] as NodeInfo[]
    ]
  },
  getSemanticDiagnostics: {
    expectedFilePath: resolveTestPath('test/expected/getSemanticDiagnostics.json'),
    '1': [resolveTestPath('test/testcases/getSemanticDiagnostics/getSemanticDiagnostics1.ets')],
    '2': [resolveTestPath('test/testcases/getSemanticDiagnostics/getSemanticDiagnostics2.ets')],
    '3': [resolveTestPath('test/testcases/getSemanticDiagnostics/getSemanticDiagnostics3.ets')]
  },
  getCurrentTokenValue: {
    expectedFilePath: resolveTestPath('test/expected/getCurrentTokenValue.json'),
    '1': [resolveTestPath('test/testcases/getCurrentTokenValue/getCurrentTokenValue1.ets'), 611],
    '2': [resolveTestPath('test/testcases/getCurrentTokenValue/getCurrentTokenValue2.ets'), 612],
    '3': [resolveTestPath('test/testcases/getCurrentTokenValue/getCurrentTokenValue3.ets'), 612],
    '4': [resolveTestPath('test/testcases/getCurrentTokenValue/getCurrentTokenValue4.ets'), 611],
    '5': [resolveTestPath('test/testcases/getCurrentTokenValue/getCurrentTokenValue5.ets'), 697]
  },
  getFileReferences: {
    expectedFilePath: resolveTestPath('test/expected/getFileReferences.json'),
    '1': [resolveTestPath('test/testcases/getFileReferences/getFileReferences1_export.ets')]
  },
  getFileSource: {
    expectedFilePath: resolveTestPath('test/expected/getFileSource.json'),
    '1': [resolveTestPath('test/testcases/getFileSource/getFileSource1.ets')]
  },
  getReferencesAtPosition: {
    expectedFilePath: resolveTestPath('test/expected/getReferencesAtPosition.json'),
    '1': [resolveTestPath('test/testcases/getReferencesAtPosition/getReferencesAtPosition1.ets'), 613],
    '2': [resolveTestPath('test/testcases/getReferencesAtPosition/getReferencesAtPosition2.ets'), 635],
    '3': [resolveTestPath('test/testcases/getReferencesAtPosition/getReferencesAtPosition4.ets'), 625],
    '4': [resolveTestPath('test/testcases/getReferencesAtPosition/getReferencesAtPosition6.ets'), 697]
  },
  getSyntacticDiagnostics: {
    expectedFilePath: resolveTestPath('test/expected/getSyntacticDiagnostics.json'),
    '1': [resolveTestPath('test/testcases/getSyntacticDiagnostics/getSyntacticDiagnostics1.ets')],
    '2': [resolveTestPath('test/testcases/getSyntacticDiagnostics/getSyntacticDiagnostics2.ets')],
    '3': [resolveTestPath('test/testcases/getSyntacticDiagnostics/getSyntacticDiagnostics3.ets')],
    '4': [resolveTestPath('test/testcases/getSyntacticDiagnostics/getSyntacticDiagnostics4.ets')]
  },
  getSuggestionDiagnostics: {
    expectedFilePath: resolveTestPath('test/expected/getSuggestionDiagnostics.json'),
    '1': [resolveTestPath('test/testcases/getSuggestionDiagnostics/getSuggestionDiagnostics1.ets')]
  },
  getQuickInfoAtPosition: {
    expectedFilePath: resolveTestPath('test/expected/getQuickInfoAtPosition.json'),
    '1': [resolveTestPath('test/testcases/getQuickInfoAtPosition/getQuickInfoAtPosition1.ets'), 626],
    '2': [resolveTestPath('test/testcases/getQuickInfoAtPosition/getQuickInfoAtPosition2.ets'), 618],
    '3': [resolveTestPath('test/testcases/getQuickInfoAtPosition/getQuickInfoAtPosition3.ets'), 663],
    '4': [resolveTestPath('test/testcases/getQuickInfoAtPosition/getQuickInfoAtPosition4.ets'), 697]
  },
  getDocumentHighlights: {
    expectedFilePath: resolveTestPath('test/expected/getDocumentHighlights.json'),
    '1': [resolveTestPath('test/testcases/getDocumentHighlights/getDocumentHighlights1.ets'), 614],
    '2': [resolveTestPath('test/testcases/getDocumentHighlights/getDocumentHighlights2.ets'), 717],
    '3': [resolveTestPath('test/testcases/getDocumentHighlights/getDocumentHighlights3.ets'), 616],
    '4': [resolveTestPath('test/testcases/getDocumentHighlights/getDocumentHighlights4.ets'), 626],
    '5': [resolveTestPath('test/testcases/getDocumentHighlights/getDocumentHighlights5.ets'), 619],
    '6': [resolveTestPath('test/testcases/getDocumentHighlights/getDocumentHighlights6.ets'), 657],
    '7': [resolveTestPath('test/testcases/getDocumentHighlights/getDocumentHighlights7.ets'), 733],
    '8': [resolveTestPath('test/testcases/getDocumentHighlights/getDocumentHighlights8.ets'), 677],
    '9': [resolveTestPath('test/testcases/getDocumentHighlights/getDocumentHighlights9.ets'), 620]
  },
  getCompletionAtPosition: {
    expectedFilePath: resolveTestPath('test/expected/getCompletionAtPosition.json'),
    '1': [resolveTestPath('test/testcases/getCompletionAtPosition/getCompletionsAtPosition1.ets'), 705],
    '2': [resolveTestPath('test/testcases/getCompletionAtPosition/getCompletionsAtPosition2.ets'), 735],
    '3': [resolveTestPath('test/testcases/getCompletionAtPosition/getCompletionsAtPosition3.ets'), 789],
    '4': [resolveTestPath('test/testcases/getCompletionAtPosition/getCompletionsAtPosition4.ets'), 767],
    '5': [resolveTestPath('test/testcases/getCompletionAtPosition/getCompletionsAtPosition5.ets'), 728],
    '6': [resolveTestPath('test/testcases/getCompletionAtPosition/getCompletionsAtPosition6.ets'), 718],
    '7': [resolveTestPath('test/testcases/getCompletionAtPosition/getCompletionsAtPosition7.ets'), 683],
    '8': [resolveTestPath('test/testcases/getCompletionAtPosition/getCompletionsAtPosition8.ets'), 614],
    '9': [resolveTestPath('test/testcases/getCompletionAtPosition/getCompletionsAtPosition9.ets'), 619],
    '10': [resolveTestPath('test/testcases/getCompletionAtPosition/getCompletionsAtPosition10.ets'), 712],
    '11': [resolveTestPath('test/testcases/getCompletionAtPosition/getCompletionsAtPosition11.ets'), 682],
    '12': [resolveTestPath('test/testcases/getCompletionAtPosition/getCompletionsAtPosition12.ets'), 720],
    '13': [resolveTestPath('test/testcases/getCompletionAtPosition/getCompletionsAtPosition13.ets'), 658],
    '14': [resolveTestPath('test/testcases/getCompletionAtPosition/getCompletionsAtPosition14.ets'), 659],
    '15': [resolveTestPath('test/testcases/getCompletionAtPosition/getCompletionsAtPosition15.ets'), 722],
    '16': [resolveTestPath('test/testcases/getCompletionAtPosition/getCompletionsAtPosition17.ets'), 764],
    '17': [resolveTestPath('test/testcases/getCompletionAtPosition/getCompletionsAtPosition17.ets'), 782]
  },
  toLineColumnOffset: {
    expectedFilePath: resolveTestPath('test/expected/toLineColumnOffset.json'),
    '1': [resolveTestPath('test/testcases/toLineColumnOffset/toLineColumnOffset1.ets'), 0],
    '2': [resolveTestPath('test/testcases/toLineColumnOffset/toLineColumnOffset1.ets'), 642],
    '3': [resolveTestPath('test/testcases/toLineColumnOffset/toLineColumnOffset2.ets'), 709]
  },
  getSpanOfEnclosingComment: {
    expectedFilePath: resolveTestPath('test/expected/getSpanOfEnclosingComment.json'),
    '1': [resolveTestPath('test/testcases/getSpanOfEnclosingComment/getSpanOfEnclosingComment1.ets'), 669, false],
    '2': [resolveTestPath('test/testcases/getSpanOfEnclosingComment/getSpanOfEnclosingComment1.ets'), 663, false],
    '3': [resolveTestPath('test/testcases/getSpanOfEnclosingComment/getSpanOfEnclosingComment2.ets'), 663, false]
  },
  provideInlayHints: {
    expectedFilePath: resolveTestPath('test/expected/provideInlayHints.json'),
    '1': [
      resolveTestPath('test/testcases/provideInlayHints/provideInlayHints1.ets'),
      { start: 712, length: 11 } as TextSpan
    ],
    '2': [
      resolveTestPath('test/testcases/provideInlayHints/provideInlayHints2.ets'),
      { start: 683, length: 5 } as TextSpan
    ]
  },
  getCodeFixesAtPosition: {
    expectedFilePath: resolveTestPath('test/expected/getCodeFixesAtPosition.json'),
    '1': [resolveTestPath('test/testcases/getCodeFixesAtPosition/getCodeFixesAtPosition1.ets'), 994, 995, [4000]]
  },
  getSignatureHelpItems: {
    expectedFilePath: resolveTestPath('test/expected/getSignatureHelpItems.json'),
    '1': [resolveTestPath('test/testcases/getSignatureHelpItems/getSignatureHelpItems1.ets'), 678]
  },
  findRenameLocations: {
    expectedFilePath: resolveTestPath('test/expected/findRenameLocations.json'),
    '1': [resolveTestPath('test/testcases/findRenameLocations/findRenameLocations2.ets'), 632],
    '2': [resolveTestPath('test/testcases/findRenameLocations/findRenameLocations1.ets'), 627],
    '3': [resolveTestPath('test/testcases/findRenameLocations/findRenameLocations1.ets'), 670],
    '4': [resolveTestPath('test/testcases/findRenameLocations/findRenameLocations1.ets'), 721],
    '5': [resolveTestPath('test/testcases/findRenameLocations/findRenameLocations2.ets'), 676],
    '6': [resolveTestPath('test/testcases/findRenameLocations/findRenameLocations2.ets'), 868],
    '7': [resolveTestPath('test/testcases/findRenameLocations/findRenameLocations1.ets'), 720],
    '8': [resolveTestPath('test/testcases/findRenameLocations/findRenameLocations3.ets'), 627]
  },
  getRenameInfo: {
    expectedFilePath: resolveTestPath('test/expected/getRenameInfo.json'),
    '1': [resolveTestPath('test/testcases/getRenameInfo/getRenameInfo1.ets'), 615],
    '2': [resolveTestPath('test/testcases/getRenameInfo/getRenameInfo2.ets'), 626],
    '3': [resolveTestPath('test/testcases/getRenameInfo/getRenameInfo3.ets'), 697]
  },
  getOffsetByColAndLine: {
    expectedFilePath: resolveTestPath('test/expected/getOffsetByColAndLine.json'),
    '1': [resolveTestPath('test/testcases/getOffsetByColAndLine/getOffsetByColAndLine1.ets'), 51, 14]
  },
  getColAndLineByOffset: {
    expectedFilePath: resolveTestPath('test/expected/getColAndLineByOffset.json'),
    '1': [resolveTestPath('test/testcases/getColAndLineByOffset/getColAndLineByOffset1.ets'), 1035]
  },
  entry: {
    expectedFilePath: '',
    '1': [resolveTestPath('test/testcases/entry/Index.ets'), 615]
  },
  generateDeclFile: {
    expectedFilePath: resolveTestPath('test/expected/generateDeclFile.json')
  }
};

export const getSpanOfEnclosingCommentTests = basicCases.getSpanOfEnclosingComment;

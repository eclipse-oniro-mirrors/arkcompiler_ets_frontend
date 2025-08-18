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
import { TextSpan } from '../src/lspNode';

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

export const testCases: TestCases = {
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
    '11': [resolveTestPath('test/testcases/getDefinitionAtPosition/getDefinitionAtPosition19.ets'), 634]
  },
  getSemanticDiagnostics: {
    expectedFilePath: resolveTestPath('test/expected/getSemanticDiagnostics.json'),
    '1': [resolveTestPath('test/testcases/getSemanticDiagnostics/getSemanticDiagnostics1.ets')],
    '2': [resolveTestPath('test/testcases/getSemanticDiagnostics/getSemanticDiagnostics2.ets')]
  },
  getCurrentTokenValue: {
    expectedFilePath: resolveTestPath('test/expected/getCurrentTokenValue.json'),
    '1': [resolveTestPath('test/testcases/getCurrentTokenValue/getCurrentTokenValue1.ets'), 611],
    '2': [resolveTestPath('test/testcases/getCurrentTokenValue/getCurrentTokenValue2.ets'), 612],
    '3': [resolveTestPath('test/testcases/getCurrentTokenValue/getCurrentTokenValue3.ets'), 612],
    '4': [resolveTestPath('test/testcases/getCurrentTokenValue/getCurrentTokenValue4.ets'), 611]
  },
  getFileReferences: {
    expectedFilePath: resolveTestPath('test/expected/getFileReferences.json'),
    '1': [resolveTestPath('test/testcases/getFileReferences/getFileReferences1_export.ets')]
  },
  getReferencesAtPosition: {
    expectedFilePath: resolveTestPath('test/expected/getReferencesAtPosition.json'),
    '1': [resolveTestPath('test/testcases/getReferencesAtPosition/getReferencesAtPosition1.ets'), 613],
    '2': [resolveTestPath('test/testcases/getReferencesAtPosition/getReferencesAtPosition2.ets'), 635],
    '3': [resolveTestPath('test/testcases/getReferencesAtPosition/getReferencesAtPosition4.ets'), 625]
  },
  getSyntacticDiagnostics: {
    expectedFilePath: resolveTestPath('test/expected/getSyntacticDiagnostics.json'),
    '1': [resolveTestPath('test/testcases/getSyntacticDiagnostics/getSyntacticDiagnostics1.ets')],
    '2': [resolveTestPath('test/testcases/getSyntacticDiagnostics/getSyntacticDiagnostics2.ets')]
  },
  getSuggestionDiagnostics: {
    expectedFilePath: resolveTestPath('test/expected/getSuggestionDiagnostics.json'),
    '1': [resolveTestPath('test/testcases/getSuggestionDiagnostics/getSuggestionDiagnostics1.ets')]
  },
  getQuickInfoAtPosition: {
    expectedFilePath: resolveTestPath('test/expected/getQuickInfoAtPosition.json'),
    '1': [resolveTestPath('test/testcases/getQuickInfoAtPosition/getQuickInfoAtPosition1.ets'), 626],
    '2': [resolveTestPath('test/testcases/getQuickInfoAtPosition/getQuickInfoAtPosition2.ets'), 618],
    '3': [resolveTestPath('test/testcases/getQuickInfoAtPosition/getQuickInfoAtPosition3.ets'), 663]
  },
  getDocumentHighlights: {
    expectedFilePath: resolveTestPath('test/expected/getDocumentHighlights.json'),
    '1': [resolveTestPath('test/testcases/getDocumentHighlights/getDocumentHighlights1.ets'), 614],
    '2': [resolveTestPath('test/testcases/getDocumentHighlights/getDocumentHighlights2.ets'), 717],
    '3': [resolveTestPath('test/testcases/getDocumentHighlights/getDocumentHighlights3.ets'), 616],
    '4': [resolveTestPath('test/testcases/getDocumentHighlights/getDocumentHighlights4.ets'), 626],
    '5': [resolveTestPath('test/testcases/getDocumentHighlights/getDocumentHighlights5.ets'), 619],
    '6': [resolveTestPath('test/testcases/getDocumentHighlights/getDocumentHighlights6.ets'), 657],
    '7': [resolveTestPath('test/testcases/getDocumentHighlights/getDocumentHighlights7.ets'), 733]
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
    '14': [resolveTestPath('test/testcases/getCompletionAtPosition/getCompletionsAtPosition14.ets'), 659]
  },
  toLineColumnOffset: {
    expectedFilePath: resolveTestPath('test/expected/toLineColumnOffset.json'),
    '1': [resolveTestPath('test/testcases/toLineColumnOffset/toLineColumnOffset1.ets'), 0],
    '2': [resolveTestPath('test/testcases/toLineColumnOffset/toLineColumnOffset1.ets'), 642]
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
    ]
  },
  getSignatureHelpItems: {
    expectedFilePath: resolveTestPath('test/expected/getSignatureHelpItems.json'),
    '1': [resolveTestPath('test/testcases/getSignatureHelpItems/getSignatureHelpItems1.ets'), 613],
    '2': [resolveTestPath('test/testcases/getSignatureHelpItems/getSignatureHelpItems1.ets'), 620],
    '3': [resolveTestPath('test/testcases/getSignatureHelpItems/getSignatureHelpItems1.ets'), 678]
  }
};

export const getSpanOfEnclosingCommentTests = testCases.getSpanOfEnclosingComment;

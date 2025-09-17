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
import fs from 'fs';
import { Lsp, LspDefinitionData, LspCompletionInfo, LspDiagsNode, ModuleDescriptor, PathConfig } from '../src/index';
import { TestCases, basicCases } from './cases';
import { LspCompletionEntry } from '../src/lsp/lspNode';
import { diff } from 'jest-diff';

interface NormalizeOptions {
  fieldsToDelete?: string[]; // try to delete these fields in the expected result, just focus on the important fields
  normalizeFileName?: boolean;
}

interface ComparisonOptions {
  subMatch?: boolean;
}

interface ComparisonOutcome {
  passed: boolean;
  expectedJSON?: string;
  actualJSON?: string;
}

let updateMode = false;

function getModules(projectRoot: string, cases: TestCases): ModuleDescriptor[] {
  return Object.keys(cases).map((name) => {
    const modulePath = path.join(projectRoot, name);
    return {
      arktsversion: '1.2',
      name,
      moduleType: 'har',
      srcPath: modulePath
    } as ModuleDescriptor;
  });
}

// CC-OFFNXT(no_explicit_any) project code style
function getExpectedResult(filePath: string): any {
  try {
    return JSON.parse(fs.readFileSync(filePath, 'utf-8'));
  } catch (err) {
    console.error(`Failed to read expected result from ${filePath}: ${err}`);
    return null;
  }
}

// CC-OFFNXT(no_explicit_any) project code style
function getFilesByDir(dirPath: string): string[] {
  try {
    return fs.readdirSync(dirPath).filter((file) => fs.statSync(path.join(dirPath, file)).isFile());
  } catch (err) {
    console.error(`Failed to load files from ${dirPath}: ${err}`);
    return [];
  }
}

function sortCompletions(completionResult: LspCompletionInfo): LspCompletionInfo {
  if (!completionResult || !completionResult.entries || !Array.isArray(completionResult.entries)) {
    return completionResult;
  }

  // Sort entries by name
  completionResult.entries.sort((a, b) => {
    const nameA = a.name.toString().toLowerCase();
    const nameB = b.name.toString().toLowerCase();
    return nameA.localeCompare(nameB);
  });

  return completionResult;
}

function sortDiagnostics(diags: LspDiagsNode): LspDiagsNode {
  if (!diags || !diags.diagnostics || !Array.isArray(diags.diagnostics)) {
    return diags;
  }

  diags.diagnostics.sort((a, b) => {
    if (a.range.start.line !== b.range.start.line) {
      return a.range.start.line - b.range.start.line;
    }

    if (a.range.start.character !== b.range.start.character) {
      return a.range.start.character - b.range.start.character;
    }

    if (a.range.end.line !== b.range.end.line) {
      return a.range.end.line - b.range.end.line;
    }

    return a.range.end.character - b.range.end.character;
  });

  return diags;
}

// CC-OFFNXT(no_explicit_any) project code style
function sortActualResult(testName: string, res: any): any {
  if (testName === 'getCompletionAtPosition') {
    return sortCompletions(res as LspCompletionInfo);
  }
  if (testName === 'getSuggestionDiagnostics') {
    return sortDiagnostics(res as LspDiagsNode);
  }
  return res;
}

// CC-OFFNXT(no_explicit_any) project code style
function normalizeData(obj: any, options: NormalizeOptions = {}): any {
  const { fieldsToDelete = [], normalizeFileName = true } = options;
  if (Array.isArray(obj)) {
    return obj.map((item) => normalizeData(item, options));
  } else if (obj && typeof obj === 'object') {
    const newObj = { ...obj };
    // always remove 'peer' field
    if ('peer' in newObj) {
      delete newObj.peer;
    }
    // remove specified fields
    fieldsToDelete.forEach((field) => {
      if (field in newObj) {
        delete newObj[field];
      }
    });
    if (normalizeFileName && newObj.fileName) {
      newObj.fileName = path.basename(newObj.fileName);
    }
    for (const key of Object.keys(newObj)) {
      newObj[key] = normalizeData(newObj[key], options);
    }
    return newObj;
  }
  return obj;
}

// CC-OFFNXT(no_explicit_any) project code style
function isSubObject(actual: any, expected: any): boolean {
  if (typeof expected !== 'object' || expected === null) {
    return actual === expected;
  }

  if (typeof actual !== 'object' || actual === null) {
    return false;
  }

  if (Array.isArray(expected)) {
    if (!Array.isArray(actual)) {
      return false;
    }
    return expected.every((expectedItem) => actual.some((actualItem) => isSubObject(actualItem, expectedItem)));
  }

  for (const key in expected) {
    if (Object.prototype.hasOwnProperty.call(expected, key)) {
      if (!Object.prototype.hasOwnProperty.call(actual, key)) {
        return false;
      }
      if (!isSubObject(actual[key], expected[key])) {
        return false;
      }
    }
  }

  return true;
}

function performComparison(
  normalizedActual: unknown,
  expected: unknown,
  options: ComparisonOptions = {}
): ComparisonOutcome {
  const { subMatch: subMatch = false } = options;
  if (subMatch) {
    if (isSubObject(normalizedActual, expected)) {
      return { passed: true };
    }
    return {
      passed: false,
      expectedJSON: JSON.stringify(expected, null, 2),
      actualJSON: JSON.stringify(normalizedActual, null, 2)
    };
  }

  const actualJSON = JSON.stringify(normalizedActual, null, 2);
  const expectedJSON = JSON.stringify(expected, null, 2);

  if (actualJSON === expectedJSON) {
    return { passed: true };
  }

  return {
    passed: false,
    expectedJSON: expectedJSON,
    actualJSON: actualJSON
  };
}

function compareResultsHelper(
  testName: string,
  normalizedActual: unknown,
  expected: unknown,
  options: ComparisonOptions = {}
): boolean {
  const comparison = performComparison(normalizedActual, expected, options);

  if (comparison.passed) {
    console.log(`[${testName}] ✅ Passed`);
    return true;
  }

  console.log(`[${testName}] ❌ Failed`);
  const diffResult = diff(comparison.expectedJSON, comparison.actualJSON);
  if (diffResult) {
    console.log(diffResult);
  }
  return false;
}

function compareGetCompletionResult(testName: string, actual: unknown, expected: unknown): [boolean, unknown] {
  const completionResult = actual as LspCompletionInfo;
  const actualEntries = completionResult.entries as LspCompletionEntry[];
  const expectedEntries = expected as {
    name: string;
    sortText: string;
    insertText: string;
    kind: number;
    data: null;
  }[];

  const actualData = normalizeData(actualEntries);
  return [
    compareResultsHelper(testName, actualData, expectedEntries, {
      subMatch: true
    } as ComparisonOptions),
    actualData
  ];
}

function compareDeclFileResult(testName: string, declgenOutDir: string, expected: unknown): [boolean, unknown] {
  let fileList: string[] = getFilesByDir(declgenOutDir);
  const actualEntries = fileList.filter((file) => file.endsWith('.d.ets'));
  const expectedEntries = expected as string[];
  const actualData = normalizeData(actualEntries);
  return [
    compareResultsHelper(testName, actualData, expectedEntries, {
      subMatch: true
    } as ComparisonOptions),
    actualData
  ];
}

function findTextDefinitionPosition(sourceCode: string): number {
  const textDefinitionPattern = /export\s+declare\s+function\s+Text\(/;
  const match = textDefinitionPattern.exec(sourceCode);
  if (match) {
    const functionTextPattern = /function\s+Text\(/;
    const subMatch = functionTextPattern.exec(sourceCode.substring(match.index));
    if (subMatch) {
      const positionOfT = match.index + subMatch.index + 'function '.length;
      return positionOfT;
    }
  }
  throw new Error('Could not find Text definition in source code');
}

// CC-OFFNXT(huge_cyclomatic_complexity, huge_depth, huge_method) false positive
function findTaskDefinitionPosition(sourceCode: string): number {
  const taskDefinitionPattern = /export\s+class\s+Task\s+{/;
  const match = taskDefinitionPattern.exec(sourceCode);
  if (match) {
    const classTaskPattern = /class\s+Task\s+{/;
    const subMatch = classTaskPattern.exec(sourceCode.substring(match.index));
    if (subMatch) {
      const positionOfT = match.index + subMatch.index + 'class '.length;
      return positionOfT;
    }
  }
  throw new Error('Could not find Task definition in source code');
}

function compareGetDefinitionResult(
  testName: string,
  actual: unknown,
  expected: Record<string, string | number>
): [boolean, unknown] {
  let expectedResult = expected;
  const actualDef = actual as LspDefinitionData;
  const fileName = actualDef.fileName as string;
  const fileContent = fs.readFileSync(fileName, 'utf8');
  // This is the definition info for the UI component.
  // File in the SDK might changed, so the offset needs to be checked dynamically.
  if (expected['fileName'] === 'text.d.ets') {
    const expectedStart = findTextDefinitionPosition(fileContent);
    expectedResult = {
      ...expected,
      start: expectedStart
    };
  }
  // This is the definition info for the class in std library.
  // File in the SDK might changed, so the offset needs to be checked dynamically.
  if (expected['fileName'] === 'taskpool.ets') {
    const expectedStart = findTaskDefinitionPosition(fileContent);
    expectedResult = {
      ...expected,
      start: expectedStart
    };
  }
  const actualData = normalizeData(actual);
  return [compareResultsHelper(testName, actualData, expectedResult), actualData];
}

function compareResults(
  caseName: string,
  actual: unknown,
  expected: unknown,
  pathConfig: PathConfig
): [boolean, unknown] {
  const testName = caseName.substring(0, caseName.indexOf(':'));
  if (testName === 'getDefinitionAtPosition') {
    return compareGetDefinitionResult(caseName, actual, expected as Record<string, string | number>);
  }
  if (testName === 'getCompletionAtPosition') {
    return compareGetCompletionResult(caseName, actual, expected);
  }
  if (testName === 'generateDeclFile') {
    const declOutPath = path.join(pathConfig.declgenOutDir, testName, 'declgen', 'static');
    return compareDeclFileResult(caseName, declOutPath, expected);
  }
  if (
    testName === 'getSemanticDiagnostics' ||
    testName === 'getSyntacticDiagnostics' ||
    testName === 'getSuggestionDiagnostics'
  ) {
    const normalizeOption: NormalizeOptions = {
      fieldsToDelete: ['source']
    };
    const actualData = normalizeData(actual, normalizeOption);
    return [compareResultsHelper(caseName, actualData, expected), actualData];
  }

  const actualData = normalizeData(actual);
  return [compareResultsHelper(caseName, actualData, expected), actualData];
}

function findDeclFileFolders(projectPath: string, declFileInfoJson: string): string[] {
  const result: string[] = [];
  try {
    const entries = fs.readdirSync(projectPath, { withFileTypes: true });
    for (const entry of entries) {
      if (entry.isDirectory()) {
        const subDirPath = path.join(projectPath, entry.name);
        const declFilePath = path.join(subDirPath, declFileInfoJson);
        if (fs.existsSync(declFilePath)) {
          result.push(entry.name);
        }
      }
    }
  } catch (error) {
    console.error(`Error searching directory ${projectPath}:`, error);
  }
  return result;
}

function setDeclFileAndJson(testDir: string, pathConfig: PathConfig): void {
  const declFileInfoJson: string = 'decl-fileInfo.json';
  const declFileFolders = findDeclFileFolders(pathConfig.projectPath, declFileInfoJson);
  declFileFolders.forEach((item: string) => {
    const dynamicPath = path.join(pathConfig.declgenOutDir, item, 'declgen', 'dynamic');
    if (!fs.existsSync(dynamicPath)) {
      fs.mkdirSync(dynamicPath, { recursive: true });
    }
    const declTestFile = item + 'Test.d.ets';
    const declFilePath = path.join(pathConfig.projectPath, item, declTestFile);
    const declJsonPath = path.join(pathConfig.projectPath, item, declFileInfoJson);
    fs.copyFileSync(declFilePath, path.join(dynamicPath, declTestFile));
    if (fs.existsSync(declJsonPath)) {
      try {
        const fileContent = fs.readFileSync(declJsonPath, 'utf8');
        const updatedContent = fileContent.replace(/\$\{absolute_path_to_build_system\}/g, testDir);
        const targetPath = path.join(dynamicPath, declFileInfoJson);
        fs.writeFileSync(targetPath, updatedContent, 'utf8');
      } catch (error) {
        console.error(`Error processing file ${declJsonPath}:`, error);
      }
    }
  });
}

function runTests(lsp: Lsp, cases: TestCases, failedList: string[], pathConfig: PathConfig): string[] {
  console.log('Running tests...');
  if (!cases) {
    return [];
  }

  for (const [testName, testConfig] of Object.entries(cases)) {
    const { expectedFilePath, ...testCaseVariants } = testConfig;
    const expectedResult = getExpectedResult(expectedFilePath);
    if (expectedResult === null) {
      console.error(`[${testName}] Skipped (expected result not found)`);
      continue;
    }
    // CC-OFFNXT(no_explicit_any) project code style
    if (typeof (lsp as any)[testName] !== 'function') {
      console.error(`[${testName}] ❌ Error: Method "${testName}" not found on Lsp object`);
      continue;
    }

    for (const [index, params] of Object.entries(testCaseVariants)) {
      let pass = false;
      let actualData = undefined;
      let actualResult = null;
      try {
        // CC-OFFNXT(no_explicit_any) project code style
        actualResult = (lsp as any)[testName](...params);
        actualResult = sortActualResult(testName, actualResult);
        [pass, actualData] = compareResults(`${testName}:${index}`, actualResult, expectedResult[index], pathConfig);
      } catch (error) {
        console.error(`[${testName}:${index}] ❌ Error: ${error}`);
      }
      if (!pass) {
        failedList.push(`${testName}:${index}`);
      }
      if (!pass && updateMode) {
        console.log(`Updating expected result for ${testName}:${index}`);
        expectedResult[index] = actualData;
      }
    }
    if (updateMode) {
      fs.writeFileSync(expectedFilePath, JSON.stringify(expectedResult, null, 2));
    }
    console.log(`Finished test: ${testName}`);
    console.log('-----------------------------------');
  }
  return failedList;
}

function run(testDir: string, pathConfig: PathConfig): void {
  let failedList: string[] = [];

  const basicModules = getModules(pathConfig.projectPath, basicCases);
  setDeclFileAndJson(testDir, pathConfig);
  const basicLsp = new Lsp(pathConfig, undefined, basicModules);
  failedList = runTests(basicLsp, basicCases, failedList, pathConfig);

  console.log('Tests completed.');
  if (failedList.length > 0) {
    console.log('❌ Failed tests:');
    failedList.forEach((failedCase: string) => {
      console.log(`- ${failedCase}`);
    });

    console.error('Tests failed without AST cache');
    process.exit(1);
  }
  console.log('Finished test without ast cache');
}

async function runWithAstCache(testDir: string, pathConfig: PathConfig): Promise<void> {
  let failedList: string[] = [];
  // for generate ast cache
  const entry_module = [
    {
      name: 'entry',
      moduleType: 'har',
      srcPath: path.join(pathConfig.projectPath, 'entry')
    },
    {
      name: 'getDefinitionAtPosition',
      moduleType: 'har',
      srcPath: path.join(pathConfig.projectPath, 'getDefinitionAtPosition')
    },
    {
      name: 'getReferencesAtPosition',
      moduleType: 'har',
      srcPath: path.join(pathConfig.projectPath, 'getReferencesAtPosition')
    },
    {
      name: 'findRenameLocations',
      moduleType: 'har',
      srcPath: path.join(pathConfig.projectPath, 'findRenameLocations')
    },
    {
      name: '1.1',
      moduleType: 'entry',
      srcPath: path.join(pathConfig.projectPath, '1.1')
    },
  ];

  const basicModules = getModules(pathConfig.projectPath, basicCases);
  setDeclFileAndJson(testDir, pathConfig);
  const basicLsp = new Lsp(pathConfig, undefined, entry_module);
  await basicLsp.initAstCache();
  basicLsp.update(basicModules);
  failedList = runTests(basicLsp, basicCases, failedList, pathConfig);

  console.log('Tests completed.');
  if (failedList.length > 0) {
    console.log('❌ Failed tests:');
    failedList.forEach((failedCase: string) => {
      console.log(`- ${failedCase}`);
    });

    console.error('Tests failed with AST cache');
    process.exit(1);
  }
  console.log('Finished test with ast cache');
}

if (require.main === module) {
  if (process.argv.length < 3) {
    console.error('Usage: node run_tests.js <test_directory>');
    process.exit(1);
  }
  // If update flag is passed, update the expected result files
  if (process.argv[3] && process.argv[3] === '--update') {
    updateMode = true;
  }

  const testDir = path.resolve(process.argv[2]);
  const pathConfig: PathConfig = {
    buildSdkPath: path.join(testDir, 'ets', 'ets1.2'),
    projectPath: path.join(testDir, 'testcases'),
    declgenOutDir: path.join(testDir, 'testcases', '.idea', '.deveco')
  };

  process.env.BINDINGS_PATH = path.join(pathConfig.buildSdkPath, 'build-tools', 'bindings');
  process.env.PANDA_LIB_PATH = path.join(pathConfig.buildSdkPath, 'build-tools', 'ets2panda', 'lib');
  process.env.PANDA_BIN_PATH = path.join(pathConfig.buildSdkPath, 'build-tools', 'ets2panda', 'bin');
  run(testDir, pathConfig);
  runWithAstCache(testDir, pathConfig).then(() => {});
  fs.writeFileSync('./finished.txt', 'success', 'utf8');
}

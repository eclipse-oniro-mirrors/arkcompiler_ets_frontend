/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import * as fs from 'fs';
import * as os from 'os';
import * as path from 'path';
import * as ts from 'typescript';
import { Command } from 'commander';

import { ExitCode, TestFile, MismatchTestResult, MismatchReportResult, TestResult } from './cookbookTest';
import {
  CheckerResult,
  Declgen,
  DeclgenOptions,
  DeclgenFeatureOptions,
  DeclgenResult,
} from '../../../src/dynamic/declgen';
import { Extension } from '../../../src/dynamic/utils/extension';
import { Logger } from '../../../src/dynamic/logger/logger';
import kleur from 'kleur';
import { diff } from 'jest-diff';

/**
 * CookbookExecutor executes a single test case and produces a structured result.
 */
export interface ExecutionResult {
  testResult: TestResult;
  duration: number;
}

function readDirRecursive(dir: string): string[] {
  const result: string[] = [];

  function walk(current: string): void {
    for (const entry of fs.readdirSync(current, { withFileTypes: true })) {
      const fullPath = path.join(current, entry.name);
      if (entry.isDirectory()) {
        walk(fullPath);
      } else {
        result.push(fullPath);
      }
    }
  }

  walk(dir);
  return result;
}

async function openFileWithoutComments(fileName: string): Promise<string> {
  const code = await fs.promises.readFile(fileName, 'utf-8');
  const sf = ts.createSourceFile(fileName, code, ts.ScriptTarget.Latest, true, ts.ScriptKind.TS);
  const printer = ts.createPrinter({ removeComments: true, newLine: ts.NewLineKind.LineFeed });
  return printer.printFile(sf);
}

async function compareSingleFile(expectedPath: string, actualPath: string): Promise<MismatchTestResult[]> {
  let actualFile: TestFile = { file: actualPath };
  let expectedFile: TestFile = { file: expectedPath };
  if (fs.existsSync(actualPath)) {
    actualFile.content = await openFileWithoutComments(actualPath);
  }
  if (fs.existsSync(expectedPath)) {
    expectedFile.content = await openFileWithoutComments(expectedPath);
  }

  if (expectedFile.content !== actualFile.content) {
    return [
      {
        expected: expectedFile,
        actual: actualFile,
      },
    ];
  }

  return [];
}

async function compareMultipleFiles(expectedDir: string, actualDir: string): Promise<MismatchTestResult[]> {
  const expectedFiles = new Set(readDirRecursive(expectedDir).map((f) => path.relative(expectedDir, f)));

  const mismatchResults: MismatchTestResult[] = [];

  for (const relFile of expectedFiles) {
    const res = await compareSingleFile(path.join(expectedDir, relFile), path.join(actualDir, relFile));
    mismatchResults.push(...res);
  }

  return mismatchResults;
}

async function compareOutputs(
  suite: string,
  testName: string,
  expectedOutput: string,
  outDir: string,
): Promise<MismatchTestResult[]> {
  if (suite !== 'package_source') {
    const actualPath = path.join(outDir, `${testName}${Extension.DETS}`);
    return compareSingleFile(expectedOutput, actualPath);
  }
  return compareMultipleFiles(expectedOutput, outDir);
}

interface TestReport {
  nodes: Array<{ line: number; column: number; faultId: string }>;
}

function toTestReport(checkResult: CheckerResult): TestReport {
  void checkResult;

  return { nodes: [] } as TestReport;
}

async function compareReports(
  checkResult: CheckerResult,
  expectedReportPath: string,
  outDir: string,
  testName: string,
): Promise<MismatchReportResult[]> {
  const actualReportPath = path.join(outDir, `${testName}${ts.Extension.Json}`);

  if (!fs.existsSync(expectedReportPath)) {
    return [{ detail: `Expected report not found: ${expectedReportPath}` }];
  }

  const expectedReport: TestReport = JSON.parse(fs.readFileSync(expectedReportPath, 'utf-8'));

  if (!expectedReport?.nodes || !Array.isArray(expectedReport.nodes)) {
    return [{ detail: `Invalid format of expected report: ${expectedReportPath}` }];
  }

  // For now the checker returns an empty report (see existing TestRunner logic)
  const actualReport: TestReport = toTestReport(checkResult);

  // Save the actual report
  if (!fs.existsSync(outDir)) {
    await fs.promises.mkdir(outDir, { recursive: true });
  }
  await fs.promises.writeFile(actualReportPath, JSON.stringify(actualReport, undefined, 4));

  if (expectedReport.nodes.length !== actualReport.nodes.length) {
    return [
      {
        detail: `Report node count mismatch: expected ${expectedReport.nodes.length}, got ${actualReport.nodes.length}`,
      },
    ];
  }

  let mismatchResults: MismatchReportResult[] = [];

  for (let i = 0; i < actualReport.nodes.length; i++) {
    const exp = expectedReport.nodes[i]!;
    const act = actualReport.nodes[i]!;
    if (exp.line !== act.line || exp.column !== act.column || exp.faultId !== act.faultId) {
      mismatchResults.push({
        detail: `Report node mismatch at index ${i}: expected ${JSON.stringify(exp)}, got ${JSON.stringify(act)}`,
      });
    }
  }

  return mismatchResults;
}

function createErrorResult(suite: string, testCase: string, message: string): TestResult {
  return {
    suite,
    testCase,
    exitCode: ExitCode.ERROR,
    outputMismatch: [],
    reportMismatch: [],
    message,
  };
}

/**
 * Resolved test inputs: either the files to feed declgen, or a ready-made
 * error result when the inputs cannot be resolved.
 */
type InputFileResolution =
  | { inputFiles: string[]; rootDir: string | undefined; error?: undefined }
  | { inputFiles?: undefined; rootDir?: undefined; error: TestResult };

function resolvePackageSourceInputs(suite: string, suitePath: string, testCase: string): InputFileResolution {
  const rootDir = path.join(suitePath, testCase);
  const inputJsonPath = path.join(rootDir, 'input.json');
  if (!fs.existsSync(inputJsonPath)) {
    return { error: createErrorResult(suite, testCase, `input.json not found: ${inputJsonPath}`) };
  }
  const inputJson = JSON.parse(fs.readFileSync(inputJsonPath, 'utf-8')) as string[];
  return { inputFiles: inputJson.map((f) => path.join(rootDir, f)), rootDir };
}

function resolveSingleSourceInput(suite: string, suitePath: string, testCase: string): InputFileResolution {
  const ext = suite === 'ts_source' ? ts.Extension.Ts : ts.Extension.Dts;
  const sourcePath = path.join(suitePath, `${testCase}${ext}`);
  if (!fs.existsSync(sourcePath)) {
    return { error: createErrorResult(suite, testCase, `Source file not found: ${sourcePath}`) };
  }
  return { inputFiles: [sourcePath], rootDir: undefined };
}

function resolveInputFiles(testDir: string, suite: string, testCase: string): InputFileResolution {
  const suitePath = path.join(testDir, suite);
  if (suite === 'package_source') {
    return resolvePackageSourceInputs(suite, suitePath, testCase);
  }
  return resolveSingleSourceInput(suite, suitePath, testCase);
}

/**
 * Loads the optional per-test feature options file. Returns the effective
 * options, or an error message when the file exists but cannot be read/parsed.
 */
function loadFeatureOptions(testDir: string, testCase: string): Required<DeclgenFeatureOptions> | string {
  const defaults: Required<DeclgenFeatureOptions> = {
    enableInteropTypesFix: false,
    removeReservedKeywordIdentifier: false,
  };
  const featureOptionPath = path.join(testDir, 'options', `${testCase}.json`);
  if (!fs.existsSync(featureOptionPath)) {
    return defaults;
  }
  try {
    const optionContent = fs.readFileSync(featureOptionPath, 'utf-8');
    const parsedOption = JSON.parse(optionContent) as DeclgenFeatureOptions;
    return Object.assign(defaults, parsedOption);
  } catch (error) {
    return `Failed to read feature options from ${featureOptionPath}: ${error}`;
  }
}

/** Runs declgen over the resolved inputs and writes its outputs to `outDir`. */
function runDeclgen(
  inputs: { inputFiles: string[]; rootDir: string | undefined },
  features: Required<DeclgenFeatureOptions>,
  outDir: string,
): DeclgenResult {
  const declgenOptions: DeclgenOptions = {
    outDir: outDir,
    rootDir: inputs.rootDir,
    inputFiles: inputs.inputFiles,
    features,
  };
  // package_source suites resolve relative imports against the package root.
  const compilerOptions: ts.CompilerOptions | undefined = inputs.rootDir ? { baseUrl: inputs.rootDir } : undefined;

  const declgen = new Declgen(declgenOptions, compilerOptions);
  const result = declgen.run();
  result.emit();
  return result;
}

async function test(testDir: string, suite: string, testCase: string, testOutDir: string): Promise<TestResult> {
  const jsonDir = path.join(testDir, 'json_storage');
  const detsDir = path.join(testDir, 'dets_output');

  // Resolve paths
  const expectedReport = path.join(jsonDir, `${testCase}${ts.Extension.Json}`);
  const expectedOutput =
    suite === 'package_source' ? path.join(detsDir, testCase) : path.join(detsDir, `${testCase}${Extension.DETS}`);
  const outDir = path.join(testOutDir, suite, testCase);

  // Build input files
  const inputs = resolveInputFiles(testDir, suite, testCase);
  if (inputs.error) {
    return inputs.error;
  }

  const featureOption = loadFeatureOptions(testDir, testCase);
  if (typeof featureOption === 'string') {
    return createErrorResult(suite, testCase, featureOption);
  }

  // Run Declgen
  const result = runDeclgen(inputs, featureOption, outDir);

  // Compare outputs
  const outputRes = await compareOutputs(suite, testCase, expectedOutput, outDir);
  const reportRes = await compareReports(result.checkResult, expectedReport, outDir, testCase);

  const allPass = outputRes.length === 0 && reportRes.length === 0;

  return {
    suite,
    testCase,
    exitCode: allPass ? ExitCode.PASS : ExitCode.FAIL,
    outputMismatch: outputRes,
    reportMismatch: reportRes,
    message: allPass ? 'Test passed' : 'Test failed with output/report mismatches',
  };
}

async function execute(testDir: string, suite: string, testCase: string, testOutDir: string): Promise<ExecutionResult> {
  initLogger();
  const startTime = performance.now();
  const testResult = await test(testDir, suite, testCase, testOutDir);
  const duration = performance.now() - startTime;
  return { testResult, duration };
}

function initLogger(): void {
  // Init a no-op logger
  Logger.init({
    doDebug: (message: string) => {},
    doInfo: (message: string) => {},
    doWarn: (message: string) => {},
    doError: (message: string) => {},
    doTrace: (message: string) => {},
  });
}

function prettyPrintError(result: ExecutionResult): void {
  console.log('Failed to execute test: ', kleur.red(result.testResult.message));
}

function prettyPrintPass(result: ExecutionResult): void {
  console.log(
    kleur.white().bgGreen('[ PASS ]'),
    `${result.testResult.testCase} ${kleur.dim(`(${result.duration}ms)`)}`,
  );
}

function prettyPrintFail(result: ExecutionResult): void {
  console.log(kleur.white().bgRed('[ FAIL ]'), `${result.testResult.testCase} ${kleur.dim(`(${result.duration}ms)`)}`);
  for (const mismatch of result.testResult.outputMismatch) {
    if (!mismatch.expected.content) {
      console.log(kleur.red(`Expected file not found: ${mismatch.expected.file}`));
    } else if (!mismatch.actual.content) {
      console.log(kleur.red(`Actual file not found: ${mismatch.actual.file}`));
    } else {
      const diffResult = diff(mismatch.actual.content || '', mismatch.expected.content || '', {
        expand: false,
        aAnnotation: `[Expected] ${mismatch.expected.file}`,
        bAnnotation: `[ Actual ] ${mismatch.actual.file}`,
      });
      console.log(diffResult);
    }
  }
}

function prettyPrint(result: ExecutionResult): void {
  console.log(`Test Suite: ${result.testResult.suite}`);
  if (result.testResult.exitCode === ExitCode.ERROR) {
    prettyPrintError(result);
    return;
  }
  if (result.testResult.exitCode === ExitCode.PASS) {
    prettyPrintPass(result);
  } else {
    prettyPrintFail(result);
  }
}

const copyrightHeader = `/*
 * Copyright (c) ${new Date().getFullYear()} Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
`;

async function createJsonStorageFile(testDir: string, testName: string): Promise<void> {
  const jsonDir = path.join(testDir, 'json_storage');
  if (!fs.existsSync(jsonDir)) {
    await fs.promises.mkdir(jsonDir, { recursive: true });
  }
  const filePath = path.join(jsonDir, `${testName}${ts.Extension.Json}`);
  if (fs.existsSync(filePath)) {
    console.log(kleur.yellow(`File already exists: ${filePath}, skipping creation.`));
    return;
  }
  const jsonContent = {
    nodes: [],
  };
  await fs.promises.writeFile(filePath, JSON.stringify(jsonContent, undefined, 4));
  console.log(kleur.green(`Created JSON storage file: ${filePath}`));
}

async function createDetsOutputFile(testDir: string, testName: string): Promise<void> {
  const detsDir = path.join(testDir, 'dets_output');
  if (!fs.existsSync(detsDir)) {
    await fs.promises.mkdir(detsDir, { recursive: true });
  }
  const filePath = path.join(detsDir, `${testName}${Extension.DETS}`);
  if (fs.existsSync(filePath)) {
    console.log(kleur.yellow(`File already exists: ${filePath}, skipping creation.`));
    return;
  }
  const content = `${copyrightHeader}` + `\n\n// TODO: Add expected output here\n`;
  await fs.promises.writeFile(filePath, content);
  console.log(kleur.green(`Created DETS output file: ${filePath}`));
}

async function createTsSourceTestFile(testDir: string, suiteName: string, testName: string): Promise<void> {
  const suitePath = path.join(testDir, suiteName);
  const filePath = path.join(suitePath, `${testName}${ts.Extension.Ts}`);
  if (fs.existsSync(filePath)) {
    console.log(kleur.yellow(`File already exists: ${filePath}, skipping creation.`));
    return;
  }
  const content = `${copyrightHeader}` + `\n\n// TODO: Add test source code here\n`;
  await fs.promises.writeFile(filePath, content);
  console.log(kleur.green(`Created TS source file: ${filePath}`));
}

async function createDtsDeclarationTestFile(testDir: string, suiteName: string, testName: string): Promise<void> {
  const suitePath = path.join(testDir, suiteName);
  const filePath = path.join(suitePath, `${testName}${ts.Extension.Dts}`);
  if (fs.existsSync(filePath)) {
    console.log(kleur.yellow(`File already exists: ${filePath}, skipping creation.`));
    return;
  }
  const content = `${copyrightHeader}` + `\n\n// TODO: Add test source code here\n`;
  await fs.promises.writeFile(filePath, content);
  console.log(kleur.green(`Created DTS declaration file: ${filePath}`));
}

async function createPackageSourceTestFiles(testDir: string, suiteName: string, testName: string): Promise<void> {
  const suitePath = path.join(testDir, suiteName);
  const testCasePath = path.join(suitePath, testName);
  const mainContent = `${copyrightHeader}` + `\n\n// TODO: Add test source code here\n`;
  const mainContentPath = path.join(testCasePath, `main${ts.Extension.Ts}`);
  if (!fs.existsSync(testCasePath)) {
    await fs.promises.mkdir(testCasePath, { recursive: true });
  }
  if (!fs.existsSync(mainContentPath)) {
    await fs.promises.writeFile(mainContentPath, mainContent);
    console.log(kleur.green(`Created package source test file: ${mainContentPath}`));
  } else {
    console.log(kleur.yellow(`File already exists: ${mainContentPath}, skipping creation.`));
  }
  const inputJsonPath = path.join(testCasePath, 'input.json');
  if (!fs.existsSync(inputJsonPath)) {
    await fs.promises.writeFile(inputJsonPath, JSON.stringify([], undefined, 2));
  } else {
    console.log(kleur.yellow(`File already exists: ${inputJsonPath}, skipping creation.`));
  }
  const mainExpectedContent = `${copyrightHeader}` + `\n\n// TODO: Add expected output here\n`;
  const mainExpectedPath = path.join(testCasePath, `main${ts.Extension.Ts}`);
  const detsOutputDir = path.join(testDir, 'dets_output', testName);
  if (!fs.existsSync(detsOutputDir)) {
    await fs.promises.mkdir(detsOutputDir, { recursive: true });
  }
  if (!fs.existsSync(mainExpectedPath)) {
    await fs.promises.writeFile(mainExpectedPath, mainExpectedContent);
    console.log(kleur.green(`Created package source expected output file: ${mainExpectedPath}`));
  } else {
    console.log(kleur.yellow(`File already exists: ${mainExpectedPath}, skipping creation.`));
  }
  console.log(kleur.green(`Created package source test file: ${inputJsonPath}`));
}

async function create(name: string, testDir: string, suites: string[]): Promise<void> {
  // Create structure for each test suite
  for (const suite of suites) {
    // Create template files based on suite type
    console.log(kleur.blue(`Creating test case '${name}' in suite '${suite}'...`));
    if (suite === 'package_source') {
      await createPackageSourceTestFiles(testDir, suite, name);
    } else if (suite === 'ts_source') {
      await createTsSourceTestFile(testDir, suite, name);
    } else if (suite === 'dts_declarations') {
      await createDtsDeclarationTestFile(testDir, suite, name);
    }
  }
  if (suites.includes('dts_declarations') || suites.includes('ts_source')) {
    await createDetsOutputFile(testDir, name);
  }
  await createJsonStorageFile(testDir, name);

  console.log(kleur.green(`Test case '${name}' created successfully in suites: ${suites.join(', ')}`));
}

function registerTestCommand(program: Command): void {
  program
    .command('test <name>')
    .description('Execute a test case')
    .requiredOption('--test-dir <path>', 'Root directory containing test suites')
    .requiredOption('-t, --test-suite <name>', 'Test suite name')
    .option('--test-out-dir <path>', 'Declgen output directory. Defaults to a temp directory')
    .option('--print-json', 'Print the json result to console', false)
    .action(async (testCase: string, options) => {
      if (!testCase) {
        console.error(kleur.red('Error: Test case name is required'));
        process.exit(1);
      }

      const testOutDir: string = options.testOutDir ?? fs.mkdtempSync(path.join(os.tmpdir(), 'declgen-cookbook-'));
      const result = await execute(options.testDir, options.testSuite, testCase, testOutDir);

      if (options.printJson) {
        // Output structured JSON to stdout for the runner to consume
        process.stdout.write(JSON.stringify(result.testResult) + '\n');
      } else {
        prettyPrint(result);
      }
      process.exit(result.testResult.exitCode);
    });
}

function registerCreateCommand(program: Command): void {
  program
    .command('create <name>')
    .description('Create a new test case')
    .requiredOption('--test-dir <path>', 'Root directory containing test suites')
    .option(
      '-t, --test-suite <name>',
      'Test suite name (can be specified multiple times)',
      (value: string, previous: string[] = []) => {
        previous.push(value);
        return previous;
      },
      ['ts_source', 'dts_declarations'] as string[],
    )
    .action(async (name: string, options) => {
      if (!name) {
        console.error(kleur.red('Error: Test case name is required'));
        process.exit(1);
      }
      // Handle multiple -t options
      const suites = Array.isArray(options.testSuite)
        ? options.testSuite
        : options.testSuite
          ? [options.testSuite]
          : ['ts_source', 'dts_declarations'];

      try {
        await create(name, options.testDir, suites);
        process.exit(0);
      } catch (error) {
        console.error(kleur.red(`Error creating test case: ${error}`));
        process.exit(1);
      }
    });
}

async function main(): Promise<void> {
  const program = new Command();

  program.name('cookbook-executor').description('Execute or create cookbook test cases for declgen.');

  registerTestCommand(program);
  registerCreateCommand(program);

  program.parse(process.argv);
}

if (require.main === module) {
  main();
}

export { execute };

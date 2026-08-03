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

import { promises as fs } from 'node:fs';
import { tmpdir } from 'node:os';
import * as path from 'node:path';

import { GlueGenDiagnosticError, GlueGenErrorList, GlueGenInternalError } from '../../src/errors';
import type { ILogger } from '../../src/logger';
import { NativeProcessError, type NativeProcessInvocation } from '../../src/native';
import { GlueGenInvocation } from '../../src/native/invocation';
import { executeNativeProcess } from '../../src/native/process';
import type { BuildConfig } from '../../src/contracts';
import { defaultNativeExecutablePath, GlueGenRunner } from '../../src/pipeline/runner';
import type { ModuleTable } from '../../src/stages/configuration';
import { runConfiguration } from '../helpers/runConfiguration';

type NativeMode =
  | 'success'
  | 'diagnostic-error'
  | 'internal-error'
  | 'inconsistent-success'
  | 'unknown-exit'
  | 'signal'
  | 'invalid-report'
  | 'missing-report'
  | 'stderr-success'
  | 'missing-output';

interface RunnerFixture {
  readonly config: BuildConfig;
  readonly runner: GlueGenRunner;
  readonly moduleTable: ModuleTable;
  readonly logger: jest.Mocked<ILogger>;
  readonly counterPath: string;
  readonly inputFilePath: string;
  readonly nativeExecutablePath: string;
}

interface FixtureOptions {
  readonly emptyInputFileList?: boolean;
}

interface FixturePaths {
  readonly projectRoot: string;
  readonly moduleRoot: string;
  readonly inputFile: string;
  readonly interopConfigPath: string;
  readonly nativeScript: string;
  readonly counterPath: string;
  readonly declgenBridgeConfigPath: string;
}

describe('GlueGenRunner data flow', () => {
  let testRoot: string;

  beforeEach(async () => {
    testRoot = await fs.mkdtemp(path.join(tmpdir(), 'gluegen-runner-'));
  });

  afterEach(async () => {
    jest.restoreAllMocks();
    await fs.rm(testRoot, { recursive: true, force: true });
  });

  it('logs and preserves an expected configuration diagnostic', async () => {
    const logger = fakeLogger();
    const getLogger = jest.fn((_subsystemCode: string): ILogger => logger);
    const config: BuildConfig = {
      plugins: [],
      buildMode: 'Debug',
      buildType: 'BUILD',
      projectRootPath: 'relative-project-root',
      cachePath: path.join(testRoot, 'invalid-config-cache'),
      compileSdkVersion: 10,
      compatibleSdkVersion: 10,
      bundleName: 'com.example.app',
      moduleType: 'entry',
      moduleName: 'entry',
      packageName: 'entry',
      buildSdkPath: 'sdk/build',
      hasMainModule: true,
      modulePath: 'entry',
      externalApiPaths: [],
      byteCodeHar: false,
      interopApiPaths: [],
      declgenBridgeConfigPath: 'declgen-bridge.json',
      interopConfigPath: '',
      dependentModuleList: [],
      getHvigorConsoleLogger: getLogger,
    };
    const runner = new GlueGenRunner(config);

    expect(getLogger).toHaveBeenCalledWith('114');
    await expect(runner.run()).rejects.toBeInstanceOf(GlueGenDiagnosticError);
    expect(logger.printError).toHaveBeenCalledWith(
      expect.objectContaining({
        code: '11420001',
        description: 'Build configuration field "projectRootPath" must be an absolute path.',
        cause: '',
        position: '',
        solutions: [],
      }),
    );
    expect(logger.printError).toHaveBeenCalledTimes(1);
    expect(logger.printErrorAndExit).not.toHaveBeenCalled();
  });

  it('invokes gluegen once with the five CLI options and reads its report', async () => {
    const fixture = await createFixture(testRoot, 'success', 'success');
    const workspaceRoot = path.join(fixture.moduleTable.mainModule.cachePath, 'gluegen');
    const inputFileListPath = path.join(workspaceRoot, 'fileInfo.txt');
    const reportPath = path.join(workspaceRoot, 'report.json');
    const arktsConfigPath = path.join(
      fixture.moduleTable.mainModule.cachePath,
      fixture.moduleTable.mainModule.packageName,
      'arktsconfig.json',
    );
    const outputPath = fixture.config.declgenBridgeConfigPath;

    await fixture.runner.run();

    await expect(fs.readFile(inputFileListPath, 'utf8')).resolves.toBe(fixture.inputFilePath);
    await expect(fs.access(arktsConfigPath)).resolves.toBeUndefined();
    await expect(readJson<Record<string, unknown>>(outputPath)).resolves.toMatchObject({
      inputFileList: inputFileListPath,
      arktsConfig: arktsConfigPath,
      cachePath: workspaceRoot,
      reportPath,
    });
    await expect(readJson<Record<string, unknown>>(reportPath)).resolves.toEqual({
      diagnostics: {
        errors: [],
        warnings: [],
      },
    });
    await expect(fs.readFile(fixture.counterPath, 'utf8')).resolves.toBe('1');
  });

  it('returns successfully without invoking native when the input file list is empty', async () => {
    const fixture = await createFixture(testRoot, 'empty-input-file-list', 'missing-report', {
      emptyInputFileList: true,
    });
    const workspaceRoot = path.join(fixture.moduleTable.mainModule.cachePath, 'gluegen');
    const inputFileListPath = path.join(workspaceRoot, 'fileInfo.txt');
    const reportPath = path.join(workspaceRoot, 'report.json');

    await expect(fixture.runner.run()).resolves.toBeUndefined();

    await expect(fs.readFile(inputFileListPath, 'utf8')).resolves.toBe('');
    await expect(fs.access(fixture.counterPath)).rejects.toMatchObject({ code: 'ENOENT' });
    await expect(fs.access(fixture.config.declgenBridgeConfigPath)).rejects.toMatchObject({
      code: 'ENOENT',
    });
    await expect(fs.access(reportPath)).rejects.toMatchObject({ code: 'ENOENT' });
    expect(fixture.logger.printInfo).not.toHaveBeenCalled();
    expect(fixture.logger.printWarn).not.toHaveBeenCalled();
    expect(fixture.logger.printError).not.toHaveBeenCalled();
  });

  it('maps an exit-code-1 report to the injected logger before rejecting', async () => {
    const fixture = await createFixture(testRoot, 'diagnostic', 'diagnostic-error');

    await expect(fixture.runner.run()).rejects.toBeInstanceOf(GlueGenErrorList);

    expect(fixture.logger.printWarn).toHaveBeenCalledWith('Native analysis warning.');
    expect(fixture.logger.printError).toHaveBeenCalledWith(
      expect.objectContaining({
        code: '11420050',
        description: 'Native generation failed.',
      }),
    );
    expect(fixture.logger.printWarn.mock.invocationCallOrder[0]).toBeLessThan(
      fixture.logger.printError.mock.invocationCallOrder[0] as number,
    );
  });

  it('reads report warnings before classifying exit-code-2 as internal', async () => {
    const fixture = await createFixture(testRoot, 'internal', 'internal-error');

    await expect(fixture.runner.run()).rejects.toBeInstanceOf(GlueGenInternalError);

    expect(fixture.logger.printWarn).toHaveBeenCalledWith('Before internal failure.');
    expect(fixture.logger.printError).toHaveBeenCalledWith(
      expect.objectContaining({
        code: '11420006',
      }),
    );
  });

  it('prints report errors without deriving process failure from them', async () => {
    const fixture = await createFixture(testRoot, 'inconsistent', 'inconsistent-success');

    await fixture.runner.run();

    expect(fixture.logger.printError).toHaveBeenCalledTimes(1);
    expect(fixture.logger.printError).toHaveBeenCalledWith(
      expect.objectContaining({
        code: '11420052',
        description: 'Error reported with successful process exit.',
      }),
    );
  });

  it.each<readonly [string, NativeMode, string | undefined, string | undefined]>([
    ['an unsupported exit code', 'unknown-exit', undefined, 'Before unknown exit.'],
    ['a signal close', 'signal', undefined, 'Before signal.'],
    ['invalid report JSON', 'invalid-report', 'valid report JSON file', undefined],
    ['a missing report JSON', 'missing-report', 'valid report JSON file', undefined],
  ])('classifies %s as an internal error', async (_description, mode, expectedDescription, expectedWarning) => {
    const fixture = await createFixture(testRoot, mode, mode);

    await expect(fixture.runner.run()).rejects.toBeInstanceOf(GlueGenInternalError);
    if (expectedDescription !== undefined) {
      expect(fixture.logger.printError).toHaveBeenCalledWith(
        expect.objectContaining({
          description: expect.stringContaining(expectedDescription),
        }),
      );
    }
    if (expectedWarning !== undefined) {
      expect(fixture.logger.printWarn).toHaveBeenCalledWith(expectedWarning);
    }
  });

  it('rejects exit-code-0 when the final output JSON is missing', async () => {
    const fixture = await createFixture(testRoot, 'missing-output', 'missing-output');

    await expect(fixture.runner.run()).rejects.toBeInstanceOf(GlueGenInternalError);
    expect(fixture.logger.printError).toHaveBeenCalledWith(
      expect.objectContaining({
        description: 'Native gluegen did not produce its output JSON file.',
      }),
    );
  });

  it('does not accept a stale report when native omits the current report', async () => {
    const fixture = await createFixture(testRoot, 'stale-report', 'missing-report');
    const reportPath = path.join(fixture.moduleTable.mainModule.cachePath, 'gluegen', 'report.json');
    await fs.mkdir(path.dirname(reportPath), { recursive: true });
    await fs.writeFile(reportPath, JSON.stringify({ diagnostics: { errors: [], warnings: [] } }), 'utf8');

    await expect(fixture.runner.run()).rejects.toBeInstanceOf(GlueGenInternalError);
    await expect(fs.access(reportPath)).rejects.toMatchObject({ code: 'ENOENT' });
  });

  it('does not present stderr text as a user diagnostic', async () => {
    const fixture = await createFixture(testRoot, 'stderr', 'stderr-success');

    await fixture.runner.run();

    expect(fixture.logger.printInfo).not.toHaveBeenCalled();
    expect(fixture.logger.printWarn).not.toHaveBeenCalled();
    expect(fixture.logger.printError).not.toHaveBeenCalled();
  });

  it('keeps concurrent builds isolated when callers provide separate directories', async () => {
    const first = await createFixture(testRoot, 'first', 'success');
    const second = await createFixture(testRoot, 'second', 'success');

    await Promise.all([first.runner.run(), second.runner.run()]);

    expect(second.config.declgenBridgeConfigPath).not.toBe(first.config.declgenBridgeConfigPath);
    await expect(fs.access(first.config.declgenBridgeConfigPath)).resolves.toBeUndefined();
    await expect(fs.access(second.config.declgenBridgeConfigPath)).resolves.toBeUndefined();
    await expect(fs.readFile(first.counterPath, 'utf8')).resolves.toBe('1');
    await expect(fs.readFile(second.counterPath, 'utf8')).resolves.toBe('1');
  });

  it('resolves the native executable from the wrapper sibling bin directory', () => {
    const executableName = process.platform === 'win32' ? 'gluegen.exe' : 'gluegen';
    expect(defaultNativeExecutablePath()).toBe(path.resolve(__dirname, '..', '..', 'bin', executableName));
  });

  it('checks required command options when building GlueGenInvocation', () => {
    const builder = GlueGenInvocation.builder({
      executable: process.execPath,
      cwd: testRoot,
      environment: process.env,
      signal: undefined,
      timeoutMs: 1000,
      maxOutputBytes: 1024,
    }).output('/tmp/output.json');

    expect(() => builder.build()).toThrow('missing required gluegen command option: --input-file-list');
  });

  it('requires the native report path when building GlueGenInvocation', () => {
    const builder = GlueGenInvocation.builder({
      executable: process.execPath,
      cwd: testRoot,
      environment: process.env,
      signal: undefined,
      timeoutMs: 1000,
      maxOutputBytes: 1024,
    })
      .inputFileList('/fileInfo.txt')
      .arktsConfig('/arktsconfig.json')
      .output('/output.json')
      .cachePath('/cache');

    expect(() => builder.build()).toThrow('missing required gluegen command option: --report-path');
  });

  it('allows GlueGenInvocation command setters in any order', async () => {
    const script = path.join(testRoot, 'print-arguments.cjs');
    await fs.writeFile(
      script,
      '#!/usr/bin/env node\n' +
        "require('node:fs').writeSync(process.stdout.fd, JSON.stringify(process.argv.slice(2)));\n",
      'utf8',
    );
    await fs.chmod(script, 0o755);
    const invocation = GlueGenInvocation.builder({
      executable: script,
      cwd: testRoot,
      environment: process.env,
      signal: undefined,
      timeoutMs: 1000,
      maxOutputBytes: 1024,
    })
      .cachePath('/cache')
      .output('/output.json')
      .arktsConfig('/arktsconfig.json')
      .inputFileList('/fileInfo.txt')
      .reportPath('/cache/report.json')
      .build();

    await expect(invocation.execute()).resolves.toMatchObject({
      exitCode: 0,
      stdout: JSON.stringify([
        '--input-file-list',
        '/fileInfo.txt',
        '--arktsconfig',
        '/arktsconfig.json',
        '--output',
        '/output.json',
        '--cache-path',
        '/cache',
        '--report-path',
        '/cache/report.json',
      ]),
    });
  });

  it('classifies a native process timeout as a process error', async () => {
    const script = path.join(testRoot, 'timeout-direct.cjs');
    await fs.writeFile(script, 'setInterval(() => undefined, 1000);\n', 'utf8');
    const invocation: NativeProcessInvocation = {
      executable: process.execPath,
      arguments: [script],
      cwd: testRoot,
      environment: process.env,
      signal: undefined,
      timeoutMs: 100,
      maxOutputBytes: 1024 * 1024,
    };

    await expect(executeNativeProcess(invocation)).rejects.toMatchObject({
      constructor: NativeProcessError,
      kind: 'timeout',
    });
  });
});

async function createFixture(
  testRoot: string,
  name: string,
  nativeMode: NativeMode,
  options: FixtureOptions = {},
): Promise<RunnerFixture> {
  const paths = fixturePaths(testRoot, name);
  await writeFixtureFiles(paths, nativeMode, options);
  const logger = fakeLogger();
  const config = fixtureBuildConfig(paths, logger, options);
  const { moduleTable } = await runConfiguration(config);
  return {
    config,
    moduleTable,
    logger,
    counterPath: paths.counterPath,
    inputFilePath: paths.inputFile,
    nativeExecutablePath: paths.nativeScript,
    runner: new GlueGenRunner(config, {
      nativeExecutablePath: paths.nativeScript,
    }),
  };
}

function fixturePaths(testRoot: string, name: string): FixturePaths {
  const fixtureRoot = path.join(testRoot, name);
  const projectRoot = path.join(fixtureRoot, 'project');
  const moduleRoot = path.join(projectRoot, 'entry');
  return {
    projectRoot,
    moduleRoot,
    inputFile: path.join(moduleRoot, 'Index.ets'),
    interopConfigPath: path.join(moduleRoot, 'interop.json5'),
    nativeScript: path.join(fixtureRoot, 'fake-native.cjs'),
    counterPath: path.join(fixtureRoot, 'native-count.txt'),
    declgenBridgeConfigPath: path.join(fixtureRoot, 'output', 'declgen-bridge.json'),
  };
}

async function writeFixtureFiles(paths: FixturePaths, nativeMode: NativeMode, options: FixtureOptions): Promise<void> {
  await fs.mkdir(paths.moduleRoot, { recursive: true });
  await fs.writeFile(paths.inputFile, 'export function entry(): void {}\n', 'utf8');
  if (!options.emptyInputFileList) {
    await fs.writeFile(paths.interopConfigPath, `{ interopEntries: { static: ['Index.ets'] } }\n`, 'utf8');
  }
  await fs.writeFile(paths.nativeScript, fakeNativeSource(nativeMode, paths.counterPath, paths.inputFile), 'utf8');
  await fs.chmod(paths.nativeScript, 0o755);
}

function fixtureBuildConfig(paths: FixturePaths, logger: jest.Mocked<ILogger>, options: FixtureOptions): BuildConfig {
  const config: BuildConfig = {
    plugins: [],
    buildMode: 'Debug',
    buildType: 'BUILD',
    projectRootPath: paths.projectRoot,
    cachePath: 'cache',
    compileSdkVersion: 10,
    compatibleSdkVersion: 10,
    bundleName: 'com.example.app',
    moduleType: 'entry',
    moduleName: 'entry',
    packageName: 'entry',
    buildSdkPath: 'sdk/build',
    hasMainModule: true,
    modulePath: paths.moduleRoot,
    externalApiPaths: [],
    byteCodeHar: false,
    interopApiPaths: [],
    declgenBridgeConfigPath: paths.declgenBridgeConfigPath,
    interopConfigPath: '',
    dependentModuleList: [
      {
        packageName: 'entry',
        moduleName: 'entry',
        moduleType: 'entry',
        modulePath: paths.moduleRoot,
        sourceRoots: ['.'],
        entryFile: 'Index.ets',
        interopConfigPath: options.emptyInputFileList ? '' : 'interop.json5',
      },
    ],
    getHvigorConsoleLogger: (_subsystemCode: string): ILogger => logger,
  };
  return config;
}

function fakeNativeSource(mode: NativeMode, counterPath: string, inputFile: string): string {
  return [
    fakeNativeSetupSource(mode, counterPath),
    fakeNativeOutputSource(),
    fakeNativeFailureSource(inputFile),
    fakeNativeFallbackSource(),
  ].join('\n');
}

function fakeNativeSetupSource(mode: NativeMode, counterPath: string): string {
  return `#!/usr/bin/env node
'use strict';
const fs = require('node:fs');
const mode = ${JSON.stringify(mode)};
const counterPath = ${JSON.stringify(counterPath)};
const currentCount = fs.existsSync(counterPath) ? Number(fs.readFileSync(counterPath, 'utf8')) : 0;
fs.writeFileSync(counterPath, String(currentCount + 1));

const option = (name) => {
    const index = process.argv.indexOf(name);
    if (index < 0 || process.argv[index + 1] === undefined) {
        throw new Error('missing command option: ' + name);
    }
    return process.argv[index + 1];
};
const inputFileList = option('--input-file-list');
const arktsConfig = option('--arktsconfig');
const output = option('--output');
const cachePath = option('--cache-path');
const reportPath = option('--report-path');
if (process.argv.includes('-i')) {
    throw new Error('wrapper must use the long input-file-list option');
}
if (!fs.existsSync(inputFileList) || !fs.existsSync(arktsConfig) || !fs.existsSync(cachePath)) {
    throw new Error('native inputs must exist before invocation');
}
if (reportPath !== require('node:path').join(cachePath, 'report.json')) {
    throw new Error('report path must be cache-path/report.json');
}
`;
}

function fakeNativeOutputSource(): string {
  return `const writeReport = (errors = [], warnings = []) => {
    fs.writeFileSync(reportPath, JSON.stringify({ diagnostics: { errors, warnings } }));
};
const writeOutput = () => {
    if (mode === 'missing-output') {
        return;
    }
    fs.writeFileSync(output, JSON.stringify({
        inputFileList,
        arktsConfig,
        cachePath,
        reportPath,
    }));
};
`;
}

function fakeNativeFailureSource(inputFile: string): string {
  return `if (mode === 'diagnostic-error') {
    process.stderr.write('native implementation detail');
    writeReport(
        [{
            code: '11420050',
            description: 'Native generation failed.',
            cause: 'Unsupported interop declaration.',
            position: ${JSON.stringify(inputFile)},
            solutions: ['Update the declaration.'],
            moreInfo: { phase: 'generation' },
        }],
        [{ code: '11420049', description: 'Native analysis warning.' }],
    );
    process.exitCode = 1;
} else if (mode === 'internal-error') {
    writeReport([], [{ code: '11420051', description: 'Before internal failure.' }]);
    process.exitCode = 2;
} else if (mode === 'inconsistent-success') {
    writeOutput();
    writeReport([{
        code: '11420052',
        description: 'Error reported with successful process exit.',
    }]);
} else if (mode === 'unknown-exit') {
    writeReport([], [{ code: '11420053', description: 'Before unknown exit.' }]);
    process.exitCode = 7;
} else if (mode === 'signal') {
    writeReport([], [{ code: '11420054', description: 'Before signal.' }]);
    process.kill(process.pid, 'SIGTERM');
}`;
}

function fakeNativeFallbackSource(): string {
  return `else {
    writeOutput();
    if (mode === 'stderr-success') {
        process.stderr.write('this is not a user diagnostic');
    }
    if (mode === 'invalid-report') {
        fs.writeFileSync(reportPath, '{invalid-json');
    } else if (mode !== 'missing-report') {
        writeReport();
    }
}
`;
}

function fakeLogger(): jest.Mocked<ILogger> {
  return {
    printInfo: jest.fn(),
    printWarn: jest.fn(),
    printDebug: jest.fn(),
    printError: jest.fn(),
    printErrorAndExit: jest.fn(),
  };
}

async function readJson<T>(filePath: string): Promise<T> {
  return JSON.parse(await fs.readFile(filePath, 'utf8')) as T;
}

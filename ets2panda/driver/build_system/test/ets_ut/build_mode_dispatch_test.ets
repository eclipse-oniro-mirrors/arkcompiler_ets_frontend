import { BuildMode } from '../../ets_src/build/build_mode';
import { BaseMode } from '../../ets_src/build/base_mode';
import { BuildConfig, ES2PANDA_MODE, OHOS_MODULE_TYPE } from '../../ets_src/types';
import { DriverError, ErrorCode } from '../../ets_src/util/error';

function assertTrue(value: boolean, message: string): void {
  if (!value) {
    throw new Error(message);
  }
}

function assertEquals(actual: string, expected: string): void {
  if (actual !== expected) {
    throw new Error('Expected ' + expected + ', got ' + actual);
  }
}

class RecordingBaseMode extends BaseMode {
  public runCount: int = 0;

  public constructor(buildConfig: BuildConfig) {
    super(buildConfig);
  }

  public run(): void {
    this.runCount++;
  }
}

function createBuildConfig(): BuildConfig {
  return {
    packageName: 'entry',
    moduleType: OHOS_MODULE_TYPE.SHARED,
    moduleRootPath: '/demo/entry',
    sourceRoots: ['./'],
    compileFiles: ['/demo/entry/a.ets'],
    loaderOutPath: '/demo/dist',
    cachePath: '/demo/dist/cache',
    buildSdkPath: '/demo/sdk',
    es2pandaMode: ES2PANDA_MODE.RUN,
    enableDeclgenEts2Ts: false,
    dependencyModuleList: [],
  };
}

function expectRejection(buildConfig: BuildConfig, expectedCause: string): void {
  let baseMode: RecordingBaseMode = new RecordingBaseMode(buildConfig);
  try {
    new BuildMode(buildConfig, baseMode).run();
  } catch (error) {
    let driverError: DriverError = error as DriverError;
    assertEquals(driverError.logData.code, ErrorCode.BUILDSYSTEM_UNSUPPORTED_FEATURE);
    assertEquals(driverError.logData.cause, expectedCause);
    assertEquals(baseMode.runCount.toString(), '0');
    return;
  }
  throw new Error('Expected BuildMode rejection ' + expectedCause);
}

function testSequentialAdmittedOnce(): void {
  let buildConfig: BuildConfig = createBuildConfig();
  let baseMode: RecordingBaseMode = new RecordingBaseMode(buildConfig);
  new BuildMode(buildConfig, baseMode).run();
  assertEquals(baseMode.runCount.toString(), '1');
}

function testUnsupportedModes(): void {
  let parallelConfig: BuildConfig = createBuildConfig();
  parallelConfig.es2pandaMode = ES2PANDA_MODE.RUN_PARALLEL;
  expectRejection(parallelConfig, 'unsupported-build-mode');
  let concurrentConfig: BuildConfig = createBuildConfig();
  concurrentConfig.es2pandaMode = ES2PANDA_MODE.RUN_CONCURRENT;
  expectRejection(concurrentConfig, 'unsupported-build-mode');
  let simultaneousConfig: BuildConfig = createBuildConfig();
  simultaneousConfig.es2pandaMode = ES2PANDA_MODE.RUN_SIMULTANEOUS;
  expectRejection(simultaneousConfig, 'unsupported-build-mode');
  let processWorkerConfig: BuildConfig = createBuildConfig();
  processWorkerConfig.useProcessWorker = true;
  expectRejection(processWorkerConfig, 'unsupported-build-mode');
  let threadWorkerConfig: BuildConfig = createBuildConfig();
  threadWorkerConfig.useThreadWorker = true;
  expectRejection(threadWorkerConfig, 'unsupported-build-mode');
  let externalConfig: BuildConfig = createBuildConfig();
  externalConfig.externalProjectPath = '/demo/external';
  expectRejection(externalConfig, 'unsupported-build-mode');
}

function testUnsupportedFeatures(): void {
  let obfuscationConfig: BuildConfig = createBuildConfig();
  obfuscationConfig.obfuscationOptions = {};
  expectRejection(obfuscationConfig, 'unsupported-feature-obfuscation');
  let declgenConfig: BuildConfig = createBuildConfig();
  declgenConfig.enableDeclgenEts2Ts = true;
  expectRejection(declgenConfig, 'unsupported-feature-declgen-v1');
}

function main(): void {
  testSequentialAdmittedOnce();
  testUnsupportedModes();
  testUnsupportedFeatures();
  assertTrue(true, 'build mode tests completed');
  console.log('build-mode-dispatch-ok');
}

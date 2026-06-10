import { initBuildConfig } from '../../ets_src/init/process_build_config';
import { BuildConfig, DependencyModuleConfig, ES2PANDA_MODE } from '../../ets_src/types';
import { DriverError, ErrorCode } from '../../ets_src/util/error';
import { ensureNonEmptyBinaryFile, readTextFile } from '../../ets_src/util/interop_helper';
import { isAbsolutePath } from '../../ets_src/util/utils';
import { assertEquals, assertStringContains, assertTrue } from './assertions';

function findDependency(buildConfig: BuildConfig, packageName: string): DependencyModuleConfig {
  let deps: DependencyModuleConfig[] = buildConfig.dependencyModuleList as DependencyModuleConfig[];
  for (let i: int = 0; i < deps.length; i++) {
    if (deps[i].packageName === packageName) {
      return deps[i];
    }
  }
  throw new Error('Dependency ' + packageName + ' missing');
}

function expandRootPlaceholder(text: string, root: string): string {
  let result: string = text;
  while (result.indexOf('${BUILD_SYSTEM_ROOT}') >= 0) {
    result = result.replace('${BUILD_SYSTEM_ROOT}', root);
  }
  return result;
}

function readGoldenText(root: string): string {
  let text: string = readTextFile(root + '/test/ets_ut/golden/demo_hap_resolved_build_config.json');
  return expandRootPlaceholder(text, root);
}

function assertGoldenResolvedConfig(buildConfig: BuildConfig, root: string): void {
  let golden: string = readGoldenText(root);
  assertStringContains(golden, '"buildSdkPath": "' + root + '/test/mock_sdk"');
  assertStringContains(golden, '"modulePath": "' + root + '/test/demo_hap/harA"');
  assertStringContains(golden, '"modulePath": "' + root + '/test/demo_hap/harB"');
  assertEquals(buildConfig.buildSdkPath as string, root + '/test/mock_sdk');
  assertTrue(isAbsolutePath(buildConfig.buildSdkPath as string), 'buildSdkPath must be absolute');
  assertTrue(isAbsolutePath(buildConfig.pandaSdkPath as string), 'pandaSdkPath must be absolute');
  assertTrue(isAbsolutePath(buildConfig.moduleRootPath as string), 'moduleRootPath must be absolute');
  let harA: DependencyModuleConfig = findDependency(buildConfig, 'harA');
  let harB: DependencyModuleConfig = findDependency(buildConfig, 'harB');
  assertEquals(harA.modulePath, root + '/test/demo_hap/harA');
  assertEquals(harB.modulePath, root + '/test/demo_hap/harB');
  assertTrue(isAbsolutePath(harA.modulePath), 'harA modulePath must be absolute');
  assertTrue(isAbsolutePath(harB.modulePath), 'harB modulePath must be absolute');
  assertEquals(harA.language, '1.2');
  assertEquals(harB.language, '1.2');
  assertEquals((harA.dependencies as string[]).join(','), 'harB');
  assertTrue(buildConfig.aliasConfig !== undefined, 'aliasConfig must be initialized');
  assertTrue(buildConfig.sdkAliasMap !== undefined, 'sdkAliasMap must be initialized');
  assertTrue(buildConfig.interopSDKPaths !== undefined, 'interopSDKPaths must be initialized');
  assertTrue((buildConfig.nativeLibraryPath as string).length > 0, 'nativeLibraryPath must be recorded');
  console.log('resolved demo paths');
  console.log('golden resolved build config comparison success');
}

function testUnsupportedMode(root: string): void {
  let configPath: string = root + '/test/ets_ut/unsupported_mode_build_config.json';
  let buildConfig: BuildConfig = initBuildConfig(configPath);
  if (buildConfig.es2pandaMode === ES2PANDA_MODE.RUN) {
    throw new Error('unsupported mode fixture was not used');
  }
}

function expectDriverError(action: () => void, code: string, cause: string): void {
  try {
    action();
  } catch (error) {
    let driverError: DriverError = error as DriverError;
    assertEquals(driverError.logData.code, code);
    assertEquals(driverError.logData.cause, cause);
    return;
  }
  throw new Error('Expected DriverError ' + code);
}

function testMissingBuildConfig(root: string): void {
  expectDriverError((): void => {
    initBuildConfig(root + '/test/ets_ut/missing_build_config.json');
  }, ErrorCode.BUILDSYSTEM_MISSING_BUILD_CONFIG, 'missing-build-config');
}

function testMissingModulePath(root: string): void {
  expectDriverError((): void => {
    initBuildConfig(root + '/test/ets_ut/missing_module_build_config.json');
  }, ErrorCode.BUILDSYSTEM_MISSING_MODULE_PATH, 'missing-module-path');
}

function testMissingSdkStubPath(root: string): void {
  expectDriverError((): void => {
    initBuildConfig(root + '/test/ets_ut/missing_sdk_build_config.json');
  }, ErrorCode.BUILDSYSTEM_SDK_NOT_EXIST_FAIL, 'missing-sdk-stub-path');
}

function testBinarySizeReasons(root: string): void {
  expectDriverError((): void => {
    ensureNonEmptyBinaryFile(root + '/test/ets_ut/missing_output.abc', 'missing-output-abc');
  }, ErrorCode.BUILDSYSTEM_MISSING_OUTPUT_ABC, 'missing-output-abc');
  expectDriverError((): void => {
    ensureNonEmptyBinaryFile(root + '/test/ets_ut/empty_output.abc', 'empty-output-abc');
  }, ErrorCode.BUILDSYSTEM_EMPTY_OUTPUT_ABC, 'empty-output-abc');
}

function main(): void {
  let root: string = '/home/huawei/w/PANDA/src/ets_frontend/ets2panda/driver/build_system';
  let buildConfig: BuildConfig = initBuildConfig(root + '/test/demo_hap/build_config.json');
  assertGoldenResolvedConfig(buildConfig, root);
  assertStringContains(buildConfig.moduleRootPath as string, '/test/demo_hap/entry');
  testUnsupportedMode(root);
  testMissingBuildConfig(root);
  testMissingModulePath(root);
  testMissingSdkStubPath(root);
  testBinarySizeReasons(root);
  console.log('process-build-config-ok');
}

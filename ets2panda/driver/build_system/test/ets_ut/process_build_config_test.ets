import { initBuildConfig } from '../../ets_src/init/process_build_config';
import { BuildConfig, DependencyModuleConfig, ES2PANDA_MODE } from '../../ets_src/types';
import { DriverError, ErrorCode } from '../../ets_src/util/error';
import { isAbsolutePath, resolvePath } from '../../ets_src/util/utils';

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

function findDependency(buildConfig: BuildConfig, packageName: string): DependencyModuleConfig {
  let deps: DependencyModuleConfig[] = buildConfig.dependencyModuleList as DependencyModuleConfig[];
  for (let i: int = 0; i < deps.length; i++) {
    if (deps[i].packageName === packageName) {
      return deps[i];
    }
  }
  throw new Error('Dependency ' + packageName + ' missing');
}

function assertDemoConfig(buildConfig: BuildConfig, root: string): void {
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
}

function testUnsupportedMode(): void {
  let root: string = '/home/huawei/w/PANDA/src/ets_frontend/ets2panda/driver/build_system';
  let configPath: string = root + '/test/ets_ut/unsupported_mode_build_config.json';
  let buildConfig: BuildConfig = initBuildConfig(configPath);
  if (buildConfig.es2pandaMode === ES2PANDA_MODE.RUN) {
    throw new Error('unsupported mode fixture was not used');
  }
}

function main(): void {
  let root: string = '/home/huawei/w/PANDA/src/ets_frontend/ets2panda/driver/build_system';
  let buildConfig: BuildConfig = initBuildConfig(root + '/test/demo_hap/build_config.json');
  assertDemoConfig(buildConfig, root);
  try {
    testUnsupportedMode();
  } catch (error) {
    let driverError: DriverError = error as DriverError;
    assertEquals(driverError.logData.code, ErrorCode.BUILDSYSTEM_UNSUPPORTED_FEATURE);
    console.log('config-resolution-ok');
    return;
  }
  throw new Error('Expected unsupported mode DriverError');
}

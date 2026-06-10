import { BuildConfig, BUILD_MODE, BUILD_TYPE, DependencyModuleConfig, ES2PANDA_MODE, OHOS_MODULE_TYPE } from '../../ets_src/types';
import { assertEquals, assertTrue } from './assertions';

function createDependency(root: string): DependencyModuleConfig {
  return {
    packageName: 'harB',
    moduleName: 'harB',
    moduleType: OHOS_MODULE_TYPE.HAR,
    modulePath: root + '/test/demo_hap/harB',
    sourceRoots: ['./'],
    entryFile: 'index.ets',
    language: '1.2',
  };
}

function main(): void {
  let root: string = '/home/huawei/w/PANDA/src/ets_frontend/ets2panda/driver/build_system';
  let dependency: DependencyModuleConfig = createDependency(root);
  let buildConfig: BuildConfig = {
    packageName: 'entry',
    moduleType: OHOS_MODULE_TYPE.SHARED,
    buildType: BUILD_TYPE.BUILD,
    buildMode: BUILD_MODE.DEBUG,
    es2pandaMode: ES2PANDA_MODE.RUN,
    moduleRootPath: root + '/test/demo_hap/entry',
    sourceRoots: ['./', 'src/main1/ets'],
    entryFile: 'a.ets',
    compileFiles: [root + '/test/demo_hap/entry/a.ets'],
    buildSdkPath: root + '/test/mock_sdk',
    dependencyModuleList: [dependency],
  };
  assertEquals(buildConfig.packageName, 'entry');
  assertEquals((buildConfig.dependencyModuleList as DependencyModuleConfig[])[0].packageName, 'harB');
  assertEquals((buildConfig.dependencyModuleList as DependencyModuleConfig[])[0].modulePath, root + '/test/demo_hap/harB');
  assertTrue((buildConfig.sourceRoots as string[]).length === 2, 'source roots');
  console.log(buildConfig.packageName);
  console.log('types-config-ok');
}

import { ArkTSConfig, ArkTSConfigGenerator } from '../../ets_src/build/generate_arktsconfig';
import { AliasConfig, BuildConfig, DependencyItem, ModuleInfo, OHOS_MODULE_TYPE } from '../../ets_src/types';
import { LANGUAGE_VERSION } from '../../ets_src/pre_define';
import { readTextFile } from '../../ets_src/util/interop_helper';
import { assertEquals, assertStringContains, assertTrue } from './assertions';

function assertArrayEquals(actual: string[] | undefined, expected: string[], message: string): void {
  if (actual === undefined) {
    throw new Error(message + ': expected array, got undefined');
  }
  assertEquals((actual as string[]).join('|'), expected.join('|'));
}

function getDependencyAlias(config: ArkTSConfig, name: string): string[] | undefined {
  let item: DependencyItem | undefined = config.dependencies.get(name);
  if (item === undefined) {
    throw new Error('Dependency ' + name + ' missing');
  }
  return item.alias;
}

function createModuleInfo(packageName: string, moduleRootPath: string, staticFiles: string[], dependencies: string[] = []): ModuleInfo {
  return {
    isMainModule: packageName === 'entry',
    packageName: packageName,
    moduleRootPath: moduleRootPath,
    moduleType: packageName === 'entry' ? OHOS_MODULE_TYPE.ENTRY : OHOS_MODULE_TYPE.HAR,
    sourceRoots: packageName === 'entry' ? ['./', 'src/main1/ets'] : ['./'],
    entryFile: packageName === 'entry' ? 'a.ets' : 'index.ets',
    arktsConfigFile: moduleRootPath + '/arktsconfig.json',
    dependencies: dependencies,
    staticDependencyModules: new Map<string, ModuleInfo>(),
    dynamicDependencyModules: new Map<string, ModuleInfo>(),
    language: LANGUAGE_VERSION.ARKTS_1_2,
    staticFiles: staticFiles,
    moduleName: packageName,
  };
}

function createBuildConfig(root: string): BuildConfig {
  return {
    packageName: 'entry',
    moduleType: OHOS_MODULE_TYPE.SHARED,
    moduleRootPath: root + '/test/demo_hap/entry',
    sourceRoots: ['./', 'src/main1/ets'],
    compileFiles: [
      root + '/test/demo_hap/entry/a.ets',
      root + '/test/demo_hap/entry/b.ets',
      root + '/test/demo_hap/entry/c.ets',
      root + '/test/demo_hap/entry/d.ets',
      root + '/test/demo_hap/harA/index.ets',
      root + '/test/demo_hap/harB/index.ets',
    ],
    buildSdkPath: root + '/test/mock_sdk',
    pandaSdkPath: '/home/huawei/w/PANDA/out/debug',
    pandaStdlibPath: '/home/huawei/w/PANDA/out/debug/plugins/ets/etsstdlib.abc',
    cachePath: root + '/test/demo_hap/entry/dist/cache',
    projectRootPath: root + '/test/demo_hap',
    externalApiPaths: [],
    interopSDKPaths: new Set<string>(),
    paths: new Map<string, string[]>(),
    aliasConfig: new Map<string, Map<string, AliasConfig>>(),
  };
}

function expandRootPlaceholder(text: string, root: string): string {
  let result: string = text;
  while (result.indexOf('${BUILD_SYSTEM_ROOT}') >= 0) {
    result = result.replace('${BUILD_SYSTEM_ROOT}', root);
  }
  return result;
}

function readGoldenText(root: string): string {
  let text: string = readTextFile(root + '/test/ets_ut/golden/demo_hap_arktsconfig.json');
  return expandRootPlaceholder(text, root);
}

function assertGoldenEntryConfig(config: ArkTSConfig, root: string): void {
  let golden: string = readGoldenText(root);
  assertStringContains(golden, '"package": "entry"');
  assertStringContains(golden, '"baseUrl": "' + root + '/test/demo_hap/entry"');
  assertStringContains(golden, '"rootDir": "' + root + '/test/demo_hap"');
  assertEquals(config.packageName, 'entry');
  assertEquals(config.compilerOptions.baseUrl, root + '/test/demo_hap/entry');
  assertEquals(config.compilerOptions.rootDir, root + '/test/demo_hap');
  assertArrayEquals(config.pathSection.get(root + '/test/mock_sdk/api'), [root + '/test/mock_sdk/api'], 'golden sdk api path');
  assertArrayEquals(config.pathSection.get(root + '/test/mock_sdk/arkts'), [root + '/test/mock_sdk/arkts'], 'golden sdk arkts path');
  assertArrayEquals(config.pathSection.get(root + '/test/mock_sdk/kits'), [root + '/test/mock_sdk/kits'], 'golden sdk kits path');
  assertArrayEquals(config.pathSection.get('entry'), [root + '/test/demo_hap/entry/src/main1/ets', root + '/test/demo_hap/entry'], 'golden entry path');
  assertArrayEquals(config.pathSection.get('entry/a'), [root + '/test/demo_hap/entry/a.ets'], 'golden entry a path');
  assertArrayEquals(config.pathSection.get('entry/b'), [root + '/test/demo_hap/entry/b.ets'], 'golden entry b path');
  assertArrayEquals(config.pathSection.get('entry/c'), [root + '/test/demo_hap/entry/c.ets'], 'golden entry c path');
  assertArrayEquals(config.pathSection.get('entry/d'), [root + '/test/demo_hap/entry/d.ets'], 'golden entry d path');
  assertArrayEquals(config.configObject.files, [root + '/test/demo_hap/entry/a.ets', root + '/test/demo_hap/entry/b.ets', root + '/test/demo_hap/entry/c.ets', root + '/test/demo_hap/entry/d.ets'], 'golden files');
  assertStringContains(config.toJSONText(), '"package":"entry"');
  console.log('golden arktsconfig comparison success');
}

function testDemoSnapshot(): void {
  let root: string = '/home/huawei/w/PANDA/src/ets_frontend/ets2panda/driver/build_system';
  let harB: ModuleInfo = createModuleInfo('harB', root + '/test/demo_hap/harB', [root + '/test/demo_hap/harB/index.ets']);
  let harA: ModuleInfo = createModuleInfo('harA', root + '/test/demo_hap/harA', [root + '/test/demo_hap/harA/index.ets'], ['harB']);
  let entry: ModuleInfo = createModuleInfo('entry', root + '/test/demo_hap/entry', [
    root + '/test/demo_hap/entry/a.ets',
    root + '/test/demo_hap/entry/b.ets',
    root + '/test/demo_hap/entry/c.ets',
    root + '/test/demo_hap/entry/d.ets',
  ], ['harA']);
  harA.staticDependencyModules.set('harB', harB);
  entry.staticDependencyModules.set('harA', harA);

  let generator: ArkTSConfigGenerator = new ArkTSConfigGenerator(createBuildConfig(root));
  let harBConfig: ArkTSConfig = generator.generateConfigForModule(harB);
  let harAConfig: ArkTSConfig = generator.generateConfigForModule(harA);
  let entryConfig: ArkTSConfig = generator.generateConfigForModule(entry);

  assertEquals(harBConfig.packageName, 'harB');
  assertEquals(harAConfig.packageName, 'harA');
  assertTrue(entryConfig.dependencies.has('std/core'), 'entry std/core dependency missing');
  assertTrue(harAConfig.dependencies.has('std/core'), 'harA std/core dependency missing');
  assertTrue(harBConfig.dependencies.has('std/core'), 'harB std/core dependency missing');
  assertGoldenEntryConfig(entryConfig, root);
}

function testIdempotentMerge(): void {
  let moduleInfo: ModuleInfo = createModuleInfo('pkg', '/pkg', ['/pkg/index.ets']);
  let config: ArkTSConfig = new ArkTSConfig(moduleInfo, '/cache', '/project', '');
  config.addPathMapping('alias', ['/a', '/b']);
  config.addPathMapping('alias', ['/b', '/c']);
  assertArrayEquals(config.pathSection.get('alias'), ['/a', '/b', '/c'], 'idempotent paths');
  config.addDependency('dep', { language: 'js', path: '/dep.d.ets', ohmUrl: 'dep', alias: ['a'] });
  config.addDependency('dep', { language: 'js', path: '/dep.d.ets', ohmUrl: 'dep', alias: ['a', 'b'] });
  assertArrayEquals(getDependencyAlias(config, 'dep'), ['a', 'b'], 'idempotent aliases');
}

function main(): void {
  testDemoSnapshot();
  testIdempotentMerge();
  console.log('generate-arktsconfig-ok');
}

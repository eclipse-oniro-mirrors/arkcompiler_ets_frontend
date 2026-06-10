import { BaseMode, CompileRequest, SequentialCompileUnit } from '../../ets_src/build/base_mode';
import { BuildConfig, DependencyModuleConfig, ModuleInfo, OHOS_MODULE_TYPE } from '../../ets_src/types';
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

function createDependency(packageName: string, root: string, dependencies: string[] = []): DependencyModuleConfig {
  let dependency: DependencyModuleConfig = {
    packageName: packageName,
    moduleName: packageName,
    moduleType: OHOS_MODULE_TYPE.HAR,
    modulePath: root + '/test/demo_hap/' + packageName,
    sourceRoots: ['./'],
    entryFile: 'index.ets',
    language: '1.2',
  };
  if (dependencies.length > 0) {
    dependency.dependencies = dependencies;
  }
  return dependency;
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
    loaderOutPath: root + '/test/demo_hap/dist',
    cachePath: root + '/test/demo_hap/dist/cache',
    buildSdkPath: root + '/test/mock_sdk',
    dependencyModuleList: [
      createDependency('harA', root, ['harB']),
      createDependency('harB', root),
    ],
  };
}

function createModuleInfo(name: string): ModuleInfo {
  return {
    isMainModule: name === 'entry',
    packageName: name,
    moduleRootPath: '/demo/' + name,
    moduleType: name === 'entry' ? OHOS_MODULE_TYPE.ENTRY : OHOS_MODULE_TYPE.HAR,
    sourceRoots: ['./'],
    entryFile: 'index.ets',
    arktsConfigFile: '',
    dependencies: [],
    staticDependencyModules: new Map<string, ModuleInfo>(),
    dynamicDependencyModules: new Map<string, ModuleInfo>(),
    staticFiles: ['/demo/' + name + '/index.ets'],
  };
}

function createUnit(name: string, dependencies: string[] = []): SequentialCompileUnit {
  return {
    moduleName: name,
    modulePath: '/demo/' + name,
    sourceFiles: ['/demo/' + name + '/index.ets'],
    outputKind: name === 'entry' ? 'entry' : 'module',
    outputPaths: ['/demo/dist/' + name + '.abc'],
    moduleOutputPath: '/demo/dist/' + name + '.abc',
    compilerConfigText: '{"compilerOptions":{"package":"' + name + '"},"files":[]}',
    dependencyNames: dependencies,
    moduleInfo: createModuleInfo(name),
  };
}

function expectGraphError(action: () => void, causeContains: string): void {
  try {
    action();
  } catch (error) {
    let driverError: DriverError = error as DriverError;
    assertEquals(driverError.logData.code, ErrorCode.BUILDSYSTEM_GRAPH_ERROR);
    assertTrue(driverError.logData.cause.indexOf(causeContains) >= 0 || driverError.logData.description.indexOf(causeContains) >= 0, 'graph error cause must mention ' + causeContains);
    return;
  }
  throw new Error('Expected graph DriverError');
}

function testCompileUnits(): void {
  let root: string = '/home/huawei/w/PANDA/src/ets_frontend/ets2panda/driver/build_system';
  let mode: BaseMode = new BaseMode(createBuildConfig(root));
  let units: SequentialCompileUnit[] = mode.collectCompileUnits();
  assertEquals(units.length.toString(), '3');
  assertEquals(units[0].moduleName, 'harB');
  assertEquals(units[1].moduleName, 'harA');
  assertEquals(units[2].moduleName, 'entry');
  assertEquals(units[0].sourceFiles.join(','), root + '/test/demo_hap/harB/index.ets');
  assertEquals(units[1].sourceFiles.join(','), root + '/test/demo_hap/harA/index.ets');
  assertEquals(units[2].sourceFiles.join(','), root + '/test/demo_hap/entry/d.ets,' + root + '/test/demo_hap/entry/c.ets,' + root + '/test/demo_hap/entry/b.ets,' + root + '/test/demo_hap/entry/a.ets');
  assertEquals(units[0].moduleOutputPath, root + '/test/demo_hap/dist/harB.abc');
  assertEquals(units[1].moduleOutputPath, root + '/test/demo_hap/dist/harA.abc');
  assertEquals(units[2].moduleOutputPath, root + '/test/demo_hap/dist/entry.abc');
  mode.validateGraph(units);
  units[2].compilerConfigText = '{"compilerOptions":{"package":"entry"},"files":[]}';
  let request: CompileRequest = mode.makeCompileRequest(units[2]);
  assertEquals(request.unitName, 'entry');
  assertEquals(request.sourceFiles.join(','), units[2].sourceFiles.join(','));
  assertEquals(request.outputPaths.join(','), root + '/test/demo_hap/dist/entry.abc');
  assertEquals(request.moduleOutputPath, root + '/test/demo_hap/dist/entry.abc');
  assertTrue(request.compilerConfigText.indexOf('entry') >= 0, 'request config text');
}

function testGraphFailures(): void {
  let mode: BaseMode = new BaseMode(createBuildConfig('/demo'));
  expectGraphError((): void => {
    mode.validateGraph([createUnit('entry', ['missing'])]);
  }, 'Unresolved dependency');
  expectGraphError((): void => {
    mode.validateGraph([createUnit('a', ['b']), createUnit('b', ['a'])]);
  }, 'Cyclic');
}

function testRequestFailures(): void {
  let mode: BaseMode = new BaseMode(createBuildConfig('/demo'));
  let unit: SequentialCompileUnit = createUnit('entry');
  unit.compilerConfigText = '';
  try {
    mode.makeCompileRequest(unit);
  } catch (error) {
    let driverError: DriverError = error as DriverError;
    assertEquals(driverError.logData.code, ErrorCode.BUILDSYSTEM_INVALID_CONFIG);
    return;
  }
  throw new Error('Expected config DriverError');
}

function main(): void {
  testCompileUnits();
  testGraphFailures();
  testRequestFailures();
  console.log('base-mode-sequential-ok');
}

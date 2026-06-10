import { Es2pandaContextState } from '@arkts-bindings';
import { CompileUnitRequest, Ets2panda, NativeCompilerConfigHandle, NativeCompilerConfigRequest, NativeCompilerContextHandle } from '../../ets_src/util/ets2panda';

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

function testModels(): void {
  let configHandle: NativeCompilerConfigHandle = new NativeCompilerConfigHandle(12 as long);
  let contextHandle: NativeCompilerContextHandle = new NativeCompilerContextHandle(34 as long);
  let configRequest: NativeCompilerConfigRequest = {
    argv: ['_', '--extension', 'ets'],
    configText: '{"compilerOptions":{"package":"harB"},"files":[]}',
  };
  let sourceFiles: string[] = ['/demo/harB/index.ets'];
  let requestedStates: Es2pandaContextState[] = [Es2pandaContextState.ES2PANDA_STATE_PARSED, Es2pandaContextState.ES2PANDA_STATE_BIN_GENERATED];
  let request: CompileUnitRequest = {
    configRequest: configRequest,
    sourceFile: '/demo/harB/index.ets',
    sourceFiles: sourceFiles,
    outputPath: '/demo/harB/index.abc',
    requestedStates: requestedStates,
  };
  new Ets2panda();
  assertTrue(configHandle.value === (12 as long), 'config handle value');
  assertTrue(contextHandle.value === (34 as long), 'context handle value');
  assertEquals(request.configRequest.argv.join(' '), '_ --extension ets');
  assertEquals(request.sourceFile, '/demo/harB/index.ets');
  assertEquals(sourceFiles.join(','), '/demo/harB/index.ets');
  assertTrue(requestedStates.length === 2, 'requested states');
}

function testCompileHarB(): void {
  let configRequest: NativeCompilerConfigRequest = {
    argv: ['_', '--extension=ets', '--ets-module', '--output=driver/build_system/test/demo_hap/harB/index.abc'],
    configText: '{"compilerOptions":{"package":"harB","baseUrl":"driver/build_system/test/demo_hap/harB","rootDir":"driver/build_system/test/demo_hap","paths":{},"dependencies":{}},"files":["driver/build_system/test/demo_hap/harB/index.ets"]}',
  };
  let request: CompileUnitRequest = {
    configRequest: configRequest,
    sourceFile: 'driver/build_system/test/demo_hap/harB/index.ets',
    outputPath: 'driver/build_system/test/demo_hap/harB/index.abc',
  };
  new Ets2panda().compileFile(request);
}

function main(): void {
  testModels();
  testCompileHarB();
  console.log('ets2panda-native-adapter-contract-ok');
}

import { Es2pandaContextState } from '@arkts-bindings';
import { CompileUnitRequest, Ets2panda, NativeCompilerConfigRequest } from '../../ets_src/util/ets2panda';
import { fileExists, fileSize } from '../../ets_src/util/interop_helper';
import { assertTrue, fail } from './assertions';

function main(): void {
  let root: string = '/home/huawei/w/PANDA/src/ets_frontend/ets2panda/driver/build_system';
  let sourceFile: string = root + '/test/demo_hap/harB/index.ets';
  let outputPath: string = root + '/test/ets_ut/native_single_file_out/harB/index.abc';
  if (fileExists(outputPath)) {
    fail('stale or pre-existing ABC output: ' + outputPath);
  }
  let configText: string = '{"compilerOptions":{"package":"harB","baseUrl":"' + root + '/test/demo_hap/harB","rootDir":"' + root + '/test/demo_hap","paths":{},"dependencies":{}},"files":["' + sourceFile + '"]}';
  let configRequest: NativeCompilerConfigRequest = {
    argv: ['_', '--extension=ets', '--ets-module', '--output=' + outputPath],
    configText: configText,
  };
  let request: CompileUnitRequest = {
    configRequest: configRequest,
    sourceFile: sourceFile,
    outputPath: outputPath,
    requestedStates: [
      Es2pandaContextState.ES2PANDA_STATE_PARSED,
      Es2pandaContextState.ES2PANDA_STATE_BOUND,
      Es2pandaContextState.ES2PANDA_STATE_CHECKED,
      Es2pandaContextState.ES2PANDA_STATE_ASM_GENERATED,
      Es2pandaContextState.ES2PANDA_STATE_BIN_GENERATED,
    ],
  };
  new Ets2panda().compileFile(request);
  assertTrue(fileExists(outputPath), 'ABC output missing');
  assertTrue(fileSize(outputPath) > 0, 'ABC output empty');
  console.log('fresh single-file ABC success');
  console.log('native-single-file-compile-ok');
}

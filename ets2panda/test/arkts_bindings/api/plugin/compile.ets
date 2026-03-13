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

import {
    global,
    Es2pandaContextState,
    EtsNativePointer,
    nullptr
} from "@arkts-bindings/index";

interface ParsedArguments {
    srcFile: string;
    arktsConfig: string;
}

enum CreateContextMode {
    FromFile,
    FromString
}

function parseArgs(args: FixedArray<string>): ParsedArguments {
    arktest.assertEQ(args.length, 2);
    arktest.assertNE(args[0], "");
    arktest.assertNE(args[1], "");
    
    return {
        srcFile: args[0],
        arktsConfig: args[1]
    };
}

function compile(es2pandaArgs: string[], srcFile: string, createContextMode: CreateContextMode): string {
    const es2panda = global.es2panda;
    const generatedEs2panda = global.generatedEs2panda;

    let config = es2panda._CreateConfig(es2pandaArgs.length, es2pandaArgs);
    arktest.assertNE(config, nullptr);

    let context = (createContextMode == CreateContextMode.FromFile)
        ? createContextFromFile(config, srcFile)
        : createContextFromString(config, srcFile);

    arktest.assertNE(context, nullptr);
    arktest.assertEQ(es2panda._ContextErrorMessage(context), "");
    arktest.assertEQ(es2panda._ContextState(context), Es2pandaContextState.ES2PANDA_STATE_NEW);

    es2panda._ProceedToState(context, Es2pandaContextState.ES2PANDA_STATE_PARSED);
    arktest.assertEQ(es2panda._ContextErrorMessage(context), "");
    arktest.assertEQ(es2panda._ContextState(context), Es2pandaContextState.ES2PANDA_STATE_PARSED);

    let program = es2panda._ContextProgram(context);
    arktest.assertNE(context, nullptr);
 
    const programDump = generatedEs2panda._ProgramDump(context, program);
    arktest.assertNE(programDump, "");

    es2panda._ProceedToState(context, Es2pandaContextState.ES2PANDA_STATE_CHECKED);
    arktest.assertEQ(es2panda._ContextErrorMessage(context), "");
    arktest.assertEQ(es2panda._ContextState(context), Es2pandaContextState.ES2PANDA_STATE_CHECKED);

    es2panda._DestroyContext(context);
    es2panda._DestroyConfig(config);

    return programDump;
}

function createContextFromFile(config: EtsNativePointer, srcFile: string): EtsNativePointer {
    return global.es2panda._CreateContextFromFile(config, srcFile);
}

function createContextFromString(config: EtsNativePointer, srcFile: string): EtsNativePointer {
    arktest.assertTrue(global.interop._FileExists(srcFile), "File doesn't exist: " + srcFile);
    const source = global.interop._ReadFile(srcFile);
    arktest.assertNE(source, "");
    return global.es2panda._CreateContextFromString(config, source, srcFile);
}

function formCommand(args: ParsedArguments): string[] {
    const ets2pandaCmd: string[] = [
        '_',
        '--extension',
        'ets',
        '--arktsconfig',
        args.arktsConfig,
        args.srcFile
    ];
    return ets2pandaCmd;
}

function runTest(args: FixedArray<string>): void {
    const parsedArgs = parseArgs(args);
    const es2pandaArgs = formCommand(parsedArgs);
    const programDump1 = compile(es2pandaArgs, parsedArgs.srcFile, CreateContextMode.FromFile);
    const programDump2 = compile(es2pandaArgs, parsedArgs.srcFile, CreateContextMode.FromString);
    arktest.assertEQ(programDump1, programDump2);
}

function main(args: FixedArray<string>): int {
    let suite = new arktest.ArkTestsuite('arkts-bindings-plugin-api');
    suite.addTest('Compile', () => runTest(args));
    return suite.run();
}
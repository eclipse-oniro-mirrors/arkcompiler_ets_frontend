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

import { EtsNativePointer as EtsPtr } from './InteropTypes';
import { loadNativeLibrary } from './LoadLibraries';
import { getNativeLibraryPath } from './NativeLibrary';
import { Es2pandaNativeModule as GeneratedEs2pandaNativeModule } from './generated/Es2pandaNativeModule';

export class Es2pandaNativeModule {
  native _CreateAstDumper(context: EtsPtr, node: EtsPtr, source: string): EtsPtr;
  native _CreateConfig(argc: int, argv: string[]): EtsPtr;
  native _DestroyConfig(config: EtsPtr): void;
  native _DestroyConfigWithoutLog(config: EtsPtr): void;
  native _CreateContextFromString(config: EtsPtr, source: string, filename: string): EtsPtr;
  native _CreateContextFromFile(config: EtsPtr, filename: string): EtsPtr;
  native _DestroyContext(context: EtsPtr): void;
  native _CreateContextFromStringWithHistory(config: EtsPtr, source: string, filename: string): EtsPtr;
  native _CreateCacheContextFromString(config: EtsPtr, source: string, filename: string,
    globalContext: EtsPtr, isExternal: boolean, isLspUsage: boolean): EtsPtr;
  native _CreateCacheContextFromFile(config: EtsPtr, sourceFileName: string, globalContext: EtsPtr,
    isExternal: boolean): EtsPtr;
  native _CreateContextGenerateAbcForExternalSourceFiles(config: EtsPtr, fileNamesCount: int,
    filenames: string[]): EtsPtr;
  native _CreateGlobalContext(configPtr: EtsPtr, externalFileList: string[], fileNum: int): EtsPtr;
  native _DestroyGlobalContext(globalContextPtr: EtsPtr): void;
  native _MemInitialize(): void;
  native _MemFinalize(): void;
  native _ProceedToState(context: EtsPtr, state: int): EtsPtr;
  native _ContextProgram(context: EtsPtr): EtsPtr;
  native _ContextState(context: EtsPtr): int;
  native _ContextErrorMessage(context: EtsPtr): string;
  native _GetAllErrorMessages(context: EtsPtr): string;
  native _ProgramAst(context: EtsPtr, program: EtsPtr): EtsPtr;
  native _GenerateTsDeclarationsFromContext(config: EtsPtr, outputDeclEts: string, outputEts: string, exportAll: boolean,
    isolated: boolean, recordFile: string, genAnnotations: boolean): int;
  native _GenerateStaticDeclarationsFromContext(context: EtsPtr, filename: string): EtsPtr;
  native _FreeCompilerPartMemory(context: EtsPtr): EtsPtr;
}

export function initEs2panda(): Es2pandaNativeModule {
  loadNativeLibrary(getNativeLibraryPath());
  return new Es2pandaNativeModule();
}

export function initGeneratedEs2panda(): GeneratedEs2pandaNativeModule {
  loadNativeLibrary(getNativeLibraryPath());
  return new GeneratedEs2pandaNativeModule();
}
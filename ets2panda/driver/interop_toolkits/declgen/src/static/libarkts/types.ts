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

export type KPointer = Uint8Array | number | bigint;

export enum Es2pandaContextState {
  ES2PANDA_STATE_NEW = 0,
  ES2PANDA_STATE_PARSED = 1,
  ES2PANDA_STATE_BOUND = 2,
  ES2PANDA_STATE_CHECKED = 3,
  ES2PANDA_STATE_LOWERED = 4,
  ES2PANDA_STATE_ASM_GENERATED = 5,
  ES2PANDA_STATE_BIN_GENERATED = 6,
  ES2PANDA_STATE_ERROR = 7,
}

export interface AstNode {
  kind: string;
  statements: AstNode[];
  source: LiteralNode;
  specifiers: ImportSpecifierNode[];
}

export interface LiteralNode {
  str: string;
  clone: Function;
}

export interface IdentifierNode {
  name: string;
}

export interface ImportSpecifierNode {
  imported?: IdentifierNode;
}

export interface ETSImportDeclaration extends AstNode {
  specifiers: ImportSpecifierNode[];
  source: LiteralNode;
}

export enum Es2pandaImportKinds {
  IMPORT_KINDS_ALL = 0,
}

export interface ArkTSGlobal {
  filePath: string;
  config: object;
  compilerContext: {
    program: object;
    peer: object;
  };
  es2panda: {
    _DestroyContext: Function;
    _MemInitialize: Function;
    _MemFinalize: Function;
    _CreateGlobalContext: Function;
    _DestroyGlobalContext: Function;
    _SetUpSoPath: Function;
    _FreeCompilerPartMemory: Function;
  };
}

export interface ArkTS {
  Config: {
    create: Function;
  };
  Context: {
    createFromString: Function;
    createFromStringWithHistory: Function;
    createCacheContextFromFile: Function;
    createContextSimultaneousMode: Function;
  };
  EtsScript: {
    fromContext: Function;
  };
  proceedToState: Function;
  createTsDeclgen: Function;
  generateTsDeclarationsAfterParsed: Function;
  generateTsDeclarationsAfterCheck: Function;
  writeTsDeclarations: Function;
  destroyTsDeclgen: Function;
  formOutputPathForFile: Function;
  generateStaticDeclarationsFromContext: Function;
  destroyConfig: Function;
  Es2pandaContextState: typeof Es2pandaContextState;
  memInitialize: Function;
  memFinalize: Function;
  createGlobalContext: Function;
  AstNode: AstNode;
  ETSImportDeclaration: ETSImportDeclaration;
  isETSModule: Function;
  isImportSpecifier: Function;
  isETSImportDeclaration: Function;
  factory: {
    createETSModule: Function;
    createImportDeclaration: Function;
    createImportSpecifier: Function;
    createIdentifier: Function;
    updateETSModule: Function;
    createStringLiteral: Function;
  };
  Es2pandaImportKinds: typeof Es2pandaImportKinds;
  ExtractDeclarationsFromAbcFile: Function;
}

export interface LibArkts {
  arkts: ArkTS;
  arktsGlobal: ArkTSGlobal;
}

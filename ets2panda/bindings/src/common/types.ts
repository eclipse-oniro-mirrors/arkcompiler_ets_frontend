/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

import { KNativePointer, KNativePointer as KPtr } from './InteropTypes';
import { global } from './global';
import { throwError } from './utils';
import { passString, passStringArray, unpackString } from './private';
import { isNullPtr } from './Wrapper';
import { Worker as ThreadWorker } from 'worker_threads';

export const arrayOfNullptr = new BigUint64Array([BigInt(0)]);

export abstract class ArktsObject {
  protected constructor(peer: KPtr) {
    this.peer = peer;
  }

  readonly peer: KPtr;
}

export abstract class Node extends ArktsObject {
  protected constructor(peer: KPtr) {
    if (isNullPtr(peer)) {
      throw new Error('trying to create new Node on NULLPTR');
    }
    super(peer);
  }

  public get originalPeer(): KPtr {
    return global.es2panda._AstNodeOriginalNodeConst(global.context, this.peer);
  }

  public set originalPeer(peer: KPtr) {
    global.es2panda._AstNodeSetOriginalNode(global.context, this.peer, peer);
  }

  protected dumpMessage(): string {
    return ``;
  }

  public dumpJson(): string {
    return unpackString(global.es2panda._AstNodeDumpJsonConst(global.context, this.peer));
  }

  public dumpSrc(): string {
    return unpackString(global.es2panda._AstNodeDumpEtsSrcConst(global.context, this.peer));
  }
}

export class Config extends ArktsObject {
  readonly path: string;
  constructor(peer: KPtr, fpath: string) {
    super(peer);
    // TODO: wait for getter from api
    this.path = fpath;
  }

  public toString(): string {
    return `Config (peer = ${this.peer}, path = ${this.path})`;
  }

  static create(input: string[], fpath: string, pandaLibPath: string = '', isEditingMode: boolean = false): Config {
    if (isEditingMode) {
      let cfg = global.es2pandaPublic._CreateConfig(input.length, passStringArray(input), pandaLibPath);
      return new Config(cfg, fpath);
    }
    if (!global.configIsInitialized()) {
      let cfg = global.es2panda._CreateConfig(input.length, passStringArray(input), pandaLibPath);
      global.config = cfg;
      return new Config(cfg, fpath);
    } else {
      return new Config(global.config, fpath);
    }
  }
}

export class Context extends ArktsObject {
  constructor(peer: KPtr) {
    super(peer);
  }

  public toString(): string {
    return `Context (peer = ${this.peer})`;
  }

  static createFromString(source: string): Context {
    if (!global.configIsInitialized()) {
      throwError(`Config not initialized`);
    }
    return new Context(
      global.es2panda._CreateContextFromString(global.config, passString(source), passString(global.filePath))
    );
  }

  static createFromStringWithHistory(source: string): Context {
    if (!global.configIsInitialized()) {
      throwError(`Config not initialized`);
    }
    return new Context(
      global.es2panda._CreateContextFromStringWithHistory(
        global.config,
        passString(source),
        passString(global.filePath)
      )
    );
  }

  static lspCreateFromString(source: string, filePath: string, cfg: Config): KPtr {
    if (cfg === undefined) {
      throwError(`Config not initialized`);
    }
    return global.es2pandaPublic._CreateContextFromString(cfg.peer, passString(source), passString(filePath));
  }

  static lspCreateCacheContextFromString(
    source: string,
    filePath: string,
    cfg: Config,
    globalContextPtr: KNativePointer,
    isExternal: boolean
  ): KPtr {
    if (cfg === undefined) {
      throwError(`Config not initialized`);
    }
    return global.es2pandaPublic._CreateCacheContextFromString(
      cfg.peer,
      passString(source),
      passString(filePath),
      globalContextPtr,
      isExternal
    );
  }
}

// ProjectConfig begins
export interface PluginsConfig {
  [pluginName: string]: string;
}

export interface ModuleConfig {
  packageName: string;
  moduleType: string;
  moduleRootPath: string;
  language: string;
  declFilesPath?: string;
  dependencies?: string[];
}

export interface PathConfig {
  buildSdkPath: string;
  projectPath: string;
  declgenOutDir: string;
  cacheDir?: string;
  externalApiPath?: string;
  aceModuleJsonPath?: string;
  interopApiPath?: string;
}

export interface DeclgenConfig {
  declgenV1OutPath?: string;
  declgenBridgeCodePath?: string;
}

export interface BuildConfig extends DeclgenConfig, ModuleConfig, PathConfig {
  plugins: PluginsConfig;
  compileFiles: string[];
  depModuleCompileFiles: string[];
  sdkAliasConfigPath?: string;
}
// ProjectConfig ends

export interface ModuleInfo {
  packageName: string;
  moduleRootPath: string;
  moduleType: string;
  entryFile: string;
  arktsConfigFile: string;
  compileFiles: string[];
  depModuleCompileFiles: string[];
  declgenV1OutPath: string | undefined;
  declgenBridgeCodePath: string | undefined;
  staticDepModuleInfos: string[];
  dynamicDepModuleInfos: string[];
  language: string;
  dependencies?: string[];
  declFilesPath?: string;
  sdkAliasConfigPath?: string;
}

export interface Job {
  id: string;
  isDeclFile: boolean;
  isInCycle?: boolean;
  fileList: string[];
  dependencies: string[];
  dependants: string[];
  isValid: boolean;
}

export interface JobInfo {
  id: string;
  filePath: string;
  arktsConfigFile: string;
  globalContextPtr: KNativePointer;
  buildConfig: BuildConfig;
  isValid: boolean;
}

export interface FileDepsInfo {
  dependencies: Record<string, string[]>;
  dependants: Record<string, string[]>;
}

export interface WorkerInfo {
  worker: ThreadWorker;
  isIdle: boolean;
}
export interface TextDocumentChangeInfo {
  newDoc: string;
  rangeStart?: number;
  rangeEnd?: number;
  updateText?: string;
}

export enum AstNodeType {
  ANNOTATION_DECLARATION = 1,
  ANNOTATION_USAGE = 2,
  AWAIT_EXPRESSION = 4,
  CALL_EXPRESSION = 10,
  CLASS_DEFINITION = 14,
  CLASS_DECLARATION = 15,
  CLASS_PROPERTY = 17,
  EMPTY_STATEMENT = 25,
  EXPORT_DEFAULT_DECLARATION = 27,
  EXPORT_NAMED_DECLARATION = 28,
  EXPORT_SPECIFIER = 29,
  EXPRESSION_STATEMENT = 30,
  FUNCTION_EXPRESSION = 35,
  IDENTIFIER = 36,
  IMPORT_DECLARATION = 39,
  IMPORT_DEFAULT_SPECIFIER = 41,
  IMPORT_NAMESPACE_SPECIFIER = 42,
  IMPORT_SPECIFIER = 43,
  MEMBER_EXPRESSION = 45,
  METHOD_DEFINITION = 47,
  PROPERTY = 56,
  REEXPORT_STATEMENT = 58,
  RETURN_STATEMENT = 59,
  SCRIPT_FUNCTION = 60,
  ETS_STRING_LITERAL_TYPE = 67,
  ETS_FUNCTION_TYPE = 69,
  ETS_TYPE_REFERENCE = 74,
  ETS_KEYOF_TYPE = 77,
  ETS_NEW_CLASS_INSTANCE_EXPRESSION = 80,
  ETS_IMPORT_DECLARATION = 81,
  ETS_PARAMETER_EXPRESSION = 82,
  SUPER_EXPRESSION = 85,
  STRUCT_DECLARATION = 86,
  TS_ENUM_DECLARATION = 89,
  TS_ENUM_MEMBER = 90,
  TS_TYPE_PARAMETER = 120,
  TS_FUNCTION_TYPE = 127,
  TS_MODULE_DECLARATION = 125,
  TS_TYPE_ALIAS_DECLARATION = 129,
  TS_TYPE_REFERENCE = 130,
  TS_INTERFACE_DECLARATION = 133,
  TS_CLASS_IMPLEMENTS = 141,
  VARIABLE_DECLARATION = 152,
  VARIABLE_DECLARATOR = 153,
  SPREAD_ELEMENT = 165,
  UNKNOWN,
}

export const astNodeTypeMap = new Map<string, AstNodeType>([
  ['IDENTIFIER', AstNodeType.IDENTIFIER],
  ['CLASS_DEFINITION', AstNodeType.CLASS_DEFINITION],
  ['ANNOTATION_DECLARATION', AstNodeType.ANNOTATION_DECLARATION],
  ['ANNOTATION_USAGE', AstNodeType.ANNOTATION_USAGE],
  ['AWAIT_EXPRESSION', AstNodeType.AWAIT_EXPRESSION],
  ['CALL_EXPRESSION', AstNodeType.CALL_EXPRESSION],
  ['CLASS_DECLARATION', AstNodeType.CLASS_DECLARATION],
  ['CLASS_PROPERTY', AstNodeType.CLASS_PROPERTY],
  ['EMPTY_STATEMENT', AstNodeType.EMPTY_STATEMENT],
  ['EXPORT_DEFAULT_DECLARATION', AstNodeType.EXPORT_DEFAULT_DECLARATION],
  ['EXPORT_NAMED_DECLARATION', AstNodeType.EXPORT_NAMED_DECLARATION],
  ['EXPORT_SPECIFIER', AstNodeType.EXPORT_SPECIFIER],
  ['EXPRESSION_STATEMENT', AstNodeType.EXPRESSION_STATEMENT],
  ['FUNCTION_EXPRESSION', AstNodeType.FUNCTION_EXPRESSION],
  ['IMPORT_DECLARATION', AstNodeType.IMPORT_DECLARATION],
  ['IMPORT_DEFAULT_SPECIFIER', AstNodeType.IMPORT_DEFAULT_SPECIFIER],
  ['IMPORT_NAMESPACE_SPECIFIER', AstNodeType.IMPORT_NAMESPACE_SPECIFIER],
  ['IMPORT_SPECIFIER', AstNodeType.IMPORT_SPECIFIER],
  ['MEMBER_EXPRESSION', AstNodeType.MEMBER_EXPRESSION],
  ['METHOD_DEFINITION', AstNodeType.METHOD_DEFINITION],
  ['PROPERTY', AstNodeType.PROPERTY],
  ['REEXPORT_STATEMENT', AstNodeType.REEXPORT_STATEMENT],
  ['RETURN_STATEMENT', AstNodeType.RETURN_STATEMENT],
  ['SCRIPT_FUNCTION', AstNodeType.SCRIPT_FUNCTION],
  ['ETS_STRING_LITERAL_TYPE', AstNodeType.ETS_STRING_LITERAL_TYPE],
  ['ETS_FUNCTION_TYPE', AstNodeType.ETS_FUNCTION_TYPE],
  ['ETS_TYPE_REFERENCE', AstNodeType.ETS_TYPE_REFERENCE],
  ['ETS_KEYOF_TYPE', AstNodeType.ETS_KEYOF_TYPE],
  ['ETS_NEW_CLASS_INSTANCE_EXPRESSION', AstNodeType.ETS_NEW_CLASS_INSTANCE_EXPRESSION],
  ['ETS_IMPORT_DECLARATION', AstNodeType.ETS_IMPORT_DECLARATION],
  ['ETS_PARAMETER_EXPRESSION', AstNodeType.ETS_PARAMETER_EXPRESSION],
  ['SUPER_EXPRESSION', AstNodeType.SUPER_EXPRESSION],
  ['STRUCT_DECLARATION', AstNodeType.STRUCT_DECLARATION],
  ['TS_ENUM_DECLARATION', AstNodeType.TS_ENUM_DECLARATION],
  ['TS_ENUM_MEMBER', AstNodeType.TS_ENUM_MEMBER],
  ['TS_TYPE_PARAMETER', AstNodeType.TS_TYPE_PARAMETER],
  ['TS_FUNCTION_TYPE', AstNodeType.TS_FUNCTION_TYPE],
  ['TS_MODULE_DECLARATION', AstNodeType.TS_MODULE_DECLARATION],
  ['TS_TYPE_ALIAS_DECLARATION', AstNodeType.TS_TYPE_ALIAS_DECLARATION],
  ['TS_TYPE_REFERENCE', AstNodeType.TS_TYPE_REFERENCE],
  ['TS_INTERFACE_DECLARATION', AstNodeType.TS_INTERFACE_DECLARATION],
  ['TS_CLASS_IMPLEMENTS', AstNodeType.TS_CLASS_IMPLEMENTS],
  ['VARIABLE_DECLARATION', AstNodeType.VARIABLE_DECLARATION],
  ['VARIABLE_DECLARATOR', AstNodeType.VARIABLE_DECLARATOR],
  ['SPREAD_ELEMENT', AstNodeType.SPREAD_ELEMENT]
]);

export interface NodeInfo {
  name: string;
  kind: AstNodeType;
}

export interface AliasConfig {
  originalAPIName: string;
  isStatic: boolean;
}

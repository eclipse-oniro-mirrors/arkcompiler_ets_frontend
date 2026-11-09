/**
 * Copyright (c) 2025 - 2026 Huawei Device Co., Ltd.
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

import { KNativePointer as KPtr } from './InteropTypes';
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

  static create(input: string[], fpath: string = '', pandaLibPath: string = '', isEditingMode: boolean = false): Config {
    if (isEditingMode) {
      let cfg = global.es2pandaPublic._CreateConfig(input.length, passStringArray(input), pandaLibPath);
      return new Config(cfg, fpath);
    }
    if (!global.configIsInitialized()) {
      let cfg = global.es2panda._CreateConfig(input.length, passStringArray(input), pandaLibPath);
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
    let ctx = global.es2panda._CreateContextFromString(global.config, passString(source), passString(global.filePath));
     return new Context(ctx);
  }

  static createFromStringWithHistory(source: string): Context {
    if (!global.configIsInitialized()) {
      throwError(`Config not initialized`);
    }
    let ctx = global.es2panda._CreateContextFromStringWithHistory(
      global.config,
      passString(source),
      passString(global.filePath)
    );
     return new Context(ctx);
  }

  static createContextSimultaneousMode(
     filenames: string[]
  ): Context {
     let ctx = global.es2panda._CreateContextSimultaneousMode(global.config, filenames.length, passStringArray(filenames));
     return new Context(ctx);
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
    globalContextPtr: KPtr,
    isExternal: boolean,
    isLspUsage: boolean
  ): KPtr {
    if (cfg === undefined) {
      throwError(`Config not initialized`);
    }
    return global.es2pandaPublic._CreateCacheContextFromString(
      cfg.peer,
      passString(source),
      passString(filePath),
      globalContextPtr,
      isExternal,
      isLspUsage
    );
  }

  get program(): Program {
      return new Program(global.es2panda._ContextProgram(this.peer));
  }
}

export class Program extends ArktsObject {
    constructor(peer: KPtr) {
        super(peer);
    }

    /**
     * Returns whether a top-level local declaration name is exported by this program.
     *
     * This API is intended for transformed top-level declarations only. Callers should assert that the queried node
     * is a top-level declaration in this program before using it. For renamed exports, query the local declaration name,
     * not the public exported name.
     */
    isLocalNameExported(name: string): boolean {
        return global.es2panda._ProgramLocalNameIsExported(global.context, this.peer, name) !== 0;
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
  permissions?: string[];
  deviceTypes?: string[];
  compatibleVersion?: string;
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
}

export interface BuildConfig extends DeclgenConfig, ModuleConfig, PathConfig {
  plugins: PluginsConfig;
  compileFiles: string[];
  depModuleCompileFiles: string[];
  sdkAliasConfigPath?: string;
  entryFile?: string;
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
  globalContextPtr: KPtr;
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
  BIGINT_LITERAL = 5,
  CALL_EXPRESSION = 10,
  CLASS_DEFINITION = 14,
  CLASS_DECLARATION = 15,
  CLASS_PROPERTY = 17,
  EMPTY_STATEMENT = 25,
  EXPORT_DEFAULT_DECLARATION = 27,
  EXPORT_NAMED_DECLARATION = 28,
  EXPORT_SPECIFIER = 29,
  EXPRESSION_STATEMENT = 30,
  FUNCTION_DECLARATION = 34,
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
  ETS_KEYOF_TYPE = 78,
  ETS_NEW_CLASS_INSTANCE_EXPRESSION = 81,
  ETS_IMPORT_DECLARATION = 82,
  ETS_PARAMETER_EXPRESSION = 83,
  ETS_MODULE = 85,
  SUPER_EXPRESSION = 86,
  STRUCT_DECLARATION = 87,
  SWITCH_STATEMENT = 89,
  TS_ENUM_DECLARATION = 90,
  TS_ENUM_MEMBER = 91,
  TS_NON_NULL_EXPRESSION = 103,
  TS_TYPE_PARAMETER = 121,
  TS_MODULE_DECLARATION = 126,
  TS_FUNCTION_TYPE = 128,
  TS_TYPE_ALIAS_DECLARATION = 130,
  TS_TYPE_REFERENCE = 131,
  TS_INTERFACE_DECLARATION = 134,
  TS_CLASS_IMPLEMENTS = 142,
  VARIABLE_DECLARATION = 153,
  VARIABLE_DECLARATOR = 154,
  SPREAD_ELEMENT = 166,
  UNKNOWN = 255
}

export const astNodeTypeMap = new Map<string, AstNodeType>([
  ['ANNOTATION_DECLARATION', AstNodeType.ANNOTATION_DECLARATION],
  ['ANNOTATION_USAGE', AstNodeType.ANNOTATION_USAGE],
  ['AWAIT_EXPRESSION', AstNodeType.AWAIT_EXPRESSION],
  ['BIGINT_LITERAL', AstNodeType.BIGINT_LITERAL],
  ['CALL_EXPRESSION', AstNodeType.CALL_EXPRESSION],
  ['CLASS_DECLARATION', AstNodeType.CLASS_DECLARATION],
  ['CLASS_DEFINITION', AstNodeType.CLASS_DEFINITION],
  ['CLASS_PROPERTY', AstNodeType.CLASS_PROPERTY],
  ['EMPTY_STATEMENT', AstNodeType.EMPTY_STATEMENT],
  ['ETS_FUNCTION_TYPE', AstNodeType.ETS_FUNCTION_TYPE],
  ['ETS_IMPORT_DECLARATION', AstNodeType.ETS_IMPORT_DECLARATION],
  ['ETS_KEYOF_TYPE', AstNodeType.ETS_KEYOF_TYPE],
  ['ETS_MODULE', AstNodeType.ETS_MODULE],
  ['ETS_NEW_CLASS_INSTANCE_EXPRESSION', AstNodeType.ETS_NEW_CLASS_INSTANCE_EXPRESSION],
  ['ETS_PARAMETER_EXPRESSION', AstNodeType.ETS_PARAMETER_EXPRESSION],
  ['ETS_STRING_LITERAL_TYPE', AstNodeType.ETS_STRING_LITERAL_TYPE],
  ['ETS_TYPE_REFERENCE', AstNodeType.ETS_TYPE_REFERENCE],
  ['EXPORT_DEFAULT_DECLARATION', AstNodeType.EXPORT_DEFAULT_DECLARATION],
  ['EXPORT_NAMED_DECLARATION', AstNodeType.EXPORT_NAMED_DECLARATION],
  ['EXPORT_SPECIFIER', AstNodeType.EXPORT_SPECIFIER],
  ['EXPRESSION_STATEMENT', AstNodeType.EXPRESSION_STATEMENT],
  ['FUNCTION_DECLARATION', AstNodeType.FUNCTION_DECLARATION],
  ['FUNCTION_EXPRESSION', AstNodeType.FUNCTION_EXPRESSION],
  ['IDENTIFIER', AstNodeType.IDENTIFIER],
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
  ['SPREAD_ELEMENT', AstNodeType.SPREAD_ELEMENT],
  ['STRUCT_DECLARATION', AstNodeType.STRUCT_DECLARATION],
  ['SUPER_EXPRESSION', AstNodeType.SUPER_EXPRESSION],
  ['SWITCH_STATEMENT', AstNodeType.SWITCH_STATEMENT],
  ['TS_CLASS_IMPLEMENTS', AstNodeType.TS_CLASS_IMPLEMENTS],
  ['TS_ENUM_DECLARATION', AstNodeType.TS_ENUM_DECLARATION],
  ['TS_ENUM_MEMBER', AstNodeType.TS_ENUM_MEMBER],
  ['TS_FUNCTION_TYPE', AstNodeType.TS_FUNCTION_TYPE],
  ['TS_INTERFACE_DECLARATION', AstNodeType.TS_INTERFACE_DECLARATION],
  ['TS_MODULE_DECLARATION', AstNodeType.TS_MODULE_DECLARATION],
  ['TS_NON_NULL_EXPRESSION', AstNodeType.TS_NON_NULL_EXPRESSION],
  ['TS_TYPE_ALIAS_DECLARATION', AstNodeType.TS_TYPE_ALIAS_DECLARATION],
  ['TS_TYPE_PARAMETER', AstNodeType.TS_TYPE_PARAMETER],
  ['TS_TYPE_REFERENCE', AstNodeType.TS_TYPE_REFERENCE],
  ['VARIABLE_DECLARATION', AstNodeType.VARIABLE_DECLARATION],
  ['VARIABLE_DECLARATOR', AstNodeType.VARIABLE_DECLARATOR],
]);

export interface NodeInfo {
  name: string;
  kind: AstNodeType;
  // Overload disambiguation: the parameter count of the method definition; optional so
  // legacy node info without it keeps the first-match behaviour.
  paramNumber?: number;
}

export interface AliasConfig {
  originalAPIName: string;
  isStatic: boolean;
}

export abstract class AstNode extends ArktsObject {
    protected constructor(peer: KPtr) {
        if (peer === null) {
            throwError(`attempted to create AstNode from nullptr`);
        }
        super(peer);
    }
}

export class EtsScript extends AstNode {
    constructor(peer: KPtr) {
        super(peer);
    }

    static fromContext(): EtsScript {
        let ctx = global.es2panda._ProgramAst(global.context, global.es2panda._ContextProgram(global.context))
        let etsScript = new EtsScript(ctx);
        return etsScript;
    }
}

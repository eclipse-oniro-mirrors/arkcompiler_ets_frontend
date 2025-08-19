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
  CLASS_DEFINITION = 14,
  CLASS_PROPERTY = 17,
  EXPORT_DEFAULT_DECLARATION = 27,
  EXPORT_NAMED_DECLARATION = 28,
  EXPORT_SPECIFIER = 29,
  IDENTIFIER = 36,
  MEMBER_EXPRESSION = 45,
  METHOD_DEFINITION = 47,
  PROPERTY = 56,
  ETS_FUNCTION_TYPE = 69,
  TS_ENUM_DECLARATION = 89,
  TS_ENUM_MEMBER = 90,
  TS_MODULE_DECLARATION = 125,
  TS_TYPE_ALIAS_DECLARATION = 129,
  TS_INTERFACE_DECLARATION = 133,
  TS_CLASS_IMPLEMENTS = 141,
  UNKNOWN,
}

export interface NodeInfo {
  name: string;
  kind: AstNodeType;
}

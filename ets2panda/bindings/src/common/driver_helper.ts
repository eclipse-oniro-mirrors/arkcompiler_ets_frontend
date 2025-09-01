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

import { Context, Config } from './types';
import { global } from './global';
import { throwError } from './utils';
import { Es2pandaContextState } from '../generated/Es2pandaEnums';
import { withStringResult } from './Platform';
import { KBoolean, KInt, KNativePointer, KPointer } from './InteropTypes';
import { passStringArray } from './private';

export class DriverHelper {
  constructor(filePath: string, cmd: string[]) {
    this._filePath = filePath;
    global.filePath = this._filePath;
    this._cfg = Config.create(cmd, filePath);
  }
  private _ctx: Context | undefined;
  private _cfg: Config;
  private _filePath: string;

  public createCtx(source: string): KPointer {
    this._ctx = Context.createFromString(source);
    return this._ctx.peer;
  }

  public toString(): string {
    return `DriverHelper (filepath = ${this._filePath}, config = ${this._cfg}, context = ${this._ctx})`;
  }

  public proceedToState(state: Es2pandaContextState): void {
    if (this._ctx === undefined) {
      throwError('Trying to proceed to state while cts is undefined');
    }
    if (state <= global.es2panda._ContextState(this._ctx.peer)) {
      return;
    }

    try {
      global.es2panda._ProceedToState(this._ctx.peer, state);
      if (global.es2panda._ContextState(this._ctx.peer) === Es2pandaContextState.ES2PANDA_STATE_ERROR) {
        const errMsg = withStringResult(global.es2panda._ContextErrorMessage(this._ctx.peer));
        if (errMsg === undefined) {
          throwError(`Couldn't get context error msg`);
        } else {
          throwError('Failed proceed to: ' + Es2pandaContextState[state] + '\n' + errMsg);
        }
      }
    } catch (e) {
      global.es2panda._DestroyContext(this._ctx.peer);
      throw e;
    }
  }

  public finalize(): void {
    if (this._cfg === undefined) {
      throwError('Call finalize before initialized config');
    }
    if (this._ctx === undefined) {
      throwError('Call finalize before initialized context');
    }
    global.es2panda._DestroyContext(this._ctx.peer);
    global.es2panda._DestroyConfig(this._cfg.peer);
    this._ctx = undefined;
    global.destroyCfg();
  }

  public generateTsDecl(
    declOutPath: string,
    etsOutPath: string,
    exportAll: boolean,
    isolated: boolean,
    recordFile: string,
    genAnnotations: boolean
  ): void {
    let exportAll_: KBoolean = exportAll ? 1 : 0;
    let isolated_: KBoolean = isolated ? 1 : 0;
    let genAnnotations_: KBoolean = genAnnotations ? 1 : 0;
    global.es2panda._GenerateTsDeclarationsFromContext(
      this._cfg.peer,
      declOutPath,
      etsOutPath,
      exportAll_,
      isolated_,
      recordFile,
      genAnnotations_
    );
  }
}

export class LspDriverHelper {
  public memInitialize(pandaLibPath: string): void {
    global.es2pandaPublic._MemInitialize(pandaLibPath);
  }

  public memFinalize(): void {
    global.es2pandaPublic._MemFinalize();
  }

  public createGlobalContext(config: KNativePointer, externalFileList: string[], fileNum: KInt): KNativePointer {
    return global.es2pandaPublic._CreateGlobalContext(config, passStringArray(externalFileList), fileNum);
  }

  public destroyGlobalContext(context: KNativePointer): void {
    global.es2pandaPublic._DestroyGlobalContext(context);
  }

  public createCfg(cmd: string[], filePath: string, pandaLibPath: string = ''): Config {
    return Config.create(cmd, filePath, pandaLibPath, true);
  }

  public createCtx(
    source: string,
    filePath: string,
    cfg: Config,
    globalContextPtr?: KNativePointer,
    isExternal: boolean = false
  ): KNativePointer {
    if (globalContextPtr) {
      return Context.lspCreateCacheContextFromString(source, filePath, cfg, globalContextPtr, isExternal);
    } else {
      return Context.lspCreateFromString(source, filePath, cfg);
    }
  }

  public proceedToState(ctx: KNativePointer, state: Es2pandaContextState): void {
    if (ctx === undefined) {
      throwError('Trying to proceed to state while cts is undefined');
    }
    if (state <= global.es2pandaPublic._ContextState(ctx)) {
      return;
    }

    try {
      global.es2pandaPublic._ProceedToState(ctx, state);
    } catch (e) {
      global.es2pandaPublic._DestroyContext(ctx);
      throw e;
    }
  }

  public destroyContext(ctx: KNativePointer): void {
    if (ctx === undefined) {
      return;
    }
    global.es2pandaPublic._DestroyContext(ctx);
  }

  public destroyConfig(cfg: Config): void {
    if (cfg === undefined) {
      return;
    }
    global.es2pandaPublic._DestroyConfig(cfg.peer);
  }
}

/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

import { Context, Config, EtsScript } from './types';
import { global } from './global';
import { throwError } from './utils';
import { Es2pandaContextState } from '../generated/Es2pandaEnums';
import { withStringResult } from './Platform';
import { KBoolean, KInt, KNativePointer, KPointer } from './InteropTypes';
import { passString, passStringArray } from './private';
import { Es2pandaNativeModule } from './Es2pandaNativeModule';

export class DriverHelper {
    private _ctx: Context | undefined;
    private _cfg: Config | undefined;

    private _filePath: string | undefined;

    get Context(): typeof Context {
        return Context;
    }

    get Config(): typeof Config {
        return Config;
    }

    get Es2pandaContextState(): typeof Es2pandaContextState {
        return Es2pandaContextState;
    }

    get EtsScript(): typeof EtsScript {
        return EtsScript;
    }
    public getGlobalEs2panda(): Es2pandaNativeModule {
        return global.es2panda;
    }

    public setContext(ctx: KPointer): void {
        this._ctx = new Context(ctx);
    }
    public createCtx(source: string): Context {
        let ctx = Context.createFromString(source);
        this._ctx = ctx;
        return ctx;
    }

    public createCfg(input: string[], pandaLibPath: string = ''): KPointer {
        let cfg = global.es2pandaPublic._CreateConfig(input.length, passStringArray(input), pandaLibPath);
        this._cfg = new Config(cfg, this._filePath!);
        return cfg;
    }
    public createCtxWithHistory(source: string): Context {
        let ctx = Context.createFromStringWithHistory(source);
        this._ctx = ctx;
        return ctx;
    }

    public toString(): string {
        return `DriverHelper (filepath = ${this._filePath!}, config = ${this._cfg!}, context = ${this._ctx})`;
    }

    public proceedToState(state: Es2pandaContextState, globalContextPtr: KNativePointer, forceDtsEmit: boolean = false): void {
        if (global.es2panda._ContextState(globalContextPtr) === Es2pandaContextState.ES2PANDA_STATE_ERROR) {
            this.processErrorState(globalContextPtr, state, forceDtsEmit);
        }
        if (state <= global.es2panda._ContextState(globalContextPtr)) {
            return;
        }
        global.es2panda._ProceedToState(globalContextPtr, state);
        this.processErrorState(globalContextPtr, state, forceDtsEmit);
    }

    public processErrorState(globalContextPtr: KNativePointer, state: Es2pandaContextState, forceDtsEmit = false): void {
        try {
            if (global.es2panda._ContextState(globalContextPtr) === Es2pandaContextState.ES2PANDA_STATE_ERROR && !forceDtsEmit) {
                const errorMessage = withStringResult(global.es2panda._ContextErrorMessage(globalContextPtr));
                if (errorMessage === undefined) {
                    throwError(`Could not get ContextErrorMessage`);
                }
                const allErrorMessages = withStringResult(global.es2panda._GetAllErrorMessages(globalContextPtr));
                if (allErrorMessages === undefined) {
                    throwError(`Could not get AllErrorMessages`);
                }


                throwError('Failed proceed to: ' + Es2pandaContextState[state] + '\n' + errorMessage);
            }
        } catch (e) {
            global.es2panda._DestroyContext(globalContextPtr);
            throw e;
        }
    }

    public destroyConfig(config: KNativePointer): void {
        global.es2panda._DestroyConfig(config);
        global.resetConfig();
    }

    public finalize(errorStatus: boolean = false): void {
        if (this._cfg === undefined) {
            throwError('Call finalize before initialized config');
        }
        if (this._ctx === undefined) {
            throwError('Call finalize before initialized context');
        }
        if (!errorStatus) {
            global.es2panda._DestroyContext(this._ctx.peer);
        }
        global.es2panda._DestroyConfig(this._cfg!.peer);
        this._ctx = undefined;
        global.resetConfig();
    }

    public generateTsDeclarationsFromContext(
        outputDeclEts: string,
        outputEts: string,
        exportAll: boolean,
        isolated: boolean,
        recordFile: string,
        genAnnotations: boolean
    ): KInt {
        return global.es2panda._GenerateTsDeclarationsFromContext(
            global.context,
            passString(outputDeclEts),
            passString(outputEts),
            exportAll ? 1 : 0,
            isolated ? 1 : 0,
            passString(recordFile),
            genAnnotations ? 1 : 0
        );
    }

    public createContextGenerateAbcForExternalSourceFiles(
        filenames: string[]
    ): KPointer {
        let ctx = global.es2panda._CreateContextGenerateAbcForExternalSourceFiles(this._cfg!.peer, filenames.length, passStringArray(filenames));
        this._ctx = new Context(ctx);
        return ctx;
    }

    public getConfig(): Config | undefined {
        return this._cfg;
    }

    public createGlobalContext(externalFileList: string[], fileNum: KInt): KNativePointer {
        let ctx = global.es2pandaPublic._CreateGlobalContext(this._cfg!.peer, passStringArray(externalFileList), fileNum);
        this._ctx = new Context(ctx);
        return ctx;
    }

    public generateStaticDeclarationsFromContext(outputPath: string): KNativePointer {
        let ctx = global.es2panda._GenerateStaticDeclarationsFromContext(global.context, outputPath);
        this._ctx = new Context(ctx);
        return ctx;
    }

    public createCacheContextFromFile(configPtr: KNativePointer, fileName: string, globalContextPtr: KNativePointer, isExternal: boolean = false): KNativePointer {
        let ctx = global.es2panda._CreateCacheContextFromFile(configPtr, fileName, globalContextPtr, isExternal);
        this._ctx = new Context(ctx);
        return ctx;
    }

    public memInitialize(): void {
        global.es2panda._MemInitialize();
    }

    public memFinalize(): void {
        global.es2panda._MemFinalize();
    }
}

export class LspDriverHelper {
    public memInitialize(pandaLibPath: string): void {
        global.es2pandaPublic._MemInitializeWithPath(pandaLibPath);
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
        isExternal: boolean = false,
        isLspUsage: boolean = false
    ): KNativePointer {
        if (globalContextPtr) {
            return Context.lspCreateCacheContextFromString(source, filePath, cfg, globalContextPtr, isExternal, isLspUsage);
        } else {
            return Context.lspCreateFromString(source, filePath, cfg);
        }
    }

    public proceedToState(state: Es2pandaContextState, ctx: KNativePointer): void {
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
        global.es2pandaPublic._DestroyConfigWithoutLog(cfg.peer);
    }
    public destroyConfigWithLog(cfg: Config): void {
        if (cfg === undefined) {
            return;
        }
        global.es2pandaPublic._DestroyConfig(cfg.peer);
    }
}

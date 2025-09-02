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

import { throwError } from './utils';
import { KNativePointer } from './InteropTypes';
import {
  initEs2panda,
  Es2pandaNativeModule,
  initGeneratedEs2panda,
  initPublicEs2panda,
  initPublicGeneratedEs2panda
} from './Es2pandaNativeModule';
import { Es2pandaNativeModule as GeneratedEs2pandaNativeModule } from '../generated/Es2pandaNativeModule';
import { initInterop, InteropNativeModule, initPublicInterop } from './InteropNativeModule';

// CC-OFFNXT(G.NAM.01) project code style
export class global {
  public static filePath: string = './examples/input/main.ets';

  private static _config?: KNativePointer;
  public static set config(config: KNativePointer) {
    if (global._config !== undefined) {
      throwError('Global.config already initialized');
    }
    global._config = config;
  }

  public static get config(): KNativePointer {
    return global._config ?? throwError('Global.config not initialized');
  }

  public static destroyCfg(): void {
    global._config = undefined;
  }

  public static configIsInitialized(): boolean {
    return global._config !== undefined;
  }

  private static _context?: KNativePointer;
  public static set context(context: KNativePointer) {
    global._context = context;
  }
  public static get context(): KNativePointer {
    return global._context ?? throwError('Global.context not initialized');
  }

  private static _es2panda: Es2pandaNativeModule | undefined = undefined;
  private static _es2pandaPublic: Es2pandaNativeModule | undefined = undefined;
  public static get es2panda(): Es2pandaNativeModule {
    if (this._es2panda === undefined) {
      this._es2panda = initEs2panda();
    }
    return this._es2panda;
  }

  public static get es2pandaPublic(): Es2pandaNativeModule {
    if (this._es2pandaPublic === undefined) {
      this._es2pandaPublic = initPublicEs2panda();
    }
    return this._es2pandaPublic;
  }

  private static _generatedEs2panda: GeneratedEs2pandaNativeModule | undefined = undefined;
  private static _generatedEs2pandaPublic: GeneratedEs2pandaNativeModule | undefined = undefined;
  public static get generatedEs2panda(): GeneratedEs2pandaNativeModule {
    if (this._generatedEs2panda === undefined) {
      this._generatedEs2panda = initGeneratedEs2panda();
    }
    return this._generatedEs2panda;
  }

  public static get generatedEs2pandaPublic(): GeneratedEs2pandaNativeModule {
    if (this._generatedEs2pandaPublic === undefined) {
      this._generatedEs2pandaPublic = initPublicGeneratedEs2panda();
    }
    return this._generatedEs2pandaPublic;
  }

  private static _interop: InteropNativeModule | undefined = undefined;
  private static _interopPublic: InteropNativeModule | undefined = undefined;
  public static get interop(): InteropNativeModule {
    if (this._interop === undefined) {
      this._interop = initInterop();
    }
    return this._interop;
  }

  public static get interopPublic(): InteropNativeModule {
    if (this._interopPublic === undefined) {
      this._interopPublic = initPublicInterop();
    }
    return this._interopPublic;
  }
}

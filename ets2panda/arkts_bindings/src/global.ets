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

import { initEs2panda, initGeneratedEs2panda, Es2pandaNativeModule } from './Es2pandaNativeModule'; 
import { Es2pandaNativeModule as GeneratedEs2pandaNativeModule } from './generated/Es2pandaNativeModule'; 
import { initInterop, InteropNativeModule } from './InteropNativeModule';

export class global {
  private static _es2panda: Es2pandaNativeModule | undefined = undefined;
  public static get es2panda(): Es2pandaNativeModule {
    return global._es2panda ?? initEs2panda();
  }

  private static _generatedEs2panda: GeneratedEs2pandaNativeModule | undefined = undefined;
  public static get generatedEs2panda(): GeneratedEs2pandaNativeModule {
    return global._generatedEs2panda ?? initGeneratedEs2panda();
  }

  private static _interop: InteropNativeModule | undefined = undefined;
  public static get interop(): InteropNativeModule {
    return global._interop ?? initInterop();
  }
}
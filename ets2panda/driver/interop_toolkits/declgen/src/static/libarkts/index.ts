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

import { DriverHelper, global as arktsGlobal } from '@es2panda/bindings';
import type { LibArkts } from './types';
import path from 'path';

let libarkts: LibArkts | undefined;
const LIBARKTS_PATH_FROM_SDK: string = './build-tools/libarkts/lib/libarkts.js';

export function loadLibArkts(pandaSdkPath: string, staticSdkPath?: string): LibArkts {
  if (libarkts === undefined) {
    if (process.env.INTEROP_TOOLKITS_DECLGEN_USE_ES2PANDA_BINDINGS === 'true') {
      const arkts = new DriverHelper();
      // @ts-ignore
      libarkts = { arkts, arktsGlobal };
      libarkts!.arktsGlobal.es2panda._SetUpSoPath(path.join(pandaSdkPath, 'lib'));
    } else {
      if (staticSdkPath === undefined) {
        throw new Error(
          'staticSdkPath is required when INTEROP_TOOLKITS_DECLGEN_USE_ES2PANDA_BINDINGS is not set to true',
        );
      }
      const libarktsPath = process.env.KOALA_WRAPPER_PATH ?? path.resolve(staticSdkPath, LIBARKTS_PATH_FROM_SDK);
      libarkts = require(libarktsPath);
      libarkts!.arktsGlobal.es2panda._SetUpSoPath(pandaSdkPath);
    }
  }
  if (libarkts === undefined) {
    throw new Error('Failed to load libarkts');
  }
  return libarkts;
}

export type { LibArkts, KPointer } from './types';

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

import * as fs from 'fs';
import * as path from 'path';
import { KNativePointer as KPtr, KInt, KUInt } from './InteropTypes';
import { loadNativeModuleLibrary, registerNativeModuleLibraryName } from './loadLibraries';
import { throwError } from './utils';

export class InteropNativeModule {
  _StringLength(ptr: KPtr): KInt {
    throw new Error('Not implemented');
  }

  _StringData(ptr: KPtr, buffer: KPtr, length: KInt): void {
    throw new Error('Not implemented');
  }

  _GetStringFinalizer(): KPtr {
    throw new Error('Not implemented');
  }

  _InvokeFinalizer(ptr: KPtr, finalizer: KPtr): void {
    throw new Error('Not implemented');
  }

  _GetPtrVectorSize(ptr: KPtr): KInt {
    throw new Error('Not implemented');
  }

  _GetPtrVectorElement(ptr: KPtr, index: KInt): KPtr {
    throw new Error('Not implemented');
  }

  _getTypeOfVariant(ptr: KPtr): KInt {
    throw new Error('Not implemented');
  }

  _getIntFromVariant(ptr: KPtr): KInt {
    throw new Error('Not implemented');
  }

  _getStringFromVariant(ptr: KPtr): KPtr {
    throw new Error('Not implemented');
  }
}

export function initInterop(): InteropNativeModule {
  let libPath = process.env.BINDINGS_PATH;
  if (libPath === undefined) {
    libPath = path.resolve(__dirname, '../../ts_bindings.node');
  } else {
    libPath = path.join(libPath, 'ts_bindings.node');
  }
  if (!fs.existsSync(libPath)) {
    throwError(`Cannot find lib path ${libPath}`);
  }
  registerNativeModuleLibraryName('InteropNativeModule', libPath);
  const instance = new InteropNativeModule();
  loadNativeModuleLibrary('InteropNativeModule', instance);
  return instance;
}

export function initPublicInterop(): InteropNativeModule {
  let libPath = process.env.BINDINGS_PATH;
  if (libPath === undefined) {
    libPath = path.resolve(__dirname, '../../public.node');
  } else {
    libPath = path.join(libPath, 'public.node');
  }
  if (!fs.existsSync(libPath)) {
    throwError(`Cannot find lib path ${libPath}`);
  }
  registerNativeModuleLibraryName('InteropNativeModule', libPath);
  const instance = new InteropNativeModule();
  loadNativeModuleLibrary('InteropNativeModule', instance);
  return instance;
}

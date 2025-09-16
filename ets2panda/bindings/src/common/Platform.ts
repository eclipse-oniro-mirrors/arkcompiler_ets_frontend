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

import { isNullPtr } from './Wrapper';
import { decodeToString } from './arrays';
import { Wrapper } from './mainWrapper';
import { global } from './global';
import { int32, KInt, KPointer, nullptr } from './InteropTypes';

export abstract class NativeStringBase extends Wrapper {
  constructor(ptr: KPointer) {
    super(ptr);
  }

  protected abstract bytesLength(): int32;
  protected abstract getData(data: Uint8Array): void;

  toString(): string {
    let length = this.bytesLength();
    let data = new Uint8Array(length);
    this.getData(data);
    return decodeToString(data);
  }

  abstract close(): void;
}

export abstract class ArrayDecoder<T> {
  abstract getArraySize(blob: KPointer): int32;
  abstract disposeArray(blob: KPointer): void;
  abstract getArrayElement(blob: KPointer, index: int32): T;

  decode(blob: KPointer): Array<T> {
    const size = this.getArraySize(blob);
    const result = new Array<T>(size);
    for (let index = 0; index < size; index++) {
      result[index] = this.getArrayElement(blob, index);
    }
    this.disposeArray(blob);
    return result;
  }
}

// TODO: the semicolons after methods in these interfaces are to
// workaround ArkTS compiler parser bug
export interface CallbackRegistry {
  // CC-OFFNXT(no_explicit_any) project code style
  registerCallback(callback: any, obj: any): KPointer;
}

export interface PlatformDefinedData {
  nativeString(ptr: KPointer): NativeStringBase;
  nativeStringArrayDecoder(): ArrayDecoder<NativeStringBase>;
  callbackRegistry(): CallbackRegistry | undefined;
}

export function withStringResult(ptr: KPointer): string | undefined {
  if (isNullPtr(ptr)) {
    return undefined;
  }
  let managedString = new NativeString(ptr);
  let result = managedString?.toString();
  managedString?.close();
  return result;
}

export class NativeString extends NativeStringBase {
  constructor(ptr: KPointer) {
    super(ptr);
  }
  protected bytesLength(): int32 {
    return global.interop._StringLength(this.ptr);
  }
  protected getData(data: Uint8Array): void {
    global.interop._StringData(this.ptr, data, data.length);
  }
  close(): void {
    global.interop._InvokeFinalizer(this.ptr, global.interop._GetStringFinalizer());
    this.ptr = nullptr;
  }
}

export class NativePtrDecoder extends ArrayDecoder<KPointer> {
  getArraySize(blob: KPointer): KInt {
    return global.interop._GetPtrVectorSize(blob);
  }
  disposeArray(blob: KPointer): void {
    // TODO
  }
  getArrayElement(blob: KPointer, index: int32): KPointer {
    return global.interop._GetPtrVectorElement(blob, index);
  }
}

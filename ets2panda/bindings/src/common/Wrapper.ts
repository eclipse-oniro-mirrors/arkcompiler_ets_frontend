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

import { int32, KPointer, nullptr } from './InteropTypes';

export function isNullPtr(value: KPointer): boolean {
  return value === nullptr;
}

export function ptrToString(ptr: KPointer): string {
  return `0x${ptr!.toString(16).padStart(8, '0')}`;
}

export function isSamePtr(a: KPointer, b: KPointer): boolean {
  return a === b;
}

export function ptrToBits(ptr: KPointer): Uint32Array | null {
  let result = new Uint32Array(2);
  let ptrBigInt = ptr as bigint;
  result[0] = Number(ptrBigInt & BigInt(0xffffffff));
  result[1] = Number((ptrBigInt >> BigInt(32)) & BigInt(0xffffffff));
  return result;
}

export function bitsToPtr(array: Int32Array, offset: int32): KPointer {
  let ptrBigInt: bigint = BigInt(array[offset + 1]) & BigInt(0xffffffff);
  ptrBigInt = (ptrBigInt << BigInt(32)) | (BigInt(array[offset]) & BigInt(0xffffffff));
  return ptrBigInt;
}

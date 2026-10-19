'use static';
/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License"),
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

class ArrayBuffer {

    constructor(length: int, maxByteLength?: int);

    public atomicAddI8(index: int, byteOffset: int, value: byte): long;

    public atomicAndI8(index: int, byteOffset: int, value: byte): long;

    public atomicCompareExchangeI8(index: int, byteOffset: int, expectedValue: byte, replacementValue: byte): long;

    public atomicExchangeI8(index: int, byteOffset: int, value: byte): long;

    public atomicLoadI8(index: int, byteOffset: int): long;

    public atomicOrI8(index: int, byteOffset: int, value: byte): long;

    public atomicStoreI8(index: int, byteOffset: int, value: byte): long;

    public atomicSubI8(index: int, byteOffset: int, value: byte): long;

    public atomicXorI8(index: int, byteOffset: int, value: byte): long;

    public atomicAddI16(index: int, byteOffset: int, value: short): long;

    public atomicAndI16(index: int, byteOffset: int, value: short): long;

    public atomicCompareExchangeI16(index: int, byteOffset: int, expectedValue: short, replacementValue: short): long;

    public atomicExchangeI16(index: int, byteOffset: int, value: short): long;

    public atomicLoadI16(index: int, byteOffset: int): long;

    public atomicOrI16(index: int, byteOffset: int, value: short): long;

    public atomicStoreI16(index: int, byteOffset: int, value: short): long;

    public atomicSubI16(index: int, byteOffset: int, value: short): long;

    public atomicXorI16(index: int, byteOffset: int, value: short): long;

    public atomicAddI32(index: int, byteOffset: int, value: int): long;

    public atomicAndI32(index: int, byteOffset: int, value: int): long;

    public atomicCompareExchangeI32(index: int, byteOffset: int, expectedValue: int, replacementValue: int): long;

    public atomicExchangeI32(index: int, byteOffset: int, value: int): long;

    public atomicLoadI32(index: int, byteOffset: int): long;

    public atomicOrI32(index: int, byteOffset: int, value: int): long;

    public atomicStoreI32(index: int, byteOffset: int, value: int): long;

    public atomicSubI32(index: int, byteOffset: int, value: int): long;

    public atomicXorI32(index: int, byteOffset: int, value: int): long;

    public atomicAddI64(index: int, byteOffset: int, value: long): long;

    public atomicAndI64(index: int, byteOffset: int, value: long): long;

    public atomicCompareExchangeI64(index: int, byteOffset: int, expectedValue: long, replacementValue: long): long;

    public atomicExchangeI64(index: int, byteOffset: int, value: long): long;

    public atomicLoadI64(index: int, byteOffset: int): long;

    public atomicOrI64(index: int, byteOffset: int, value: long): long;

    public atomicStoreI64(index: int, byteOffset: int, value: long): long;

    public atomicSubI64(index: int, byteOffset: int, value: long): long;

    public atomicXorI64(index: int, byteOffset: int, value: long): long;

    public atomicAddU8(index: int, byteOffset: int, value: byte): long;

    public atomicAndU8(index: int, byteOffset: int, value: byte): long;

    public atomicCompareExchangeU8(index: int, byteOffset: int, expectedValue: byte, replacementValue: byte): long;

    public atomicExchangeU8(index: int, byteOffset: int, value: byte): long;

    public atomicLoadU8(index: int, byteOffset: int): long;

    public atomicOrU8(index: int, byteOffset: int, value: byte): long;

    public atomicStoreU8(index: int, byteOffset: int, value: byte): long;

    public atomicSubU8(index: int, byteOffset: int, value: byte): long;

    public atomicXorU8(index: int, byteOffset: int, value: byte): long;

    public atomicAddU16(index: int, byteOffset: int, value: short): long;

    public atomicAndU16(index: int, byteOffset: int, value: short): long;

    public atomicCompareExchangeU16(index: int, byteOffset: int, expectedValue: short, replacementValue: short): long;

    public atomicExchangeU16(index: int, byteOffset: int, value: short): long;

    public atomicLoadU16(index: int, byteOffset: int): long;

    public atomicOrU16(index: int, byteOffset: int, value: short): long;

    public atomicStoreU16(index: int, byteOffset: int, value: short): long;

    public atomicSubU16(index: int, byteOffset: int, value: short): long;

    public atomicXorU16(index: int, byteOffset: int, value: short): long;

    public atomicAddU32(index: int, byteOffset: int, value: int): long;

    public atomicAndU32(index: int, byteOffset: int, value: int): long;

    public atomicCompareExchangeU32(index: int, byteOffset: int, expectedValue: int, replacementValue: int): long;

    public atomicExchangeU32(index: int, byteOffset: int, value: int): long;

    public atomicLoadU32(index: int, byteOffset: int): long;

    public atomicOrU32(index: int, byteOffset: int, value: int): long;

    public atomicStoreU32(index: int, byteOffset: int, value: int): long;

    public atomicSubU32(index: int, byteOffset: int, value: int): long;

    public atomicXorU32(index: int, byteOffset: int, value: int): long;

    public atomicAddU64(index: int, byteOffset: int, value: long): long;

    public atomicAndU64(index: int, byteOffset: int, value: long): long;

    public atomicCompareExchangeU64(index: int, byteOffset: int, expectedValue: long, replacementValue: long): long;

    public atomicExchangeU64(index: int, byteOffset: int, value: long): long;

    public atomicLoadU64(index: int, byteOffset: int): long;

    public atomicOrU64(index: int, byteOffset: int, value: long): long;

    public atomicStoreU64(index: int, byteOffset: int, value: long): long;

    public atomicSubU64(index: int, byteOffset: int, value: long): long;

    public atomicXorU64(index: int, byteOffset: int, value: long): long;
}

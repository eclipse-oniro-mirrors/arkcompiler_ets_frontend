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

export const COLLECTIONS_TEXT = 'collections';
export const ARKTS_COLLECTIONS_MODULE = '@arkts.collections';
export const BIT_VECTOR = 'BitVector';
export const COLLECTIONS_MODULES = ['@arkts.collections', '@kit.ArkTS'];

export type BitVectorUsage = undefined | { ns: string; used: boolean };

export const DEPRECATED_SENDABLE_CONTAINER_LIST = [
  'Array',
  'Map',
  'Set',
  'ArrayBuffer',
  'Int8Array',
  'Uint8ClampedArray',
  'Uint8Array',
  'Int16Array',
  'Uint16Array',
  'Int32Array',
  'Uint32Array',
  'Float32Array'
];

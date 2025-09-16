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
import collections from './api/@arkts.collections'

const a: collections.Array<string> = new collections.Array<string>();
const b: collections.Float32Array = new collections.Float32Array();
const c: collections.Uint8Array = new collections.Uint8Array();
const d: collections.Uint8ClampedArray = new collections.Uint8ClampedArray();
const e: collections.Uint16Array = new collections.Uint16Array();
const f: collections.Uint32Array = new collections.Uint32Array();
const a1: collections.Array<string> = new collections.Array<string>("111");
const a2: collections.Array<number> = new collections.Array(2);
const a3: collections.Array<number> = new collections.Array();
let arr1 = new collections.Array(2)
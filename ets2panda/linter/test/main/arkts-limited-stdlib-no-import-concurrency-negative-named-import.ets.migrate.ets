/*
 * Copyright (c) 2022-2026 Huawei Device Co., Ltd.
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

import { ArkTSUtils, stream } from './oh_modules/@kit.ArkTS'

import {ArkTSUtils as AU} from './oh_modules/@kit.ArkTS'
import { stream as st } from './oh_modules/@kit.ArkTS'


let func: () => void = () => {};
taskpool.execute(func);
taskpool.execute(func);
let result = process.isIsolatedProcess();
let result1 = process.isIsolatedProcess();
let lock: AsyncLock = new AsyncLock();
let lock1: AsyncLock = new AsyncLock();
let lck: AsyncLock = AsyncLock.request("lock");
let lck1: AsyncLock = AsyncLock.request("lock");
let pro = new SendableLruCache<number, number>();
let pro1 = new SendableLruCache<number, number>();
let write: stream.Writable = new stream.Writable();
let map126: Map<Array<number>, number> = new Map();

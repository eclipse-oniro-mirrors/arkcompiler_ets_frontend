/*
 * Copyright (c) 2022-2025 Huawei Device Co., Ltd.
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

import { taskpool, ArkTSUtils, stream } from './oh_modules/@kit.ArkTS'
import { process } from './oh_modules/@kit.ArkTS'
import {taskpool as tp, ArkTSUtils as AU} from './oh_modules/@kit.ArkTS'
import { process as pc, stream as st } from './oh_modules/@kit.ArkTS'
import {collections as cos }  from './oh_modules/@kit.ArkTS';

let func: () => void = () => {};
taskpool.execute(func);
tp.execute(func)
let result = pc.isIsolatedProcess();
let result1 = process.isIsolatedProcess();
let lock: ArkTSUtils.locks.AsyncLock = new ArkTSUtils.locks.AsyncLock()
let lock1: AU.locks.AsyncLock = new AU.locks.AsyncLock()
let lck: ArkTSUtils.locks.AsyncLock = ArkTSUtils.locks.AsyncLock.request("lock")
let lck1: AU.locks.AsyncLock = AU.locks.AsyncLock.request("lock")
let pro = new ArkTSUtils.SendableLruCache<number, number>()
let pro1 = new AU.SendableLruCache<number, number>()
let write: stream.Writable = new stream.Writable()
let map126: Map<cos.Array<number>, number> = new Map();

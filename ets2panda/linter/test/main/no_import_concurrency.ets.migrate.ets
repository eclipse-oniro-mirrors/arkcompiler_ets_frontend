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


import { foo, util } from '@ohos.taskpool';
import { anyClass } from '@kit.ArkTS'; //legal
import { fooClass } from '@ohos.taskpool'; //legal
import defaultImport, { roo } from '@kit.ArkTS';
import koo from '@kit.ArkTS';
import doo, { fooModule as fooAs, too } from '@kit.ArkTS';
import bbbb from '@kit.ArkTS';


import process from '@ohos.process';





taskpool.getTaskPoolInfo()

function concurrency () {
    const aaa = 123;
    console.log(aaa);
}

process.isIsolatedProcess();
taskpool.cancel();
Set()
Set()
fooAs.getFoo();

let lock1: AsyncLock = AsyncLock.request('lock1');
let lock2 = new AsyncLock();

let aa:AsyncLock= new AsyncLock()
let lock_: AsyncLock = new AsyncLock();
let lock: AsyncLock = AsyncLock.request("lock_1");
let state: AsyncLockState = AsyncLock.query('queryTestLock');
let held: AsyncLockInfo[] = state.held
let state2 = AsyncLock.queryAll();
lock.lockAsync(async () => {
}, AsyncLockMode.SHARED);
lock.lockAsync(async () => {
}, AsyncLockMode.EXCLUSIVE);
let s: AbortSignal<string> = { aborted: false, reason: 'Aborted' };
let options = new AsyncLockOptions<string>();
options.isAvailable = false;
options.signal = s;
let conditionVariable = new ConditionVariable();
let conditionVariable1 = ConditionVariable.request("conditionName");

let pro = new SendableLruCache<number, number>();

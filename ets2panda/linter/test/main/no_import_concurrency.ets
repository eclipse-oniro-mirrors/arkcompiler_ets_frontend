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


import { foo, util, taskpool as tpp } from '@ohos.taskpool';
import { anyClass } from '@kit.ArkTS'; //legal
import foo2, { fooClass } from '@ohos.taskpool'; //legal
import defaultImport, { ArkTSUtils as Atu, roo } from '@kit.ArkTS';
import koo, { ArkTSUtils as Ark, process as pr } from '@kit.ArkTS';
import doo, { fooModule as fooAs, too } from '@kit.ArkTS';
import bbbb, { taskpool as tsk, process as prs } from '@kit.ArkTS';
import fooke from '@ohos.process';
import { process as ps, collections as clt } from '@kit.ArkTS';
import process from '@ohos.process';
import aaa from '@ohos.taskpool'
import aaaa from '@arkts.collections';
import utils from './oh_modules/@arkts.utils'
import { ArkTSUtils } from './oh_modules/@kit.ArkTS';

aaa.getTaskPoolInfo()

function concurrency () {
    const aaa = 123;
    console.log(aaa);
}

ps.isIsolatedProcess();
tsk.cancel();
aaaa.Set()
clt.Set()
fooAs.getFoo();

let lock1: utils.locks.AsyncLock = utils.locks.AsyncLock.request('lock1');
let lock2 = new ArkTSUtils.locks.AsyncLock();

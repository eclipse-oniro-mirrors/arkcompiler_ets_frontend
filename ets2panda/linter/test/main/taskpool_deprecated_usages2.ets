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

import taskpool from '@ohos.taskpool';
import { otherTaskPool as taskpool1 } from './@ohos.taskpool';
import { taskpool as taskpool2 } from './@ohos.taskpool';
function test1(){}
const array1: number[] =[1]
const transfer: ArrayBuffer[] =[]
let task = new taskpool1.Task(test1);
task.setCloneList(array1)
task.setTransferList(transfer)
new taskpool1.Task(test1).setTransferList(transfer);
function test(){
  const task3 = new taskpool2.Task(test1);
  typeof task3.setTransferList(transfer);
  return new taskpool2.Task(test1).setTransferList();
}
let task4 = new taskpool.Task(test1);
task4.setCloneList([]) //error
task4.setCloneList(transfer) //error
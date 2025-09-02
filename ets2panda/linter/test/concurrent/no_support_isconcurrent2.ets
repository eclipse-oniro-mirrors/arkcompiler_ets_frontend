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
import { taskpool } from '@ohos.taskpool';
import { taskpool as taskpool2 } from './@ohos.taskpool';
import { otherTaskPool as taskpool1 } from './@ohos.taskpool';

function test1(){
}
function isConcurrent() {}

typeof taskpool.isConcurrent() ; //error
function test(){
  taskpool.isConcurrent(test5()) ; //error
}

console.log(''+taskpool2.isConcurrent(test1));

taskpool2.isConcurrent(test1)
taskpool2.isConcurrent(test1)
taskpool2.isConcurrent(isConcurrent)
function test5(){
  taskpool2.isConcurrent(test1)
  taskpool2.isConcurrent(test1)
  taskpool2.isConcurrent(isConcurrent)
}
class Demo{
  get(){
    return taskpool1.isConcurrent(test);
  }
}
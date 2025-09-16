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

import {Stack} from './api/@ohos.util.Stack';
import Queue from './api/@ohos.util.Queue';
import {LinkedList} from './api/@ohos.util.LinkedList';
import {PlainArray} from './api/@ohos.util.PlainArray';

let stack = new Stack<number>();
stack.push(1);
const a: number = stack[0]; //error


let queue = new Queue<number>();
queue.add(1); //error
const b: number = queue[0]; //error


let linkedList = new LinkedList<number>();
linkedList.add(1);
const c: number = linkedList[0]; //error


let plainArray = new PlainArray<number>();
stack.push(1);
const d: number = plainArray[0]; //error
const d: number = plainArray[];
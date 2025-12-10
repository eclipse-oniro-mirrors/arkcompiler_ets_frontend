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

// User-defined worker - legal
import { worker } from './oh_modules/defined_worker';
import { worker as aliasWorker } from './oh_modules/defined_worker';
let worker6: worker.ThreadWorker('script.js');

export class SendableObject {
  a: number = 45;
}

const worker1: aliasWorker.ThreadWorker = new aliasWorker.ThreadWorker('script.js');
worker1.postMessage("message");
let obj: SendableObject = new SendableObject()
worker1.postMessageWithSharedSendable(obj)
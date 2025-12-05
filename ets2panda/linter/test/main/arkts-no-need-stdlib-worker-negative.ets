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

// Example 1: Default import with different names. (in the IDE, the import module specifier should be just '@ohos.worker')
import worker from './oh_modules/@ohos.worker'; // Error: Worker are not supported

import workerAlias from './oh_modules/@ohos.worker'; // Error: Worker are not supported

// Example 2: Import from Kit.ArkTS (in the IDE, the import module specifier should be just '@kit.ArkTS')
import { ErrorEvent, MessageEvents, ThreadWorkerGlobalScope, worker as kitWorker } from './oh_modules/@kit.ArkTS'; // Error: Worker are not supported

// Example 3: Basic usage
export class SendableObject {
  a: number = 45;
}

const worker1: worker.ThreadWorker = new worker.ThreadWorker('script.js'); // Error: Cannot find type 'worker'

worker1.postMessage("message");
let obj: SendableObject = new SendableObject()
worker1.postMessageWithSharedSendable(obj)

function testWorkerUsage() {
  // Example 4: Usage with alias
  const worker2 = new workerAlias.ThreadWorker('script.js'); // Error: Cannot find type 'worker'. 

  // Example 5: Usage from @kit.ArkTS with alias
  const worker3 = new kitWorker.ThreadWorker('script.js'); // Error: Cannot find type 'worker'. 
}
//Example 6: Definition and usage different line
let worker4: workerAlias.ThreadWorker // Error: Cannot find type 'worker'
worker4 = new worker.ThreadWorker('script.js'); // Error: Cannot find type 'worker'

// Example 7: Usage with different type given
const workerPort: ThreadWorkerGlobalScope = worker.workerPort; // Error: Cannot find type 'worker'

workerPort.onmessage = (event: MessageEvents) => {
};

workerPort.onmessageerror = (event: MessageEvents) => {
};

workerPort.onerror = (event: ErrorEvent) => {
};
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

export const ERROR_PROP_LIST: Set<string> = new Set([
  'name',
  'message',
  'stack',
  'code',
  'toString',
  'toLocaleString',
  'valueOf',
  'hasOwnProperty',
  'isPrototypeOf',
  'propertyIsEnumerable'
]);

export const ERROR_TASKPOOL_PROP_LIST: { [key: string]: Set<string> } = {
  'taskpool.TaskInfo': new Set(['duration']),
  'taskpool.ThreadInfo': new Set(['taskIds', 'priority']),
  'StdProcess.ConditionType': new Set(['timeout', 'killSignal', 'maxBuffer'])
};

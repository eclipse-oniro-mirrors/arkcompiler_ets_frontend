/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

export class ExportClass1 {
  class1: string = 'hello1';
}

export class ExportClass2 {
  class2: string = 'hello2';
}

export const MAX_RETRY = 5;
export let globalCounter = 0;

export function initConfig() {}

export enum ErrorCode {
  OK = 0,
  TIMEOUT = 1
}
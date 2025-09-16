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

export namespace process {
  function isAppUid(v: number): boolean;
  function getUidForName(v: string): number;
  function getThreadPriority(v: number): number;
  function getSystemConfig(name: number): number;
  function getEnvironmentVar(name: string): string;
  function exit(code: number): void;
  function kill(signal: number, pid: number): boolean;
}

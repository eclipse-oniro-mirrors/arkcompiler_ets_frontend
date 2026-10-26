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

export interface InterfaceAA {}
export class ClassAA implements InterfaceAA {}
export class ClassBB extends ClassAA {}
export class ClassCC extends ClassBB {}
export class ClassDD extends ClassCC {}

let cond2 = false
export function getInterfaceAA(): InterfaceAA {
  if (cond1) {
    return new ClassCC();
  }
  return new ClassAA();
}

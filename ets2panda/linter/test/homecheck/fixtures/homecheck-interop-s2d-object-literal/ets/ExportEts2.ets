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

'use static'

export class X {
  name: string = '';

  constructor(arg: string) {
    this.name = arg;
  }
}

export interface Y {
  data: number;
}

export function foo(arg: X) {}

export function bar(arg1: number, arg2: Y): string {}

export function createY(d: number): Y {
  let y: Y = { data: d };
  return y;
}
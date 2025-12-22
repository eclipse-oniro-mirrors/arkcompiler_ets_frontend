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

export function foo(): void {}
export class A {}

export {
  foo as f1,
  foo as f2,
  A as a1,
  A as a2
}
/* @@? 21:3 Warning Warning W32596: 'foo' is explicitly used with alias several times */
/* @@? 23:3 Warning Warning W32596: 'A' is explicitly used with alias several times */
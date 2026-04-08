/*
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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

namespace Space1 {
   export function foo() {  }
   export let variable = 1234;
   export const constant = 1234;
}

export namespace Space2 {
   export function foo(p: number) {  }
   export let variable = "1234";
}
/* @@? 17:4 Warning Warning W71336: Exported function 'foo' requires type annotation. */
/* @@? 18:15 Warning Warning W71336: Exported class property 'variable' requires type annotation. */
/* @@? 19:17 Warning Warning W71336: Exported class property 'constant' requires type annotation. */
/* @@? 23:4 Warning Warning W71336: Exported function 'foo' requires type annotation. */
/* @@? 24:15 Warning Warning W71336: Exported class property 'variable' requires type annotation. */

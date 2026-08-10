/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

function foo(x: number): number {
  return 1;
}

function foo(x: string): string {
  return "1";
}

function main(): void {
  foo(true);
}

/* @@? 25:3 Error Semantic error ESE0127: No matching call signature for foo(Boolean) */
/* @@? 25:7 Error Semantic error ESE0046: Type 'Boolean' is not compatible with type 'String' at index 1 */

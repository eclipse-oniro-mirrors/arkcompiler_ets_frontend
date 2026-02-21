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

// NOTE! Temporary test - should be removed (or moved into runtime suite) after the `void` type refactoring

type T<R> = R;

function main() {
  let x: NonNullable<int | undefined> = 5;
  /* @@ label1 */console.println(x instanceof T<NonNullable</* @@ label2 */void>>);
}

/* @@@ label1 Error Semantic error ESE0127: No matching call signature for println(Boolean) */
/* @@@ label2 Error Semantic error ESE0232: 'void' used as type annotation. */

/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

class A<T> {}
class B {}

function main(): void {
  let abn : A<B | null> = /* @@ label */new A<B>(); // should not work: non nullable B is the subtype of nullable B, but T has no variance mark
  let ab : A<B> = /* @@ label1 */abn; // should not work: nullable B (the type of abn) is not the subtype of non nullable B
}

/* @@@ label Error TypeError: Type 'A<B>' cannot be assigned to type 'A<B|null>'  */
/* @@@ label1 Error TypeError: Type 'A<B|null>' cannot be assigned to type 'A<B>'  */

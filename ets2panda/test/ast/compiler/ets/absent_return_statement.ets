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

function foo1(bar: () => int|string|undefined) {
   let x = bar();
   console.println("x = " + x);
}

function foo2(bar: () => int|string) {
   let x = bar();
   console.println("x = " + x);
}

class C{}

function main(): void {
  foo1(() => { return /* @@ label1 */new C(); });
  foo1(() => { console.println("TEST"); });
  foo1(() => /* @@ label2 */new C());
  foo1(() => console.println("TEST"));

  foo2(() => { return /* @@ label3 */new C(); });
  foo2(() => /* @@ label4 */{ console.println("TEST"); });
  foo2(() => /* @@ label5 */new C());
  foo2(() => /* @@ label6 */console.println("TEST"));
}


/* @@@ label1 Error Semantic error ESE0091: Type 'C' is not compatible with the enclosing method's return type 'Int|String|undefined' */
/* @@@ label2 Error Semantic error ESE0091: Type 'C' is not compatible with the enclosing method's return type 'Int|String|undefined' */
/* @@@ label3 Error Semantic error ESE0091: Type 'C' is not compatible with the enclosing method's return type 'Int|String' */
/* @@@ label4 Error Semantic error ESE0091: Type 'void' is not compatible with the enclosing method's return type 'Int|String' */
/* @@@ label5 Error Semantic error ESE0091: Type 'C' is not compatible with the enclosing method's return type 'Int|String' */
/* @@@ label6 Error Semantic error ESE0091: Type 'void' is not compatible with the enclosing method's return type 'Int|String' */
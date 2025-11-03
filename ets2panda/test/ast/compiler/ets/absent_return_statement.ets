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

/* @@? 29:3 Error TypeError: No matching call signature for foo1((() => Int|String|undefined)) */
/* @@? 29:38 Error TypeError: Type 'C' is not compatible with the enclosing method's return type 'Int|String|undefined' */
/* @@? 31:3 Error TypeError: No matching call signature for foo1((() => Int|String|undefined)) */
/* @@? 31:29 Error TypeError: Type 'C' is not compatible with the enclosing method's return type 'Int|String|undefined' */
/* @@? 34:3 Error TypeError: No matching call signature for foo2((() => Int|String)) */
/* @@? 34:38 Error TypeError: Type 'C' is not compatible with the enclosing method's return type 'Int|String' */
/* @@? 35:3 Error TypeError: No matching call signature for foo2((() => Int|String)) */
/* @@? 35:29 Error TypeError: Type 'void' is not compatible with the enclosing method's return type 'Int|String' */
/* @@? 36:3 Error TypeError: No matching call signature for foo2((() => Int|String)) */
/* @@? 36:29 Error TypeError: Type 'C' is not compatible with the enclosing method's return type 'Int|String' */
/* @@? 37:3 Error TypeError: No matching call signature for foo2((() => Int|String)) */
/* @@? 37:29 Error TypeError: Type 'void' is not compatible with the enclosing method's return type 'Int|String' */
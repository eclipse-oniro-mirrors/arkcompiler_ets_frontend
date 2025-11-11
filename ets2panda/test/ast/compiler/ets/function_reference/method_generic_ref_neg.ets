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

class C {
  gen<T>(x: T): T { return x; }
  static sgen<T>(x: T): T { return x; }
}
interface I {
  gen<T>(x: T): T;
}
declare const i: I;

function main() {
  let c = new C();

  let a = c.gen;
  let b = C.sgen;
  let d = i.gen;
}
/* @@? 28:11 Error Semantic error ESE655063: Generic function gen requires explicit type arguments when used as a value. */
/* @@? 29:11 Error Semantic error ESE655063: Generic function sgen requires explicit type arguments when used as a value. */
/* @@? 30:11 Error Semantic error ESE655063: Generic function gen requires explicit type arguments when used as a value. */

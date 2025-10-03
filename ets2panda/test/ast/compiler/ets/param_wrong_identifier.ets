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

class A {
  foo1(a: number) { }
  foo2(a: Number) { }
  foo3(a: Int) { }
  foo4(a: String) { }
  static foo5(a: Boolean) { }
  static foo6(a: number) { }
}
function foo7(a: number) { }

function main() {
  let a = new A();
  a.foo1(Number);
  a.foo2(Number);
  a.foo3(Int);
  a.foo4(String);
  A.foo5(Boolean);
  A.foo6(Number);
  foo7(Number);
  const v1 = Number.isSafeInteger(Number);
  try {
    const v1 = Number.isSafeInteger(Number);
  } catch (e) { }
}

/* @@? 28:3 Error Semantic error ESE0127: No matching call signature for foo1(Double) */
/* @@? 28:10 Error Semantic error ESE0122: Class name can't be the argument of function or method. */
/* @@? 29:3 Error Semantic error ESE0127: No matching call signature for foo2(Double) */
/* @@? 29:10 Error Semantic error ESE0122: Class name can't be the argument of function or method. */
/* @@? 30:3 Error Semantic error ESE0127: No matching call signature for foo3(Int) */
/* @@? 30:10 Error Semantic error ESE0122: Class name can't be the argument of function or method. */
/* @@? 30:10 Error Semantic error ESE132307: Class 'Int' cannot be used as object */
/* @@? 31:3 Error Semantic error ESE0127: No matching call signature for foo4(String) */
/* @@? 31:10 Error Semantic error ESE0122: Class name can't be the argument of function or method. */
/* @@? 31:10 Error Semantic error ESE132307: Class 'String' cannot be used as object */
/* @@? 32:3 Error Semantic error ESE0127: No matching call signature for foo5(Boolean) */
/* @@? 32:10 Error Semantic error ESE0122: Class name can't be the argument of function or method. */
/* @@? 32:10 Error Semantic error ESE132307: Class 'Boolean' cannot be used as object */
/* @@? 33:3 Error Semantic error ESE0127: No matching call signature for foo6(Double) */
/* @@? 33:10 Error Semantic error ESE0122: Class name can't be the argument of function or method. */
/* @@? 34:3 Error Semantic error ESE0127: No matching call signature for foo7(Double) */
/* @@? 34:8 Error Semantic error ESE0122: Class name can't be the argument of function or method. */
/* @@? 35:14 Error Semantic error ESE0127: No matching call signature for isSafeInteger(Double) */
/* @@? 35:35 Error Semantic error ESE0122: Class name can't be the argument of function or method. */
/* @@? 37:16 Error Semantic error ESE0127: No matching call signature for isSafeInteger(Double) */
/* @@? 37:37 Error Semantic error ESE0122: Class name can't be the argument of function or method. */
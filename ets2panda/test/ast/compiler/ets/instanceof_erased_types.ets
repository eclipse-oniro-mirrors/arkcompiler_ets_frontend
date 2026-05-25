/**
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

function foo(x: Object): void {
  // Dummy code 'console.log(...)' will be never executed. Just placeholder.
  if (x instanceof /* @@ label1 */Array<Any>) {
    console.log("Array<Any>");
  } else if (x instanceof /* @@ label2 */Array<Any> | undefined) {
    console.log("Array<Any> | undefined");
  } else if (x instanceof /* @@ label3 */Array<undefined>) {
    console.log("Array<undefined>");
  } else if (x instanceof /* @@ label4 */Array<Int>) {
    console.log("Array<Int>");
  } else if (x instanceof /* @@ label5 */FixedArray<Int>) {
    console.log("FixedArray<Int>");
  } else if (x instanceof FixedArray<Int | undefined>) {
    console.log("FixedArray<Int | undefined>");
  } else if (x instanceof FixedArray<undefined>) {
    console.log("FixedArray<Int | undefined>");
  } else if (x instanceof /* @@ label6 */(p: number) => void) {
    console.log("(p: number) => void");
  } else if (x instanceof (p: Any, q: Any) => never) {
    console.log("(p: Any, q: Any) => never");
  } else if (x instanceof /* @@ label7 */[number]) {
    console.log("[number]");
  } else if (x instanceof /* @@ label8 */FixedArray</* @@ label9 */Array<Int>>) {
    console.log("FixedArray<Array<Int>>");
  } else if (x instanceof /* @@ label10 */FixedArray<Array>) {
    console.log("FixedArray<Array<Int>>");
  } else if (x instanceof FixedArray<Array | undefined>) {
    console.log("FixedArray<Array<Int>>");
  } else if (x instanceof ValueArray<Int>) {
    console.log("ValueArray<Int>");
  } else if (x instanceof /* @@ label11 */ValueArray<Int | undefined>) {
    console.log("ValueArray<Int | undefined>");
  }
}

/* @@@ label1 Error Syntax error ESY18871: Type parameter is erased from type 'Array' when used in instanceof expression. */
/* @@@ label2 Error Syntax error ESY18871: Type parameter is erased from type 'Array' when used in instanceof expression. */
/* @@@ label3 Error Syntax error ESY18871: Type parameter is erased from type 'Array' when used in instanceof expression. */
/* @@@ label4 Error Syntax error ESY18871: Type parameter is erased from type 'Array' when used in instanceof expression. */
/* @@@ label5 Error Semantic error ESE38251: Type 'FixedArray<Int>' cannot be used on the right hand side of an 'instanceof' expression */
/* @@@ label6 Error Semantic error ESE38251: Type '((p: number)=> void)' cannot be used on the right hand side of an 'instanceof' expression */
/* @@@ label7 Error Semantic error ESE38251: Type '[number]' cannot be used on the right hand side of an 'instanceof' expression */
/* @@@ label8 Error Semantic error ESE38251: Type 'FixedArray<Array<Int>>' cannot be used on the right hand side of an 'instanceof' expression */
/* @@@ label9 Error Syntax error ESY18871: Type parameter is erased from type 'Array' when used in instanceof expression. */
/* @@@ label10 Error Semantic error ESE38251: Type 'FixedArray<Array>' cannot be used on the right hand side of an 'instanceof' expression */
/* @@@ label11 Error Semantic error ESE1547180: ValueArray requires a primitive type argument: numeric, boolean or char type. */

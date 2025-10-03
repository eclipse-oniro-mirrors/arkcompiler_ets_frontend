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

class A{
    field1:int;
    field2:int;
    method1(){};
}

function foo(){}

function main():void{
    /* @@ label1 */type UnionA = A|string|undefined
    /* @@ label2 */type keyofUnion = keyof unionA;

    /* @@ label3 */type keyofNever = keyof never;

    /* @@ label4 */type keyofFunction = keyof foo;

   /* @@ label5 */ type keyofVoid = keyof void;
}



/* @@@ label1 Error Syntax error ESY0040: Illegal start of Type Alias expression. */
/* @@@ label2 Error Syntax error ESY0040: Illegal start of Type Alias expression. */
/* @@@ label3 Error Syntax error ESY0040: Illegal start of Type Alias expression. */
/* @@@ label4 Error Syntax error ESY0040: Illegal start of Type Alias expression. */
/* @@? 32:20 Error Syntax error ESY0040: Illegal start of Type Alias expression. */
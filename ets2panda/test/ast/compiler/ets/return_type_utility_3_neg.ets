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

type Incorrect1 = ReturnType<string>;
type Incorrect2 = ReturnType<(()=>double) | int>;
type Incorrect3 = ReturnType<foo3>;
type Incorrect4 = ReturnType<()=>void>;
type Incorrect5 = ReturnType<null>;
type Incorrect6 = ReturnType<Any>;
type Incorrect7 = ReturnType<[()=>double, ()=>int]>;
type Incorrect8 = ReturnType<(()=>double| ()=>int)[]>;


function foo1<T>(a0: ReturnType<T>): void {}
function foo2<P extends Function>(a0: ReturnType<P>): P|undefined { return undefined; }
function foo3(): Int {}

class A {}

function main(): void {
    let a: A|undefined = foo2(new A());
}

/* @@? 16:29 Error TypeError: Type '<string>' cannot be used as type parameter for ReturnType utilty type */
/* @@? 17:29 Error TypeError: Type '<((()=> double) | int)>' cannot be used as type parameter for ReturnType utilty type */
/* @@? 18:30 Error TypeError: Cannot find type 'foo3'. */
/* @@? 19:29 Error TypeError: 'void' used as type annotation. */
/* @@? 20:29 Error TypeError: Type '<null>' cannot be used as type parameter for ReturnType utilty type */
/* @@? 21:29 Error TypeError: Type '<Any>' cannot be used as type parameter for ReturnType utilty type */
/* @@? 22:29 Error TypeError: Type '<[(()=> double), (()=> int)]>' cannot be used as type parameter for ReturnType utilty type */
/* @@? 23:29 Error TypeError: Type '<Array<(()=> (double | (()=> int)))>>' cannot be used as type parameter for ReturnType utilty type */
/* @@? 26:32 Error TypeError: Type '<T>' cannot be used as type parameter for ReturnType utilty type */
/* @@? 33:26 Error TypeError: Type 'Function|undefined' cannot be assigned to type 'A|undefined' */

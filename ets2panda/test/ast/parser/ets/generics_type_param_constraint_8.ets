/*
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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


class X <K extends Comparable<T>,T> {}
function main(){
    const myCharClass = new X/* @@ label */<Char,String>();
}

class A {}

class C {
    f<T extends A>(a: T) { }
}

class D {}
/* @@ label2 */(new C()).f(new D());

/* @@@ label Error Semantic error ESE0228: Type argument 'Char' should be a subtype of 'Comparable<String>' constraint */

/* @@@ label2 Error Semantic error ESE0127: No matching call signature for f(D) */
/* @@@ label2 Error Semantic error ESE8745: Type parameter 'T' can not be inferred for the call to 'f'. Specify the type argument explicitly. */

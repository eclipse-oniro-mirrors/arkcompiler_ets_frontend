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

class Test{
    foo1(a:[string,string,string]){
        console.log("invoke1")
    }

    overload foo111(foo1,foo2,foo3){} //syntaxError, do not crash
    overload foo112(foo1,foo2) //syntaxError, do not crash
}

function main(){
    let a:Test = new Test();
    a.foo111(["abc","abc",1]);
}


/* @@? 21:20 Error Syntax error ESY0112: Unexpected token, expected an identifier. */
/* @@? 21:20 Error Syntax error ESY0228: Unexpected token, expected '{'. */
/* @@? 21:20 Error Semantic error ESE0384: overloaded name must refer to an accessible method. */
/* @@? 21:21 Error Syntax error ESY0229: Unexpected token, expected ',' or '}'. */
/* @@? 21:21 Error Semantic error ESE0351: Variable 'foo1' has already been declared. */
/* @@? 21:25 Error Syntax error ESY0227: Unexpected token ','. */
/* @@? 21:25 Error Syntax error ESY0121: Field type annotation expected. */
/* @@? 21:30 Error Syntax error ESY0227: Unexpected token ','. */
/* @@? 21:30 Error Syntax error ESY0121: Field type annotation expected. */
/* @@? 21:35 Error Syntax error ESY0121: Field type annotation expected. */
/* @@? 21:35 Error Syntax error ESY0227: Unexpected token ')'. */
/* @@? 21:36 Error Syntax error ESY0227: Unexpected token '{'. */
/* @@? 22:20 Error Syntax error ESY0228: Unexpected token, expected '{'. */
/* @@? 22:20 Error Syntax error ESY0112: Unexpected token, expected an identifier. */
/* @@? 22:20 Error Semantic error ESE0384: overloaded name must refer to an accessible method. */
/* @@? 22:21 Error Syntax error ESY0229: Unexpected token, expected ',' or '}'. */
/* @@? 22:21 Error Semantic error ESE0143: Unresolved reference foo1 */
/* @@? 22:25 Error Syntax error ESY0227: Unexpected token ','. */
/* @@? 22:26 Error Syntax error ESY0227: Unexpected token 'foo2'. */
/* @@? 22:26 Error Semantic error ESE0143: Unresolved reference foo2 */
/* @@? 22:30 Error Syntax error ESY0227: Unexpected token ')'. */
/* @@? 23:1 Error Syntax error ESY0227: Unexpected token '}'. */
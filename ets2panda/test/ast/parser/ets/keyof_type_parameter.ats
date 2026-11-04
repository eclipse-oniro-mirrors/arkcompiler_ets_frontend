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

function getProperty1<T>(obj: A, key: keyof T/* @@ label1 */){}
function getProperty2<T,K extends keyof T/* @@ label2 */>(){}
class B<T,K extends keyof T/* @@ label3 */>{}

function main():void{
    let a = new A();
    getProperty1<A>(a,"field1")
    getProperty1<A>(a,"field2")
    getProperty1<A>(a,"method1")
}


/* @@@ label1 Error Semantic error ESE0276: The `keyof` keyword can only be used for class or interface type. */
/* @@@ label2 Error Semantic error ESE0276: The `keyof` keyword can only be used for class or interface type. */
/* @@@ label3 Error Semantic error ESE0276: The `keyof` keyword can only be used for class or interface type. */
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

class C<T extends keyof A>{}

function getProperty<K extends keyof A>(key: K){}

function main():void{
    let d = new C/* @@ label1 */<"field12345">;

    let a:A = new A();
    /* @@ label2 */getProperty<keyof A>(/* @@ label3 */"field12345");
}

/* @@@ label1 Error TypeError: Type argument '"field12345"' should be a subtype of '"method1"|"field1"|"field2"'-constraint */
/* @@@ label2 Error TypeError: No matching call signature for getProperty("field12345")  */
/* @@@ label3 Error TypeError: Type '"field12345"' is not compatible with type '"method1"|"field1"|"field2"' at index 1  */

/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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
class A<T extends A<T>>{

    foo<S extends A<S>>(x:T){}
}

class C extends A<C>{}
class D extends A<D>{}
function main(){
    let y:D
    let a_obj = new A<C>()
    /* @@ label */a_obj.foo<C>(/* @@ label1 */y) 
}

/* @@@ label Error Semantic error ESE0127: No matching call signature for foo(D) */
/* @@@ label1 Error Semantic error ESE0046: Type 'D' is not compatible with type 'C' at index 1 */
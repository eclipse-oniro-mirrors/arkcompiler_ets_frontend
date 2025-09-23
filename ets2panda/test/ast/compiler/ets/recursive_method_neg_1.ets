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
class E{}
function main(){
    let x:C
    let a_obj = new A<C>()
    /* @@ label */a_obj.foo<E>(x)
}

/* @@@ label Error TypeError: Type argument 'E' should be a subtype of 'A<E>' constraint  */
/* @@@ label Error TypeError: No matching call signature for foo(C)  */
